
/*
 * Copyright (c) Beijing Tsing Micro Co.,Ltd. All rights reserved.
 * Description: base type definitions of alg
 * Author: Tsing Micro solution-application group
 * Create: 2024/06/01
 */

#ifndef _TS_ALG_NN_H_
#define _TS_ALG_NN_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief nms for YOLOv5
 * @param[in] torchCat, pointer of input candidate BBoxes
 * @param[in] len1, torchCat data length
 * @param[in] len2, detect channel num
 * @param[out] nmsOut, pointer of BBoxes afer nms
 * @param[in] thresh, IOU threshold
 * @param[in] maxDet, max num of nms detect
 * @param[in] scoreOffset, offset of BBox score in channel
 * @return nms result num
 */
int TS_ALG_NMS_V5(float *torchCat, int len1, int len2, int *nmsOut, float thresh, int maxDet, int scoreOffset);

/**
 * @brief nms for YOLOv8
 * @param[in] bboxes, pointer of input candidate BBoxes
 * @param[in] iouThehold, IOU threshold
 * @param[in] bboxes_num, bboxes data length
 * @param[in] chn, detect channel num
 * @param[in] maxDet, max num of nms detect
 * @param[out] nmsOut, pointer of BBoxes afer nms
 * @return nms result num
 */
int TS_ALG_NMS_V8(float *bboxes,  float iouThehold, int bboxes_num, int chn, int maxDet, int *nmsOut);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
