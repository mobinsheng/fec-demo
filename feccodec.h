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
//#include <memory>

#include "packet.h"

using namespace std;


class FecCodec
{
public:
    FecCodec(int k,int n,int packetSize){
        fec_k = k;
        fec_n = n;
        packet_count = 0;
        fec_packet_size = packetSize;

        fec = fec_new(k,n);
        assert(fec);
        
        for(int i = 0; i < max_n; ++i){
            input_data[i] = new uint8_t[max_packet_len];
            output_data[i] = new uint8_t[max_packet_len];
            marks[i] = false;
        }
    }
    ~FecCodec(){
        for(int i = 0; i < max_n; ++i){
            delete[] input_data[i];
            delete[] output_data[i];
        }
    }
    
    // 如果data==NULL or size==0，那么就表示编码冗余块
    vector<FecPacket> Encode(uint8_t* data,size_t size){
        vector<FecPacket> ret;

        // 如果是原始数据，那么先复制到数组中，然后返回，因为要凑齐k个数据才能fec编码
        if(packet_count < fec_k){
            memcpy(input_data[packet_count],data,size);

            FecPacket packet;
            packet.fec_k = fec_k;
            packet.fec_n = fec_n;
            packet.fec_index = packet_count;
            packet.group_id = 0;

            memcpy(packet.data,data,size);
            packet.size = size;
            ret.push_back(packet);

            ++packet_count;

            return ret;
        }

        // 冗余块的索引数组
        unsigned redundancyBlockIndexArray[max_n];
        for(int i = 0; i < fec_n - fec_k; ++i){
            redundancyBlockIndexArray[i] = fec_k + i;
        }
        // 冗余块的索引数组的长度
        int redundancyBlockIndexArraySize = fec_n - fec_k;
        // 由原始块构建冗余块
        fec_encode(fec,input_data,input_data,redundancyBlockIndexArray,redundancyBlockIndexArraySize,fec_packet_size);
        for(int i = fec_k; i < fec_n;++i){
            FecPacket packet;
            packet.fec_k = fec_k;
            packet.fec_n = fec_n;
            packet.fec_index = i;
            packet.group_id = 0;

            memcpy(packet.data,input_data[i],size);
            packet.size = size;
            ret.push_back(packet);
        }
        return ret;
    }

    // 如果packet == NULL，那么表示开始利用冗余块去恢复原始块
    vector<FecPacket> Decode(FecPacket* packet){
        vector<FecPacket> ret;

        // 接收到的包的数量小于k，数据无法恢复
        if(packet == NULL && packet_count < fec_k){
            return ret;
        }

        if(packet){
            if(marks[packet->fec_index] == false){

                memcpy(input_data[packet_count],packet->data,packet->size);
                marks[packet->fec_index] = true;
                // 属于原始包，那么返回它；冗余包不能返回
                if(packet_count < fec_k){
                    FecPacket retPacket;
                    memcpy(retPacket.data,packet->data,packet->size);
                    retPacket.size = packet->size;
                    retPacket.fec_index = packet->fec_index;
                    retPacket.fec_k = packet->fec_k;
                    retPacket.fec_n = packet->fec_n;
                    retPacket.group_id = packet->group_id;
                    retPacket.seq_num = packet->seq_num;
                    retPacket.version = packet->version;
                    ret.push_back(retPacket);
                }
                ++packet_count;
            }
            return ret;
        }

        // 无法恢复
        if(packet_count < fec_k){
            return ret;
        }

        uint8_t* recovery[max_n] = {0};
        unsigned recoveryIndex[max_n] = {0};

        for(int i = 0; i < fec_k; ++i){
            if(marks[i] == false){
                bool found = false;
                for(int index = fec_k; index < fec_n; ++index){
                    if(marks[index]){
                        recovery[i] = input_data[index];
                        recoveryIndex[i] = index;

                        marks[index] = false;
                        found = true;
                        break;
                    }
                }
                if(found == false){
                    return ret;
                }
            }
            else{
                recovery[i] = input_data[i];
                recoveryIndex[i] = i;
            }
            // https://blog.csdn.net/liangxanhai/article/details/8129641
        }

        fec_decode(fec,recovery,output_data,recoveryIndex,fec_packet_size);

        int index = 0;
        for(int i = 0; i < fec_k; ++i){
            if(marks[i] == false){
                /*FecPacket retPacket;
                memcpy(retPacket.data,output_data[index],packet->size);
                retPacket.size = packet->size;
                retPacket.fec_index = packet->fec_index;
                retPacket.fec_k = packet->fec_k;
                retPacket.fec_n = packet->fec_n;
                retPacket.group_id = packet->group_id;
                retPacket.seq_num = packet->seq_num;
                retPacket.version = packet->version;
                ret.push_back(retPacket);*/
                ++index;
            }
        }
        //fec_decode(fec,input_data,output_data,);
    }

private:
    fec_t* fec;
    enum{
        max_n = 128, // k或者n的最大值，当然这里只是简单的demo，所以简单设置为128
        max_packet_len = 1024, // 简单的设置为1024
    };
    
    // k和n
    uint32_t fec_k;
    uint32_t fec_n;

    uint32_t fec_packet_size;
    uint32_t packet_count;
    uint8_t* input_data[max_n];
    uint8_t* output_data[max_n];

    bool marks[max_n];
};

#endif // FECCODEC_H
