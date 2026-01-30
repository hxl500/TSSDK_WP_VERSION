/* sample_alg_cpm.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "qi_sample_alg_cpm.h"

typedef struct {
    int alg_id;
    char name[ALG_NAME_LEN];
    alg_create_func create_func;
    alg_destroy_func destroy_func;
    void* handle;
    int initialized;
    int running;
    pthread_t thread_id;
} alg_entry_t;

static alg_entry_t g_alg_entries[MAX_ALGS] = {0};
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* alg_thread_func(void* arg)
{
    alg_entry_t* entry = (alg_entry_t*)arg;

    printf("algorithm thread started for %s(id:%d)\n", entry->name, entry->alg_id);

    while(entry->running)
    {
        // 模拟算法处理
        printf("processing algorithm %s(id:%d)\n", entry->name, entry->alg_id);
        usleep(50000); // 20fps
    }

    printf("algorithm thread stopped for %s(id:%d)\n", entry->name, entry->alg_id);

    return NULL;
}

int sample_alg_cpm_init(void)
{
    memset(g_alg_entries, 0, sizeof(g_alg_entries));

    VIDEO_ALG_CatDetect_Init();
    printf("sample_alg_cpm_init success\n");
    return 0;
}

int sample_alg_cpm_deinit(void)
{
    for(int i = 0; i < MAX_ALGS; i++)
    {
        if(g_alg_entries[i].initialized)
        {
            sample_alg_cpm_stop(i);
            sample_alg_cpm_unregister(i);
        }
    }

    printf("sample_alg_cpm_deinit success\n");
    return 0;
}

int sample_alg_cpm_register(int alg_id, const char* name, alg_create_func create_func, alg_destroy_func destroy_func)
{
    if(alg_id < 0 || alg_id >= MAX_ALGS)
    {
        printf("invalid algorithm id %d\n", alg_id);
        return -1;
    }

    if(name == NULL || create_func == NULL || destroy_func == NULL)
    {
        printf("invalid parameters\n");
        return -1;
    }

    pthread_mutex_lock(&g_mutex);

    alg_entry_t* entry = &g_alg_entries[alg_id];

    if(entry->initialized)
    {
        printf("algorithm %d already registered\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    entry->alg_id = alg_id;
    strncpy(entry->name, name, ALG_NAME_LEN-1);
    entry->create_func = create_func;
    entry->destroy_func = destroy_func;

    // 创建算法实例
    entry->handle = entry->create_func();
    if(entry->handle == NULL)
    {
        printf("create algorithm %s failed\n", name);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    entry->initialized = 1;

    pthread_mutex_unlock(&g_mutex);

    printf("algorithm %s(id:%d) registered successfully\n", name, alg_id);

    return 0;
}

int sample_alg_cpm_unregister(int alg_id)
{
    if(alg_id < 0 || alg_id >= MAX_ALGS)
    {
        printf("invalid algorithm id %d\n", alg_id);
        return -1;
    }

    pthread_mutex_lock(&g_mutex);

    alg_entry_t* entry = &g_alg_entries[alg_id];

    if(!entry->initialized)
    {
        printf("algorithm %d not registered\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    // 销毁算法实例
    if(entry->destroy_func && entry->handle)
    {
        entry->destroy_func(entry->handle);
    }

    memset(entry, 0, sizeof(alg_entry_t));

    pthread_mutex_unlock(&g_mutex);

    printf("algorithm %d unregistered successfully\n", alg_id);

    return 0;
}

int sample_alg_cpm_start(int alg_id)
{
    if(alg_id < 0 || alg_id >= MAX_ALGS)
    {
        printf("invalid algorithm id %d\n", alg_id);
        return -1;
    }

    pthread_mutex_lock(&g_mutex);

    alg_entry_t* entry = &g_alg_entries[alg_id];

    if(!entry->initialized)
    {
        printf("algorithm %d not initialized\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    if(entry->running)
    {
        printf("algorithm %d already running\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    entry->running = 1;

    // 创建算法处理线程
    int ret = pthread_create(&entry->thread_id, NULL, alg_thread_func, entry);
    if(ret != 0)
    {
        printf("pthread_create failed for algorithm %d\n", alg_id);
        entry->running = 0;
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    pthread_mutex_unlock(&g_mutex);

    printf("algorithm %d started successfully\n", alg_id);

    return 0;
}

int sample_alg_cpm_stop(int alg_id)
{
    if(alg_id < 0 || alg_id >= MAX_ALGS)
    {
        printf("invalid algorithm id %d\n", alg_id);
        return -1;
    }

    pthread_mutex_lock(&g_mutex);

    alg_entry_t* entry = &g_alg_entries[alg_id];

    if(!entry->initialized)
    {
        printf("algorithm %d not initialized\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    if(!entry->running)
    {
        printf("algorithm %d not running\n", alg_id);
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    entry->running = 0;
    pthread_join(entry->thread_id, NULL);

    pthread_mutex_unlock(&g_mutex);

    printf("algorithm %d stopped successfully\n", alg_id);

    return 0;
}