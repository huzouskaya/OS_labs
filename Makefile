test_runner: main.o runner.o
	gcc main.o runner.o -o test_runner

main.o: main.c runner.h
	gcc -c main.c

runner.o: runner.c runner.h
	gcc -c runner.c
