#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct os_mutex os_mutex_t;
os_mutex_t* os_mutex_create(const char* name);
void os_mutex_lock(os_mutex_t* mutex);
int os_mutex_trylock(os_mutex_t* mutex);
void os_mutex_unlock(os_mutex_t* mutex);
void os_mutex_destroy(os_mutex_t* mutex);
void os_mutex_unlink(const char* name);

typedef struct os_shm os_shm_t;
os_shm_t* os_shm_create(const char* name, size_t size);
void* os_shm_get_ptr(os_shm_t* shm);
void os_shm_detach(os_shm_t* shm);
void os_shm_unlink(const char* name);

int32_t os_get_pid(void);
int os_get_executable_path(char* buf, size_t buf_size);
int32_t os_spawn_process(const char* app_path, const char* args[]);
int os_is_process_alive(int32_t pid);

void os_sleep_ms(uint32_t ms);
void os_get_time_string(char* buf, size_t buf_size);

int os_cli_has_input(void);
int os_cli_read_line(char* buf, size_t buf_size);

typedef void* (*os_thread_func_t)(void*);
typedef struct os_thread os_thread_t;
os_thread_t* os_thread_create(os_thread_func_t func, void* arg);
void os_thread_join(os_thread_t* thread);
void os_thread_destroy(os_thread_t* thread);

FILE* os_fopen_shared(const char* path, const char* mode);

#ifdef __cplusplus
}
#endif
#endif