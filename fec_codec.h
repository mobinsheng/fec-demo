#ifndef FECCODEC_H
#define FECCODEC_H

#ifdef __cplusplus
extern "C"{
#endif
#include "fec.h"
#ifdef __cplusplus
}
#endif


#include <assert.h>
#include <vector>
#include <map>
#include <memory>

#include "packet.h"

using namespace std;

// FEC编码回调
typedef void (*fec_encode_callback)(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

// FEC解码回调
typedef void (*fec_decode_callback)(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

class FecCodec
{
public:
    FecCodec(int k,int n,int packetSize){
        fec_k = k;
        fec_n = n;
        packet_count = 0;
        raw_packet_count = 0;
        packet_size = packetSize;
        last_packet_size = packetSize;

        last_group_id = 0;
        fec = fec_new(k,n);
        assert(fec);
        
        for(int i = 0; i < kMaxN; ++i){
            input_data[i] = new uint8_t[MAX_FEC_PACKET_SIZE];
            output_data[i] = new uint8_t[MAX_FEC_PACKET_SIZE];
            used_marks[i] = false;
        }
    }
    ~FecCodec(){
        for(int i = 0; i < kMaxN; ++i){
            delete[] input_data[i];
            delete[] output_data[i];
        }
        fec_free(fec);
    }


    void Encode(uint8_t* data,size_t size,fec_encode_callback callback){
        // 如果是原始数据，那么先复制到数组中，然后返回，因为要凑齐k个数据才能fec编码
        if(packet_count < fec_k){
            memcpy(input_data[packet_count],data,size);

            fec_encode(fec,(void**)input_data,output_data[packet_count],packet_count,size);

            callback(last_group_id,fec_k,fec_n,packet_count,output_data[packet_count],size);

            ++packet_count;
            ++raw_packet_count;
        }

        if(packet_count < fec_k)
            return ;

        for(int i = fec_k; i < fec_n; ++i){
            fec_encode(fec,(void**)input_data,output_data[i],i,packet_size);
            callback(last_group_id,fec_k,fec_n,i,output_data[i],packet_size);
            ++packet_count;
        }

        ++last_group_id;

        // 每次处理完一个group，就清理一次，准备下一个group
        Clear();
    }

    // 如果packet == NULL，那么表示开始利用冗余块去恢复原始块
    void Decode(FecPacketHead* head,uint8_t* data,size_t size,fec_decode_callback callback){

        last_packet_size = head->last_packet_size;

        if(head->fec_group_id != last_group_id){
            Clear();
            last_group_id = head->fec_group_id;
        }

        // 如果index指向的位置还没有数据，表示是第一次接收到这块数据，那么把数据放进buffer中
        // 另外，如果这块数据是原始数据，那么还需要回调给上层
        if(used_marks[head->fec_index] == false){

            memcpy(input_data[head->fec_index],data,size);
            used_marks[head->fec_index] = true;

            // 属于原始包，那么返回它；冗余包不能返回
            if(head->fec_index < fec_k){
                callback(last_group_id,head->fec_k,head->fec_n,head->fec_index,data,size);
                ++raw_packet_count;
            }

            ++packet_count;
        }

        if(packet_count < fec_k){
            return ;
        }

        // 原始数据没有丢失，不需要恢复
        if(raw_packet_count == fec_k){
            return;
        }

        // 开始恢复

        uint8_t* recovery[kMaxN] = {0};
        int recoveryIndex[kMaxN] = {0};

        bool tempMarks[kMaxN] = {false};
        memcpy(tempMarks,used_marks,sizeof(bool) * kMaxN);

        // 先把原始数据放到指定位置
        for(int i = 0; i < fec_k; ++i){
            if(tempMarks[i] == true){
                recovery[i] = input_data[i];
                recoveryIndex[i] = i;
            }
        }

        // 把冗余数据放到指定位置
        for(int i = 0; i < fec_k; ++i){
            if(tempMarks[i]){
                continue;
            }

            for(int j = fec_k; j < fec_n; ++j){
                if(tempMarks[j] == true){
                    tempMarks[j] = false;
                    tempMarks[i] = true;
                    recovery[i] = input_data[j];
                    recoveryIndex[i] = j;
                    break;
                }
            }
        }

        for(int i = 0; i < fec_k; ++i){
            if(recovery[i] == NULL || tempMarks[i] == false){
                return;
            }
        }

        // 数据恢复
        fec_decode(fec,(void**)recovery,recoveryIndex,packet_size);//recovery

        for(int i =0; i < fec_k;++i){
            if(recoveryIndex[i]== i){
                continue;
            }
            callback(last_group_id,fec_k,fec_n,i,recovery[i],packet_size);

            ++raw_packet_count;
            ++packet_count;
        }
        /*for(int i = 0; i < fec_k; ++i){
            if(used_marks[i]){
                continue;
            }

            callback(last_group_id,fec_k,fec_n,i,input_data[i],packet_size);

            ++raw_packet_count;
            ++packet_count;

        }*/
    }

    enum{
        kMaxN = 128, // k或者n的最大值，简单设置为128，没必要设置得太大

    };

private:
    void Clear(){
        packet_count = 0;
        raw_packet_count = 0;
        memset(used_marks,0,sizeof(bool) * kMaxN);
    }


    void * fec;

    // k和n
    int16_t fec_k;
    int16_t fec_n;

    // 普通的包的大小（不包括FEC头部，只是数据部分），由构造函数传进来的参数决定
    int16_t packet_size;

    // 最后一个包的小（不包括FEC头部，只是数据部分）
    // 对于一个文件或者一段数据来说，它的长度一般不可能是FEC包的整数倍，因此
    // 最后一个包的长度可能稍微小一点
    int16_t last_packet_size;

    // fec包的数量：原始包+冗余包
    int16_t packet_count;
    // fec原始包的数量
    int16_t raw_packet_count;

    uint8_t* input_data[kMaxN];
    uint8_t* output_data[kMaxN];

    // 解码时使用，用于标记一个group中的某一个包是否存在
    bool used_marks[kMaxN];

    // 当前group的id
    // k个原始包加上n-k个冗余包构成一个group
    uint64_t last_group_id;
};

#endif // FECCODEC_H
