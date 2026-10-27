hello: hello.o
	gcc hello.o -o hello
hello.o: hello.c
	gcc -g -c hello.c -o hello.o
clean:
	rm -f hello hello.o
