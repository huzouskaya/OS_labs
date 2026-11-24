#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int32_t counter; int32_t initialized; } shared_data_t;
#define LOG_FILE "lab1_log.txt"

void* timer_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(ctx->shm);
    while (ctx->running) {
        os_sleep_ms(300);
        if (!ctx->running) break;
        os_mutex_lock(ctx->mutex);
        data->counter += 1;
        os_mutex_unlock(ctx->mutex);
    }
    return NULL;
}

void* logger_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(ctx->shm);
    FILE* log_file = os_fopen_shared(LOG_FILE, "a");
    if (!log_file) return NULL;

    while (ctx->running) {
        os_sleep_ms(1000);
        if (!ctx->running) break;
        char time_buf[32];
        os_get_time_string(time_buf, sizeof(time_buf));
        os_mutex_lock(ctx->mutex);
        int32_t current_counter = data->counter;
        os_mutex_unlock(ctx->mutex);
        fprintf(log_file, "[%s] PID: %d, Counter: %d\n", time_buf, os_get_pid(), current_counter);
        fflush(log_file);
    }
    fclose(log_file);
    return NULL;
}

/**
 * Копия 1: 
 * - записывает в лог PID и время запуска
 * - увеличивает счётчик на 10
 * - записывает время выхода и завершается
 */
void* slave1_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(ctx->shm);
    
    FILE* log_file = os_fopen_shared(LOG_FILE, "a");
    if (!log_file) {
        fprintf(stderr, "[SLAVE1] Ошибка открытия лог-файла!\n");
        return NULL;
    }
    
    char time_buf[32];
    os_get_time_string(time_buf, sizeof(time_buf));
    fprintf(log_file, "[SLAVE1 START] %s PID: %d\n", time_buf, os_get_pid());
    fflush(log_file); 

    os_mutex_lock(ctx->mutex);
    data->counter += 10;
    int32_t new_value = data->counter;
    os_mutex_unlock(ctx->mutex);
    
    printf("[SLAVE1] Счётчик увеличен на 10. Новое значение: %d\n", new_value);
    
    os_get_time_string(time_buf, sizeof(time_buf));
    fprintf(log_file, "[SLAVE1 END] %s PID: %d, Counter: %d\n", 
            time_buf, os_get_pid(), new_value);
    fflush(log_file);
    
    fclose(log_file);
    printf("[SLAVE1] Завершена.\n");
    return NULL;
}

/**
 * Копия 2:
 * - записывает в лог PID и время запуска
 * - увеличивает счётчик в 2 раза
 * - ждёт 2 секунды
 * - уменьшает счётчик в 2 раза
 * - записывает время выхода и завершается
 */
void* slave2_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(ctx->shm);
    
    FILE* log_file = os_fopen_shared(LOG_FILE, "a");
    if (!log_file) {
        fprintf(stderr, "[SLAVE2] Ошибка открытия лог-файла!\n");
        return NULL;
    }
    
    char time_buf[32];
    os_get_time_string(time_buf, sizeof(time_buf));
    fprintf(log_file, "[SLAVE2 START] %s PID: %d\n", time_buf, os_get_pid());
    fflush(log_file);
    
    os_mutex_lock(ctx->mutex);
    data->counter *= 2;
    int32_t after_multiply = data->counter;
    os_mutex_unlock(ctx->mutex);
    
    printf("[SLAVE2] Счётчик умножен на 2. Значение: %d\n", after_multiply);
    
    printf("[SLAVE2] Ожидание 2 секунды...\n");
    os_sleep_ms(2000);
    
    os_mutex_lock(ctx->mutex);
    data->counter /= 2;
    int32_t after_divide = data->counter;
    os_mutex_unlock(ctx->mutex);
    
    printf("[SLAVE2] Счётчик разделён на 2. Значение: %d\n", after_divide);
    
    os_get_time_string(time_buf, sizeof(time_buf));
    fprintf(log_file, "[SLAVE2 END] %s PID: %d, Counter: %d\n", 
            time_buf, os_get_pid(), after_divide);
    fflush(log_file);
    
    fclose(log_file);
    printf("[SLAVE2] Завершена.\n");
    return NULL;
}

void* spawner_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    char exe_path[1024];
    if (os_get_executable_path(exe_path, sizeof(exe_path)) != 0) return NULL;

    const char* args1[] = { exe_path, "--role=slave1", NULL };
    const char* args2[] = { exe_path, "--role=slave2", NULL };
    int32_t pid1 = -1, pid2 = -1;

    while (ctx->running) {
        os_sleep_ms(3000);
        if (!ctx->running) break;

        if (pid1 == -1 || !os_is_process_alive(pid1)) {
            pid1 = os_spawn_process(exe_path, args1);
        }
        if (pid2 == -1 || !os_is_process_alive(pid2)) {
            pid2 = os_spawn_process(exe_path, args2);
        }
    }
    return NULL;
}

void* cli_thread_func(void* arg) {
    runner_context_t* ctx = (runner_context_t*)arg;
    shared_data_t* data = (shared_data_t*)os_shm_get_ptr(ctx->shm);
    printf("[CLI] Введите число для смены счетчика или 'q' для выхода:\n");

    while (ctx->running) {
        os_sleep_ms(100);
        if (!ctx->running) break;
        if (os_cli_has_input()) {
            char buf[64];
            int len = os_cli_read_line(buf, sizeof(buf));
            if (len > 0) {
                if (buf[0] == 'q' || buf[0] == 'Q') {
                    ctx->running = 0;
                    break;
                }
                int new_val = atoi(buf);
                os_mutex_lock(ctx->mutex);
                data->counter = new_val;
                os_mutex_unlock(ctx->mutex);
                printf("[CLI] Счетчик изменен на %d\n", new_val);
            }
        }
    }
    return NULL;
}