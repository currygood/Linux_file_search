#define _DEFAULT_SOURCE
#include<dirent.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include"pthreadpool.h"
#include <unistd.h>
//level up：部分匹配
int all_ans = 0;
char* target_file_name = NULL;
char** ans_dir = NULL;
bool ishave = false;
pthread_mutex_t mutexhave;
pthread_mutex_t mutexhavename;
ThreadPool* pool;
char* begindir = NULL;

void search(void* arg)
{
    ToSearchName* file_this = (ToSearchName*)arg;
    struct dirent* entry_next;

    if (!file_this->file_or_dir)
    {
        DIR* dir_s;
        dir_s = opendir(file_this->name);
        if (dir_s == NULL)
        {
            fprintf(stderr, "无法打开目录 : %s\n", strerror(errno));
            free(file_this); // 确保内存释放
            return;
        }

        while ((entry_next = readdir(dir_s)) != NULL)
        {
            // 根据 d_type 判断文件类型
            switch (entry_next->d_type)
            {
            case DT_REG:
                ToSearchName* tos1 = malloc(sizeof(ToSearchName));
                strcpy(tos1->name, file_this->name);
                if (tos1->name[strlen(tos1->name) - 1] != '/')
                {
                    strcat(tos1->name, "/");
                }
                strcat(tos1->name, entry_next->d_name);
                tos1->file_or_dir = true;
                //printf("One %s dir add to search ...\n", tos1->name);
                addtask(pool, search, tos1);
                break;
            case DT_DIR:
                // 跳过.和..目录
                if (strcmp(entry_next->d_name, ".") == 0 || strcmp(entry_next->d_name, "..") == 0)
                    continue;

                ToSearchName* tos2 = malloc(sizeof(ToSearchName));
                strcpy(tos2->name, file_this->name);
                if (tos2->name[strlen(tos2->name) - 1] != '/')
                {
                    strcat(tos2->name, "/");
                }
                strcat(tos2->name, entry_next->d_name);
                tos2->file_or_dir = false;
                //printf("One %s dir add to search ...\n", tos2->name);
                addtask(pool, search, tos2);
                break;
            case DT_LNK:
                //printf("  [链接] %s -> (指向: %s)  不会进行搜索\n", entry_next->d_name, "需要进一步读取");
                break;
            default:
                //printf("  [其他] %s (类型: %d)   不会进行搜索\n", entry_next->d_name, entry_next->d_type);
            }
        }
        if (closedir(dir_s) != 0)
        {
            fprintf(stderr, "无法关闭目录: %s\n", strerror(errno));
        }
    }
    else
    {
        // 从完整路径中提取最后一个文件名
        char* filename = strrchr(file_this->name, '/');  // 查找最后一个'/'的位置
        if (filename == NULL)  // 如果路径中没有'/'，则完整路径即为文件名
            filename = file_this->name;
        else
            filename++;  // 跳过'/'，指向文件名部分
        // 比较提取的文件名与目标文件名
        if (strcmp(filename, target_file_name) == 0)
        {
            pthread_mutex_lock(&mutexhavename);
            // 动态分配内存
            ans_dir = (char**)realloc(ans_dir, (all_ans + 1) * sizeof(char*));
            ans_dir[all_ans] = (char*)malloc((strlen(file_this->name) + 1) * sizeof(char));
            strcpy(ans_dir[all_ans++], file_this->name);  // 保存完整路径（结果需要完整路径）
            pthread_mutex_unlock(&mutexhavename);
            pthread_mutex_lock(&mutexhave);
            ishave = true;
            pthread_mutex_unlock(&mutexhave);
        }
    }
    free(file_this);
}

int main()
{
    pool = threadinit(12, 10000000);
    pthread_mutex_init(&mutexhave, NULL);
    pthread_mutex_init(&mutexhavename, NULL);

    DIR* dir;
    struct dirent* entry;
    printf("请输入需要查找的文件名称:");
    // 假设用户输入不会超过256字节，这里简单处理
    target_file_name = (char*)malloc(256 * sizeof(char));
    if (target_file_name == NULL) 
    {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    scanf("%s", target_file_name);
    // 直接从根目录开始吧
    begindir = (char*)malloc(2 * sizeof(char)); // 假设路径长度不超过1024字节
    if (begindir == NULL)
    {
        fprintf(stderr, "内存分配失败\n");
        free(target_file_name);
        return 1;
    }
    strcpy(begindir, "/");
    // 打开目录
    dir = opendir(begindir);
    if (dir == NULL)
    {
        fprintf(stderr, "无法打开目录 : %s\n", strerror(errno));
        free(target_file_name);
        free(begindir);
        return 1;
    }

    // 提交初始任务
    while ((entry = readdir(dir)) != NULL)
    {
        // 根据 d_type 判断文件类型
        switch (entry->d_type)
        {
        case DT_REG:
            ToSearchName* to1 = malloc(sizeof(ToSearchName));
            strcpy(to1->name, begindir);
            if (to1->name[strlen(to1->name) - 1] != '/')
            {
                strcat(to1->name, "/");
            }
            strcat(to1->name, entry->d_name);
            to1->file_or_dir = true;
            //printf("One dir %s add to search... \n", to1->name);
            addtask(pool, search, to1);
            break;
        case DT_DIR:
            // 跳过.和..目录
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            ToSearchName* to2 = malloc(sizeof(ToSearchName));
            strcpy(to2->name, begindir);
            if (to2->name[strlen(to2->name) - 1] != '/')
            {
                strcat(to2->name, "/");
            }
            strcat(to2->name, entry->d_name);
            to2->file_or_dir = false;
            //printf("One %s dir add to search...\n", to2->name);
            addtask(pool, search, to2);
            break;
        case DT_LNK:
            //printf("  [链接] %s -> (指向: %s)  不会进行搜索\n", entry->d_name, "需要进一步读取");
            break;
        default:
            //printf("  [其他] %s (类型: %d)   不会进行搜索\n", entry->d_name, entry->d_type);
        }
    }

    // 关闭目录
    if (closedir(dir) != 0)
    {
        fprintf(stderr, "无法关闭目录: %s\n", strerror(errno));
        free(target_file_name);
        free(begindir);
        return 1;
    }

    pthread_mutex_lock(&pool->mutexpool);  // 加锁访问线程池状态
    while (pool->tasksize > 0)
    {
        // 若还有任务未完成，等待条件变量唤醒（避免忙等）
        pthread_cond_wait(&pool->empty, &pool->mutexpool);
    }
    pthread_mutex_unlock(&pool->mutexpool);
    // 确认所有任务完成后，再销毁线程池
    //printf("所有任务处理完成，开始销毁线程池...\n");
    threaddestroy(pool);

    if (ishave)
    {
        printf("Have %d this filename\n", all_ans);
        for (int i = 0; i < all_ans; ++i)
        {
            printf("%d  Have file %s in %s\n", i + 1, target_file_name, ans_dir[i]);
        }
    }
    else
    {
        printf("Have not file %s\n", target_file_name);
    }

    // 释放动态分配的内存
    for (int i = 0; i < all_ans; ++i) {
        free(ans_dir[i]);
    }
    free(ans_dir);
    free(target_file_name);
    free(begindir);

    pthread_mutex_destroy(&mutexhave);
    pthread_mutex_destroy(&mutexhavename);

    return 0;
}