#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG_GERADOR   1

#define N_ACESSOS       4
#define N_TEMPOS        10
#define N_INTERVALOS    10
#define PROB_INT_0      5
#define PROB_INT_1      8
#define SEC2NANO        1000000000
#define LOG_FILE        "gerador.log"
#define FIFO_N          "tmp/fifoN"
#define FIFO_E          "tmp/fifoE"
#define FIFO_S          "tmp/fifoS"
#define FIFO_O          "tmp/fifoO"
#define FIFO            "tmp/fifo"
#define FIFO_NAME_SIZE  19
#define FIFO_CONTENT    50

/*
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; variavel global com inicializacao do mutex;
pthread_mutex_lock(&mutex); bloqueia para iniciar secção critica
pthread_mutex_unlock(&mutex); desbloqueia para encerrar secção critica e da liberdade a outro thread para entrar
pthread_join(threadId, NULL); para que um determinado thread espere por outro; no nosso caso temos mais q 2 e portanto temos q guardar o threadId[i] num array...
*/

typedef enum {
    N, E, S, O
} Acesso;

typedef enum {
    entrada, cheio, saida, fechado
} Evento;

struct Viatura {
    unsigned identificador;                                                                                             // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
    clock_t start;                                                                                                      // start time of vehicle
    clock_t start_gen;                                                                                                  // start time of generator
    pthread_mutex_t *mutex;                                                                                             // generator mutex
};

struct Viatura *create_viatura(unsigned identificador, unsigned tempo, Acesso acesso,
                               clock_t start, clock_t start_gen, pthread_mutex_t *mutex) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->identificador = identificador;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
    viatura->start = start;
    viatura->start_gen = start_gen;
    viatura->mutex = mutex;
    return viatura;
}

static unsigned parse_uint(char *str) {                                                                                 // to parse string argument to unsigned integer
    errno = 0;
    char *endptr;
    unsigned val;
    val = (unsigned) strtoul(str, &endptr, 10);
    if ((errno == ERANGE && val == UINT_MAX) || (errno != 0 && val == 0)) {
        perror("strtoul");
        return UINT_MAX;
    }
    if (endptr == str) {
        printf("parse_uint: no digits were found in %s \n", str);
        return UINT_MAX;
    }
    return val;                                                                                                         // successful conversion
}

void ticksleep(unsigned ticks, long ticks_seg) {                                                                        // sleep for desired number of clock ticks, with number of ticks per second given
    double seconds = (double)ticks / ticks_seg;
    struct timespec *req;
    req = malloc(sizeof(struct timespec));
    req->tv_sec = (time_t) (seconds);
    req->tv_nsec = (long) (seconds * SEC2NANO - req->tv_sec * SEC2NANO);
#ifdef DEBUG_GERADOR
    printf("sleep: %u ticks, %d sec, %ld nsec\n", ticks, (int)req->tv_sec, req->tv_nsec);
#endif
    nanosleep(req, NULL);
    free(req);
}

