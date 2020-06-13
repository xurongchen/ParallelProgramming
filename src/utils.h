#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "sat.h"

// #define DEBUG 
// #define DEBUG_SLOW_OUTPUT
// #define TRACE 

#ifdef TRACE
// #define TRACE_QueryIdleProcess
// #define TRACE_QueryTask
// #define TRACE_QueryStop
#define TRACE_Work
#endif

#define MAX_PROCESS_NUM 16

typedef struct {
    int M_Type;
    int M_Content;
} SimpleMessage;

// Status Refresh
#define MESSAGE_TYPE_REF 0; 
// For REF message, M_Content is *int, pointed to refresh value; 
// Task Request
#define MESSAGE_TYPE_REQ 1; 
// Task Acknowledge
#define MESSAGE_TYPE_ACK 2;
// DATA INFO
#define MESSAGE_TYPE_DATA 3;


#define TAG_QUE 0x000000001
#define TAG_REQ 0x000000002
#define TAG_ACK 0x000000004

#define TAG_DATA 0x0000000f0

#define TAG_RAW_DATA 0x0000000ff

// Communication timeout setting: s
#define COMMUNICATION_TIMEOUT (CLOCKS_PER_SEC/10)

typedef char PROCESS_STATUS;
#define PROCESS_STATUS_BUSY 1
#define PROCESS_STATUS_IDLE 0
#define PROCESS_STATUS_SOLVED 2

int Initialize(int *argc, char ***argv, int *p_id, int *p_num, PROCESS_STATUS* p_status);

typedef int WorkState;

#define WORK_STATE_INIT 0
#define WORK_STATE_ENTRY 1
#define WORK_STATE_DFS 2
#define WORK_STATE_QUERY_TASK 10
#define WORK_STATE_SAT 20


void Work(int *p_id, int *p_num, PROCESS_STATUS* p_status, WorkState initstate, SATData* data, int varNow);

int QueryTask(int *p_id, int *p_num, PROCESS_STATUS* p_status, int* Datalen);
int QueryIdleProcess(int *p_id, int *p_num, PROCESS_STATUS* p_status);
int QuerySolved(int *p_id, int *p_num, PROCESS_STATUS* p_status);
int QueryStopIdle(int *p_id, int *p_num, PROCESS_STATUS* p_status);
// void fooJob(int l,int r, int *p_id, int *p_num, PROCESS_STATUS* p_status);

#define MIN_NO_PARALLEL 30