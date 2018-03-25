#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "marshall.h"

#define FEC_K 4
#define FEC_N 8
#define MAX_FEC_PACKET_SIZE 1024 // 简单的设置为1024

enum NetPacketType{
    kCommandType = 1,
    kMediaType ,
};

enum Command{
    kCmdStart = 1,
    kCmdFinish ,
};

class NetPacketHead:public Marshallable{
public:
    NetPacketHead(){
        version = 0;
    }

    NetPacketHead(const char* buf,size_t len):Marshallable(buf,len){
        version = 0;
    }

    NetPacketHead(const NetPacketHead& v){
        this->type = v.type;
        this->version = v.version;
        this->cmd = v.cmd;
        this->packet_size = v.packet_size;
    }

    NetPacketHead& operator =(const NetPacketHead& v){
        if(this == &v){
            return *this;
        }
        this->type = v.type;
        this->version = v.version;
        this->cmd = v.cmd;
        this->packet_size = v.packet_size;
        return *this;
    }

    // 序列化
    virtual void marshal(){
        write_int16(version);
        write_int16(type);
        write_int16(cmd);
        write_int16(packet_size);
    }

    // 反序列化
    virtual void unmarshal(){
        version = read_int16();
        type = read_int16();
        cmd = read_int16();
        packet_size = read_int16();
    }

    enum{
        kHeadSize = 8, // 包头大小
    };

    int16_t version; // 2B
    int16_t type; // 2B
    int16_t cmd; // 2B
    int16_t packet_size; // 2B 数据部分的长度
};


// fec包头部信息
/*
 * 发送的时候，先利用原始数据生成冗余数据，然后每一块数据（原始数据和冗余数据）都要一个包头
 * 先把头部序列化，然后和数据一起发送，头部在前面
 *
 * 接收的时候，收到的数据块是头部+数据，先解析头部，然后利用收到的数据进行数据恢复
 *
 * 假设每一块数据的大小是packet-size，packet-size不超过1024
 * 最后一块的数据可能小于packet-size，那么就在后面填充0
 */
class FecPacketHead:public Marshallable{
public:
    FecPacketHead(){
        version = 0;
    }

    FecPacketHead(const char* buf,size_t len):Marshallable(buf,len){
        version = 0;
    }

    FecPacketHead(const FecPacketHead& v){
        this->fec_index = v.fec_index;
        this->fec_k = v.fec_k;
        this->fec_n = v.fec_n;
        this->fec_group_id = v.fec_group_id;
        this->seq_num = v.seq_num;
        this->packet_size = v.packet_size;
        this->version = v.version;
    }

    FecPacketHead& operator =(const FecPacketHead& v){
        if(this == &v){
            return *this;
        }
        this->fec_index = v.fec_index;
        this->fec_k = v.fec_k;
        this->fec_n = v.fec_n;
        this->fec_group_id = v.fec_group_id;
        this->seq_num = v.seq_num;
        this->packet_size = v.packet_size;
        this->version = v.version;
        return *this;
    }

    // 序列化
    virtual void marshal(){
        write_int16(version);
        write_int16(fec_k);
        write_int16(fec_n);
        write_int16(fec_index);
        write_uint64(fec_group_id);
        write_uint64(seq_num);
        write_int16(packet_size);
        write_int16(last_packet_size);
        write_int16(is_last_packet);
    }

    // 反序列化
    virtual void unmarshal(){
        version = read_int16();
        fec_k = read_int16();
        fec_n = read_int16();
        fec_index = read_int16();
        fec_group_id = read_uint64();
        seq_num = read_uint64();
        packet_size = read_int16();
        last_packet_size = read_int16();
        is_last_packet = read_int16();
    }

    enum{
        kHeadSize = 30, // 包头大小
    };

    int16_t version; // 2B
    int16_t fec_k; // 2B
    int16_t fec_n; // 2B
    int16_t fec_index; //2B
    uint64_t fec_group_id; // 8B
    uint64_t seq_num; // 8B
    int16_t packet_size; // 2B
    int16_t last_packet_size; // 2B
    int16_t is_last_packet; // 2B
};


#endif // PACKET_H
