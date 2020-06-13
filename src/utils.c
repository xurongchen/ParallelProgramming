#include "utils.h"
#include <time.h>
#include "sat.h"

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



void Work(int *p_id, int *p_num, PROCESS_STATUS* p_status, WorkState initstate, SATData* data, int varNow){

    WorkState state = initstate;
    // WorkState state = *p_id == 0 ? WORK_STATE_ENTRY: WORK_STATE_QUERY_TASK;
    #ifdef DEBUG
    // printf("Process %d is ok! (varNow = %d, WorkState = %d)\n", *p_id, varNow, state);
    #endif
    int flagHalf, taskPID, idlePID, solvedId;

    SimpleMessage msg;
    MPI_Status status;

    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 }; 
    MPI_Aint disp[2]; 
    MPI_Aint base; 
    MPI_Get_address(&msg, &base);
    MPI_Get_address(&msg.M_Type, disp);
    MPI_Get_address(&msg.M_Content, disp + 1);
    disp[1] -= base;
    disp[0] -= base;
    MPI_Type_create_struct(2, blocklen, disp, type, &SimpleType);   
    MPI_Type_commit(&SimpleType); 

    while(1){
        #ifdef DEBUG_SLOW_OUTPUT
        // usleep(10);
        #endif
        #ifdef TRACE
        // printf("[P%d] Work: 01, state = %d\n", *p_id, state);
        #endif
        switch (state)
        {
        case WORK_STATE_INIT:
            // data = LoadData("../uuf50-218/uuf50-090.cnf");
            data = LoadData("../uf50-218/uf50-066.cnf");
            #ifdef DEBUG
            printf("[P%d] LoadData Finished, vNum = %d, cNum = %d\n", *p_id, data->vNum, data->cNum);
            #endif
            Work(p_id, p_num, p_status, WORK_STATE_ENTRY, data, 1);
            return;
        case WORK_STATE_ENTRY:
            // Broadcast Busy state:
            msg.M_Type = MESSAGE_TYPE_REF;
            msg.M_Content = PROCESS_STATUS_BUSY;
            for(int i = 1; i< *p_num; ++i){
                int target = (*p_id + i) % *p_num;
                MPI_Send(   /* data = */ &msg,
                            /* count = */ 1, 
                            /* datatype = */ SimpleType, 
                            /* dest = */ target, 
                            /* tag = */ TAG_QUE, 
                            /* comm = */ MPI_COMM_WORLD);
            }

        case WORK_STATE_DFS:
            solvedId = QuerySolved(p_id, p_num, p_status);
            if(solvedId != -1){
                #ifdef TRACE_Work
                printf("[P%d] Find P%d has solved, so stop from runnning.\n", *p_id, solvedId);
                #endif
                // return;
                msg.M_Type = MESSAGE_TYPE_REF;
                msg.M_Content = PROCESS_STATUS_SOLVED;
                for(int i = 1; i< *p_num; ++i){
                    int target = (*p_id + i)% *p_num;
                    MPI_Send(   /* data = */ &msg,
                                /* count = */ 1, 
                                /* datatype = */ SimpleType, 
                                /* dest = */ target, 
                                /* tag = */ TAG_QUE, 
                                /* comm = */ MPI_COMM_WORLD);
                    #ifdef TRACE_Work
                    printf("[P%d] Send an SOLVED message to P%d\n", *p_id, target);
                    #endif
                }
                MPI_Finalize();
                exit(0);
            }
            // check itself is already finished:
            if(varNow > data -> vNum){
                printf("[P%d] Find SAT:\n", *p_id);
                for(int i = 1; i <= data->vNum; ++i){
                    if(data -> V[i] == 1)
                        printf("v%2d:T;\t", i);
                    else{
                        printf("v%2d:F;\t", i);
                    }
                    if(i % 10 == 0 || i == data->vNum)
                        printf("\n");
                }
                *(p_status + *p_id) = PROCESS_STATUS_SOLVED;
                state = WORK_STATE_SAT;
                break; // Find a solution
            }

            int varLeft = data -> vNum - varNow;
            int isParalleled = 0;
            int ASucc = AssignValue(data, varNow, -1); // Test Neg
            if(ASucc == 0){
                if(varLeft > MIN_NO_PARALLEL)
                { 
                    idlePID = QueryIdleProcess(p_id, p_num, p_status);
                    isParalleled = 1;
                    if(idlePID == -1){
                        Work(p_id, p_num, p_status, WORK_STATE_DFS, data, varNow + 1);
                    }
                    else { 
                        // try parallel:
                        char* encode;
                        size_t len;
                        EncodeData(data, &encode, &len);

                        msg.M_Type = MESSAGE_TYPE_DATA;
                        msg.M_Content = len;

                        MPI_Send(   /* data = */ &msg,
                                    /* count = */ 1, 
                                    /* datatype = */ SimpleType, 
                                    /* dest = */ idlePID, 
                                    /* tag = */ TAG_DATA, 
                                    /* comm = */ MPI_COMM_WORLD);

                        MPI_Send(   /* data = */ encode,
                                    /* count = */ len, 
                                    /* datatype = */ MPI_CHAR, 
                                    /* dest = */ idlePID, 
                                    /* tag = */ TAG_RAW_DATA, 
                                    /* comm = */ MPI_COMM_WORLD);
                        #ifdef DEBUG
                        printf("[P%d] Send a RAW_DATA to %d, len:%d\n", *p_id, idlePID, len);
                        #endif
                        free(encode);
                        
                    }
                }
                else{
                    Work(p_id, p_num, p_status, WORK_STATE_DFS, data, varNow + 1);
                }
                DeassignValue(data, varNow);
            }

            ASucc = AssignValue(data, varNow, 1); // Test Pos
            if(ASucc == 0){
                if(isParalleled == 0 && varLeft > MIN_NO_PARALLEL){
                    idlePID = QueryIdleProcess(p_id, p_num, p_status);
                    isParalleled = 1;
                    if(idlePID == -1){
                        Work(p_id, p_num, p_status, WORK_STATE_DFS, data, varNow + 1);
                    }
                    else { 
                        // try parallel:
                        char* encode;
                        size_t len;
                        EncodeData(data, &encode, &len);

                        msg.M_Type = MESSAGE_TYPE_DATA;
                        msg.M_Content = len;

                        MPI_Send(   /* data = */ &msg,
                                    /* count = */ 1, 
                                    /* datatype = */ SimpleType, 
                                    /* dest = */ idlePID, 
                                    /* tag = */ TAG_DATA, 
                                    /* comm = */ MPI_COMM_WORLD);

                        MPI_Send(   /* data = */ encode,
                                    /* count = */ len, 
                                    /* datatype = */ MPI_CHAR, 
                                    /* dest = */ idlePID, 
                                    /* tag = */ TAG_RAW_DATA, 
                                    /* comm = */ MPI_COMM_WORLD);
                        #ifdef DEBUG
                        printf("[P%d] Send a RAW_DATA(1) to %d, len:%d\n", *p_id, idlePID, len);
                        #endif
                        free(encode);
                    }
                }
                else{
                    Work(p_id, p_num, p_status, WORK_STATE_DFS, data, varNow + 1);
                }
                DeassignValue(data, varNow);
            }
            

            // /* code */
            // flagHalf = 0;
            // if(1) // Whether choose use other process?
            // {
            //     #ifdef TRACE
            //     // printf("[P%d] Work: 02\n", *p_id);
            //     #endif
            //     idlePID = QueryIdleProcess(p_id, p_num, p_status);
            //     if(idlePID == -1){
            //         flagHalf = 0;
            //     }
            //     else {
            //         flagHalf = 1;
            //         // Send data message to idlePID: (half task, WORK_STATE_ENTRY)

            //         // Solve another half (half task, WORK_STATE_DFS)
            //     }
            // }
            // if(flagHalf == 0){
            //     // Solve one half (half task, WORK_STATE_DFS)
            //     // Solve another half (half task, WORK_STATE_DFS)
            // }

            // If it is an entrance, broadcast idle infomation...
            if(state == WORK_STATE_ENTRY){ 
                *(p_status + *p_id) = PROCESS_STATUS_IDLE;

                msg.M_Type = MESSAGE_TYPE_REF;
                msg.M_Content = PROCESS_STATUS_IDLE;
                // for(int i = 1; i< *p_num; ++i){
                for(int i = 1; i< 2; ++i){
                    int target = (*p_id + i)% *p_num;
                    MPI_Send(   /* data = */ &msg,
                                /* count = */ 1, 
                                /* datatype = */ SimpleType, 
                                /* dest = */ target, 
                                /* tag = */ TAG_QUE, 
                                /* comm = */ MPI_COMM_WORLD);
                    #ifdef TRACE_Work
                    printf("[P%d] Send an Idle message to P%d\n", *p_id, target);
                    #endif
                }
                state = WORK_STATE_QUERY_TASK;
                DestroyData(data);
                data = NULL;
                break; // Task Finish
            }
            return; // DFS branch Finish
        case WORK_STATE_QUERY_TASK:
            #ifdef TRACE_Work
            printf("[P%d] Work: 03\n", *p_id);
            #endif
            if(QueryStopIdle(p_id, p_num, p_status)){
                if(* p_id == 0){
                    printf("UNSAT\n", *p_id);
                }
                return;
            }
            int DataLen;
            taskPID = QueryTask(p_id, p_num, p_status, &DataLen);
            #ifdef TRACE_Work
            printf("[P%d] Work: 08, taskPID = %d\n", *p_id, taskPID);
            #endif
            if(taskPID != -1)
            {
                // Prepare DATA:
                char* encodeData;
                encodeData = (char *) malloc(DataLen);
                #ifdef DEBUG
                // printf("[P%d] Before Recv: ec[0] = %d, ec[1] = %d\n", *p_id, *((int*) encodeData), *((int*) encodeData + 1));
                #endif    
                MPI_Recv(   /* data = */ encodeData,
                            /* count = */ DataLen, 
                            /* datatype = */ MPI_CHAR, 
                            /* source = */ taskPID, 
                            /* tag = */ TAG_RAW_DATA, 
                            /* comm = */ MPI_COMM_WORLD,
                            /* status = */ &status);
                #ifdef DEBUG
                // printf("[P%d] Before decode: a0 = %d, a1 = %d, a2 = %d, a3 = %d, a4 = %d, a5 = %d\n", *p_id, encodeData, DataLen, MPI_CHAR, taskPID, TAG_RAW_DATA, MPI_COMM_WORLD);

                // printf("[P%d] Before decode: ec[0] = %d, ec[1] = %d, ec[2] = %d, ec[3] = %d, ec[4] = %d, ec[5] = %d\n", *p_id, *((int*) encodeData), *((int*) encodeData + 1), *((int*) encodeData + 2), *((int*) encodeData + 3), *((int*) encodeData + 4), *((int*) encodeData + 5));
                #endif
                data = DecodeData(encodeData);
                varNow = 1;
                while(data->V[varNow] != 0){
                    varNow += 1;
                }
                #ifdef DEBUG
                printf("[P%d] DecodeData Finished, vNum = %d, cNum = %d, DataLen = %d\n", *p_id, data->vNum, data->cNum, DataLen);
                #endif
                // Goto WORK_STATE_ENTRY:
                state = WORK_STATE_ENTRY;
            }
            break;
        case WORK_STATE_SAT:
            msg.M_Type = MESSAGE_TYPE_REF;
            msg.M_Content = PROCESS_STATUS_SOLVED;
            for(int i = 1; i< *p_num; ++i){
                int target = (*p_id + i)% *p_num;
                MPI_Send(   /* data = */ &msg,
                            /* count = */ 1, 
                            /* datatype = */ SimpleType, 
                            /* dest = */ target, 
                            /* tag = */ TAG_QUE, 
                            /* comm = */ MPI_COMM_WORLD);
                #ifdef TRACE_Work
                printf("[P%d] Send an SOLVED message to P%d\n", *p_id, target);
                #endif
            }
            return;
        default:
            break;
        }

    }
}
int QuerySolved(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    if(*(p_status + *p_id) == PROCESS_STATUS_SOLVED){
        return *p_id;
    }
    SimpleMessage msg;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;

    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 }; 
    MPI_Aint disp[2]; 
    MPI_Aint base; 
    MPI_Get_address(&msg, &base);
    MPI_Get_address(&msg.M_Type, disp);
    MPI_Get_address(&msg.M_Content, disp + 1);
    disp[1] -= base;
    disp[0] -= base;
    MPI_Type_create_struct(2, blocklen, disp, type, &SimpleType);   
    MPI_Type_commit(&SimpleType); 

    for(int i = 1; i< *p_num; ++i){
        int target = (*p_id + i) % *p_num;

        while(1){
            MPI_Irecv(  /* buffer = */ &msg, 
                                    /* count = */ 1, 
                                    /* datatype = */ SimpleType, 
                                    /* source = */ target, 
                                    /* tag = */ TAG_QUE, 
                                    /* comm = */ MPI_COMM_WORLD, 
                                    /* request = */ &request);

            MPI_Test(&request, &recvFlag, &status);
            if(!recvFlag){ 
                MPI_Cancel(&request);
                MPI_Request_free(&request);
                if(*(p_status + target) == PROCESS_STATUS_SOLVED){
                    
                    return target; // Solved
                }
                break;
            }
            *(p_status + target) = msg.M_Content;
            #ifdef DEBUG
            printf("[P%d] Get a REF from P%d (ctx = %d).[QuerySolved]\n", *p_id, target, msg.M_Content);
            #endif
        }
        
    }
    return -1;// Not solved
}

