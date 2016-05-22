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


int main(int argc, char ** argv){
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
        exit(1);
    }

    parking_lots = atoi(argv[1]);
    int open_duration = atoi(argv[2]);

    if(parking_lots <= 0)
    {
        fprintf(stderr, "Insuficient number of parks");
        exit(2);
    }

    if(open_duration <= 0){
        fprintf(stderr, "Insuficient open time");
        exit(3);

    }

// create MUTEX

    pthread_mutex_t mutex_park = PTHREAD_MUTEX_INITIALIZER;

    int fdN, fdE, fdS, fdO;
    pthread_t tid_n, tid_e, tid_s, tid_o;

// create FIFOs

    mkfifo("/tmp/fifoN", 0600);
    mkfifo("/tmp/fifoE", 0600);
    mkfifo("/tmp/fifoS", 0600);
    mkfifo("/tmp/fifoO", 0600);

// create controller

    pthread_create(&tid_n, NULL, tcontroller, "/tmp/fifoN");
    pthread_create(&tid_e, NULL, tcontroller, "/tmp/fifoE");
    pthread_create(&tid_s, NULL, tcontroller, "/tmp/fifoS");
    pthread_create(&tid_o, NULL, tcontroller, "/tmp/fifoO");


// open FIFOs

    fdN = open("/tmp/fifoN", O_WRONLY);
    fdE = open("/tmp/fifoE", O_WRONLY);
    fdS = open("/tmp/fifoS", O_WRONLY);
    fdO = open("/tmp/fifoO", O_WRONLY);

    sleep(duration);
// park is open


// close park


    pthread_mutex_lock(&mutex_park);	 //wait MUTEX


    Viatura stop_vehicle = (Viatura*)malloc(sizeof(Viatura));
    stop_vehicle.identificador = -1;
    stop_vehicle.tempo = 0;
    stop_vehicle.acesso = 'N';

    write(fdN, &vehicle_stop, sizeof(Viatura));
    write(fdE, &vehicle_stop, sizeof(Viatura));
    write(fdS, &vehicle_stop, sizeof(Viatura));
    write(fdO, &vehicle_stop, sizeof(Viatura));
    close(fdN);
    close(fdE);
    close(fdS);
    close(fdO);

    //wait_controller_end
    pthread_join(&controller, NULL);

    pthread_mutex_unlock(&mutex_park);	 //signal MUTEX


    // todo destroy fifos - possible unlink(fdN); unlink(fdE); unlink(fdS); unlink(fdO);
    pthread_exit(NULL);
}
