#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <share.h>

FILE* os_fopen_shared(const char* path, const char* mode) {
    return _fsopen(path, mode, _SH_DENYNO);
}

struct os_mutex { HANDLE handle; };
os_mutex_t* os_mutex_create(const char* name) {
    os_mutex_t* m = malloc(sizeof(os_mutex_t));
    m->handle = CreateMutexA(NULL, FALSE, name);
    return (m->handle == NULL) ? (free(m), NULL) : m;
}
void os_mutex_lock(os_mutex_t* m) { WaitForSingleObject(m->handle, INFINITE); }
int os_mutex_trylock(os_mutex_t* m) { return (WaitForSingleObject(m->handle, 0) == WAIT_OBJECT_0) ? 1 : 0; }
void os_mutex_unlock(os_mutex_t* m) { ReleaseMutex(m->handle); }
void os_mutex_destroy(os_mutex_t* m) { if(m) { CloseHandle(m->handle); free(m); } }
void os_mutex_unlink(const char* name) { (void)name; }

struct os_shm { HANDLE handle; void* ptr; size_t size; };
os_shm_t* os_shm_create(const char* name, size_t size) {
    os_shm_t* s = malloc(sizeof(os_shm_t));
    s->handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name);
    if (s->handle == NULL) return (free(s), NULL);
    s->ptr = MapViewOfFile(s->handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (s->ptr == NULL) { CloseHandle(s->handle); return (free(s), NULL); }
    s->size = size; return s;
}
void* os_shm_get_ptr(os_shm_t* s) { return s ? s->ptr : NULL; }
void os_shm_detach(os_shm_t* s) {
    if (s) { if (s->ptr) UnmapViewOfFile(s->ptr); if (s->handle) CloseHandle(s->handle); free(s); }
}
void os_shm_unlink(const char* name) { (void)name; }

int32_t os_get_pid(void) { return (int32_t)GetCurrentProcessId(); }
int os_get_executable_path(char* buf, size_t buf_size) {
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
    return (len > 0 && len < buf_size) ? 0 : -1;
}

int32_t os_spawn_process(const char* app_path, const char* args[]) {
    char cmd_line[1024] = {0};
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\"", app_path);
    for (int i = 0; args[i] != NULL; i++) { 
        strcat(cmd_line, " "); 
        strcat(cmd_line, args[i]); 
    }
    
    STARTUPINFOA si = {0}; 
    si.cb = sizeof(si); 
    PROCESS_INFORMATION pi = {0};
    
    if (CreateProcessA(app_path, cmd_line, NULL, NULL, FALSE, 
                        CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread); 
        CloseHandle(pi.hProcess);
        return (int32_t)pi.dwProcessId;
    }
    return -1;
}

int os_is_process_alive(int32_t pid) {
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
    if (h == NULL) return 0;
    DWORD res = WaitForSingleObject(h, 0); CloseHandle(h);
    return (res == WAIT_TIMEOUT) ? 1 : 0;
}

void os_sleep_ms(uint32_t ms) { Sleep(ms); }
void os_get_time_string(char* buf, size_t buf_size) {
    SYSTEMTIME st; GetLocalTime(&st);
    snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

#include <conio.h>
int os_cli_has_input(void) { return _kbhit() ? 1 : 0; }
int os_cli_read_line(char* buf, size_t buf_size) {
    size_t i = 0;
    while (_kbhit() && i < buf_size - 1) {
        char c = _getch();
        if (c == '\r' || c == '\n') break;
        if (c == '\b' && i > 0) { i--; printf("\b \b"); }
        else if (c >= 32 && c <= 126) { buf[i++] = c; printf("%c", c); }
    }
    buf[i] = '\0';
    if (i > 0) printf("\n");
    return (int)i;
}

struct thread_args { os_thread_func_t func; void* arg; };
DWORD WINAPI win_thread_proc(LPVOID param) {
    struct thread_args* args = (struct thread_args*)param;
    void* res = args->func(args->arg);
    free(args);
    return (DWORD)(uintptr_t)res;
}
struct os_thread { HANDLE handle; };
os_thread_t* os_thread_create(os_thread_func_t func, void* arg) {
    os_thread_t* t = malloc(sizeof(os_thread_t));
    struct thread_args* args = malloc(sizeof(struct thread_args));
    args->func = func; args->arg = arg;
    t->handle = CreateThread(NULL, 0, win_thread_proc, args, 0, NULL);
    return t;
}
void os_thread_join(os_thread_t* t) { WaitForSingleObject(t->handle, INFINITE); }
void os_thread_destroy(os_thread_t* t) { CloseHandle(t->handle); free(t); }