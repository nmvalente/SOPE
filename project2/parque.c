#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/times.h>
#include <fcntl.h>

#include "utils.h"

#define DEBUG           1

#define LOG_FILE        "parque.log"

unsigned short create_log_file() {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "w")) == NULL)
        return 0;
    fprintf(log_file, "t(ticks) ; nlug ; id_viat ; observ\n");                                                          // first row in the parque.log file
    fclose(log_file);
    return 1;
}

unsigned short log_event(clock_t start_par, int n_ocupados, int identificador, Evento evento) {
    FILE *log_file;
    if ((log_file = fopen(LOG_FILE, "a")) == NULL)
        return 0;
    struct tms now_tms;
    clock_t now = times(&now_tms);
    fprintf(log_file, "%8d ; ", (int) (now - start_par));
    fprintf(log_file, "%4d ;      ", n_ocupados);
    fprintf(log_file, "%7d ; ", identificador);
    fprintf(log_file, "%s\n", get_evento_par(evento));
    fclose(log_file);
    return 1;
}

void close_exit_arru(Parking_Viat *viat, int clos, char *perro, int exi) {
    if (perro != NULL) perror(perro);
    if (clos >= 0) close(clos);
    free(viat);
    if (exi > 0) exit(exi);
    else pthread_exit(NULL);
}

void *arrumador_viatura(void *arg) {
    if(pthread_detach(pthread_self()) != 0) {
        perror("error detaching parking assistant thread\n");
        exit(10);
    }
    Parking_Viat *viat = (Parking_Viat*) arg;
    pthread_mutex_lock(viat->mutex);                                                                                    // lock mutex
    int fd = open(viat->fifo, O_WRONLY);
    if (fd == -1)
        close_exit_arru(viat, -1, viat->fifo, 11);
    if (viat->n_lugares - *viat->n_ocupados <= 0) {                                                                     // parking lot is full
        Evento evento = cheio;
        if (write(fd, &evento, sizeof(char)) == -1)
            close_exit_arru(viat, fd, viat->fifo, 12);
        log_event(viat->start_par, *viat->n_ocupados, viat->identificador, evento);
        pthread_mutex_unlock(viat->mutex);                                                                              // unlock mutex
        close_exit_arru(viat, fd, NULL, 0);
    }
    (*viat->n_ocupados)++;
    Evento evento = entrada;
    if (write(fd, &evento, sizeof(char)) == -1)
        close_exit_arru(viat, fd, viat->fifo, 13);
    close(fd);
    log_event(viat->start_par, *viat->n_ocupados, viat->identificador, evento);
    pthread_mutex_unlock(viat->mutex);                                                                                  // unlock mutex
    ticksleep(viat->tempo, sysconf(_SC_CLK_TCK));
    evento = saida;
    pthread_mutex_lock(viat->mutex);                                                                                    // lock mutex
    fd = open(viat->fifo, O_WRONLY);
    if (fd == -1)
        close_exit_arru(viat, -1, viat->fifo, 14);
    (*viat->n_ocupados)--;
    if (write(fd, &evento, sizeof(char)) == -1)
        close_exit_arru(viat, fd, viat->fifo, 15);
    close(fd);
    log_event(viat->start_par, *viat->n_ocupados, viat->identificador, evento);
    pthread_mutex_unlock(viat->mutex);                                                                                  // unlock mutex
    close_exit_arru(viat, -1, NULL, 0);
    return NULL;
}

void close_exit_cont(Controlador *controlador, Viat *viat, int clos, char *perro, int exi, int *retval) {
    if (perro != NULL) perror(perro);
    if (clos >= 0) close(clos);
    free(controlador);
    free(viat);
    *retval = exi;
    pthread_exit(retval);
}

Viat *readViat(int fd){
    Viat* viat = malloc(sizeof(Viat *));
    size_t size = sizeof(Viat);
    size_t read_size = 0;
    while(read_size != size){
        ssize_t value = read(fd, viat + read_size, size - read_size);
        if( value == -1 || value == 0) {
            free(viat);
            return NULL;
        }
        read_size += value;
    }
    return viat;
}

