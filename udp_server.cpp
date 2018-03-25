#include "udp_server.h"
#include "packet.h"
#include "fec_codec.h"
#include <stdint.h>
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>

using namespace std;

static FILE* fp_out = NULL;


#define MAX_GROUPS 4

typedef vector<uint8_t> ByteArray;

struct Group{
    uint64_t group_id;
    map<uint64_t,shared_ptr<ByteArray> > packet_list; // key是seq-num，value是包的数据
};


struct Server{
    Server(int k,int n,int maxPacketSize):codec(k,n,maxPacketSize){
        server_fd = 0;
    }

    map<uint64_t,shared_ptr<Group> > groups;
    unordered_map<uint64_t,bool> group_marks;
    map<int16_t,vector<uint8_t> >reorder_container;

    FecCodec codec;
    int server_fd;

};

static Server serv(FEC_K,FEC_N,MAX_FEC_PACKET_SIZE);

void decode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

void reorder_data(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

void decode_fec_group(uint64_t groupID);

void collect_packet(uint8_t* data,size_t size);

void decode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

void reorder_data(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size);

void flush_buffer();

int create_server_socket (const char* ip,int port);

int udp_server(const char *ip, int port){
    fp_out = fopen("/home/mbs/1.jpg","wb");

    serv.server_fd = create_server_socket(ip,port);
    if(serv.server_fd < 0){
        return EXIT_FAILURE;
    }
    struct sockaddr_in clientAddr;
    socklen_t addrLen = 0;

    const int buf_size = 1024 * 8;
    char buf[buf_size];

    while (true) {
        int ret = recvfrom(serv.server_fd,buf,buf_size,0,(sockaddr*)&clientAddr,&addrLen);
        if(ret <= 0){
            break;
        }
        if(ret < NetPacketHead::kHeadSize){
            continue;
        }

        NetPacketHead netHead(buf,ret);
        netHead.unmarshal();

        if(netHead.type == kCommandType){
            if(netHead.cmd == kCmdStart){
                // 开始传输数据，不用做什么处理，直接开始接收下一个数据包
                continue;
            }

            if(netHead.cmd == kCmdFinish){
                // 数据发送结束，退出循环
                break;
            }
        }
        else{

            if(ret - NetPacketHead::kHeadSize < FecPacketHead::kHeadSize){
                continue;
            }

            // 收集数据包，构成一个group之后（或者集齐了group中的k个包），开始进行fec解码
            collect_packet((uint8_t*)buf + NetPacketHead::kHeadSize,ret - NetPacketHead::kHeadSize);
        }

    }
    flush_buffer();

    close(serv.server_fd);

    fclose(fp_out);
}

int create_server_socket (const char* ip,int port){
    socklen_t l;
    int sfd;

    struct sockaddr_in sock_serv;

    sfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sfd == -1){
        perror("socket fail");
        return EXIT_FAILURE;
    }

    l=sizeof(struct sockaddr_in);
    bzero(&sock_serv,l);

    if(ip == NULL || strlen(ip) == 0){
        sock_serv.sin_family=AF_INET;
        sock_serv.sin_port=htons(port);
        sock_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    }
    else{
        sock_serv.sin_family=AF_INET;
        sock_serv.sin_port=htons(port);
        if (inet_pton(AF_INET,ip,&sock_serv.sin_addr)==0){
            printf("Invalid IP adress\n");
            return EXIT_FAILURE;
        }
    }

    if(::bind(sfd,(struct sockaddr*)&sock_serv,l)==-1){
        perror("bind fail");
        return EXIT_FAILURE;
    }

    return sfd;
}

void decode_fec_group(uint64_t groupID){
    if(serv.groups.find(groupID) == serv.groups.end()){
        return;
    }
    shared_ptr<Group> group = serv.groups.find(groupID)->second;

    do{
        if(group->packet_list.size() < FEC_N){
            //printf("lost packet[%lld]\n",group->group_id);
        }

        if(group->packet_list.size() < FEC_K){
            printf("can not fec decode\n");
            group->packet_list.clear();
            break;
        }
        int count = 0;

        map<uint64_t,shared_ptr<ByteArray> >::iterator it;

        for(it = group->packet_list.begin();it != group->packet_list.end(); ++it){
            shared_ptr<ByteArray> rawData = it->second;

            FecPacketHead head((char*)&(*rawData)[0],rawData->size());

            head.unmarshal();

            ++count;

            if(count > FEC_K){
                continue;
            }

            serv.codec.Decode(&head,(unsigned char*)&(*rawData)[0]+FecPacketHead::kHeadSize,rawData->size() - FecPacketHead::kHeadSize,decode_callback);

        }
    }while(0);
}

