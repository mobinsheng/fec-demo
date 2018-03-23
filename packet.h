#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "marshall.h"

#define FEC_K 4
#define FEC_N 8

class FecPacket:public Marshallable{
public:
    FecPacket(){
        version = 0;
    }

    FecPacket(const char* buf,size_t len):Marshallable(buf,len){
        version = 0;
    }

    // 序列化
    virtual void marshal(){
        write_int16(version);
        write_int16(fec_k);
        write_int16(fec_n);
        write_int16(fec_index);
        write_int64(group_id);
        write_uint64(seq_num);
        write_int16(size);
        write((char*)data,size);
    }

    // 反序列化
    virtual void unmarshal(){
        version = read_int16();
        fec_k = read_int16();
        fec_n = read_int16();
        fec_index = read_int16();
        group_id = read_int64();
        seq_num = read_uint64();
        size = read_int16();
        assert(size <= 1024);
        read((char*)data,size);
    }

    void WriteData(const char* buf,int16_t len){
        memcpy(data,buf,len);
        size = len;
    }

    int16_t version;

    int16_t fec_k;
    int16_t fec_n;
    int16_t fec_index;
    uint64_t group_id;

    uint64_t seq_num;

    int16_t size;

    char data[1024];
};

class NetPacket{
public:
private:
};
#endif // PACKET_H
