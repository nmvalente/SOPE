#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <unistd.h>

static unsigned long parse_ulong(char *str);

int main(int argc, char *argv[]) {
    clock_t start_time;
    struct tms start_tms;
    struct tms now_tms;
    start_time = times(&start_tms);
    long ticks_seg = sysconf(_SC_CLK_TCK);
    unsigned long t_geracao, u_relogio;
    if (argc != 3
        || (t_geracao = parse_ulong(argv[1])) == ULONG_MAX
        || (u_relogio = parse_ulong(argv[2])) == ULONG_MAX) {                                                                                                    // number of arguments must be 3
        fprintf(stderr, "Usage: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
        exit(1);
    }

    // create_MUTEX();
    while (((times(&now_tms)) - start_time)/ticks_seg < t_geracao) {
//        create_vehicle(vehicle)
//        create_vehicle_tracker(vehicle)
    }
}

static unsigned long parse_ulong(char *str) {
    errno = 0;
    char *endptr;
    unsigned long val;
    val = strtoul(str, &endptr, 10);
    if ((errno == ERANGE && val == ULONG_MAX) || (errno != 0 && val == 0)) {
        perror("strtoul");
        return ULONG_MAX;
    }
    if (endptr == str) {
        printf("parse_uint: no digits were found in %s \n", str);
        return ULONG_MAX;
    }
    /* Successful conversion */
    return val;
}

typedef enum {N, E, S, O} acesso;

struct Viatura {
    unsigned short tempo_est;
    acesso acesso_est;
};

struct Viatura *createViatura(unsigned short tempo_est, acesso acesso_est) {
    struct Viatura *viatura = malloc(sizeof(struct Viatura));
    viatura->tempo_est = tempo_est;
    viatura->acesso_est = acesso_est;
    return viatura;
}