int QueryStopIdle(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    

    SimpleMessage msg;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;

    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 }; 
    MPI_Aint disp[2]; 
    MPI_Aint base; 
    MPI_Get_address(&msg, &base);
    MPI_Get_address(&msg.M_Type, disp);
    MPI_Get_address(&msg.M_Content, disp + 1);
    disp[1] -= base;
    disp[0] -= base;
    MPI_Type_create_struct(2, blocklen, disp, type, &SimpleType);   
    MPI_Type_commit(&SimpleType); 

    int solvedId = QuerySolved(p_id, p_num, p_status);
    if(solvedId != -1){
        // #ifdef TRACE_Work
        // printf("[P%d] Find P%d has solved, so stop from Idle.", *p_id, solvedId);
        // #endif
        // return 1; // Stop
        #ifdef TRACE_Work
        printf("[P%d] Find P%d has solved, so stop from runnning.\n", *p_id, solvedId);
        #endif
        msg.M_Type = MESSAGE_TYPE_REF;
        msg.M_Content = PROCESS_STATUS_SOLVED;
        for(int i = 1; i< *p_num; ++i){
            int target = (*p_id + i)% *p_num;
            MPI_Send(   /* data = */ &msg,
                        /* count = */ 1, 
                        /* datatype = */ SimpleType, 
                        /* dest = */ target, 
                        /* tag = */ TAG_QUE, 
                        /* comm = */ MPI_COMM_WORLD);
            #ifdef TRACE_Work
            printf("[P%d] Send an SOLVED message to P%d\n", *p_id, target);
            #endif
        }
        MPI_Finalize();
        exit(0);
    } 

    for(int i = 1; i< *p_num; ++i){
        int target = (*p_id + i) % *p_num;

        while(1){
            MPI_Irecv(  /* buffer = */ &msg, 
                                    /* count = */ 1, 
                                    /* datatype = */ SimpleType, 
                                    /* source = */ target, 
                                    /* tag = */ TAG_QUE, 
                                    /* comm = */ MPI_COMM_WORLD, 
                                    /* request = */ &request);

            MPI_Test(&request, &recvFlag, &status);
            if(!recvFlag){ 
                MPI_Cancel(&request);
                MPI_Request_free(&request);
                if(*(p_status + target) == PROCESS_STATUS_BUSY){
                    #ifdef DEBUG
                    printf("[P%d] Avoid stop because P%d is busy.\n", *p_id, target);
                    #endif
                    #ifdef DEBUG_SLOW_OUTPUT
                    usleep(100000);
                    #endif
                    return 0; // No stop
                }
                break;
            }
            *(p_status + target) = msg.M_Content;
            #ifdef DEBUG
            printf("[P%d] Get a REF from P%d (ctx = %d).[QueryStopIdle]\n", *p_id, target, msg.M_Content);
            #endif
        }
        
    }
    #ifdef DEBUG
    printf("[P%d] Found to stop.\n", *p_id);
    #endif
    return 1;// Stop
}

