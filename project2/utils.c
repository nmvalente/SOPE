#include <stdlib.h>
#include <sys/times.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "utils.h"

struct Viatura *create_viatura(int identificador, unsigned tempo, Acesso acesso, clock_t start,
                               clock_t start_gen, pthread_mutex_t *mutex_fifo, pthread_mutex_t *mutex_log) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->identificador = identificador;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
    viatura->start = start;
    viatura->start_gen = start_gen;
    viatura->mutex_fifo = mutex_fifo;
    viatura->mutex_log = mutex_log;
    return viatura;
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
        case encerrado:
            return "encerrado!";
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

void get_fifo_viatura(struct Viatura *viatura, char *fifo_viatura) {
    snprintf(fifo_viatura, FIFO_NAME_SIZE, "%s%u", FIFO, viatura->identificador);
}

void get_fifo_parque_content(struct Viatura *viatura, char *fifo_parque_content, char *fifo_viatura) {
    snprintf(fifo_parque_content, FIFO_PARQ_SIZE, "%s ; %10u ; %10u ; %s\n",
             get_acesso(viatura->acesso), viatura->tempo, viatura->identificador, fifo_viatura);
}


unsigned parse_uint(char *str) {                                                                                 // to parse string argument to unsigned integer
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
