#ifndef RUNNER_H
#define RUNNER_H

#ifdef _WIN32
	#include <windows.h>
	typedef PROCESS_INFORMATION ProcessHandle;
#else
	#include <sys/types.h>
	typedef pid_t ProcessHandle;
#endif

int bck_run(const char *program, const char *args, ProcessHandle *handle);
int proc_wait(ProcessHandle *handle, int *exit_code);

#endif
