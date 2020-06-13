#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "sat.h"

int main(int argc, char** argv) {
  // Initialize the MPI environment
  MPI_Init(NULL, NULL);
  // Find out rank, size
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // We are assuming at least 2 processes for this task
  if (world_size < 2) {
    fprintf(stderr, "World size must be greater than 1 for %s\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  int number;
  if (world_rank == 0) {
    // If we are rank 0, set the number to -1 and send it to process 1
    SATData *data = LoadData("../uuf50-218/uuf50-090.cnf");
    char* encode = NULL;
    int datalen = 0;
    EncodeData(data, &encode, &datalen);
    printf("P0\n");
    MPI_Send(
      /* data         = */ &datalen, 
      /* count        = */ 1, 
      /* datatype     = */ MPI_INT, 
      /* destination  = */ 1, 
      /* tag          = */ 0, 
      /* communicator = */ MPI_COMM_WORLD);
    printf("P0\n");
    MPI_Send(
      /* data         = */ encode, 
      /* count        = */ datalen, 
      /* datatype     = */ MPI_CHAR, 
      /* destination  = */ 1, 
      /* tag          = */ 0,
      /* communicator = */ MPI_COMM_WORLD);
    printf("P0\n");

  } else if (world_rank == 1) {
    char* encode = NULL;
    int datalen = 0;
    printf("P1\n");
    MPI_Recv(
      /* data         = */ &datalen, 
      /* count        = */ 1, 
      /* datatype     = */ MPI_INT, 
      /* source       = */ 0, 
      /* tag          = */ 0, 
      /* communicator = */ MPI_COMM_WORLD, 
      /* status       = */ MPI_STATUS_IGNORE);
    encode = (char*) malloc(datalen);
    printf("P1\n");
    MPI_Recv(
      /* data         = */ encode, 
      /* count        = */ datalen, 
      /* datatype     = */ MPI_CHAR, 
      /* source       = */ 0, 
      /* tag          = */ 0, 
      /* communicator = */ MPI_COMM_WORLD, 
      /* status       = */ MPI_STATUS_IGNORE);
    SATData *data = DecodeData(encode);
    printf("P1\n");
    SAT(data, 1);
  }
  MPI_Finalize();
}