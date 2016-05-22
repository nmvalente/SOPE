#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <semaphore.h>
#include <fcntl.h>

#include "utils.h"


#define OUT_OF_PARK 0
#define IN_PARK	    1
#define FULL        2
#define CLOSED      3
#define MAX_LENGHT 5000


void *car_assistant(void *arg){

    Viatura vehicle = *(Viatura*) arg;
    int fd, info_park;

    char fifo[MAX_LENGHT];
    sprintf(fifo, "/tmp/viatura%d", vehicle.identificador);

    if((fd = open(fifo, O_WRONLY)) == -1){
        closeAndError(fifo, fd, 4);
        /*perror(fifo);
        unlink(fifo);
        close(fd);
        exit(4);*/
    }


    if(parking_lots <= 0) // if no places to store the vehicle, full park
    {
        info_park = FULL;
        if(write(fd, &info_park, sizeof(char)) == -1){
            closeAndError(fifo, fd, 5);
            /*perror(fifo);
            unlink(fifo);
            close(fd);
            exit(5);*/
        }
    }
    else{
        info_park = IN_PARK;
        if(write(fd, &info_park, sizeof(char)) == -1){
            closeAndError(fifo, fd, 6);
            /*perror(fifo);
            unlink(fifo);
            close(fd);
            exit(6);*/
        }
        parking_lots--;
        printf("vehicle.tempo = %d\n", vehicle.tempo);

        // passado algum tempo o carro  vai sair do parque, precisamos de contar um tempo qq aqui

        info_park = OUT_OF_PARK;
        if (write(fd, &info_park, sizeof(char)) == -1){
            closeAndError(fifo, fd, 7);
            /*perror(fifo);
            unlink(fifo);
            close(fd);
            exit(7);*/
        }
        parking_lots++;
    }
    return NULL;
}

void *controller(void *arg){
    char * fifo_gate = (char *) arg;
    int fdr, closed = 0;
    struct Viatura vehicle;
    char info;

    //open FIFO Read only
    if((fdr = open(fifo_portao, O_RDONLY)) == -1){
        closeAndError(fifo_gate, fdr, 8);
        /*perror(fifo_gate);
        unlink(fifo_gate);
        close(fdr);
        exit(8);*/
    }

    // FIFO not closed
    while(read(fdr, &vehicle, sizeof(Viatura)) != 0){
        if(vehicle.identificador == -1){ // if stop_vehicle
            closed = 1;
        }

        else if(closed){
            int fd_generator;
            char fifo[MAX_LENGHT];
            sprintf(fifo, "/tmp/viatura%d", vehicle.identificador);
            if((fd_generator = open(fifo, O_WRONLY)) == -1){
                closeAndError(fifo, fd_generator, 9);
                /*perror(fifo);
                unlink(fifo);
                close(fd_generator);
                exit(9);*/
            }
            info_park = CLOSED;
            if(write(fd_generator, &info_park, sizeof(char)) == -1){
                closeAndError(fifo, fd_generator, 10);
                /*perror(fifo);
                unlink(fifo);
                close(fd_generator);
                exit(10);*/
            }
        }
        else{
            pthread_t tid;
            if(pthread_create(&tid, NULL, car_assistant, &vehicle)){
                printf("thread failed.\n");
                exit(11);
            }
            pthread_detach(tid);
        }
    }
    close(fdr);
    unlink(fifo_gate);
    return NULL;
}

void closeAndError(char *fifo, int file_descriptor, int nr){
    perror(fifo);
    unlink(fifo);
    close(file_descriptor);
    exit(nr);
}

int main(int argc, char ** argv){
    unsigned open_duration;
    unsigned parking_lots;
    if (argc != 3
        || (parking_lots = parse_uint(argv[1])) == UINT_MAX
        || (open_duration = parse_uint(argv[2])) == UINT_MAX) {                                                         // number of arguments must be 2 and both arguments must be integers
        printf("Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }
    pthread_mutex_t mutex_park = PTHREAD_MUTEX_INITIALIZER;                                                             // create MUTEX
    int fd[N_ACESSOS];
    pthread_t tid[N_ACESSOS];
    char* fifos[N_ACESSOS] = {FIFO_N, FIFO_E, FIFO_S, FIFO_O};
    int i;
    for (i = 0; i < N_ACESSOS; i++) {
        mkfifo(fifos[i], FIFO_MODE);                                                                                    // create FIFO
        pthread_create(&tid[i], NULL, controller, fifos[i]);                                                            // create controller
        fd[i] = open(fifos[i], O_WRONLY);                                                                               // open FIFO
    }
    sleep(open_duration);
    struct Viatura *stop_vehicle = create_viatura(-1, 0, N, NULL, NULL, NULL, NULL);
    char fifo_viatura[FIFO_NAME_SIZE];
    get_fifo_viatura(stop_vehicle, fifo_viatura);
    char fifo_parque_content[FIFO_PARQ_SIZE];
    get_fifo_parque_content(stop_vehicle, fifo_parque_content, fifo_viatura);
    pthread_mutex_lock(&mutex_park);	                                                                                // wait MUTEX
    int j;
    for (i = 0; i < N_ACESSOS; i++) {
        if (write(fd[i], fifo_parque_content, sizeof(fifo_parque_content)) == -1) {
            printf("error writing to %s\n", FIFO_N);
            for (j = 0; j < N_ACESSOS; j++) {
                unlink(fifos[j]);
                close(fd[j]);
            }
            free(stop_vehicle);
            exit(4);
        }
        close(fd[i]);
        pthread_join(tid[i], NULL);                                                                                     //wait_controller_end

    }
    pthread_mutex_unlock(&mutex_park);	                                                                                //signal MUTEX
    free(stop_vehicle);
    for (j = 0; j < N_ACESSOS; j++) {
        unlink(fifos[j]);
    }
    pthread_exit(NULL);
}
