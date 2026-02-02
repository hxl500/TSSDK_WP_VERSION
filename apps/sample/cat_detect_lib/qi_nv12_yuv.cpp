#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "qi_nv12_yuv.h"
// #include <opencv2/opencv.hpp>

// using namespace cv;

unsigned char bilinear_interpolate(unsigned char* data, int width, int height, float x, float y) {
    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    if (x1 >= width) x1 = width - 1;
    if (y1 >= height) y1 = height - 1;
    
    float dx = x - x0;
    float dy = y - y0;
    
    int idx00 = y0 * width + x0;
    int idx01 = y0 * width + x1;
    int idx10 = y1 * width + x0;
    int idx11 = y1 * width + x1;
    
    float val = (1 - dx) * (1 - dy) * data[idx00] +
                dx * (1 - dy) * data[idx01] +
                (1 - dx) * dy * data[idx10] +
                dx * dy * data[idx11];
    
    return (unsigned char)(val + 0.5f);
}

unsigned char bilinear_interpolate_uv(unsigned char* data, int width, int height, float x, float y, int is_u) {
    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    if (x1 >= width) x1 = width - 1;
    if (y1 >= height) y1 = height - 1;
    
    float dx = x - x0;
    float dy = y - y0;
    
    int idx00 = (y0 * width + x0) * 2 + is_u;
    int idx01 = (y0 * width + x1) * 2 + is_u;
    int idx10 = (y1 * width + x0) * 2 + is_u;
    int idx11 = (y1 * width + x1) * 2 + is_u;
    
    float val = (1 - dx) * (1 - dy) * data[idx00] +
                dx * (1 - dy) * data[idx01] +
                (1 - dx) * dy * data[idx10] +
                dx * dy * data[idx11];
    
    return (unsigned char)(val + 0.5f);
}

