#define DEBUG 

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "utils.h"



int main(argc, argv)
     int argc;
     char *argv[];
{
  int p_id, p_num;
  PROCESS_STATUS p_status[MAX_PROCESS_NUM];

  int InitRet = Initialize(&argc, &argv, &p_id, &p_num, p_status);
  if(InitRet != 0){
      return 0;
  }

  Work(&p_id, &p_num, p_status);

  // printf( "Hello world from process %d of %d\n", p_id, p_num );
  MPI_Finalize();
  return 0;
}