int QueryTask(int *p_id, int *p_num, PROCESS_STATUS* p_status, int* Datalen){
    SimpleMessage msg;
    // int msgContent[] = {0};
    msg.M_Content = 0;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;
    #ifdef TRACE_QueryTask
    printf("[P%d] QueryTask: 00\n", *p_id);
    #endif
    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 }; 
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

    for(int i = 1; i< *p_num; ++i){
        int target = (*p_id + i) % *p_num;
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
        printf("[P%d] QueryTask: 03A, target = %d, msg.M_Type = %d, msg.M_Content = %d, &msg=%u\n", *p_id, target, msg.M_Type, msg.M_Content, &msg);
        #endif
        #ifdef DEBUG
        int REQ_pid = msg.M_Content;
        printf("[P%d] Received a REQ from P%d\n", *p_id, REQ_pid);
        #endif
        // MPI_Request_free(&request);

        msg.M_Type = MESSAGE_TYPE_ACK;
        // int content[] = {*p_id};
        msg.M_Content = *p_id;
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
        clock_t t_start = clock();
        clock_t t_now = clock();
        #ifdef TimeCheck
        printf("Time: %d\n", t_now);
        #endif

        while(recvFlag == 0 && (t_now - t_start) <= COMMUNICATION_TIMEOUT){
            MPI_Test(&request, &recvFlag, &status);
            t_now = clock();
            #ifdef TimeCheck
            if((t_now - t_start) > COMMUNICATION_TIMEOUT)
                printf("TimeOut time = %d\n", t_now);
            #endif
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
        *Datalen = msg.M_Content;
        return target;
    }
    return -1; // No task application...
}

