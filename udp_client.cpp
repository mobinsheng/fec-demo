#include "udp_client.h"
#include "packet.h"
//#include "fec.h"

int create_client_socket (const char* ip,int port,struct sockaddr_in* sock_serv);



int udp_client(const char *ip, int port){
    struct sockaddr_in sock_serv;
    int fd = create_client_socket(ip,port,&sock_serv);
    if(fd < 0){
        return EXIT_FAILURE;
    }

    FILE* fp = fopen("/Users/mobinsheng/work/fec-demo/test.jpg","rb");
    assert(fp);

    const int buffer_size = 1024;
    char buf[buffer_size] = {0};
    uint64_t seqNum = 0;
    while (!feof(fp)) {
        int ret = fread(buf,1,buffer_size,fp);
        if(ret <= 0){
            break;
        }
        FecPacket packet;
        packet.WriteData((char*)buf,ret);
        packet.seq_num = seqNum++;
        packet.marshal();
        const char* sendData = packet.get_data();
        int sendLen = packet.get_size();

        socklen_t addrLen=sizeof(struct sockaddr_in);

        sendto(fd,sendData,sendLen,0,(sockaddr*)&sock_serv,addrLen);
    }

    close(fd);
}

int create_client_socket (const char* ip,int port,struct sockaddr_in* sock_serv){
    int l;
    int sfd;
    sfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sfd == -1){
        perror("socket fail");
        return EXIT_FAILURE;
    }

    //preparation de l'adresse de la socket destination
    l=sizeof(struct sockaddr_in);
    bzero(sock_serv,l);

    sock_serv->sin_family=AF_INET;
    sock_serv->sin_port=htons(port);
    if (inet_pton(AF_INET,ip,&sock_serv->sin_addr)==0){
        printf("Invalid IP adress\n");
        return EXIT_FAILURE;
    }

    return sfd;
}
