#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

#include <pthread.h>

#include "udp_server.h"
#include "udp_client.h"
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

static const char* server_ip = "127.0.0.1";
static const int port = 9898;

void* server_thread(void*){
    udp_server(server_ip,port);
    return NULL;
}

void* client_thread(void*){
    udp_client(server_ip,port);
    return NULL;
}
void test_fec();
void test_org_fec();


int main()
{
    pthread_t t1,t2;
    pthread_create(&t1,NULL,server_thread,NULL);
    pthread_create(&t2,NULL,client_thread,NULL);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

    return 0;
}