int QueryIdleProcess(int *p_id, int *p_num, PROCESS_STATUS* p_status){
    #ifdef TRACE_QueryIdleProcess
    printf("[P%d] QueryIdleProcess: 00\n", *p_id);
    #endif
    SimpleMessage msg;
    // int msgContent[] = {0};
    msg.M_Content = 0;
    MPI_Request request;
    MPI_Status status;
    int recvFlag;

    MPI_Datatype SimpleType;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 }; 
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

    for(int i = 1; i< *p_num; ++i){
        int target = (*p_id + i) % *p_num;

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
            *(p_status + target) = msg.M_Content;
            // MPI_Request_free(&request);
            #ifdef DEBUG
            printf("[P%d] Get a REF from P%d (ctx = %d). [QueryIdleProcess]\n", *p_id, target, msg.M_Content);
            #endif
        } 

        #ifdef TRACE_QueryIdleProcess
        printf("[P%d] QueryIdleProcess: 04, target = %d\n", *p_id, target);
        #endif

        if(*(p_status + target) == PROCESS_STATUS_IDLE)
        { // Make a request
            msg.M_Type = MESSAGE_TYPE_REQ;
            // int content[] = {*p_id};
            #ifdef TRACE_QueryIdleProcess
            printf("[P%d] QueryIdleProcess: 06, msg = %u, msg.M_Type=%u, msg.M_Content=%u\n", *p_id, &msg, msg.M_Type, msg.M_Content);
            #endif
            msg.M_Content = *p_id;
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
            printf("[P%d] QueryIdleProcess: 07, msg.M_Content = %d, &msg=%u, &msg.M_Content=%u\n", *p_id, msg.M_Content,&msg, &msg.M_Content);
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
            clock_t t_start = clock();
            clock_t t_now = clock();

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
            int targetMSG = msg.M_Content;
            // MPI_Request_free(&request);
            printf("[P%d] Receive an ACK from %d\n", *p_id, target);
            #endif
            // Receive ACK message, trying to establish communication...
            return target; // Find an idle process target
        }
    }
    return -1; // Failed to find an idle process.
}

// void fooJob(int l,  int r, int *p_id, int *p_num, PROCESS_STATUS* p_status){
//     printf("[P%d] Foo job %d is solved\n", *p_id, l);
//     if(l + 1 < r)
//        Work(p_id, p_num, p_status, p_id == 0? PROCESS_STATUS_BUSY:PROCESS_STATUS_IDLE); 
// }

