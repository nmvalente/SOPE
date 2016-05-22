#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>

#include "utils.h"

#define DEBUG_GERADOR   1

#define N_TEMPOS        10
#define N_INTERVALOS    10
#define PROB_INT_0      5
#define PROB_INT_1      8
#define LOG_FILE        "gerador.log"

/*
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; variavel global com inicializacao do mutex;
pthread_mutex_lock(&mutex); bloqueia para iniciar secção critica
pthread_mutex_unlock(&mutex); desbloqueia para encerrar secção critica e da liberdade a outro thread para entrar
pthread_join(threadId, NULL); para que um determinado thread espere por outro; no nosso caso temos mais q 2 e portanto temos q guardar o threadId[i] num array...
*/

unsigned short create_log_file() {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return 0;
    fprintf(log_file, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"); 					                // first row in the gerador.log file
    fclose(log_file);
    return 1;
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
        exit(5);
    }
    struct Viatura *viatura = (struct Viatura *) arg;
#ifdef DEBUG_GERADOR
    printf("tracker: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
    char fifo_viatura[FIFO_NAME_SIZE];
    get_fifo_viatura(viatura, fifo_viatura);
    if (mkfifo(fifo_viatura, FIFO_MODE) != 0) {
        perror(fifo_viatura);
        free(viatura);
        exit(6);
    }
    pthread_mutex_lock(viatura->mutex_fifo);                                                                            // lock mutex fifo
    char *fifo_parque = get_fifo(viatura->acesso);
    int fd = open(fifo_parque, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        perror(fifo_parque);
        free(viatura);
        pthread_mutex_unlock(viatura->mutex_fifo);
        exit(7);
    }
    char fifo_parque_content[FIFO_PARQ_SIZE];
    get_fifo_parque_content(viatura, fifo_parque_content, fifo_viatura);
    if (write(fd, fifo_parque_content, sizeof(fifo_parque_content)) == -1) {
        printf("error writing to %s\n", fifo_parque);
        unlink(fifo_viatura);
        close(fd);
        free(viatura);
        pthread_mutex_unlock(viatura->mutex_fifo);
        exit(8);
    }
    close(fd);
    pthread_mutex_unlock(viatura->mutex_fifo);                                                                          // unlock mutex fifo
#ifdef DEBUG_GERADOR
    printf("%s", fifo_parque_content);
#endif
    fd = open(fifo_viatura, O_RDONLY);
    if (fd == -1) {
        perror(fifo_viatura);
        unlink(fifo_viatura);
        free(viatura);
        exit(9);
    }
    char evento;
#ifdef DEBUG_GERADOR
    printf("tracker: id %u waiting read %s\n", viatura->identificador, fifo_viatura);
#endif
    if (read(fd, &evento, sizeof(char)) == -1) {
        printf("error reading %s\n", fifo_viatura);
        unlink(fifo_viatura);
        close(fd);
        free(viatura);
        exit(10);
    }
    pthread_mutex_lock(viatura->mutex_log);                                                                             // lock mutex log
    log_event(viatura, (Evento) evento);
    pthread_mutex_unlock(viatura->mutex_log);                                                                           // unlock mutex log
    if (evento == entrada) {
        if (read(fd, &evento, sizeof(char)) == -1) {
            printf("error reading %s\n", fifo_viatura);
            unlink(fifo_viatura);
            close(fd);
            free(viatura);
            exit(11);
        }
        pthread_mutex_lock(viatura->mutex_log);                                                                         // lock mutex log
        log_event(viatura, (Evento) evento);
        pthread_mutex_unlock(viatura->mutex_log);                                                                       // unlock mutex log
    }
    unlink(fifo_viatura);
    close(fd);
    free(viatura);
    pthread_exit(NULL);
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
    pthread_mutex_t mutex_fifo = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
    if (mkdir(FIFO_DIR, 0777) == -1 && errno != EEXIST) {                                                               // could not create directory and it is not because it already exists
        printf("error creating %s directory: %s\n", FIFO_DIR, strerror(errno));
        exit(2);
    }
    if (!create_log_file()) {
        printf("error creating log file for generator");
        exit(3);
    }
    while (times(&now_tms) - start_time < t_geracao_ticks) {
        acesso = (Acesso) (rand() % N_ACESSOS);                                                                         // randomly get N, E, S or O for park access
        tempo = (rand() % N_TEMPOS + 1) * u_relogio;                                                                    // random parking time
        pre_intervalo = (unsigned short) (rand() % N_INTERVALOS);
        if (pre_intervalo < PROB_INT_0) intervalo = 0;
        else if (pre_intervalo < PROB_INT_1) intervalo = u_relogio;
        else intervalo = 2 * u_relogio;
        ticksleep(intervalo, ticks_seg);                                                                                // sleep for random waiting period in clock ticks before generating vehicle
        struct Viatura *viatura = create_viatura(identificador, tempo, acesso, times(&now_tms),
                                                 start_time, &mutex_fifo, &mutex_log);                                  // create new vehicle
#ifdef DEBUG_GERADOR
        printf("viatura: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
        pthread_t thread_viatura;
        if (pthread_create(&thread_viatura, NULL, tracker_viatura, viatura) != 0) {                                     // create vehicle tracker thread
            printf("error creating vehicle thread");
            exit(4);
        }
        identificador++;                                                                                                // no pthread_detach(thread_viatura) - thread detaches itself
    }
    return 0;
}
