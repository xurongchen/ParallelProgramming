#pragma once
#include <stdio.h>
typedef struct{
    // int n; // number of term
    int Un; // number of unknown term
    int trueVar;
    // int *term; // which vars (+1 => v; -1 => \neg v)
}Clause;

typedef struct{
    // int times;
    int cN;
    int *term;  // Index From 0, which clause
}VarAppear;


typedef struct{
    int vNum;  // Number of vars
    int cNum; // Number of clauses
    // int* App; // Appear times in current clauses for each vars
    int* V; // Valuations to vars, +1<=>True, -1<=>False, 0<=>Undetermined
    Clause* C; // Clauses, // Index From 1
    VarAppear* A; // Index From 1
}SATData;


int SAT(SATData* data, int varNow);

int AssignValue(SATData* data, int var, int value);
void DeassignValue(SATData* data, int var);

#define ABS(X) (X > 0 ? X: -X)

#define SIGN(X) (X >= 0 ? 1: -1)

#define IO_MAX_BUFF 1024

SATData* LoadData(char* filePath);

void DestroyData(SATData* data);


// #define TRACE_LoadData
// #define TRACE_SAT
// #define TRACE_AssignValue
// #define TRACE_DestroyData
// #define TRACE_DecodeData
// #define TRACE_EncodeData

void EncodeData(SATData* data, char** encode, size_t *length);

SATData* DecodeData(char* encode);