unsigned short create_log_file() {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return 0;
    fprintf(log_file, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"); 					                // first row in the gerador.log file
    fclose(log_file);
    return 1;
}

char* get_acesso(Acesso acesso) {
    switch (acesso) {
        case N:
            return "N";
        case E:
            return "E";
        case S:
            return "S";
        case O:
            return "O";
    }
    return NULL;
}

char* get_evento(Evento evento) {
    switch (evento) {
        case entrada:
            return "entrada";
        case cheio:
            return "cheio!";
        case saida:
            return "saida";
        case fechado:
            return "fechado!";
    }
    return NULL;
}

char* get_fifo(Acesso acesso) {
    switch (acesso) {
        case N:
            return FIFO_N;
        case E:
            return FIFO_E;
        case S:
            return FIFO_S;
        case O:
            return FIFO_O;
    }
    return NULL;
}

unsigned short log_event(struct Viatura *viatura, Evento evento) {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return 0;
    struct tms now_tms;
    clock_t now = times(&now_tms);
    fprintf(log_file, "%8d ; ", (int) (now - viatura->start_gen));
    fprintf(log_file, "%7d ;      ", viatura->identificador);
    fprintf(log_file, "%s ; ", get_acesso(viatura->acesso));
    fprintf(log_file, "%10d ; ", viatura->tempo);
    if (evento == saida)
        fprintf(log_file, "%6d ; ", (int) (now - viatura->start));
    else
        fprintf(log_file, "     ? ; ");
    fprintf(log_file, "%s\n", get_evento(evento));
    fclose(log_file);
    return 1;
}

void *tracker_viatura(void *arg) {                                                                                      
    if(pthread_detach(pthread_self()) != 0){
        perror("error detaching vehicle thread\n");
        exit(3);
    }
    struct Viatura *viatura = (struct Viatura *) arg;
#ifdef DEBUG_GERADOR
    printf("tracker: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
    char fifo_viatura[FIFO_NAME_SIZE];
    snprintf(fifo_viatura, FIFO_NAME_SIZE, "%s %u", FIFO, viatura->identificador);
    if (mkfifo(fifo_viatura, S_IRWXU) != 0) {
        perror(fifo_viatura);
        free(viatura);
        exit(5);
    }
    pthread_mutex_lock(viatura->mutex);                                                                                 // lock mutex
    char *fifo_parque = get_fifo(viatura->acesso);
    int fd = open(fifo_parque, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        perror(fifo_parque);
        free(viatura);
        pthread_mutex_unlock(viatura->mutex);
        exit(6);
    }
    char fifo_content[FIFO_CONTENT];
    snprintf(fifo_content, FIFO_CONTENT, "%s ; %10u ; %10u; %s\n",
             get_acesso(viatura->acesso), viatura->tempo, viatura->identificador, fifo_viatura);
    if (write(fd, fifo_content, sizeof(fifo_content)) == -1) {
        printf("error writing to %s\n", fifo_parque);
        unlink(fifo_viatura);
        close(fd);
        free(viatura);
        pthread_mutex_unlock(viatura->mutex);
        exit(7);
    }
    close(fd);
#ifdef DEBUG_GERADOR
    printf("tracker: id %u write %s\n", viatura->identificador, fifo_parque);
#endif
    pthread_mutex_unlock(viatura->mutex);                                                                               // unlock mutex




    free(viatura);
    return NULL; 													                                                    // or pthread_exit(NULL);
};

int main(int argc, char *argv[]) {
    clock_t start_time;
    struct tms start_tms, now_tms;
    start_time = times(&start_tms);
    long ticks_seg = sysconf(_SC_CLK_TCK);
    unsigned t_geracao, u_relogio;
    if (argc != 3
        || (t_geracao = parse_uint(argv[1])) == UINT_MAX
        || (u_relogio = parse_uint(argv[2])) == UINT_MAX) {                                                             // number of arguments must be 2 and both arguments must be integers
        printf("Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }
    long t_geracao_ticks = t_geracao * ticks_seg;
    srand((unsigned) time(NULL));
    unsigned identificador = 0, tempo, pre_intervalo, intervalo;
    Acesso acesso;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    if (!create_log_file()) {
        printf("error creating log file for generator");
        exit(4);
    }
    while (times(&now_tms) - start_time < t_geracao_ticks) {
        acesso = (Acesso) (rand() % N_ACESSOS);                                                                         // randomly get N, E, S or O for park access
        tempo = (rand() % N_TEMPOS + 1) * u_relogio;                                                                    // random parking time
        pre_intervalo = (unsigned short) (rand() % N_INTERVALOS);
        if (pre_intervalo < PROB_INT_0) intervalo = 0;
        else if (pre_intervalo < PROB_INT_1) intervalo = u_relogio;
        else intervalo = 2 * u_relogio;
        ticksleep(intervalo, ticks_seg);                                                                                // sleep for random waiting period in clock ticks before generating vehicle
        struct Viatura *viatura = create_viatura(identificador, tempo, acesso, times(&now_tms), start_time, &mutex);    // create new vehicle
#ifdef DEBUG_GERADOR
        printf("viatura: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
        pthread_t thread_viatura;
        if (pthread_create(&thread_viatura, NULL, tracker_viatura, viatura) != 0) {                                     // create vehicle tracker thread
            printf("error creating vehicle thread");
            exit(2);
        }
        identificador++;                                                                                                // no pthread_detach(thread_viatura) - thread detaches itself
    }
    return 0;
}