int nv12_scale_ex(unsigned char* src, int src_width, int src_height,
                  unsigned char* dst, int dst_width, int dst_height,
                  int keep_aspect) {
    if (!src || !dst || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
        return -1;
    }
    
    int src_y_size = src_width * src_height;
    int dst_y_size = dst_width * dst_height;
    
    unsigned char* src_y = src;
    unsigned char* src_uv = src + src_y_size;
    unsigned char* dst_y = dst;
    unsigned char* dst_uv = dst + dst_y_size;
    
    if (keep_aspect) {
        float src_aspect = (float)src_width / src_height;
        float target_aspect = (float)dst_width / dst_height;
        
        int scaled_width, scaled_height;
        
        if (src_aspect > target_aspect) {
            scaled_width = dst_width;
            scaled_height = (int)(dst_width / src_aspect + 0.5f);
        } else {
            scaled_height = dst_height;
            scaled_width = (int)(dst_height * src_aspect + 0.5f);
        }
        
        int offset_x = (dst_width - scaled_width) / 2;
        int offset_y = (dst_height - scaled_height) / 2;
        
        memset(dst_y, 0, dst_y_size);
        memset(dst_uv, 128, dst_y_size / 2);
        
        float x_ratio = (float)src_width / scaled_width;
        float y_ratio = (float)src_height / scaled_height;
        
        for (int y = 0; y < scaled_height; y++) {
            for (int x = 0; x < scaled_width; x++) {
                float src_x = x * x_ratio;
                float src_y_pos = y * y_ratio;
                
                int dst_x = offset_x + x;
                int dst_y_pos = offset_y + y;
                
                dst_y[dst_y_pos * dst_width + dst_x] = bilinear_interpolate(src_y, src_width, src_height, src_x, src_y_pos);
            }
        }
        
        int src_uv_width = src_width / 2;
        int src_uv_height = src_height / 2;
        int dst_uv_width = dst_width / 2;
        int dst_uv_height = dst_height / 2;
        
        int scaled_uv_width = scaled_width / 2;
        int scaled_uv_height = scaled_height / 2;
        int offset_uv_x = offset_x / 2;
        int offset_uv_y = offset_y / 2;
        
        float uv_x_ratio = (float)src_uv_width / scaled_uv_width;
        float uv_y_ratio = (float)src_uv_height / scaled_uv_height;
        
        for (int y = 0; y < scaled_uv_height; y++) {
            for (int x = 0; x < scaled_uv_width; x++) {
                float src_x = x * uv_x_ratio;
                float src_y_pos = y * uv_y_ratio;
                
                int dst_x = offset_uv_x + x;
                int dst_y_pos = offset_uv_y + y;
                
                if (dst_x < dst_uv_width && dst_y_pos < dst_uv_height) {
                    int dst_idx = dst_y_pos * dst_uv_width + dst_x;
                    dst_uv[dst_idx * 2] = bilinear_interpolate_uv(src_uv, src_uv_width, src_uv_height, src_x, src_y_pos, 0);
                    dst_uv[dst_idx * 2 + 1] = bilinear_interpolate_uv(src_uv, src_uv_width, src_uv_height, src_x, src_y_pos, 1);
                }
            }
        }
    } else {
        float x_ratio = (float)src_width / dst_width;
        float y_ratio = (float)src_height / dst_height;
        
        for (int y = 0; y < dst_height; y++) {
            for (int x = 0; x < dst_width; x++) {
                float src_x = x * x_ratio;
                float src_y_pos = y * y_ratio;
                
                dst_y[y * dst_width + x] = bilinear_interpolate(src_y, src_width, src_height, src_x, src_y_pos);
            }
        }
        
        int src_uv_width = src_width / 2;
        int src_uv_height = src_height / 2;
        int dst_uv_width = dst_width / 2;
        int dst_uv_height = dst_height / 2;
        
        float uv_x_ratio = (float)src_uv_width / dst_uv_width;
        float uv_y_ratio = (float)src_uv_height / dst_uv_height;
        
        for (int y = 0; y < dst_uv_height; y++) {
            for (int x = 0; x < dst_uv_width; x++) {
                float src_x = x * uv_x_ratio;
                float src_y_pos = y * uv_y_ratio;
                
                int dst_idx = y * dst_uv_width + x;
                dst_uv[dst_idx * 2] = bilinear_interpolate_uv(src_uv, src_uv_width, src_uv_height, src_x, src_y_pos, 0);
                dst_uv[dst_idx * 2 + 1] = bilinear_interpolate_uv(src_uv, src_uv_width, src_uv_height, src_x, src_y_pos, 1);
            }
        }
    }
    
    return 0;
}

int nv12_vertical_concat_correct(const unsigned char *src_nv12_top,
                                 const unsigned char *src_nv12_bottom,
                                 unsigned char *dst_nv12,
                                 int width, int height,
                                 int strideY, int strideUV)
{
    if (!src_nv12_top || !src_nv12_bottom || !dst_nv12)
        return -1;

    int dst_height = height * 2;

    int top_y_size = strideY * height;
    int top_uv_size = strideUV * height / 2;
    int bottom_y_size = strideY * height;
    int bottom_uv_size = strideUV * height / 2;

    const unsigned char *src_y_top = src_nv12_top;
    const unsigned char *src_uv_top = src_nv12_top + top_y_size;
    const unsigned char *src_y_bottom = src_nv12_bottom;
    const unsigned char *src_uv_bottom = src_nv12_bottom + bottom_y_size;

    unsigned char *dst_y = dst_nv12;
    unsigned char *dst_uv = dst_nv12 + (strideY * dst_height);

    for (int row = 0; row < height; row++)
    {
        memcpy(dst_y, src_y_top, width);
        dst_y += strideY;
        src_y_top += strideY;
    }
    for (int row = 0; row < height; row++)
    {
        memcpy(dst_y, src_y_bottom, width);
        dst_y += strideY;
        src_y_bottom += strideY;
    }

    int uv_height = height / 2;
    for (int row = 0; row < uv_height; row++)
    {
        memcpy(dst_uv, src_uv_top, width);
        dst_uv += strideUV;
        src_uv_top += strideUV;
    }
    for (int row = 0; row < uv_height; row++)
    {
        memcpy(dst_uv, src_uv_bottom, width);
        dst_uv += strideUV;
        src_uv_bottom += strideUV;
    }

    return 0;
}

