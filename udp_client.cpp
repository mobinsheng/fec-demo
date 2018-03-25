#include "udp_client.h"
#include "packet.h"
#include "fec_codec.h"
#include <stdint.h>
#include <map>
#include <memory>
#include <vector>
#include "random_generator.h"
#include "packet_queue.h"

using namespace std;

struct Client{

    int packet_size;

    int last_packet_size;

    FecCodec codec;

    uint64_t seq_num ;

    int client_fd;

    struct sockaddr_in server_addr;

    RandomGenerator rd;

    PacketQueue packet_queue;

    Client(int k,int n,int maxPacketSize):codec(k,n,maxPacketSize){
        client_fd = 0;
        packet_size = 0;
        last_packet_size = 0;
        seq_num = 0;

        rd.SetRandomDistributionParam(0,100); //  设置随机数产生器产生0～100的随机数
    }

};

static Client client(FEC_K,FEC_N,MAX_FEC_PACKET_SIZE);

int create_client_socket (const char* ip,int port,struct sockaddr_in* server_addr);

void send_data(const uint8_t* data,size_t size);

void encode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

static uint32_t get_file_size(const char* name);

static void* send_thread(void*);

int udp_client(const char *ip, int port){

    const char* fileName = "/home/mbs/work/cpp/streaming-media/fec-demo/test.jpg";

    client.client_fd = create_client_socket(ip,port,&client.server_addr);
    if(client.client_fd < 0){
        return EXIT_FAILURE;
    }

    uint32_t file_size =get_file_size(fileName);
    client.packet_size = MAX_FEC_PACKET_SIZE;
    client.last_packet_size = file_size % client.packet_size;
    if(client.last_packet_size == 0){
        client.last_packet_size = client.packet_size;
    }

    // 发送一个命令给服务器，表示开始
    NetPacketHead netHead;
    netHead.version = 0;
    netHead.type = kCommandType;
    netHead.cmd = kCmdStart;
    netHead.packet_size = 0;
    netHead.marshal();
    send_data((uint8_t*)netHead.get_data(),netHead.get_size());

    FILE* fp = fopen(fileName,"rb");
    assert(fp);

    const int buffer_size = 1024;
    char buf[buffer_size] = {0};

    int count = 0;
    while (!feof(fp)) {
        memset(buf,0,1024);

        int ret = fread(buf,1,buffer_size,fp);

        if(ret <= 0){
            break;
        }

        client.codec.Encode((unsigned char*)buf,ret,encode_callback);

        ++count;
        if(count % FEC_K == 0){
            usleep(2000); // 睡眠的比较久，是为了让上一个group的数据完全被服务器接收到
        }
    }

    usleep(2000);

    // 发送一个命令给服务器，表示结束
    netHead.clear();
    netHead.version = 0;
    netHead.type = kCommandType;
    netHead.cmd = kCmdFinish;
    netHead.packet_size = 0;
    netHead.marshal();
    send_data((uint8_t*)netHead.get_data(),netHead.get_size());

    close(client.client_fd);
}

int create_client_socket (const char* ip,int port,struct sockaddr_in* server_addr){
    int addrLen;
    int sclient_fd;
    sclient_fd = socket(AF_INET,SOCK_DGRAM,0);
    if (sclient_fd == -1){
        perror("socket fail");
        return EXIT_FAILURE;
    }
    addrLen=sizeof(struct sockaddr_in);
    bzero(server_addr,addrLen);

    server_addr->sin_family=AF_INET;
    server_addr->sin_port=htons(port);
    if (inet_pton(AF_INET,ip,&server_addr->sin_addr)==0){
        printf("Invalid IP adress\n");
        return EXIT_FAILURE;
    }
    return sclient_fd;
}


void send_data(const uint8_t* data,size_t size){
    socklen_t addrLen=sizeof(struct sockaddr_in);
    sendto(client.client_fd,data,size,0,(sockaddr*)&client.server_addr,addrLen);
}

void encode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size){

    if(groupId % 2 == 0){
        if(index < FEC_K){
            //printf("client drop [%lld][%d]\n",groupId,index);
            return;
        }
    }
    else{
        if(index > FEC_K){
            //printf("client drop [%lld][%d]\n",groupId,index);
            return;
        }
    }

    NetPacketHead netHead;
    netHead.type = kMediaType;
    netHead.version = 0;
    netHead.packet_size = FecPacketHead::kHeadSize + size;
    netHead.marshal();

    FecPacketHead head;
    head.fec_index = index;
    head.fec_k = k;
    head.fec_n = n;
    head.fec_group_id = groupId;
    head.seq_num = client.seq_num++;
    head.last_packet_size = client.last_packet_size;
    head.packet_size = client.packet_size;
    head.version = 0;

    head.marshal();

    uint8_t buf[1024 * 2] = {0};

    memmove(buf,netHead.get_data(),NetPacketHead::kHeadSize);
    memmove(buf + NetPacketHead::kHeadSize,head.get_data(),FecPacketHead::kHeadSize);
    memmove(buf + NetPacketHead::kHeadSize + FecPacketHead::kHeadSize,data,size);

    int sendSize = NetPacketHead::kHeadSize +FecPacketHead::kHeadSize + size;

    send_data(buf,sendSize);

    usleep(100);
}

static uint32_t get_file_size(const char* name){
    FILE* fp = fopen(name,"rb");
    assert(fp);
    fseek(fp,0L,SEEK_END);
    uint32_t file_size =ftell(fp);
    fclose(fp);
    return file_size;
}
