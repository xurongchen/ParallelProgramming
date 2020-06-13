#include "utils.h"
#include <time.h>

int Initialize(int *argc, char ***argv, int *p_id, int *p_num, PROCESS_STATUS* p_status){
    MPI_Init (argc, argv);	/* starts MPI */
    MPI_Comm_size(MPI_COMM_WORLD, p_num);	/* get number of processes */
    if(*p_num > MAX_PROCESS_NUM){
        printf("The number of process is larger than default max setting!");
        return -1;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, p_id);	/* get current process id */

    *p_status = PROCESS_STATUS_BUSY;
    for(int proc = 1; proc < *p_num; ++ proc){
        *(p_status + proc) = PROCESS_STATUS_IDLE;
    }

    return 0;
}



void Work(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    WorkState state = *p_id == 0 ? WORK_STATE_ENTRY: WORK_STATE_QUERY_TASK;
    #ifdef DEBUG
    printf("Process %d is ok!\n", *p_id);
    #endif
    int flagHalf, taskPID, idlePID;
    while(1){
        #ifdef TRACE
        // printf("[P%d] Work: 01\n", *p_id);
        #endif
        switch (state)
        {
        case WORK_STATE_ENTRY:
            /* code */
            flagHalf = 0;
            if(1) // Whether choose use other process?
            {
                #ifdef TRACE
                // printf("[P%d] Work: 02\n", *p_id);
                #endif
                idlePID = QueryIdleProcess(p_id, p_num, p_status);
                if(idlePID == -1){
                    flagHalf = 0;
                }
                else {
                    flagHalf = 1;
                    // Send data message to idlePID: (half task)

                    // Solve another half (half task)
                }
            }
            if(flagHalf == 0){
                // Solve one half (half task)
                // Solve another half (half task)
            }
            return; // DEBUG
            break;
        case WORK_STATE_QUERY_TASK:
            #ifdef TRACE
            // printf("[P%d] Work: 03\n", *p_id);
            #endif
            taskPID = QueryTask(p_id, p_num, p_status);
            if(taskPID != -1)
            {
                // Prepare DATA:

                // Goto WORK_STATE_ENTRY:
                state = WORK_STATE_ENTRY;
            }
            break;
        default:
            break;
        }

    }
}

int QueryTask(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    Message msg;
    int msgContent[] = {0};
    msg.M_Content = msgContent;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;
    #ifdef TRACE_QueryTask
    printf("[P%d] QueryTask: 00\n", *p_id);
    #endif
    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_CHAR, MPI_CHAR };
    int blocklen[2] = { 1, 4 }; 
    MPI_Aint disp[2]; 
    MPI_Aint base; 
    MPI_Get_address(&msg, &base);
    MPI_Get_address(&msg.M_Type, disp);
    MPI_Get_address(&msg.M_Content, disp + 1);
    disp[1] -= base;
    disp[0] -= base;
    #ifdef TRACE_QueryTask
    printf("[P%d] QueryTask: 01, disp[0] = %d, disp[1] = %d, base = %d\n", *p_id, disp[0],disp[1],base);
    #endif

    MPI_Type_create_struct(2, blocklen, disp, type, &SimpleType);   
    MPI_Type_commit(&SimpleType); 

    for(int i = *p_id + 1; i< *p_id + *p_num; ++i){
        int target = i % *p_num;
        #ifdef TRACE_QueryTask
        printf("[P%d] QueryTask: 02, target = %d\n", *p_id, target);
        printf("[P%d] QueryTask: 02@1, msg = %lld, msg.M_Type = %d, msg.M_Content = %u\n", *p_id, msg, msg.M_Type, msg.M_Content);
        #endif
        MPI_Irecv(  /* buffer = */ &msg, 
                        /* count = */ 1, 
                        /* datatype = */ SimpleType, 
                        /* source = */ target, 
                        /* tag = */ TAG_REQ, 
                        /* comm = */ MPI_COMM_WORLD, 
                        /* request = */ &request);

        MPI_Test(&request, &recvFlag, &status);
        if(!recvFlag){ 
            MPI_Cancel(&request);
            MPI_Request_free(&request);
            continue;
        }
        #ifdef TRACE_QueryTask
        printf("[P%d] QueryTask: 03, target = %d, &request = %u\n", *p_id, target, &request);
        printf("[P%d] QueryTask: 03@1, msg = %lld\n", *p_id, msg);
        #endif
        #ifdef TRACE_QueryTask
        printf("[P%d] QueryTask: 03A, target = %d, msg.M_Type = %d, &msg.M_Content = %u, &msg=%u\n", *p_id, target, msg.M_Type, &msg.M_Content, &msg);
        #endif
        #ifdef DEBUG
        int REQ_pid = *((int*) msg.M_Content);
        printf("[P%d] Received a REQ from P%d\n", *p_id, REQ_pid);
        #endif
        MPI_Request_free(&request);

        msg.M_Type = MESSAGE_TYPE_ACK;
        int content[] = {*p_id};
        msg.M_Content = content;
        #ifdef DEBUG
        printf("[P%d] Send an ACK to P%d\n", *p_id, REQ_pid);
        #endif
        MPI_Send(   /* data = */ &msg,
                    /* count = */ 1, 
                    /* datatype = */ SimpleType, 
                    /* dest = */ target, 
                    /* tag = */ TAG_ACK, 
                    /* comm = */ MPI_COMM_WORLD);


        MPI_Irecv(  /* buffer = */ &msg, 
                    /* count = */ 1, 
                    /* datatype = */ SimpleType, 
                    /* source = */ target, 
                    /* tag = */ TAG_DATA, 
                    /* comm = */ MPI_COMM_WORLD, 
                    /* request = */ &request);
        MPI_Test(&request, &recvFlag, &status);
        time_t t_start = clock();
        time_t t_now = clock();

        while(recvFlag == 0 && (t_now - t_start) <= COMMUNICATION_TIMEOUT){
            MPI_Test(&request, &recvFlag, &status);
            t_now = clock();
        }

        if(recvFlag == 0){ 
            MPI_Cancel(&request);
            MPI_Request_free(&request);
            #ifdef DEBUG
            printf("[P%d] Failed to received a DATA from P%d\n", *p_id, REQ_pid);
            #endif
            continue; // Failed to receive DATA message
        }
        #ifdef DEBUG
        printf("[P%d] Received a DATA from P%d\n", *p_id, REQ_pid);
        #endif
        return target;
    }
    return -1; // No task application...
}

