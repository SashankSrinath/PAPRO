#define _GNU_SOURCE
#include "threads.h"
#include "play_utils.h"
#include<stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include<time.h>

#define PERIOD_MS 50

bool data[MAP_WIDTH][MAP_HEIGHT];
int quit = 0;
int xrange,yrange,is,js;

bool object_detected, current_thread;
double worst_time = 0.0;

pthread_mutex_t mutex;
pthread_barrier_t barrier;

pthread_mutex_t* get_address(){
    return &mutex;
}

pthread_barrier_t* get_barrier(){
    return &barrier;
}

void timespec_add(const struct timespec *a, const struct timespec *b, struct timespec *res)
{
    res->tv_nsec = a->tv_nsec + b->tv_nsec;
    res->tv_sec = a->tv_sec + b->tv_sec;
    while (res->tv_nsec >= 1000000000)
    {
        res->tv_nsec -= 1000000000;
        res->tv_sec += 1;
    }
}

void *GUI(void *args)
{
    play_init();

    struct timespec next, period = {0, PERIOD_MS * 1000000};
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (quit == 0)
    {
        timespec_add(&next, &period, &next);
        play_move_ball();
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}

void *generate_data(void *args)
{

    struct timespec next, period = {0, PERIOD_MS * 1000000};
    clock_gettime(CLOCK_MONOTONIC, &next);
    while (quit == 0)
    {
        timespec_add(&next, &period, &next);
        for (int i = 0; i < MAP_WIDTH; i++)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            {
                data[i][j] = ((i >= getBall()->x) && (i <= getBall()->x + BALL_WIDTH) && (j >= getBall()->y) && (j >= getBall()->y + BALL_HEIGHT)) ? true : false;
            }
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}

// pthread code -----------------------------------------------

void *locate_object(void *args){
    double cpu_time_used;
    clock_t start, end;
    struct timespec next, period = {0, PERIOD_MS * 1000000};
    clock_gettime(CLOCK_MONOTONIC, &next);
    
    int *t_id = (int*)args;
    int id = *t_id;

    cpu_set_t cpuset;
    int cpu = id;
    CPU_ZERO(&cpuset);
    CPU_SET( cpu , &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    while(quit == 0){
		pthread_mutex_lock(&mutex);
        object_detected = false;
        pthread_mutex_unlock(&mutex);

        start = clock();
        
        pthread_barrier_wait(&barrier);
        for (int i = id; i < MAP_WIDTH; i+=num_threads)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            { 
                if(data[i][j]){
					pthread_mutex_lock(&mutex);
                    object_detected = true;
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                if(object_detected) break;
            }
        }
		pthread_barrier_wait(&barrier);
        end = clock();
        cpu_time_used = ((end - start)) /(double) CLOCKS_PER_SEC;
        pthread_mutex_lock(&mutex);
        if(cpu_time_used>worst_time) worst_time=cpu_time_used;
        pthread_mutex_unlock(&mutex);
        printf("Time to detect object : %lf, worst time: %lf\n",cpu_time_used, worst_time);                    
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}

//openMP code -----------------------------------------------

void locate_open_mp(){
int nthreads = 0;
double cpu_time_used;
clock_t start, end;
struct timespec next, period = {0, PERIOD_MS * 1000000};
clock_gettime(CLOCK_MONOTONIC, &next);
int id = omp_get_thread_num();
#pragma omp parallel
{
    nthreads = omp_get_num_threads();
}
#pragma omp parallel
{
    while(quit == 0)
    {
        #pragma omp critical
        {
            object_detected = false;
        } 
        start = clock();
        #pragma omp barrier
        for (int i = id; i < MAP_WIDTH; i+=nthreads)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            { 
                if(data[i][j]){
		        #pragma omp critical
                {
                    object_detected = false;
                } 
                break;
                }
                if(object_detected) break;
            }
        }
        #pragma omp barrier
        end = clock();
        cpu_time_used = ((end - start)) /(double) CLOCKS_PER_SEC;
                #pragma omp critical
                {
                    if(cpu_time_used>worst_time) worst_time=cpu_time_used;
                }   
        printf("Time to detect object : %lf, worst time: %lf\n",cpu_time_used, worst_time);                    
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}
}


//openMP_for code -----------------------------------------------

void open_mp_for(){
int nthreads = 0;

double cpu_time_used;
clock_t start, end;
struct timespec next, period = {0, PERIOD_MS * 1000000};
clock_gettime(CLOCK_MONOTONIC, &next);

#pragma omp parallel
{
    nthreads = omp_get_num_threads();
}
#pragma omp parallel
{
    while(quit == 0)
    {
        #pragma omp critical
        {
            object_detected = false;
        }   
        start = clock();

#pragma omp barrier

#pragma omp parallel for schedule (dynamic, MAP_HEIGHT/nthreads)
        for (int i = 0; i < MAP_WIDTH; i++)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            { 
                if(data[i][j]){
                #pragma omp critical
                {
                    object_detected = true;
                }                           
                break;
                }
                if(object_detected) break;
            }
        }

#pragma omp barrier
        end = clock();
        cpu_time_used = ((end - start)) /(double) CLOCKS_PER_SEC;
        #pragma omp critical
        {
            if(cpu_time_used>worst_time) worst_time=cpu_time_used;
        }   
        printf("Time to detect object : %lf, worst time: %lf\n",cpu_time_used, worst_time);                    
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}
}


//openMP_for code with restricted range ---------- (TBD) -------------------------------

void open_mp_for_restrict(){
int nthreads = 0;

double cpu_time_used;
clock_t start, end;
struct timespec next, period = {0, PERIOD_MS * 1000000};
clock_gettime(CLOCK_MONOTONIC, &next);

# pragma omp parallel
{
    nthreads = omp_get_num_threads();
}
#pragma omp parallel
{
    while(quit == 0)
    {
        #pragma omp critical
        {
            object_detected = false;
        }   
        start = clock();

#pragma omp barrier

#pragma omp parallel for schedule (dynamic, MAP_HEIGHT/nthreads)

        for (int i = 0; i < MAP_WIDTH; i++)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            { 
                if(data[i][j]){
                #pragma omp critical
                {
                    object_detected = true;
                }                           
                break;
                }
                if(object_detected) break;
            }
        }

# pragma omp barrier
        end = clock();
        cpu_time_used = ((end - start)) /(double) CLOCKS_PER_SEC;
        #pragma omp critical
        {
            if(cpu_time_used>worst_time) worst_time=cpu_time_used;
        }   
        printf("Time to detect object : %lf, worst time: %lf\n",cpu_time_used, worst_time);                    
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}
}

/* Serial code

void *locate_object(void *args){
    bool stop;
    clock_t start, end;
    double cpu_time_used, worst_time;
    struct timespec next, period = {0, PERIOD_MS * 1000000};
    clock_gettime(CLOCK_MONOTONIC, &next);
    worst_time = 0.0;

    while(quit == 0){
        start = clock();
        for (int i = 0; i < MAP_WIDTH; i++)
        {
            for (int j = 0; j < MAP_HEIGHT; j++)
            {
                stop = false;
                if(data[i][j]){
                    //printf("Object at i: %d, j: %d\n",i,j);
                    object_detected = true;
                    stop = true;
                    break;
                }
                if(stop) break;
            }
        }
        end = clock();
        cpu_time_used = ((end - start)) /(double) CLOCKS_PER_SEC;
        if(cpu_time_used>worst_time){worst_time=cpu_time_used;}
        printf("Time to detect object : %lf, worst time: %lf\n",cpu_time_used, worst_time);                    
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }
}

*/