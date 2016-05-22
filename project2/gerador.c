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

typedef enum {
    N, E, S, O
} Acesso;

struct Viatura {
    unsigned identificador;                                                                                             // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
};

struct Viatura *create_viatura(unsigned identificador, unsigned tempo, Acesso acesso) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->identificador = identificador;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
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
    req->tv_nsec = (long) (seconds * SEC2NANO);
    nanosleep(req, NULL);
    free(req);
}

void *tracker_viatura(void *arg) {                                                                                      // ?? mudar nome disto e ainda não sei muito bem o que tem que fazer
    struct Viatura *viatura = (struct Viatura *) arg;
    printf("id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
    exit(0);
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
    // create_MUTEX();
    srand((unsigned) time(NULL));
    unsigned identificador = 0;
    unsigned tempo;
    unsigned short pre_intervalo;
    unsigned intervalo;
    Acesso acesso;
    while ((double)((times(&now_tms)) - start_time) / ticks_seg < t_geracao) {
        acesso = (Acesso) (rand() % N_ACESSOS);                                                                         // randomly get N, E, S or O for park access
        tempo = (rand() % N_TEMPOS + 1) * u_relogio;                                                                    // random parking time
        pre_intervalo = (unsigned short) (rand() % N_INTERVALOS);
        if (pre_intervalo < PROB_INT_0) intervalo = 0;
        else if (pre_intervalo < PROB_INT_1) intervalo = u_relogio;
        else intervalo = 2 * u_relogio;
        ticksleep(intervalo, ticks_seg);                                                                                // sleep for random waiting period in clock ticks before generating vehicle
        struct Viatura *viatura = create_viatura(identificador, tempo, acesso);                                         // create new vehicle
        printf("id %u, t %u, a %u\n", viatura->identificador, viatura->tempo, viatura->acesso);
        pthread_t thread_viatura;
        pthread_create(&thread_viatura, NULL, tracker_viatura, viatura);                                                  // create vehicle tracker thread
        identificador++;
    }
    pthread_exit(NULL);
}

