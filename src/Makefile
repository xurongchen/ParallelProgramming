clean:
	rm *.o temp
build:
	mkdir ../bin
	mpicc core.c -c -o ../bin/core.o
	mpicc sat.c -c -o ../bin/sat.o
	mpicc entry.c ../bin/core.o ../bin/sat.o -o ../bin/entry.o

test:
	clang sat.c test.c -o test.o

test2:
	mpicc sat.c test2.c -o test2.o
