#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>
#include <pthread.h>

#include "utils.h"

#define DEBUG           1

#define N_TEMPOS        10
#define N_INTERVALOS    10
#define PROB_INT_0      5
#define PROB_INT_1      8
#define LOG_FILE        "gerador.log"
#define LOCK_FILE        "tmp/gerador"

unsigned short create_log_file() {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return 0;
    fprintf(log_file, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"); 					                // first row in the gerador.log file
    fclose(log_file);
    return 1;
}

unsigned short log_event(Viatura *viatura, Evento evento) {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "a")) == NULL)
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
    fprintf(log_file, "%s\n", get_evento_ger(evento));
    fclose(log_file);
    return 1;
}

void close_exit(Viatura *viatura, Viat *viat, int clos, char *unlin, int exi) {
    if (clos >= 0) close(clos);
    if (unlin != NULL) unlink(unlin);
    free(viatura);
    free(viat);
    if (exi > 0)
        unlink(LOCK_FILE);
    rmdir(FIFO_DIR);
    if (exi > 0) exit(exi);
}

void *tracker_viatura(void *arg) {
    if(pthread_detach(pthread_self()) != 0) {
        perror("error detaching vehicle thread\n");
        exit(5);
    }
    Viatura *viatura = (Viatura *) arg;
#ifdef DEBUG
    printf("tracker: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
    Viat *viat = create_viat(viatura->identificador, viatura->tempo, viatura->acesso);
    if (mkfifo(viat->fifo, S_IRWXU) != 0) {
        perror(viat->fifo);
        close_exit(viatura, viat, -1, NULL, 6);
    }
#ifdef DEBUG
    printf("tracker: created %s\n", viat->fifo);
#endif
    pthread_mutex_lock(viatura->mutex_fifo);                                                                            // lock mutex fifo
    char *fifo_parque = get_fifo(viatura->acesso);
    int fd = open(fifo_parque, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        pthread_mutex_unlock(viatura->mutex_fifo);
        Evento evento = encerrado;
        pthread_mutex_lock(viatura->mutex_log);                                                                         // lock mutex log
        log_event(viatura, (Evento) evento);
        pthread_mutex_unlock(viatura->mutex_log);                                                                       // unlock mutex log
        close_exit(viatura, viat, -1, viat->fifo, 0);
        pthread_exit(NULL);
    }
    if (write(fd, viat, sizeof(Viat)) == -1) {
        printf("error writing to %s\n", fifo_parque);
        pthread_mutex_unlock(viatura->mutex_fifo);
        close_exit(viatura, viat, fd, viat->fifo, 7);
    }
    close(fd);
    pthread_mutex_unlock(viatura->mutex_fifo);                                                                          // unlock mutex fifo
#ifdef DEBUG
    printf("tracker: %s ; %10u ; %10u ; %s\n", get_acesso(viat->acesso), viat->tempo, viat->identificador, viat->fifo);
#endif
    fd = open(viat->fifo, O_RDONLY);
    if (fd == -1) {
        perror(viat->fifo);
        close_exit(viatura, viat, -1, viat->fifo, 8);
    }
    char evento;
#ifdef DEBUG
    printf("tracker: id %u waiting read %s\n", viat->identificador, viat->fifo);
#endif
    if (read(fd, &evento, sizeof(char)) == -1) {
        printf("error reading %s\n", viat->fifo);
        close_exit(viatura, viat, fd, viat->fifo, 9);
    }
    pthread_mutex_lock(viatura->mutex_log);                                                                             // lock mutex log
    log_event(viatura, (Evento) evento);
    pthread_mutex_unlock(viatura->mutex_log);                                                                           // unlock mutex log
    if (evento == entrada) {
        close(fd);
        fd = open(viat->fifo, O_RDONLY);
        if (fd == -1) {
            perror(viat->fifo);
            close_exit(viatura, viat, -1, viat->fifo, 10);
        }
        if (read(fd, &evento, sizeof(char)) == -1) {
            printf("error reading %s\n", viat->fifo);
            close_exit(viatura, viat, fd, viat->fifo, 11);
        }
        pthread_mutex_lock(viatura->mutex_log);                                                                         // lock mutex log
        log_event(viatura, (Evento) evento);
        pthread_mutex_unlock(viatura->mutex_log);                                                                       // unlock mutex log
    }
    close_exit(viatura, viat, fd, viat->fifo, 0);
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
    FILE *lock_file;
    if ((lock_file = fopen(LOCK_FILE, "w")) != NULL)
        fclose(lock_file);
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
        Viatura *viatura = create_viatura(identificador, tempo, acesso, times(&now_tms),
                                                 start_time, &mutex_fifo, &mutex_log);                                  // create new vehicle
#ifdef DEBUG
        printf("viatura: id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
#endif
        pthread_t thread_viatura;
        if (pthread_create(&thread_viatura, NULL, tracker_viatura, viatura) != 0) {                                     // create vehicle tracker thread
            printf("error creating vehicle thread");
            exit(4);
        }
        identificador++;
    }
    unlink(LOCK_FILE);
    rmdir(FIFO_DIR);
    pthread_exit(NULL);
}