int STITCH_SCALE_Init(STITCH_SCALE_PARAM_S *pParam, int single_width, int single_height, 
                     int dst_width, int dst_height)
{
    if (NULL == pParam) {
        return -1;
    }

    pParam->src_width = single_width;
    pParam->src_height = single_height * 2;
    pParam->dst_width = dst_width;
    pParam->dst_height = dst_height;
    pParam->single_width = single_width;
    pParam->single_height = single_height;

    int concat_frame_size = single_width * (single_height * 2) * 3 / 2;
    int scaled_frame_size = dst_width * dst_height * 3 / 2;

    pParam->frame_top = NULL;
    pParam->frame_bottom = NULL;
    pParam->frame_concat = (uint8_t *)malloc(concat_frame_size);
    pParam->frame_scaled = (uint8_t *)malloc(scaled_frame_size);

    if (NULL == pParam->frame_concat || NULL == pParam->frame_scaled) {
        STITCH_SCALE_Deinit(pParam);
        return -1;
    }

    return 0;
}

int STITCH_SCALE_Deinit(STITCH_SCALE_PARAM_S *pParam)
{
    if (NULL == pParam) {
        return -1;
    }

    if (NULL != pParam->frame_top) {
        free(pParam->frame_top);
        pParam->frame_top = NULL;
    }
    if (NULL != pParam->frame_bottom) {
        free(pParam->frame_bottom);
        pParam->frame_bottom = NULL;
    }
    if (NULL != pParam->frame_concat) {
        free(pParam->frame_concat);
        pParam->frame_concat = NULL;
    }
    if (NULL != pParam->frame_scaled) {
        free(pParam->frame_scaled);
        pParam->frame_scaled = NULL;
    }

    return 0;
}

