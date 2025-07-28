#ifndef _PTHREADPOOL_H
#define _PTHREADPOOL_H
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

typedef struct ToSearchName
{
    char name[1000];
    bool file_or_dir;   //1 is file  0 is dir
}ToSearchName;

typedef struct Task
{
    void (*function)(void* arg);
    void* arg;
}Task;

typedef struct ThreadPool
{
    int tasksize;//目前的任务个数
    int taskcapcity;//任务池的容量
    int front;//队头
    int rear;//队尾
    Task* task;//任务队列

    pthread_t* threadids;//线程数组
    int threadnum;//多少个线程
    pthread_mutex_t mutexpool;//线程池的锁
    pthread_cond_t full;    //满了，生产者那里开始暂停
    pthread_cond_t empty;   //空了，消费者开始暂停

    bool shutdown;//线程池是否关闭   1close  0open
}ThreadPool;

//线程池初始化
ThreadPool* threadinit(int num, int queuesize);
//添加任务，生产者
void addtask(ThreadPool* pool, void (*func)(void*), void* arg);
//摧毁线程池
bool threaddestroy(ThreadPool* pool);
//工作的线程，消费者
void* worker(void* arg);
#endif  // _PTHREADPOOL_H
