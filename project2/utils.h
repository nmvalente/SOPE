#ifndef PROJECT2_UTILS_H
#define PROJECT2_UTILS_H

#define N_ACESSOS       4
#define SEC2NANO        1000000000
#define FIFO_DIR        "tmp/"
#define FIFO_N          "tmp/fifoN"
#define FIFO_E          "tmp/fifoE"
#define FIFO_S          "tmp/fifoS"
#define FIFO_O          "tmp/fifoO"
#define FIFO            "tmp/fifo"
#define FIFO_NAME_SIZE  19
#define FIFO_PARQ_SIZE  50
#define FIFO_MODE       0660

typedef enum {
    N, E, S, O
} Acesso;

typedef enum {
    entrada, cheio, saida, encerrado
} Evento;

struct Viatura {
    int identificador;                                                                                                  // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
    clock_t start;                                                                                                      // start time of vehicle
    clock_t start_gen;                                                                                                  // start time of generator
    pthread_mutex_t *mutex_fifo;                                                                                        // generator mutex for fifo
    pthread_mutex_t *mutex_log;                                                                                         // generator mutex for log
};

struct Viatura *create_viatura(int identificador, unsigned tempo, Acesso acesso, clock_t start,
                               clock_t start_gen, pthread_mutex_t *mutex_fifo, pthread_mutex_t *mutex_log);

char* get_acesso(Acesso acesso);

char* get_evento(Evento evento);

char* get_fifo(Acesso acesso);

void get_fifo_viatura(struct Viatura *viatura, char *fifo_viatura);

void get_fifo_parque_content(struct Viatura *viatura, char *fifo_parque_content, char *fifo_viatura);

unsigned parse_uint(char *str);

void ticksleep(unsigned ticks, long ticks_seg);

#endif //PROJECT2_UTILS_H
