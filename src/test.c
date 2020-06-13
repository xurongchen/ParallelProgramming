#include "sat.h"

int main(){
    SATData *data = LoadData("../uuf50-218/uuf50-090.cnf");
    // SATData *data = LoadData("../uf20-91/uf20-090.cnf");
    // SATData *data = LoadData("../ufme/1.cnf");
    printf("[OK] LoadData\n");

    char* encode = NULL;
    size_t datalen = 0;
    EncodeData(data, &encode, &datalen);
    DestroyData(data);
    printf("[OK] EncodeData len=%zu\n", datalen);

    data = DecodeData(encode);
    printf("[OK] DecodeData\n");


    free(encode);
    EncodeData(data, &encode, &datalen);
    printf("[OK] EncodeData len2=%zu\n", datalen);
    DestroyData(data);
    data = DecodeData(encode);
    printf("[OK] DecodeData\n");




    SAT(data, 1);
    printf("[OK] SAT\n");

    DestroyData(data);

}