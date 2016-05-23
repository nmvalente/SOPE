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

typedef enum {
    N, E, S, O
} Acesso;

typedef enum {
    entrada, cheio, saida, encerrado
} Evento;

typedef struct {
    int identificador;                                                                                                  // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
    clock_t start;                                                                                                      // start time of vehicle
    clock_t start_gen;                                                                                                  // start time of generator
    pthread_mutex_t *mutex_fifo;                                                                                        // generator mutex for fifo
    pthread_mutex_t *mutex_log;                                                                                         // generator mutex for log
} Viatura;

typedef struct {
    int identificador;                                                                                                  // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
    char fifo[FIFO_NAME_SIZE];                                                                                          // vehicle fifo
} Viat;

typedef struct {
    int identificador;                                                                                                  // unique identifier for vehicle
    unsigned tempo;                                                                                                     // parking time for vehicle
    Acesso acesso;                                                                                                      // park access for vehicle
    char fifo[FIFO_NAME_SIZE];                                                                                          // vehicle fifo
    int n_lugares;                                                                                                      // park total number of spaces, -1 if park closed
    unsigned *n_ocupados;                                                                                               // park numbr of occupied spaces
    clock_t start_par;                                                                                                  // start time of park
    pthread_mutex_t *mutex;                                                                                             // park mutex
} Parking_Viat;

typedef struct {
    Acesso acesso;                                                                                                      // park access of controller
    unsigned n_lugares;                                                                                                 // total number of parking spaces
    unsigned *n_ocupados;                                                                                               // occupied parking spaces
    clock_t start_par;                                                                                                  // start time of park
    pthread_mutex_t *mutex;                                                                                             // park mutex
} Controlador;

Viatura *create_viatura(int identificador, unsigned tempo, Acesso acesso, clock_t start,
                               clock_t start_gen, pthread_mutex_t *mutex_fifo, pthread_mutex_t *mutex_log);

Viat *create_viat(int identificador, unsigned tempo, Acesso acesso);

Parking_Viat *create_parking_viat(int identificador, unsigned tempo, Acesso acesso, int n_lugares,
                                  unsigned *n_ocupados, clock_t start_par, pthread_mutex_t *mutex);

Controlador *create_controlador(Acesso acesso, unsigned n_lugares, unsigned *n_ocupados,
                                clock_t start_par, pthread_mutex_t *mutex);

char* get_acesso(Acesso acesso);

char* get_evento_ger(Evento evento);

char* get_evento_par(Evento evento);

char* get_fifo(Acesso acesso);

unsigned parse_uint(char *str);

void ticksleep(unsigned ticks, long ticks_seg);

#endif //PROJECT2_UTILS_H
