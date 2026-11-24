#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h> 

FILE* os_fopen_shared(const char* path, const char* mode) {
    return fopen(path, mode);
}

struct os_mutex { sem_t* sem; };
os_mutex_t* os_mutex_create(const char* name) {
    os_mutex_t* m = malloc(sizeof(os_mutex_t));
    m->sem = sem_open(name, O_CREAT | O_RDWR, 0666, 1);
    return (m->sem == SEM_FAILED) ? (free(m), NULL) : m;
}
void os_mutex_lock(os_mutex_t* m) { sem_wait(m->sem); }
int os_mutex_trylock(os_mutex_t* m) { return sem_trywait(m->sem) == 0 ? 1 : 0; }
void os_mutex_unlock(os_mutex_t* m) { sem_post(m->sem); }
void os_mutex_destroy(os_mutex_t* m) { if(m) { sem_close(m->sem); free(m); } }
void os_mutex_unlink(const char* name) { sem_unlink(name); }

struct os_shm { int fd; void* ptr; size_t size; };
os_shm_t* os_shm_create(const char* name, size_t size) {
    os_shm_t* s = malloc(sizeof(os_shm_t));
    int created = 0;
    s->fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (s->fd == -1) {
        s->fd = shm_open(name, O_RDWR, 0666);
        if (s->fd == -1) return (free(s), NULL);
    } else { created = 1; }
    if (created) ftruncate(s->fd, size);
    s->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, s->fd, 0);
    if (s->ptr == MAP_FAILED) { close(s->fd); return (free(s), NULL); }
    s->size = size;
    return s;
}
void* os_shm_get_ptr(os_shm_t* s) { return s ? s->ptr : NULL; }
void os_shm_detach(os_shm_t* s) {
    if (s) { if (s->ptr) munmap(s->ptr, s->size); if (s->fd != -1) close(s->fd); free(s); }
}
void os_shm_unlink(const char* name) { shm_unlink(name); }

int32_t os_get_pid(void) { return (int32_t)getpid(); }
int os_get_executable_path(char* buf, size_t buf_size) {
    ssize_t len = readlink("/proc/self/exe", buf, buf_size - 1);
    if (len == -1) {
        return -1;
    }
    buf[len] = '\0';
    return 0;
}
int32_t os_spawn_process(const char* app_path, const char* args[]) {
    pid_t pid = fork();
    if (pid == 0) { execv(app_path, (char* const*)args); exit(1); }
    return (int32_t)pid;
}
int os_is_process_alive(int32_t pid) {
    int status; return (waitpid(pid, &status, WNOHANG) == 0) ? 1 : 0;
}

void os_sleep_ms(uint32_t ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
void os_get_time_string(char* buf, size_t buf_size) {
    struct timeval tv; struct tm tm_info;
    gettimeofday(&tv, NULL); localtime_r(&tv.tv_sec, &tm_info);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", &tm_info);
    snprintf(buf + strlen(buf), buf_size - strlen(buf), ".%03ld", tv.tv_usec / 1000);
}

int os_cli_has_input(void) {
    fd_set fds; struct timeval tv = {0, 0};
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0 ? 1 : 0;
}

int os_cli_read_line(char* buf, size_t buf_size) {
    if (fgets(buf, buf_size, stdin) == NULL) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return (int)strlen(buf);
}

struct os_thread { pthread_t id; };
os_thread_t* os_thread_create(os_thread_func_t func, void* arg) {
    os_thread_t* t = malloc(sizeof(os_thread_t));
    pthread_create(&t->id, NULL, func, arg);
    return t;
}
void os_thread_join(os_thread_t* t) { pthread_join(t->id, NULL); }
void os_thread_destroy(os_thread_t* t) { free(t); }