void collect_packet(uint8_t* data,size_t size){
    // 先把fec头部解析出来，下面会有用
    FecPacketHead tempHead((char*)data ,size);
    tempHead.unmarshal();
    uint64_t seqNum = tempHead.fec_index;
    uint64_t groupId = tempHead.fec_group_id;
    int16_t index = tempHead.fec_index;

    shared_ptr<Group> group= nullptr;

    // 太老的包，直接丢弃
    if(serv.groups.size() >= MAX_GROUPS){
        group = serv.groups.begin()->second;
        if(groupId < group->group_id){ // 过期
            printf("drop[%lld][%d]",groupId,index);
            return;
        }
    }

    // 如果某一个group已经解码了，但是后面又受到这个group中的一个包，那么直接丢弃
    if(serv.group_marks.find(groupId) != serv.group_marks.end()){
        if(serv.group_marks[groupId]){
            printf("drop[%lld][%d]\n",groupId,index);
            return ;
        }
    }

    // 检查新包所属的group是否存在，不存在就创建
    if(serv.groups.find(groupId) == serv.groups.end()){
        shared_ptr<Group> g(new Group);
        g->group_id = groupId;
        serv.groups[groupId] = g;
        group = g;
    }
    else{
        group = serv.groups.find(groupId)->second;
    }
    // 保存数据
    shared_ptr< ByteArray> tempData(new ByteArray(data,data+size));
    group->packet_list[seqNum] = tempData;

    // 这个包所属的group还没有被处理
    serv.group_marks[groupId] = false;

    //  检查，是否有太老的group
    uint64_t groupIdDiff = 0;
    groupIdDiff = serv.groups.rbegin()->second->group_id - serv.groups.begin()->second->group_id;

    while(groupIdDiff > MAX_GROUPS){
        shared_ptr<Group> g = serv.groups.begin()->second;
        if(g->packet_list.size() >= FEC_K){
            decode_fec_group(g->group_id);
        }
        else{
            printf("drop[%lld]\n",groupId);
        }

        serv.groups.erase(serv.groups.begin());
        serv.group_marks[g->group_id] = true;

        groupIdDiff = serv.groups.rbegin()->second->group_id - serv.groups.begin()->second->group_id;
    }

    // 再检查所有的group，如果某一个group的包的数量达到了 fec_n 那么可以进行fec解码了
    // 如果还不够 fec_n个包，那还可以继续缓存，因为乱序、丢包等问题可能导致数据迟到
    map<uint64_t,shared_ptr<Group> >::iterator it = serv.groups.begin();
    while (it != serv.groups.end()) {
        shared_ptr<Group> g = it->second;
        if(g->packet_list.size() == FEC_N){
            decode_fec_group(g->group_id);
            it = serv.groups.erase(it);
            serv.group_marks[g->group_id] = true;
        }
        else{
            ++it;
        }
    }
}

void decode_callback(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size){
    reorder_data(groupId,k,n,index,data,size);
}

void reorder_data(uint64_t groupId,int16_t k,int16_t n,int16_t index,uint8_t* data,size_t size){
    vector<uint8_t> v(data,data+size);
    serv.reorder_container[index] = v;

    if(serv.reorder_container.size() == k){
        map<int16_t,vector<uint8_t> >::iterator it;
        for(it = serv.reorder_container.begin(); it != serv.reorder_container.end();++it){
            v = it->second;
            fwrite(&v[0],1,v.size(),fp_out);
            fflush(fp_out);
        }
        serv.reorder_container.clear();
    }
}

void flush_buffer(){
    map<uint64_t,shared_ptr<Group> >::iterator it = serv.groups.begin();
    while (it != serv.groups.end()) {
        shared_ptr<Group> g = it->second;
        /*if(g->packet_list.size() >= FEC_K){
            decode_fec_group(g->group_id);
            serv.group_marks[g->group_id] = true;
        }
        else{
            printf("drop[%lld]\n",g->group_id);
        }*/
        decode_fec_group(g->group_id);
        serv.group_marks[g->group_id] = true;
        it = serv.groups.erase(it);
    }

    map<int16_t,vector<uint8_t> >::iterator it2;
    for(it2 = serv.reorder_container.begin(); it2 != serv.reorder_container.end();++it2){
        vector<uint8_t> v = it2->second;
        fwrite(&v[0],1,v.size(),fp_out);
        fflush(fp_out);
    }
}
