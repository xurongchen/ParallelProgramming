#include "sat.h"
#include <stdlib.h>
#include <string.h>

int SAT(SATData* data, int varNow){
    if(varNow > data -> vNum){
    // if(data-> V[data -> vNum] != 0){
        for(int i = 1; i <= data->vNum; ++i){
            if(data -> V[i] == 1)
                printf("v%2d:T;\t", i);
            else{
                printf("v%2d:F;\t", i);
            }
            if(i % 10 == 0 || i == data->vNum)
                printf("\n");
        }
        return 1; // Find a solution
    }

    int ASucc, satResult = 0;
    
    ASucc = AssignValue(data, varNow, -1);
    if(ASucc == 0){
        satResult += SAT(data, varNow + 1);
        DeassignValue(data, varNow);
    }
    if(satResult == 1) {
        #ifdef TRACE_SAT
        printf("[Trace] SAT @ 10, varNow = %d\n", varNow);
        #endif
        return 1;
    }
    
    ASucc = AssignValue(data, varNow, 1);
    if(ASucc == 0){
        satResult += SAT(data, varNow + 1);
        DeassignValue(data, varNow);
    }
    
    if(varNow == 1 && satResult == 0){
        printf("UNSAT\n");
    }
    
    return satResult;

}

int AssignValue(SATData* data, int var, int value){
    for(int i = 0; i < data->A[var].cN; ++i){
        int clauseId = ABS(data->A[var].term[i]);
        int clauseSIGN = SIGN(data->A[var].term[i]);
        #ifdef TRACE_AssignValue
        printf("[Trace] TRACE_AssignValue @ 1: var = %d, value = %d, clauseId = %d, clauseSIGN = %d\n", var, value, clauseId, clauseSIGN);
        printf("[Trace] TRACE_AssignValue @ 1A: C[clauseId].Un = %d\n", data -> C[clauseId].Un);
        #endif
        if(data -> C[clauseId].Un == 1 && clauseSIGN != value){ // Failed, return
            return -1;
        }
    }
    // Try successfully:
    data -> V[var] = value;

    for(int i = 0; i < data->A[var].cN; ++i){
        int clauseId = ABS(data->A[var].term[i]);
        int clauseSIGN = SIGN(data->A[var].term[i]);
        if(data -> C[clauseId].Un > 0){ 
            if(clauseSIGN == value) // Clause become true;
            {
                data -> C[clauseId].Un = - data -> C[clauseId].Un;
                data -> C[clauseId].trueVar = var;
            }
            else {
                data -> C[clauseId].Un -= 1;
            }
        }
    }
    return 0;
}

void DeassignValue(SATData* data, int var){
    data-> V[var] = 0; 
    for(int i = 0; i < data->A[var].cN; ++i){
        int clauseId = ABS(data->A[var].term[i]);
        if(data -> C[clauseId].Un >= 0){ 
            data -> C[clauseId].Un += 1;
        }
        else {
            if(data -> C[clauseId].trueVar == var){
                data -> C[clauseId].Un = - data -> C[clauseId].Un;
            }
        }
    }
}

SATData* LoadData(char* filePath){
    FILE * fp = fopen(filePath, "r");
    if(fp == NULL){
        printf("Data file not found.\n");
        return NULL;
    }
    char buff[IO_MAX_BUFF];
    int varN, clauseN;
    while(1){
        fscanf(fp, "%c", buff);
        #ifdef TRACE_LoadData
        printf("[Trace] Judge char: %c\n", buff[0]);
        #endif
        if(buff[0] == 'p'){
            
            fscanf(fp, "%s", buff);
            fscanf(fp, "%d", &varN);
            fscanf(fp, "%d", &clauseN);
            break;
        }
        else if(buff[0] == 'c'){
            *fgets(buff, IO_MAX_BUFF, fp);
        }
    }
    #ifdef TRACE_LoadData
    printf("[Trace] VarN: %d, ClauseN:%d\n", varN, clauseN);
    #endif
    SATData* data = (SATData*) malloc(sizeof(SATData));
    data -> vNum = varN;
    data -> cNum = clauseN;
    data -> V = (int*) malloc(sizeof(int) * (varN + 1));
    for(int i = 0; i<= varN; ++i){
        data -> V[i] =  0; // Initialize V
    }
    data -> C = (Clause*) malloc(sizeof(Clause) * (clauseN + 1));
    for(int i = 0; i<= clauseN; ++i){
        data -> C[i].Un = 0; // Initialize C
        data -> C[i].trueVar = 0; // Initialize C
    }
    data -> A = (VarAppear*) malloc(sizeof(VarAppear) * (varN + 1));
    for(int i = 0; i<= varN; ++i){
        data -> A[i].cN = 0; // Initialize A
        data -> A[i].term = (int*) malloc(sizeof(int) * clauseN); // Initialize A
    }

    for(int i = 1; i <= clauseN; ++i){

        int value;
        while(1){
            fscanf(fp, "%d", &value);
            if(value == 0)
                break;

            int varId = ABS(value);
            int varSIGN = SIGN(value);

            data->A[varId].term[data->A[varId].cN] = varSIGN * i;
            data->A[varId].cN += 1;

            data->C[i].Un += 1;
        }
    }
    fclose(fp);
    return data;
}

