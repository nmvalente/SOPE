#include <stdlib.h>
#include <sys/times.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "utils.h"

Viatura *create_viatura(int identificador, unsigned tempo, Acesso acesso, clock_t start,
                               clock_t start_gen, pthread_mutex_t *mutex_fifo, pthread_mutex_t *mutex_log) {
    Viatura *viatura = malloc(sizeof(Viatura));
    viatura->identificador = identificador;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
    viatura->start = start;
    viatura->start_gen = start_gen;
    viatura->mutex_fifo = mutex_fifo;
    viatura->mutex_log = mutex_log;
    return viatura;
}

Viat *create_viat(int identificador, unsigned tempo, Acesso acesso) {
    Viat *viat = malloc(sizeof(Viat));
    viat->identificador = identificador;
    viat->tempo = tempo;
    viat->acesso = acesso;
    snprintf(viat->fifo, FIFO_NAME_SIZE, "%s%u", FIFO, identificador);
    return viat;
}

Parking_Viat *create_parking_viat(int identificador, unsigned tempo, Acesso acesso, int n_lugares,
                                  unsigned *n_ocupados, clock_t start_par, pthread_mutex_t *mutex) {
    Parking_Viat *viat = malloc(sizeof(Parking_Viat));
    viat->identificador = identificador;
    viat->tempo = tempo;
    viat->acesso = acesso;
    snprintf(viat->fifo, FIFO_NAME_SIZE, "%s%u", FIFO, identificador);
    viat->n_lugares = n_lugares;
    viat->n_ocupados = n_ocupados;
    viat->start_par = start_par;
    viat->mutex = mutex;
    return viat;
}

Controlador *create_controlador(Acesso acesso, unsigned n_lugares, unsigned *n_ocupados,
                                clock_t start_par, pthread_mutex_t *mutex) {
    Controlador *controlador = malloc(sizeof(Controlador));
    controlador->acesso = acesso;
    controlador->n_lugares = n_lugares;
    controlador->n_ocupados = n_ocupados;
    controlador->start_par = start_par;
    controlador->mutex = mutex;
    return controlador;
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

char* get_evento_ger(Evento evento) {
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

char* get_evento_par(Evento evento) {
    switch (evento) {
        case entrada:
            return "estacionamento";
        case cheio:
            return "cheio";
        case saida:
            return "saida";
        case encerrado:
            return "encerrado";
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

unsigned parse_uint(char *str) {                                                                                        // to parse string argument to unsigned integer
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

struct timespec getTimeSpecTicks(unsigned ticks) {
    struct timespec ts;
    double seconds = (double)ticks / sysconf(_SC_CLK_TCK);
    ts.tv_sec = (time_t) (seconds);
    ts.tv_nsec = (long) (seconds * SEC2NANO - ts.tv_sec * SEC2NANO);
    return ts;
}

void ticksleep(unsigned ticks) {                                                                                        // sleep for desired number of clock ticks, with number of ticks per second given
    struct timespec timesleep = getTimeSpecTicks(ticks);
#ifdef DEBUG
    printf("sleep: %u ticks, %d sec, %ld nsec\n", ticks, (int)req->tv_sec, req->tv_nsec);
#endif
    nanosleep(&timesleep, NULL);
}
