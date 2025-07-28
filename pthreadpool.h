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
    int tasksize;//Ŀǰ���������
    int taskcapcity;//����ص�����
    int front;//��ͷ
    int rear;//��β
    Task* task;//�������

    pthread_t* threadids;//�߳�����
    int threadnum;//���ٸ��߳�
    pthread_mutex_t mutexpool;//�̳߳ص���
    pthread_cond_t full;    //���ˣ����������￪ʼ��ͣ
    pthread_cond_t empty;   //���ˣ������߿�ʼ��ͣ

    bool shutdown;//�̳߳��Ƿ�ر�   1close  0open
}ThreadPool;

//�̳߳س�ʼ��
ThreadPool* threadinit(int num, int queuesize);
//�������������
void addtask(ThreadPool* pool, void (*func)(void*), void* arg);
//�ݻ��̳߳�
bool threaddestroy(ThreadPool* pool);
//�������̣߳�������
void* worker(void* arg);
#endif  // _PTHREADPOOL_H