void DestroyData(SATData* data){
    int varN = data -> vNum;
    int clauseN = data -> cNum;
    #ifdef TRACE_DestroyData
    printf("[Trace]: DestroyData @ 0\n");
    #endif
    free(data -> V);
    #ifdef TRACE_DestroyData
    printf("[Trace]: DestroyData @ 1\n");
    #endif
    free(data -> C);
    #ifdef TRACE_DestroyData
    printf("[Trace]: DestroyData @ 2\n");
    #endif
    for(int i = 0; i<= varN; ++i){
        #ifdef TRACE_DestroyData
        printf("[Trace]: DestroyData @ 3\n");
        #endif
        free(data -> A[i].term);
    }
    #ifdef TRACE_DestroyData
    printf("[Trace]: DestroyData @ 4\n");
    #endif
    free(data -> A);

    free(data);
}

// Encode format:
// vNum | cNum | V:[int * (vNum+1)] | C:[Clause * (cNum+1)] | A: sum_{vNum + 1} [int[cN] + int * (cN)]

void EncodeData(SATData* data, char** encode, size_t *length){
    int vNum = data->vNum;
    int cNum = data->cNum;
    *length = 0;
    // Calculate malloc size:
    *length += sizeof(int) * 2; // store vName and cNum
    *length += sizeof(int) * (vNum + 1); // store array V
    *length += sizeof(Clause) * (cNum + 1); // store array C: Un | trueVar

    for(int i = 0; i <= vNum; ++i){
        int cN = data->A[i].cN;
        *length += sizeof(int) * (cN + 1); 
    }

    *encode = (char*) malloc(*length);

    // Write data:
    char* ptr = *encode;

    memcpy(ptr, &data->vNum, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &data->cNum, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, data->V, sizeof(int) * (vNum+1));
    ptr += sizeof(int) * (vNum+1);
    memcpy(ptr, data->C, sizeof(Clause) * (cNum + 1));
    ptr += sizeof(Clause) * (cNum + 1);

    #ifdef TRACE_EncodeData
    printf("[Trace] Encode result: v=%d, c=%d\n", *(int*) *encode, *((int*) *encode + 1));
    // printf("[Trace] Encode result: ec[2]=%d, ec[2]=%d\n", *((int*) *encode + 2), *((int*) *encode + 3));
    #endif

    for(int i = 0; i <= vNum; ++i){
        int cN = data->A[i].cN;
        memcpy(ptr, &data->A[i].cN, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, data->A[i].term, sizeof(int) * cN);
        ptr += sizeof(int) * cN;
    }
}

// Encode format:
// vNum | cNum | V:[int * (vNum+1)] | C:[ Clause * (cNum+1)] | A: sum_{vNum + 1} [int[cN] + int * (cN)]

SATData* DecodeData(char* encode){
    SATData* data = (SATData*) malloc(sizeof(SATData));
    char* ptr = encode;
    int vNum, cNum;
    vNum = *(int*) ptr;
    ptr += sizeof(int);
    cNum = *(int*) ptr;
    ptr += sizeof(int);
    data -> vNum = vNum;
    data -> cNum = cNum;
    // memcpy(&data->vNum, ptr, sizeof(int));
    // vNum = data->vNum;
    // ptr += sizeof(int);
    // memcpy(&data->cNum, ptr, sizeof(int));
    // vNum = data->cNum;
    // ptr += sizeof(int);

    #ifdef TRACE_DecodeData
    printf("[Trace] Decode result: v=%d, c=%d\n", data->vNum, data->cNum);
    #endif

    data -> V = (int*) malloc(sizeof(int) * (vNum + 1));
    memcpy(data -> V, ptr, sizeof(int) * (vNum + 1));
    ptr += sizeof(int) * (vNum + 1);

    data -> C = (Clause*) malloc(sizeof(Clause) * (cNum + 1));
    memcpy(data->C, ptr, sizeof(Clause) * (cNum + 1));
    ptr += sizeof(Clause) * (cNum + 1);

    data -> A = (VarAppear*) malloc(sizeof(VarAppear) * (vNum + 1));
    for(int i = 0; i<= vNum; ++i){
        int cN = *(int*) ptr;
        data -> A[i].cN = cN; 
        ptr += sizeof(int);
        data -> A[i].term = (int*) malloc(sizeof(int) * cN); 
        memcpy(data -> A[i].term, ptr, sizeof(int) * cN);
        ptr += sizeof(int) * cN;
    }
    return data;
}
