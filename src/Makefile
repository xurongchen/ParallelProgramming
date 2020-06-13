clean:
	rm *.o temp
build:
	mpicc utils.c -c -o utils.o
	mpicc entry.c utils.o -o entry.o
