#include "udp_server.h"
#include "packet.h"
int create_server_socket (const char* ip,int port);

int udp_server(const char *ip, int port){
    int server_fd = create_server_socket(ip,port);
    if(server_fd < 0){
        return EXIT_FAILURE;
    }
    struct sockaddr_in clientAddr;
    socklen_t addrLen = 0;

    const int buf_size = 1024 * 8;
    char buf[buf_size];
    while (true) {
        int ret = recvfrom(server_fd,buf,buf_size,0,(sockaddr*)&clientAddr,&addrLen);
        if(ret <= 0){
            break;
        }

        FecPacket packet(buf,ret);
        packet.unmarshal();
        printf("%lld\n",packet.seq_num);

    }
    close(server_fd);
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

    //preparation de l'adresse de la socket destination
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

    //Affecter une identité au socket
    if(::bind(sfd,(struct sockaddr*)&sock_serv,l)==-1){
        perror("bind fail");
        return EXIT_FAILURE;
    }

    return sfd;
}