int STITCH_SCALE_Process(STITCH_SCALE_PARAM_S *pParam, 
                       uint8_t *pTopFrame, uint8_t *pBottomFrame)
{
    if (NULL == pParam || NULL == pTopFrame || NULL == pBottomFrame) {
        return -1;
    }

    int single_frame_size = pParam->single_width * pParam->single_height * 3 / 2;

    int ret = nv12_vertical_concat_correct(pTopFrame, pBottomFrame, 
                                          pParam->frame_concat, pParam->single_width, 
                                          pParam->single_height, pParam->src_width, 
                                          pParam->src_height);
    if (ret != 0) {
        return -1;
    }

    ret = nv12_scale_ex(pParam->frame_concat, pParam->src_width, pParam->src_height,
                       pParam->frame_scaled, pParam->dst_width, pParam->dst_height, 1);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

// int STITCH_SCALE_Process_Fast(STITCH_SCALE_PARAM_S *pParam, 
//                               uint8_t *pTopFrame, uint8_t *pBottomFrame)
// {
//     if (NULL == pParam || NULL == pTopFrame || NULL == pBottomFrame) {
//         return -1;
//     }

//     int ret = nv12_vertical_concat_correct(pTopFrame, pBottomFrame, 
//                                           pParam->frame_concat, pParam->single_width, 
//                                           pParam->single_height, pParam->src_width, 
//                                           pParam->src_height);
//     if (ret != 0) {
//         return -1;
//     }

//     ret = nv12_scale_fixed(pParam->frame_concat, pParam->src_width, pParam->src_height,
//                           pParam->frame_scaled, pParam->dst_width, pParam->dst_height, 1);
//     if (ret != 0) {
//         return -1;
//     }

//     return 0;
// }

// uint8_t* STITCH_SCALE_GetScaledFrame(STITCH_SCALE_PARAM_S *pParam)
// {
//     if (NULL == pParam) {
//         return NULL;
//     }
//     return pParam->frame_scaled;
// }

// int nv12_to_mat_opencv(const uint8_t* nv12_data, int width, int height, Mat& yuv_mat)
// {
//     if (!nv12_data || width <= 0 || height <= 0) {
//         return -1;
//     }

//     int y_size = width * height;
//     int uv_size = width * height / 2;

//     yuv_mat.create(height * 3 / 2, width, CV_8UC1);
    
//     memcpy(yuv_mat.data, nv12_data, y_size);
//     memcpy(yuv_mat.data + y_size, nv12_data + y_size, uv_size);

//     return 0;
// }

// int mat_to_nv12_opencv(const Mat& yuv_mat, uint8_t* nv12_data, int width, int height)
// {
//     if (yuv_mat.empty() || !nv12_data || width <= 0 || height <= 0) {
//         return -1;
//     }

//     int y_size = width * height;
//     int uv_size = width * height / 2;

//     memcpy(nv12_data, yuv_mat.data, y_size);
//     memcpy(nv12_data + y_size, yuv_mat.data + y_size, uv_size);

//     return 0;
// }

// int nv12_vertical_concat_opencv(const uint8_t* src_nv12_top, const uint8_t* src_nv12_bottom,
//                                 uint8_t* dst_nv12, int width, int height)
// {
//     if (!src_nv12_top || !src_nv12_bottom || !dst_nv12 || width <= 0 || height <= 0) {
//         return -1;
//     }

//     Mat yuv_top, yuv_bottom, yuv_concat;

//     if (nv12_to_mat_opencv(src_nv12_top, width, height, yuv_top) != 0) {
//         return -1;
//     }

//     if (nv12_to_mat_opencv(src_nv12_bottom, width, height, yuv_bottom) != 0) {
//         return -1;
//     }

//     vconcat(yuv_top, yuv_bottom, yuv_concat);

//     if (mat_to_nv12_opencv(yuv_concat, dst_nv12, width, height * 2) != 0) {
//         return -1;
//     }

//     return 0;
// }

// int nv12_scale_opencv(const uint8_t* src_nv12, int src_width, int src_height,
//                       uint8_t* dst_nv12, int dst_width, int dst_height, int keep_aspect)
// {
//     if (!src_nv12 || !dst_nv12 || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
//         return -1;
//     }

//     Mat yuv_src, yuv_dst;

//     if (nv12_to_mat_opencv(src_nv12, src_width, src_height, yuv_src) != 0) {
//         return -1;
//     }

//     if (keep_aspect) {
//         float src_aspect = (float)src_width / src_height;
//         float dst_aspect = (float)dst_width / dst_height;

//         int scaled_width, scaled_height;

//         if (src_aspect > dst_aspect) {
//             scaled_width = dst_width;
//             scaled_height = (int)(dst_width / src_aspect + 0.5f);
//         } else {
//             scaled_height = dst_height;
//             scaled_width = (int)(dst_height * src_aspect + 0.5f);
//         }

//         Mat yuv_scaled;
//         resize(yuv_src, yuv_scaled, Size(scaled_width, scaled_height * 3 / 2), 0, 0, INTER_LINEAR);

//         yuv_dst.create(dst_height * 3 / 2, dst_width, CV_8UC1);
//         yuv_dst.setTo(Scalar(0));

//         int offset_x = (dst_width - scaled_width) / 2;
//         int offset_y = (dst_height - scaled_height) / 2;

//         Rect roi(offset_x, offset_y, scaled_width, scaled_height * 3 / 2);
//         yuv_scaled.copyTo(yuv_dst(roi));
//     } else {
//         resize(yuv_src, yuv_dst, Size(dst_width, dst_height * 3 / 2), 0, 0, INTER_LINEAR);
//     }

//     if (mat_to_nv12_opencv(yuv_dst, dst_nv12, dst_width, dst_height) != 0) {
//         return -1;
//     }

//     return 0;
// }

// int STITCH_SCALE_Process_Opencv(STITCH_SCALE_PARAM_S *pParam, 
//                                 uint8_t *pTopFrame, uint8_t *pBottomFrame)
// {
//     if (NULL == pParam || NULL == pTopFrame || NULL == pBottomFrame) {
//         return -1;
//     }

//     int ret = nv12_vertical_concat_opencv(pTopFrame, pBottomFrame, 
//                                           pParam->frame_concat, pParam->single_width, 
//                                           pParam->single_height);
//     if (ret != 0) {
//         return -1;
//     }

//     ret = nv12_scale_opencv(pParam->frame_concat, pParam->src_width, pParam->src_height,
//                             pParam->frame_scaled, pParam->dst_width, pParam->dst_height, 1);
//     if (ret != 0) {
//         return -1;
//     }

//     return 0;
// }

// int nv12_scale_nearest(unsigned char* src, int src_width, int src_height,
//                        unsigned char* dst, int dst_width, int dst_height,
//                        int keep_aspect) {
//     if (!src || !dst || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
//         return -1;
//     }
    
//     int src_y_size = src_width * src_height;
//     int dst_y_size = dst_width * dst_height;
    
//     unsigned char* src_y = src;
//     unsigned char* src_uv = src + src_y_size;
//     unsigned char* dst_y = dst;
//     unsigned char* dst_uv = dst + dst_y_size;
    
//     if (keep_aspect) {
//         float src_aspect = (float)src_width / src_height;
//         float target_aspect = (float)dst_width / dst_height;
        
//         int scaled_width, scaled_height;
        
//         if (src_aspect > target_aspect) {
//             scaled_width = dst_width;
//             scaled_height = (int)(dst_width / src_aspect + 0.5f);
//         } else {
//             scaled_height = dst_height;
//             scaled_width = (int)(dst_height * src_aspect + 0.5f);
//         }
        
//         int offset_x = (dst_width - scaled_width) / 2;
//         int offset_y = (dst_height - scaled_height) / 2;
        
//         memset(dst_y, 0, dst_y_size);
//         memset(dst_uv, 128, dst_y_size / 2);
        
//         float x_ratio = (float)src_width / scaled_width;
//         float y_ratio = (float)src_height / scaled_height;
        
//         for (int y = 0; y < scaled_height; y++) {
//             int src_y_pos = (int)(y * y_ratio + 0.5f);
//             if (src_y_pos >= src_height) src_y_pos = src_height - 1;
            
//             for (int x = 0; x < scaled_width; x++) {
//                 int src_x = (int)(x * x_ratio + 0.5f);
//                 if (src_x >= src_width) src_x = src_width - 1;
                
//                 int dst_x = offset_x + x;
//                 int dst_y_pos = offset_y + y;
                
//                 dst_y[dst_y_pos * dst_width + dst_x] = src_y[src_y_pos * src_width + src_x];
//             }
//         }
        
//         int src_uv_width = src_width / 2;
//         int src_uv_height = src_height / 2;
//         int dst_uv_width = dst_width / 2;
//         int dst_uv_height = dst_height / 2;
        
//         int scaled_uv_width = scaled_width / 2;
//         int scaled_uv_height = scaled_height / 2;
//         int offset_uv_x = offset_x / 2;
//         int offset_uv_y = offset_y / 2;
        
//         float uv_x_ratio = (float)src_uv_width / scaled_uv_width;
//         float uv_y_ratio = (float)src_uv_height / scaled_uv_height;
        
//         for (int y = 0; y < scaled_uv_height; y++) {
//             int src_y_pos = (int)(y * uv_y_ratio + 0.5f);
//             if (src_y_pos >= src_uv_height) src_y_pos = src_uv_height - 1;
            
//             for (int x = 0; x < scaled_uv_width; x++) {
//                 int src_x = (int)(x * uv_x_ratio + 0.5f);
//                 if (src_x >= src_uv_width) src_x = src_uv_width - 1;
                
//                 int dst_x = offset_uv_x + x;
//                 int dst_y_pos = offset_uv_y + y;
                
//                 if (dst_x < dst_uv_width && dst_y_pos < dst_uv_height) {
//                     int dst_idx = dst_y_pos * dst_uv_width + dst_x;
//                     int src_idx = src_y_pos * src_uv_width + src_x;
//                     dst_uv[dst_idx * 2] = src_uv[src_idx * 2];
//                     dst_uv[dst_idx * 2 + 1] = src_uv[src_idx * 2 + 1];
//                 }
//             }
//         }
//     } else {
//         float x_ratio = (float)src_width / dst_width;
//         float y_ratio = (float)src_height / dst_height;
        
//         for (int y = 0; y < dst_height; y++) {
//             int src_y_pos = (int)(y * y_ratio + 0.5f);
//             if (src_y_pos >= src_height) src_y_pos = src_height - 1;
            
//             for (int x = 0; x < dst_width; x++) {
//                 int src_x = (int)(x * x_ratio + 0.5f);
//                 if (src_x >= src_width) src_x = src_width - 1;
                
//                 dst_y[y * dst_width + x] = src_y[src_y_pos * src_width + src_x];
//             }
//         }
        
//         int src_uv_width = src_width / 2;
//         int src_uv_height = src_height / 2;
//         int dst_uv_width = dst_width / 2;
//         int dst_uv_height = dst_height / 2;
        
//         float uv_x_ratio = (float)src_uv_width / dst_uv_width;
//         float uv_y_ratio = (float)src_uv_height / dst_uv_height;
        
//         for (int y = 0; y < dst_uv_height; y++) {
//             int src_y_pos = (int)(y * uv_y_ratio + 0.5f);
//             if (src_y_pos >= src_uv_height) src_y_pos = src_uv_height - 1;
            
//             for (int x = 0; x < dst_uv_width; x++) {
//                 int src_x = (int)(x * uv_x_ratio + 0.5f);
//                 if (src_x >= src_uv_width) src_x = src_uv_width - 1;
                
//                 int dst_idx = y * dst_uv_width + x;
//                 int src_idx = src_y_pos * src_uv_width + src_x;
//                 dst_uv[dst_idx * 2] = src_uv[src_idx * 2];
//                 dst_uv[dst_idx * 2 + 1] = src_uv[src_idx * 2 + 1];
//             }
//         }
//     }
    
//     return 0;
// }

// unsigned char bilinear_interpolate_fixed(unsigned char* data, int width, int height, int x_fixed, int y_fixed) {
//     int x0 = x_fixed >> 16;
//     int y0 = y_fixed >> 16;
//     int x1 = x0 + 1;
//     int y1 = y0 + 1;
    
//     if (x1 >= width) x1 = width - 1;
//     if (y1 >= height) y1 = height - 1;
    
//     int dx = x_fixed & 0xFFFF;
//     int dy = y_fixed & 0xFFFF;
    
//     int idx00 = y0 * width + x0;
//     int idx01 = y0 * width + x1;
//     int idx10 = y1 * width + x0;
//     int idx11 = y1 * width + x1;
    
//     int val = ((65536 - dx) * (65536 - dy) * data[idx00] +
//                 dx * (65536 - dy) * data[idx01] +
//                 (65536 - dx) * dy * data[idx10] +
//                 dx * dy * data[idx11]) >> 32;
    
//     return (unsigned char)val;
// }

// int nv12_scale_fixed(unsigned char* src, int src_width, int src_height,
//                      unsigned char* dst, int dst_width, int dst_height,
//                      int keep_aspect) {
//     if (!src || !dst || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
//         return -1;
//     }
    
//     int src_y_size = src_width * src_height;
//     int dst_y_size = dst_width * dst_height;
    
//     unsigned char* src_y = src;
//     unsigned char* src_uv = src + src_y_size;
//     unsigned char* dst_y = dst;
//     unsigned char* dst_uv = dst + dst_y_size;
    
//     if (keep_aspect) {
//         float src_aspect = (float)src_width / src_height;
//         float target_aspect = (float)dst_width / dst_height;
        
//         int scaled_width, scaled_height;
        
//         if (src_aspect > target_aspect) {
//             scaled_width = dst_width;
//             scaled_height = (int)(dst_width / src_aspect + 0.5f);
//         } else {
//             scaled_height = dst_height;
//             scaled_width = (int)(dst_height * src_aspect + 0.5f);
//         }
        
//         int offset_x = (dst_width - scaled_width) / 2;
//         int offset_y = (dst_height - scaled_height) / 2;
        
//         memset(dst_y, 0, dst_y_size);
//         memset(dst_uv, 128, dst_y_size / 2);
        
//         int x_ratio_fixed = (int)((float)src_width / scaled_width * 65536.0f + 0.5f);
//         int y_ratio_fixed = (int)((float)src_height / scaled_height * 65536.0f + 0.5f);
        
//         for (int y = 0; y < scaled_height; y++) {
//             int y_fixed = y * y_ratio_fixed;
            
//             for (int x = 0; x < scaled_width; x++) {
//                 int x_fixed = x * x_ratio_fixed;
                
//                 int dst_x = offset_x + x;
//                 int dst_y_pos = offset_y + y;
                
//                 dst_y[dst_y_pos * dst_width + dst_x] = bilinear_interpolate_fixed(src_y, src_width, src_height, x_fixed, y_fixed);
//             }
//         }
        
//         int src_uv_width = src_width / 2;
//         int src_uv_height = src_height / 2;
//         int dst_uv_width = dst_width / 2;
//         int dst_uv_height = dst_height / 2;
        
//         int scaled_uv_width = scaled_width / 2;
//         int scaled_uv_height = scaled_height / 2;
//         int offset_uv_x = offset_x / 2;
//         int offset_uv_y = offset_y / 2;
        
//         int uv_x_ratio_fixed = (int)((float)src_uv_width / scaled_uv_width * 65536.0f + 0.5f);
//         int uv_y_ratio_fixed = (int)((float)src_uv_height / scaled_uv_height * 65536.0f + 0.5f);
        
//         for (int y = 0; y < scaled_uv_height; y++) {
//             int y_fixed = y * uv_y_ratio_fixed;
            
//             for (int x = 0; x < scaled_uv_width; x++) {
//                 int x_fixed = x * uv_x_ratio_fixed;
                
//                 int dst_x = offset_uv_x + x;
//                 int dst_y_pos = offset_uv_y + y;
                
//                 if (dst_x < dst_uv_width && dst_y_pos < dst_uv_height) {
//                     int dst_idx = dst_y_pos * dst_uv_width + dst_x;
//                     dst_uv[dst_idx * 2] = bilinear_interpolate_fixed(src_uv, src_uv_width, src_uv_height, x_fixed * 2, y_fixed);
//                     dst_uv[dst_idx * 2 + 1] = bilinear_interpolate_fixed(src_uv, src_uv_width, src_uv_height, x_fixed * 2 + 1, y_fixed);
//                 }
//             }
//         }
//     } else {
//         int x_ratio_fixed = (int)((float)src_width / dst_width * 65536.0f + 0.5f);
//         int y_ratio_fixed = (int)((float)src_height / dst_height * 65536.0f + 0.5f);
        
//         for (int y = 0; y < dst_height; y++) {
//             int y_fixed = y * y_ratio_fixed;
            
//             for (int x = 0; x < dst_width; x++) {
//                 int x_fixed = x * x_ratio_fixed;
                
//                 dst_y[y * dst_width + x] = bilinear_interpolate_fixed(src_y, src_width, src_height, x_fixed, y_fixed);
//             }
//         }
        
//         int src_uv_width = src_width / 2;
//         int src_uv_height = src_height / 2;
//         int dst_uv_width = dst_width / 2;
//         int dst_uv_height = dst_height / 2;
        
//         int uv_x_ratio_fixed = (int)((float)src_uv_width / dst_uv_width * 65536.0f + 0.5f);
//         int uv_y_ratio_fixed = (int)((float)src_uv_height / dst_uv_height * 65536.0f + 0.5f);
        
//         for (int y = 0; y < dst_uv_height; y++) {
//             int y_fixed = y * uv_y_ratio_fixed;
            
//             for (int x = 0; x < dst_uv_width; x++) {
//                 int x_fixed = x * uv_x_ratio_fixed;
                
//                 int dst_idx = y * dst_uv_width + x;
//                 dst_uv[dst_idx * 2] = bilinear_interpolate_fixed(src_uv, src_uv_width, src_uv_height, x_fixed * 2, y_fixed);
//                 dst_uv[dst_idx * 2 + 1] = bilinear_interpolate_fixed(src_uv, src_uv_width, src_uv_height, x_fixed * 2 + 1, y_fixed);
//             }
//         }
//     }
    
//     return 0;
// }