int QueryIdleProcess(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    #ifdef TRACE_QueryIdleProcess
    printf("[P%d] QueryIdleProcess: 00\n", *p_id);
    #endif
    Message msg;
    int msgContent[] = {0};
    msg.M_Content = msgContent;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;

    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_CHAR, MPI_CHAR };
    int blocklen[2] = { 1, 4 }; 
    MPI_Aint disp[2]; 
    MPI_Aint base; 
    MPI_Get_address(&msg, &base);
    MPI_Get_address(&msg.M_Type, disp);
    MPI_Get_address(&msg.M_Content, disp + 1);
    disp[1] -= base;
    disp[0] -= base;
    
    MPI_Type_create_struct(2, blocklen, disp, type, &SimpleType);   
    MPI_Type_commit(&SimpleType); 

    #ifdef TRACE_QueryIdleProcess
    printf("[P%d] QueryIdleProcess: 01\n", *p_id);
    #endif

    for(int i = *p_id + 1; i< *p_id + *p_num; ++i){
        int target = i % *p_num;

        #ifdef TRACE_QueryIdleProcess
        printf("[P%d] QueryIdleProcess: 02, target = %d\n", *p_id, target);
        #endif

        while(1) {
            MPI_Irecv(  /* buffer = */ &msg, 
                        /* count = */ 1, 
                        /* datatype = */ SimpleType, 
                        /* source = */ target, 
                        /* tag = */ TAG_QUE, 
                        /* comm = */ MPI_COMM_WORLD, 
                        /* request = */ &request);
            MPI_Test(&request, &recvFlag, &status);
            if(recvFlag == 0){ 
                MPI_Cancel(&request);
                MPI_Request_free(&request);
                break;
            }
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 03, request = %u\n", *p_id, &request);
            #endif
            *(p_status + target) = *((int*) msg.M_Content);
            MPI_Request_free(&request);
        } 

        #ifdef TRACE_QueryIdleProcess
        printf("[P%d] QueryIdleProcess: 04, target = %d\n", *p_id, target);
        #endif

        if(*(p_status + target) == PROCESS_STATUS_IDLE)
        { // Make a request
            msg.M_Type = MESSAGE_TYPE_REQ;
            int content[] = {*p_id};
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 06, content = %u, msg = %u, msg.M_Type=%u, msg.M_Content=%u\n", *p_id, content, &msg, msg.M_Type, msg.M_Content);
            #endif
            msg.M_Content = content;
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 06A\n", *p_id);
            #endif

            
            MPI_Send(   /* data = */ &msg,
                        /* count = */ 1, 
                        /* datatype = */ SimpleType, 
                        /* dest = */ target, 
                        /* tag = */ TAG_REQ, 
                        /* comm = */ MPI_COMM_WORLD);
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 07, *msg.M_Content = %d, msg.M_Content = %d, &msg=%u, &msg.M_Content=%u\n", *p_id, *(int*) msg.M_Content, msg.M_Content,&msg,&msg.M_Content);
            #endif
            #ifdef DEBUG
            printf("[P%d] Send a REQ to %d\n", *p_id, target);
            #endif
            MPI_Irecv(  /* buffer = */ &msg, 
                        /* count = */ 1, 
                        /* datatype = */ SimpleType, 
                        /* source = */ target, 
                        /* tag = */ TAG_ACK, 
                        /* comm = */ MPI_COMM_WORLD, 
                        /* request = */ &request);

            // usleep(COMMUNICATION_TIMEOUT*1000);
            MPI_Test(&request, &recvFlag, &status);
            time_t t_start = clock();
            time_t t_now = clock();

            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 08\n", *p_id);
            #endif

            while(recvFlag == 0 && (t_now - t_start) <= COMMUNICATION_TIMEOUT){
                MPI_Test(&request, &recvFlag, &status);
                t_now = clock();
            }
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 09\n", *p_id);
            #endif
            if(recvFlag == 0){ 
                MPI_Cancel(&request);
                MPI_Request_free(&request);
                #ifdef DEBUG
                printf("[P%d] Failed to receive an ACK from %d\n", *p_id, target);
                #endif
                #ifdef TRACE_QueryIdleProcess
                printf("[P%d] QueryIdleProcess: 10, request = %u\n", *p_id, &request);
                #endif
                continue; // Failed to receive ACK message
            }
            #ifdef DEBUG
            int targetMSG = * (int*) msg.M_Content;
            MPI_Request_free(&request);
            printf("[P%d] Idle target is %d (logic), %d (msg)\n", *p_id, target, targetMSG);
            #endif
            // Receive ACK message, trying to establish communication...
            return target; // Find an idle process target
        }
    }
    return -1; // Failed to find an idle process.
}

void fooJob(int l,  int r, int *p_id, int *p_num, PROCESS_STATUS* p_status){
    printf("[P%d] Foo job %d is solved\n", *p_id, l);
    if(l + 1 < r)
       Work(p_id, p_num, p_status); 
}
