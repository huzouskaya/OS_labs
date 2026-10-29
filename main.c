#include <stdio.h>
#include "runner.h"

int main() {
	ProcessHandle handle;
	int exit_code;

#ifdef _WIN32
	const char *prog = "ping";
	const char *args = "-n 3 127.0.0.1";
#else
	const char *prog = "sleep";
	const char *args = "2";
#endif

	printf("Backload: %s %s\n", prog, args);

	if (bck_run(prog, args, &handle) != 0) {
		fprintf(stderr, "Load error!\n");
		return 1;
	}

	if (proc_wait(&handle, &exit_code) == 0) {
		printf("Exit code is %d\n ", exit_code);
	}
	else {
		fprintf(stderr, "Wait error!\n");
	}
	return 0;
}
