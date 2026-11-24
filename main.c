#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "platform.h"
#include "runner.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define SHM_NAME "LAB1_SHARED_MEM"
#define MUTEX_NAME "LAB1_MASTER_MUTEX"

volatile sig_atomic_t g_shutdown_requested = 0;

#ifdef _WIN32
BOOL WINAPI win_ctrl_handler(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_CLOSE_EVENT) { 
        g_shutdown_requested = 1; 
        return TRUE; 
    }
    return FALSE;
}
#else
void posix_signal_handler(int sig) {
    if (sig == SIGINT) g_shutdown_requested = 1;
}
#endif

typedef struct { int32_t counter; int32_t initialized; } shared_data_t;

void init_shared_memory(os_shm_t* shm, os_mutex_t* mutex) {
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(shm);
    os_mutex_lock(mutex);
    if (data->initialized == 0) {
        data->counter = 0; 
        data->initialized = 1;
        printf("[IPC] Разделяемая память инициализирована (counter = 0)\n");
    } else {
        printf("[IPC] Разделяемая память уже инициализирована (counter = %d)\n", data->counter);
    }
    os_mutex_unlock(mutex);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    int role = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role=slave1") == 0) role = 1;
        else if (strcmp(argv[i], "--role=slave2") == 0) role = 2;
    }

    printf("Процесс запущен. PID: %d, Роль: %s\n", 
            os_get_pid(), 
            role == 0 ? "возможный родитель" : (role == 1 ? "КОПИЯ_1" : "КОПИЯ_2"));

#ifdef _WIN32
    SetConsoleCtrlHandler(win_ctrl_handler, TRUE);
#else
    signal(SIGINT, posix_signal_handler);
    signal(SIGPIPE, SIG_IGN);
#endif

    os_shm_t* shm = os_shm_create(SHM_NAME, sizeof(shared_data_t));
    os_mutex_t* mutex = os_mutex_create(MUTEX_NAME);
    if (!shm || !mutex) { 
        fprintf(stderr, "[ERROR] Ошибка IPC!\n"); 
        return 1; 
    }

    init_shared_memory(shm, mutex);

    int is_master = 0;
    if (role == 0) {
        is_master = os_mutex_trylock(mutex);
        if (is_master) {
            printf("[MASTER] стал мастером\n");
            os_mutex_unlock(mutex);
        } else {
            printf("[SLAVE] мьютекс занят\n");
        }
    }

    if (role == 1) {
        printf("[SLAVE1] Запуск. PID: %d\n", os_get_pid());
        runner_context_t ctx = { .shm = shm, .mutex = mutex, .is_master = 0, .running = 1 };
        slave1_thread_func(&ctx); 
        os_shm_detach(shm);
        return 0;
    }

    if (role == 2) {
        printf("[SLAVE2] Запуск. PID: %d\n", os_get_pid());
        runner_context_t ctx = { .shm = shm, .mutex = mutex, .is_master = 0, .running = 1 };
        slave2_thread_func(&ctx); 
        os_shm_detach(shm);
        return 0;
    }

    runner_context_t ctx;
    ctx.shm = shm;
    ctx.mutex = mutex;
    ctx.is_master = is_master;
    ctx.running = 1;

    printf("[MAIN] Запуск потоков...\n");
    
    os_thread_t* t_cli = os_thread_create(cli_thread_func, &ctx);
    os_thread_t* t_timer = os_thread_create(timer_thread_func, &ctx);
    os_thread_t* t_logger = NULL;
    os_thread_t* t_spawner = NULL;

    if (is_master) {
        t_logger = os_thread_create(logger_thread_func, &ctx);
        t_spawner = os_thread_create(spawner_thread_func, &ctx);
        printf("[MAIN] Запущены: CLI, Timer, Logger, Spawner\n");
    } else {
        printf("[MAIN] Запущены: CLI, Timer\n");
    }

    printf("[MAIN] Ctrl+C или введите 'q' для завершения.\n");
    
    while (!g_shutdown_requested && ctx.running) {
        os_sleep_ms(500);
    }

    printf("\n[MAIN] Завершение работы...\n");
    ctx.running = 0;

    os_thread_join(t_cli); 
    os_thread_destroy(t_cli);
    printf("[MAIN] CLI поток завершён\n");
    
    os_thread_join(t_timer); 
    os_thread_destroy(t_timer);
    printf("[MAIN] Timer поток завершён\n");
    
    if (is_master) {
        if (t_logger) { 
            os_thread_join(t_logger); 
            os_thread_destroy(t_logger); 
            printf("[MAIN] Logger поток завершён\n");
        }
        if (t_spawner) { 
            os_thread_join(t_spawner); 
            os_thread_destroy(t_spawner); 
            printf("[MAIN] Spawner поток завершён\n");
        }
    }

    if (is_master) {
        printf("[MAIN] Очистка ресурсов Мастера...\n");
        os_mutex_destroy(mutex);
        os_mutex_unlink(MUTEX_NAME);
        os_shm_detach(shm);
        os_shm_unlink(SHM_NAME);
    } else {
        printf("[MAIN] Очистка ресурсов Слейва...\n");
        os_shm_detach(shm);
    }

    printf("[MAIN] Программа корректно завершена.\n");
    return 0;
}