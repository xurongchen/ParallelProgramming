#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define DEBUG 
#define TRACE 
#define TRACE_QueryIdleProcess
#define TRACE_QueryTask

#define MAX_PROCESS_NUM 16

typedef struct {
    char M_Type;
    void* M_Content;
} Message;

// Status Refresh
#define MESSAGE_TYPE_REF 0; 
// For REF message, M_Content is *int, pointed to refresh value; 
// Task Request
#define MESSAGE_TYPE_REQ 1; 
// Task Acknowledge
#define MESSAGE_TYPE_ACK 2;

#define TAG_QUE 0x00000001
#define TAG_REQ 0x000000002
#define TAG_ACK 0x000000004

#define TAG_DATA 0x0000000f0

// Communication timeout setting: us
#define COMMUNICATION_TIMEOUT 100000

typedef char PROCESS_STATUS;
#define PROCESS_STATUS_BUSY 1
#define PROCESS_STATUS_IDLE 0

int Initialize(int *argc, char ***argv, int *p_id, int *p_num, PROCESS_STATUS* p_status);

typedef int WorkState;

#define WORK_STATE_ENTRY 1
#define WORK_STATE_QUERY_TASK 10


void Work(int *p_id, int *p_num, PROCESS_STATUS* p_status);

int QueryTask(int *p_id, int *p_num, PROCESS_STATUS* p_status);
int QueryIdleProcess(int *p_id, int *p_num, PROCESS_STATUS* p_status);

void fooJob(int l,int r, int *p_id, int *p_num, PROCESS_STATUS* p_status);