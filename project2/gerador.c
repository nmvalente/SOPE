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

typedef enum {N, E, S, O} Acesso;

struct Viatura {
    unsigned identificador;
    unsigned intervalo;
    unsigned tempo;
    Acesso acesso;
};

struct Viatura *create_viatura(unsigned identificador, unsigned intervalo, unsigned tempo, Acesso acesso) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->identificador = identificador;
    viatura->intervalo = intervalo;
    viatura->tempo = tempo;
    viatura->acesso = acesso;
    return viatura;
}

static unsigned parse_uint(char *str) {
    errno = 0;
    char *endptr;
    unsigned val;
    val = (unsigned)strtoul(str, &endptr, 10);
    if ((errno == ERANGE && val == UINT_MAX) || (errno != 0 && val == 0)) {
        perror("strtoul");
        return UINT_MAX;
    }
    if (endptr == str) {
        printf("parse_uint: no digits were found in %s \n", str);
        return UINT_MAX;
    }
    /* Successful conversion */
    return val;
}

void *tracker_viatura(void *); // ?? mudar nome disto e ainda não sei muito bem o que tem que fazer

int main(int argc, char *argv[]) {
    clock_t start_time;
    struct tms start_tms;
    struct tms now_tms;
    start_time = times(&start_tms);
    long ticks_seg = sysconf(_SC_CLK_TCK);
    unsigned t_geracao, u_relogio;
    unsigned identificador = 0;
    unsigned tempo;
    unsigned short pre_intervalo;
    unsigned intervalo;
    Acesso acesso;
    if (argc != 3
        || (t_geracao = parse_uint(argv[1])) == UINT_MAX
        || (u_relogio = parse_uint(argv[2])) == UINT_MAX) {                                                             // number of arguments must be 3
        fprintf(stderr, "Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }
    // create_MUTEX();
    while (((times(&now_tms)) - start_time)/ticks_seg < t_geracao) {
        acesso = (Acesso) (rand() % N_ACESSOS);
        tempo = (rand() % N_TEMPOS + 1) * u_relogio;
        pre_intervalo = (unsigned short)(rand() % N_INTERVALOS);
        if (pre_intervalo < PROB_INT_0) intervalo = 0;
        else if (pre_intervalo < PROB_INT_1) intervalo = u_relogio;
        else intervalo = 2 * u_relogio;
        struct Viatura *viatura = create_viatura(identificador, intervalo, tempo, acesso);
        pthread_t *thread_viatua;
        pthread_create(thread_viatua, NULL, tracker_viatura, viatura);

//        create_vehicle_tracker(vehicle)
    }


}

