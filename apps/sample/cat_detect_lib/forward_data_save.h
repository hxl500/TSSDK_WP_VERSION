#ifndef __FORWARD_DATE_SAVE_H__
#define __FORWARD_DATE_SAVE_H__

#ifdef __cplusplus
extern "C" {
#endif
// typedef struct {
//     void* virAddr;
//     void* phyAddr;
//     int s32W;
//     int s32H;
//     int s32C;
//     int s32Stride;
//     int u32Size;
//     float fCoeff[16];
// } RNE_BLOB_S;

// typedef struct {
//     RNE_BLOB_S* stpBlob;
//     int s32BlobNum;
// } RNE_BLOBS_S;
#include "ts_rne_c_api.h"
#include "ts_rne_log.h"
#include "ts_rne_version.h"
#include "ts_rne_time.h"

#include "ts_alg_log.h"
 
#include "ts_rne_nn_input.h"
#include "arrr_diff.h"

#include "ts_alg_nn.h"
#include "ts_rne_nn_output.h"
// int save_blob_complete(RNE_BLOBS_S* outputBlobs, const char* output_dir);
// TS_S32 TS_ALG_RNE_RecordFile(RNE_BLOBS_S *blobs, TS_CHAR *debugLayerName, 
//                               TS_CHAR *outputPath, TS_S32 frameIndex, 
//                               TS_S32 frameCount);
#ifdef __cplusplus
}
#endif

#endif