void *tracker_controlador(void *arg) {
    Controlador *controlador = (Controlador *) arg;
    char *fifo = get_fifo(controlador->acesso);
    int fdr = open(fifo, O_RDONLY);
    if (fdr == -1)
        close_exit_cont(controlador, NULL, -1, fifo, 5, controlador->retval);
    Viat *viat;
    int closed = 0;
    while ((viat = readViat(fdr)) != NULL) {
        if (viat->identificador == -1) {
            closed = 1;
            continue;
        }
        if (closed) {
            pthread_mutex_lock(controlador->mutex);                                                                     // lock mutex
            int fd = open(viat->fifo, O_WRONLY);
            if (fd == -1)
                close_exit_cont(controlador, viat, fdr, fifo, 7, controlador->retval);
            Evento evento = encerrado;
            if (write(fd, &evento, sizeof(char)) == -1) {
                close(fd);
                close_exit_cont(controlador, viat, fdr, fifo, 8, controlador->retval);
            }
            close(fd);
            log_event(controlador->start_par, *controlador->n_ocupados, viat->identificador, evento);
            pthread_mutex_unlock(controlador->mutex);                                                                   // unlock mutex
        }
        else {
            Parking_Viat *pviat = create_parking_viat(viat->identificador, viat->tempo, viat->acesso,
                                                      controlador->n_lugares, controlador->n_ocupados,
                                                      controlador->start_par, controlador->mutex);
            if (pthread_create(NULL, NULL, arrumador_viatura, pviat) != 0) {
                printf("error creating parking assistant thread");
                close_exit_cont(controlador, NULL, fdr, NULL, 9, controlador->retval);
            }
        }
    }
    unlink(fifo);
    close_exit_cont(controlador, NULL, fdr, NULL, 0, controlador->retval);
    return NULL;
}

int main(int argc, char **argv) {
    clock_t start_time;
    struct tms start_tms;
    start_time = times(&start_tms);
    unsigned t_abertura;
    unsigned n_lugares;
    if (argc != 3
        || (n_lugares = parse_uint(argv[1])) == UINT_MAX
        || (t_abertura = parse_uint(argv[2])) == UINT_MAX) {                                                            // number of arguments must be 2 and both arguments must be integers
        printf("Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }
    unsigned n_ocupados = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;                                                                  // create fifo mutex
    if (mkdir(FIFO_DIR, 0777) == -1 && errno != EEXIST) {                                                               // could not create directory and it is not because it already exists
        printf("error creating %s directory: %s\n", FIFO_DIR, strerror(errno));
        exit(2);
    }
    if (!create_log_file()) {
        printf("error creating log file for parking lot");
        exit(3);
    }
    int fd[N_ACESSOS];
    pthread_t tid[N_ACESSOS];
    char *fifos[N_ACESSOS] = {FIFO_N, FIFO_E, FIFO_S, FIFO_O};
    Controlador *controladores[4];
    int retvals[4];
    int i;
    for (i = 0; i < N_ACESSOS; i++) {
        mkfifo(fifos[i], FIFO_MODE);                                                                                    // create FIFO
        controladores[i] = create_controlador((Acesso) i, n_lugares, &n_ocupados, &retvals[i], start_time, &mutex);     // create controller
        pthread_create(&tid[i], NULL, tracker_controlador, controladores[i]);                                           // create controller tracker thread
        fd[i] = open(fifos[i], O_WRONLY);                                                                               // open FIFO
    }
    sleep(t_abertura);
    Viat *stop_vehicle = create_viat(-1, 0, N);
    pthread_mutex_lock(&mutex);                                                                                         // lock mutex
    int j;
    for (i = 0; i < N_ACESSOS; i++) {
        if (write(fd[i], stop_vehicle, sizeof(Viat)) == -1) {
            printf("error writing to %s\n", FIFO_N);
            for (j = 0; j < N_ACESSOS; j++) {
                unlink(fifos[j]);
                close(fd[j]);
            }
            free(stop_vehicle);
            exit(4);
        }
        close(fd[i]);
        pthread_join(tid[i], NULL);                                                                                     // wait controller end
    }
    pthread_mutex_unlock(&mutex);                                                                                       // unlock mutex
    free(stop_vehicle);
    for (j = 0; j < N_ACESSOS; j++) {
        unlink(fifos[j]);
    }
    pthread_exit(NULL);
}
