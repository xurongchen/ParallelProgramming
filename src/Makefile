clean:
	rm *.o temp
build:
	mpicc utils.c -c -o utils.o
	mpicc sat.c -c -o sat.o
	mpicc entry.c utils.o sat.o -o entry.o

test:
	clang sat.c test.c -o test.o

test2:
	mpicc sat.c test2.c -o test2.o