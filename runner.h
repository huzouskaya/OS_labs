#ifndef RUNNER_H
#define RUNNER_H
#include "platform.h"

typedef struct {
    os_shm_t* shm;
    os_mutex_t* mutex;
    int is_master;
    volatile int running;
} runner_context_t;

void* timer_thread_func(void* arg);
void* logger_thread_func(void* arg);
void* slave1_thread_func(void* arg);
void* slave2_thread_func(void* arg); 
void* spawner_thread_func(void* arg);
void* cli_thread_func(void* arg);
#endif