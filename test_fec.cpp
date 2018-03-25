#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C"{
#endif

#include "fec.h"

#ifdef __cplusplus
}
#endif
#include "fec_codec.h"
#include "packet.h"

using namespace std;

static const int SIZE = 10;
static unsigned char data[FEC_N][SIZE] = {0};
static unsigned char outdata[FEC_N][SIZE] = {0};
static unsigned char recovery[FEC_N][SIZE]= {0};

// https://github.com/cedricde/fec-rs-vdm

static void encode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size){
    memcpy(outdata[index],data,size);
}

static void decode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size){
    memcpy(outdata[index],data,size);
}

void test_fec(){
    int SIZE = 10;
    FecCodec encode(FEC_K,FEC_N,SIZE);
    FecCodec decode(FEC_K,FEC_N,SIZE);

    int num = 0;
    for(int index = 0; index < FEC_K;++index){
        for(int j = 0;j < SIZE;++j){
            data[index][j] = num++;
        }
    }


    for(int i = 0;i < FEC_K; ++i){
        encode.Encode(data[i],SIZE,encode_callback);
    }

    for(int index = 0; index < FEC_N;++index){
        for(int j = 0;j < SIZE;++j){
            printf("%d ",outdata[index][j]);
        }
        printf("\n");

    }
    printf("\n");

    memset(outdata[0],0,SIZE);
    memset(outdata[3],0,SIZE);

    FecPacketHead heads[FEC_N];

    for(int index = 0; index < FEC_N;++index){
        heads[index].fec_group_id = 0;
        heads[index].fec_index = index;
        heads[index].fec_k = FEC_K;
        heads[index].fec_n = FEC_N;
    }


    for(int index = 0; index < FEC_K;++index){
        if(index == 0 ){
            decode.Decode(heads + FEC_K,outdata[FEC_K],SIZE,decode_callback);
        }
        else if(index == 3){
            decode.Decode(heads + FEC_K + 1,outdata[FEC_K + 1],SIZE,decode_callback);
        }
        else{
            decode.Decode(heads + index,outdata[index],SIZE,decode_callback);
        }
    }


    for(int index = 0; index < FEC_N;++index){
        for(int j = 0;j < SIZE;++j){
            printf("%d ",outdata[index][j]);
        }
        printf("\n");

    }
}


void test_org_fec(){
    const int SIZE = 10;
    unsigned char* input_data[FEC_K] = {0};
    unsigned char* temp[FEC_N]= {0};
    unsigned char* output_data[FEC_N] = {0};

    int num = 0;
    for(int index = 0; index < FEC_K;++index){
        input_data[index] = new unsigned char[SIZE];
        for(int j = 0;j < SIZE;++j){
            input_data[index][j] = num++;
        }
    }

    for(int index = 0; index < FEC_N;++index){
        output_data[index] = new unsigned char[SIZE];
    }

    void* fec = fec_new(FEC_K,FEC_N);

    for(int i = 0;i < FEC_N; ++i){
        fec_encode(fec,(void**)input_data,output_data[i],i,SIZE);
    }

    for(int index = 0; index < FEC_N;++index){
        for(int j = 0;j < SIZE;++j){
            printf("%d ",output_data[index][j]);
        }
        printf("\n");

    }
    printf("\n");
    printf("\n");

    memset(output_data[0],0,SIZE);
    memset(output_data[3],0,SIZE);

    for(int index = 0; index < FEC_K;++index){
        for(int j = 0;j < SIZE;++j){
            printf("%d ",output_data[index][j]);
        }
        printf("\n");

    }
    printf("\n");
    printf("\n");

    int index[FEC_K] = {FEC_K,1,2,FEC_K+1};
    temp[0] = output_data[FEC_K];
    temp[1] = output_data[1];
    temp[2] = output_data[2];
    temp[3] = output_data[FEC_K + 1];
    fec_decode(fec,(void**)temp,index,SIZE);

    for(int index = 0; index < FEC_K;++index){
        for(int j = 0;j < SIZE;++j){
            printf("%d ",temp[index][j]);
        }
        printf("\n");
    }

    for(int index = 0; index < FEC_K;++index){
        delete[] input_data[index] ;
    }

    for(int index = 0; index < FEC_N;++index){
        delete[] output_data[index] ;
    }
    fec_free(fec);
}

