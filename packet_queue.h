#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <deque>
#include <vector>
#include <memory>
#include <stdint.h>
#include <pthread.h>

using namespace std;

class PacketQueue
{
public:
    PacketQueue(){
        pthread_mutex_init(&lock,NULL);
        pthread_cond_init(&cond,NULL);
    }

    ~PacketQueue(){
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);
    }

    void Push(uint8_t* data,size_t size){

        Lock();
        shared_ptr<vector<uint8_t> > vec(new vector<uint8_t>(data,data+size));
        bool empty = inner_queue.empty();
        inner_queue.push_back(vec);
        if(empty){
            Signal();
        }
        Unlock();
    }

    shared_ptr<vector<uint8_t> > Pop(){
        Lock();
        Wait();
        shared_ptr<vector<uint8_t> >  ret = inner_queue[0];
        inner_queue.pop_front();
        Unlock();
        return ret;
    }

    size_t Size(){
        Lock();
        size_t size = inner_queue.size();
        Unlock();
        return size;
    }

    bool Empty(){
        Lock();
        bool empty = inner_queue.empty();
        Unlock();
        return empty;
    }

private:
    void Lock(){
        pthread_mutex_lock(&lock);
    }

    void Unlock(){
        pthread_mutex_unlock(&lock);
    }

    void Wait(){
        while(inner_queue.empty()){
            pthread_cond_wait(&cond,&lock);
        }
    }

    void Signal(){
        pthread_cond_signal(&cond);
    }

    pthread_mutex_t lock;
    pthread_cond_t cond;
    deque<shared_ptr<vector<uint8_t> > > inner_queue;
};

#endif // PACKET_QUEUE_H
