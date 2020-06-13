#include "sat.h"

int main(){
    SATData *data = LoadData("../uuf50-218/uuf50-090.cnf");
    // SATData *data = LoadData("../uf20-91/uf20-090.cnf");
    // SATData *data = LoadData("../ufme/1.cnf");
    printf("[OK] LoadData\n");
    SAT(data, 1);
    printf("[OK] SAT\n");

    DestroyData(data);

}