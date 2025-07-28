#include"pthreadpool.h"

ThreadPool* threadinit(int num, int queuesize)
{
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	do
	{
		if (!pool)
		{
			printf("malloc threadpool fail...\n");
			break;
		}
		//threadids
		pool->threadids = (pthread_t*)malloc(sizeof(pthread_t) * num);
		if (!pool->threadids)
		{
			printf("malloc threadids fail...\n");
			break;
		}
		pool->threadnum = num;
		if (pthread_mutex_init(&pool->mutexpool, NULL) != 0 ||
			pthread_cond_init(&pool->full, NULL) != 0 ||
			pthread_cond_init(&pool->empty, NULL) != 0)
		{
			printf("mutex or condition init fail...\n");
			break;
		}
		//task
		pool->front = 0;
		pool->rear = 0;
		pool->taskcapcity = queuesize;
		pool->tasksize = 0;
		pool->task = (Task*)malloc(sizeof(Task) * queuesize);
		pool->shutdown = false;

		//创建线程
		for (int i = 0; i < num; ++i)
		{
			pthread_create(&pool->threadids[i], NULL, worker, pool);
		}
		return pool;
	} while (0);

	if (!pool->task)		free(pool->task);
	if (!pool->threadids)	free(pool->threadids);
	if (!pool)	free(pool);
	return NULL;
}

void addtask(ThreadPool* pool, void(*func)(void*), void* arg)
{
	if (!pool)
	{
		printf("pool is empty!...\n");
		return;
	}
	pthread_mutex_lock(&pool->mutexpool);
	while (pool->tasksize >= pool->taskcapcity)
	{
		//满了阻塞生产者线程
		pthread_cond_wait(&pool->full, &pool->mutexpool);
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->mutexpool);
		return;
	}
	pool->task[pool->rear].function = func;
	pool->task[pool->rear].arg = arg;
	pool->rear = (pool->rear + 1) % pool->taskcapcity;
	pool->tasksize++;
	pthread_cond_signal(&pool->empty);
	pthread_mutex_unlock(&pool->mutexpool);
}

bool threaddestroy(ThreadPool* pool)
{
	if (!pool)
	{
		return false;
	}
	pool->shutdown = true;
	for (int i = 0; i < pool->threadnum; ++i)
	{
		pthread_cond_broadcast(&pool->empty);
		pthread_join(pool->threadids[i], NULL);
	}
	if (pool->task)	free(pool->task);
	if (pool->threadids)	free(pool->threadids);
	pthread_mutexattr_destroy(&pool->mutexpool);
	pthread_cond_destroy(&pool->full);
	pthread_cond_destroy(&pool->empty);
	if (pool)	free(pool);
	pool = NULL;
	return true;
}

void* worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (1)
	{
		pthread_mutex_lock(&pool->mutexpool);
		while (pool->tasksize <= 0 && !pool->shutdown)
		{
			pthread_cond_wait(&pool->empty, &pool->mutexpool);
		}
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexpool);
			break;
		}
		Task ta;
		ta.function = pool->task[pool->front].function;
		ta.arg = pool->task[pool->front].arg;
		pool->front = (pool->front + 1) % pool->taskcapcity;
		pool->tasksize--;
		pthread_cond_signal(&pool->full);
		pthread_mutex_unlock(&pool->mutexpool);
		ta.function(ta.arg);
		// 任务处理完成，更新计数
		pthread_mutex_lock(&pool->mutexpool);
		// 若队列空且无正在运行的任务，可通知主线程（可选，用于快速唤醒）
		if (pool->tasksize == 0)
		{
			pthread_cond_broadcast(&pool->empty);  // 唤醒可能等待的主线程
		}
		pthread_mutex_unlock(&pool->mutexpool);
	}
}
