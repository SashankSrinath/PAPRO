#ifndef THREADS_H
#define THREADS_H

#include <time.h>
#include <pthread.h>

#define num_threads 8

void * GUI(void *args);
void *generate_data(void *args); 
void *locate_object(void *args); // pthread function with affinity
void locate_open_mp(); // openmp function 
void open_mp_for(); //openmp parallel for function 

void timespec_add(const struct timespec *a, const struct timespec *b, struct timespec * res);

pthread_mutex_t* get_address();
pthread_barrier_t* get_barrier();

#endif
