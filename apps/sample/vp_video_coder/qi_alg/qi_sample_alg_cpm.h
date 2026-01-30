/* sample_alg_cpm.h */
#ifndef __SAMPLE_ALG_CPM_H__
#define __SAMPLE_ALG_CPM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALGS 16
#define ALG_NAME_LEN 32

typedef void* (*alg_create_func)(void);
typedef void (*alg_destroy_func)(void* handle);

int sample_alg_cpm_init(void);
int sample_alg_cpm_deinit(void);

int sample_alg_cpm_register(int alg_id, const char* name, alg_create_func create_func, alg_destroy_func destroy_func);
int sample_alg_cpm_unregister(int alg_id);

int sample_alg_cpm_start(int alg_id);
int sample_alg_cpm_stop(int alg_id);

#ifdef __cplusplus
}
#endif

#endif /* __SAMPLE_ALG_CPM_H__ */