#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <unistd.h>
#include <pthread.h>

#define N_ACESSOS       4
#define N_TEMPOS        10
#define N_INTERVALOS    10
#define PROB_INT_0      5
#define PROB_INT_1      8
#define SEC2NANO        1000000000
#define LOG_FILE        "gerador.log"

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
};

struct Viatura *create_viatura(unsigned identificador, unsigned tempo,
                               Acesso acesso, clock_t start, clock_t start_gen) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->identificador = identificador;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
    viatura->start = start;
    viatura->start_gen = start_gen;
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
    printf("sleep: %u ticks, %d sec, %ld nsec\n", ticks, (int)req->tv_sec, req->tv_nsec);
    nanosleep(req, NULL);
    free(req);
}

void *tracker_viatura(void *arg) {                                                                                      // ?? mudar nome disto e ainda não sei muito bem o que tem que fazer
    if(pthread_detach(pthread_self()) != 0){
        perror("thread detach failed\n");
        exit(3);
    }
    struct Viatura *viatura = (struct Viatura *) arg;
    printf("tracker: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
    free(viatura);
    return NULL; 													                                                    // or pthread_exit(NULL);
};

int main(int argc, char *argv[]) {
    clock_t start_time;
    struct tms start_tms;
    struct tms now_tms;
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
    unsigned identificador = 0;
    unsigned tempo;
    unsigned short pre_intervalo;
    unsigned intervalo;
    Acesso acesso;
    // create_MUTEX();
    while (times(&now_tms) - start_time < t_geracao_ticks) {
        acesso = (Acesso) (rand() % N_ACESSOS);                                                                         // randomly get N, E, S or O for park access
        tempo = (rand() % N_TEMPOS + 1) * u_relogio;                                                                    // random parking time
        pre_intervalo = (unsigned short) (rand() % N_INTERVALOS);
        if (pre_intervalo < PROB_INT_0) intervalo = 0;
        else if (pre_intervalo < PROB_INT_1) intervalo = u_relogio;
        else intervalo = 2 * u_relogio;
        ticksleep(intervalo, ticks_seg);                                                                                // sleep for random waiting period in clock ticks before generating vehicle
        struct Viatura *viatura = create_viatura(identificador, tempo, acesso, times(&now_tms), start_time);            // create new vehicle
        printf("viatura: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
        pthread_t thread_viatura;
        if (pthread_create(&thread_viatura, NULL, tracker_viatura, viatura) != 0) {                                     // create vehicle tracker thread
            printf("Error creating vehicle thread!");
            exit(2);
        }
        //pthread_detach(thread_viatura);
        identificador++;
    }
    return 0;
}

FILE *create_log_file() {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return NULL;
    fprintf(log_file, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"); 					                // 1st row in the gerador.log file
    return log_file;
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

void log_event(FILE *log_file, struct Viatura *viatura, Evento evento) {
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
}
