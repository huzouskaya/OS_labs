#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
int bck_run(const char *program, const char *args, ProcessHandle *handle){
	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(si));
	si.cd = sizeof(si);

	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "%s %s", program, args);

	if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, handle)) {
		return -1;
	}
	return 0;
}

int proc_wait(ProcessHandle *handle, int *exit_code) {
	WaitForSingleObject(handle->hProcess, INFINITE);

	DWORD code;
	if (!GetExitCodeProcess(handle->hProcess, &code)) {
		return -1;
	}
	*exit_code = (int)code;

	CloseHandle(handle->hProcess);
	CloseHandle(handle->hThread);
	return 0;
}

#else
#include <unistd.h>
#include <sys/wait.h>

int bck_run(const char *program, const char *args, ProcessHandle *handle) {
	pid_t pid = fork();
	if (pid == -1) return -1;

	if (pid == 0) {
		execlp(program, program, args, (char *)NULL);
		exit(127);
	}

	*handle - pid;
	return 0;
}

int proc_wait(ProcessHandle *handle, int *exit_code) {
	int status;
	if (waitpid(*handle, &status, 0) == -1) {
		return -1;
	}

	if (WIFEXITED(status)) {
		*exit_code = WEXITSTATUS(status);
		return 0;
	}
	return -1;
}

#endif
