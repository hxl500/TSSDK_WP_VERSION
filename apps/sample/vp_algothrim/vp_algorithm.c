#include <string.h>
#include "vp_algorithm.h"
// #include "vp_sensor_config.h"
#include "vp_video_coder_type.h"
#include "vp_video_encoder.h"
#include "vp_events.h"
#include "vp_printf.h"
#include "vp_video_osd.h"
#include "vp_time.h"
#include "vp_lock.h"
#include "vp_observers.h"
#include "vp_pthread.h"
// #include "vp_status_param.h"
#include "vp_motion_detect.h"
#include "vp_human_detect.h"
#include "vp_convergence_detect.h"
#include "vp_context_config.h"
#include "vp_fusion_detect.h"
#include "vp_multiobject_detect.h"
#include "ts_alg_type.h"
#include "ts_alg_imgproc.h"
// #include "ts_alg_singletargettrack.h"

#define VP_ALGORITHM_TYPE           1
#define VP_ALGORITHM_TYPE_TS        1

#if VP_ALGORITHM_TYPE == VP_ALGORITHM_TYPE_YTLD

#include "ytld/vp_algorithm_ytld.h"

#endif
#if VP_PLATFORM_IS_JZ(VP_SDK_PLATFORM)

#include "vp_track.h"

#if VP_SDK_PLATFORM == VP_SDK_PLATFORM_T23

#include "T23/vp_video_encoder.h"

#endif

#endif

// static void * g_singletrack_handle = NULL;

typedef struct {
    volatile uint8_t enable: 1;
    volatile uint8_t pause: 1;
    volatile uint8_t update: 1;
    volatile uint8_t notify: 1;
    volatile uint8_t paused: 1;
    volatile uint8_t has_notify: 1;
    uint64_t last_notify;
    uint64_t detect_timestamp;
    vp_algorithm_param_t param;
    vp_algorithm_result_t result;
    void *handle;
} vp_algorithm_info_t;

typedef struct {
    uint16_t frame_width;
    uint16_t frame_height;

    uint32_t data_size;
    uint32_t jpeg_size;
    uint8_t *jpeg_data;

    vp_events_p events;
    uint64_t human_timestamp;
    vp_lock_t lock;
    vp_algorithm_info_t infos[VP_ALGORITHM_TYPE_MAX];
} vp_algorithm_channel_t;

typedef struct {
    vp_algorithm_state_t state;
    vp_events_p events;
    // vp_track_opt_t track;
    // vp_track_obj_t track_objs[15];
    vp_algorithm_channel_t channels[VP_SENSOR_NUM][vp_video_chn_max];
} vp_algorithm_context_t;

static vp_algorithm_context_t g_context = {0};

static int vp_algorithm_human_draw_boxes(uint8_t idx, vp_video_chn_t chn, uint32_t width, uint32_t height,
                                         vp_human_detect_result_t *result) {

    vp_video_encoder_config_t config;
    vp_video_encoder_get_config(idx, chn, &config);
    vp_area_rect_t *rect;
    if (result->count < VP_VIDEO_OSD_RECT_MAX) {
        for (int i = result->count; i < VP_VIDEO_OSD_RECT_MAX; ++i) {
            vp_video_osd_hide_rect(idx, chn, i);
        }
    }
    for (int i = 0; i < result->count && i < VP_VIDEO_OSD_RECT_MAX; ++i) {
        rect = &result->objs[i].rect;
        uint32_t dest_x = rect->x, dest_y = rect->y;
        uint32_t dest_w = rect->w, dest_h = rect->h;
        if (config.width != width) {
            dest_x = rect->x * config.width / width;
            dest_w = rect->w * config.width / width;
        }
        if (config.height != height) {
            dest_y = rect->y * config.height / height;
            dest_h = rect->h * config.height / height;
        }
        vp_video_osd_show_rect(idx, chn, i);

        vp_video_osd_update_rect(idx, chn, i, dest_x, dest_y, dest_w, dest_h);
    }
    return 0;
}

static void vp_algorithm_human_clear_boxes(uint8_t idx, vp_video_chn_t chn) {
    for (int i = 0; i < VP_VIDEO_OSD_RECT_MAX; ++i) {
        vp_video_osd_hide_rect(idx, chn, i);
    }
}

static int vp_algorithm_detect_init(vp_algorithm_ivs_args_t *args) {
    (void) args;
    vp_debug("start");
#if VP_ALGORITHM_TYPE == VP_ALGORITHM_TYPE_YTLD
    while (vp_algorithm_ytld_load()) {
        sleep(1);
    }
#endif
    vp_debug("end");

    ALG_IMAGE_S* pimage = calloc(1, sizeof(ALG_IMAGE_S));
    if (pimage == NULL) return -1;

    if (pimage != NULL && pimage->pData == NULL) {
        pimage->s32W = 640;
        pimage->s32H = 384;
        pimage->s32C = 4;
        int alg_rgba_size = pimage->s32W * pimage->s32H * pimage->s32C;
        if (TS_MPI_SYS_MmzAlloc_Cached(&(pimage->pDataPhy), &(pimage->pData), NULL, NULL, alg_rgba_size)) {
            free(pimage);
            vp_error("Failed to call TS_MPI_SYS_MmzAlloc_Cached.\n");
            return -1;
        }

        memset(pimage->pData, 114, alg_rgba_size);
    }

    args->user_data = pimage;

    // if (TS_ALG_SingleTgtTrack_Init(&g_singletrack_handle)) {
    //     vp_error("Failed to Call TS_ALG_SingleTgtTrack_Init.\n");
    //     g_singletrack_handle = NULL;
    // }


    return 0;
}

static void vp_algorithm_detect_pause(vp_algorithm_type_t type, vp_algorithm_info_t *info) {
    int ret = 0;
    switch (type) {
#if VP_ALGORITHM_FUSION && VP_ALGORITHM_HUMAN
        case VP_ALGORITHM_TYPE_HUMAN_DETECT:
        case VP_ALGORITHM_TYPE_MOTION_DETECT: {
            vp_fusion_detect_pause(info->handle, type);
        }
#elif VP_ALGORITHM_MULTIOBJECT && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_HUMAN_DETECT:
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                vp_multiobject_detect_pause(info->handle, type);
            }
#else
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                ret = vp_motion_detect_pause(info->handle);
                break;
            }
            case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                ret = vp_human_detect_pause(info->handle);
                break;
            }
#endif
        case VP_ALGORITHM_TYPE_CONVERGENCE_DETECT: {
            // ret = vp_convergence_detect_pause(info->handle);
            break;
        }
        case VP_ALGORITHM_TYPE_MAX:
        default:
            break;
    }
    vp_stack("ret:%d type:%d", ret, type);
    info->paused = 1;
}

static void vp_algorithm_detect_resume(vp_algorithm_type_t type, vp_algorithm_info_t *info) {
    int ret = 0;
    switch (type) {
#if VP_ALGORITHM_FUSION && VP_ALGORITHM_HUMAN
        case VP_ALGORITHM_TYPE_HUMAN_DETECT:
        case VP_ALGORITHM_TYPE_MOTION_DETECT: {
            vp_fusion_detect_resume(info->handle, type);
            break;
        }
#elif VP_ALGORITHM_MULTIOBJECT && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_HUMAN_DETECT:
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                vp_multiobject_detect_resume(info->handle, type);
                break;
            }
#else
            case VP_ALGORITHM_TYPE_MOTION_DETECT:
                ret = vp_motion_detect_resume(info->handle);
                break;
            case VP_ALGORITHM_TYPE_HUMAN_DETECT:
                ret = vp_human_detect_resume(info->handle);
                break;
#endif
        case VP_ALGORITHM_TYPE_CONVERGENCE_DETECT:
            // ret = vp_convergence_detect_resume(info->handle);
            break;
        case VP_ALGORITHM_TYPE_MAX:
        default:
            break;
    }
    vp_stack("ret:%d type:%d", ret, type);
    info->paused = 0;
}

static int vp_algorithm_detect_before(uint8_t idx, uint8_t chn, vp_algorithm_ivs_args_t *args,
                                      vp_video_source_t *frame) {
    (void) args;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_events_clear(channel->events, VP_EVENT_BIT(VP_ALGORITHM_TYPE_MAX));
    vp_algorithm_info_t *info;
    vp_algorithm_param_t *param;
    uint8_t has_jpeg = 0;
    for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
        info = &channel->infos[i];
        if (info->enable == 0 || info->pause) continue;
        param = &info->param;
        switch (i) {
#if VP_ALGORITHM_FUSION && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                if (info->handle == NULL) {
                    info->result.type = i;
                    info->handle = vp_fusion_detect_create(frame->width, frame->height);
                    vp_stack("create idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                             info->handle, param->notify, param->interval, param->jpeg);
                    vp_fusion_detect_set_motion_param(info->handle, &info->param.motion);
                } else {
                    if (info->update) {
                        vp_fusion_detect_set_motion_param(info->handle, &info->param.motion);
                        vp_stack("update idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);
                        info->update = 0;
                    }
                }
                break;
            }
            case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                if (info->handle == NULL) {
                    info->result.type = i;
                    info->handle = vp_fusion_detect_create(frame->width, frame->height);
                    vp_stack("create idx:%d chn:%d human handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                             info->handle, param->notify, param->interval, param->jpeg);
                    vp_fusion_detect_set_human_param(info->handle, &info->param.human);
                    if (info->param.human.draw_box) {
                        vp_osd_config_rect_t rect = {
                                .color = VP_OSD_RECT_GREEN,
                        };
                        for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                            rect.line = 12;
                            vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                            rect.line = 1;
                            vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                            vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                        }
                    }
                } else {
                    if (info->update) {
                        vp_fusion_detect_set_human_param(info->handle, &info->param.human);
                        vp_stack("update idx:%d chn:%d human handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);
                        info->update = 0;
                        if (info->param.human.draw_box) {
                            vp_osd_config_rect_t rect = {
                                    .color = VP_OSD_RECT_GREEN,
                            };
                            for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                                rect.line = 12;
                                vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                                rect.line = 1;
                                vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                                vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                            }
                        }
                    }
                }
                break;
            }

#elif VP_ALGORITHM_MULTIOBJECT && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                if (info->handle == NULL) {
                    info->result.type = i;
                    info->handle = vp_multiobject_detect_create(frame->width, frame->height);
                    if (info->handle != NULL) {
                        vp_stack("T32 Multiobject create success, idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);
                        vp_multiobject_detect_set_motion_param(info->handle, &info->param.motion);
                    }else{
                        sleep(1);
                    }
                } else {
                    if (info->update) {
                        vp_multiobject_detect_set_motion_param(info->handle, &info->param.motion);
                        vp_stack("update idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);
                        info->update = 0;
                    }
                }
                break;
            }
            case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                if (info->handle == NULL) {
                    info->result.type = i;
                    info->handle = vp_multiobject_detect_create(frame->width, frame->height);
                    vp_multiobject_detect_set_human_param(info->handle, &info->param.human);
                    if (info->param.human.draw_box) {
                        vp_osd_config_rect_t rect = {
                                .color = VP_OSD_RECT_GREEN,
                        };
                        for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                            rect.line = 12;
                            vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                            rect.line = 1;
                            vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                            vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                        }
                    }
                } else {
                    if (info->update) {
                        vp_multiobject_detect_set_human_param(info->handle, &info->param.human);
                        // vp_stack("update idx:%d chn:%d human handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                        //           info->handle, param->notify, param->interval, param->jpeg);
                        info->update = 0;
                        if (info->param.human.draw_box) {
                            vp_osd_config_rect_t rect = {
                                    .color = VP_OSD_RECT_GREEN,
                            };
                            for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                                rect.line = 12;
                                vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                                rect.line = 1;
                                vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                                vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                            }
                        }
                    }
                }
                break;
            }
#else
                case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                    if (info->handle == NULL) {
                        info->result.type = i;
                        info->handle = vp_motion_detect_create(frame->width, frame->height, &info->param.motion);
                        vp_stack("create idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);

                    } else {
                        if (info->update) {
                            vp_motion_detect_set_param(info->handle, &info->param.motion);
                            vp_stack("update idx:%d chn:%d motion handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                     info->handle, param->notify, param->interval, param->jpeg);
                            info->update = 0;
                        }
                    }
                    break;
                }
                case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                    if (info->handle == NULL) {
                        info->result.type = i;
                        info->handle = vp_human_detect_create(frame->width, frame->height, &info->param.human);
                        vp_stack("create idx:%d chn:%d human handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                 info->handle, param->notify, param->interval, param->jpeg);
                        if (info->param.human.draw_box) {
                            vp_osd_config_rect_t rect = {
                                    .color = VP_OSD_RECT_GREEN,
                            };
                            for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                                rect.line = 3;
                                vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                                rect.line = 1;
                                vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                                vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                            }
                        }
                    } else {
                        if (info->update) {
                            vp_human_detect_set_param(info->handle, &info->param.human);
                            vp_stack("update idx:%d chn:%d human handle:%p notify:%d interval:%d jpeg:%d", idx, chn,
                                     info->handle, param->notify, param->interval, param->jpeg);
                            info->update = 0;
                            if (info->param.human.draw_box) {
                                vp_osd_config_rect_t rect = {
                                        .color = VP_OSD_RECT_GREEN,
                                };
                                for (int j = 0; j < VP_VIDEO_OSD_RECT_MAX; ++j) {
                                    rect.line = 2;
                                    vp_video_osd_config_rect(idx, vp_video_chn_main, j, &rect);
                                    rect.line = 1;
                                    vp_video_osd_config_rect(idx, vp_video_chn_sec, j, &rect);
                                    vp_video_osd_config_rect(idx, vp_video_chn_thr, j, &rect);
                                }
                            }
                        }
                    }
                    break;
                }
#endif
            // case VP_ALGORITHM_TYPE_CONVERGENCE_DETECT: {
            //     if (info->handle == NULL) {
            //         info->result.type = i;
            //         info->handle = vp_convergence_detect_create(frame->width, frame->height, idx,
            //                                                     &param->convergence);
            //         vp_debug("create convergence handle:%p", info->handle);
            //     } else {
            //         if (info->update) {
            //             vp_convergence_detect_set_param(info->handle, &param->convergence);
            //             vp_debug("update convergence param:%p", info->handle);
            //             info->update = 0;
            //         }
            //     }
            //     break;
            // }
            case VP_ALGORITHM_TYPE_MAX:
            default:
                break;
        }

        if (info->paused) vp_algorithm_detect_resume(i, info);
        if (info->handle && info->param.jpeg) has_jpeg = 1;
    }
    if (channel->frame_width != frame->width || channel->frame_height != frame->height) {
        channel->frame_width = frame->width;
        channel->frame_height = frame->height;
    }

    if (channel->jpeg_data == NULL && has_jpeg) {
        channel->data_size = 128 * 1024;
        channel->jpeg_data = (uint8_t *) malloc(channel->data_size);
        channel->jpeg_size = 0;
    }

    return 0;
}


static TS_S32 vp_algorithm_yuv2rgb(TS_U8 *y_image, TS_U8 *uv_image, TS_U8 *rgb_image,TS_U32 src_width, TS_U32 src_height, TS_U32 des_width, TS_U32 des_height, ALG_RGB_TYPE_E rgb_type)
{
	TS_ALG_YUV2RGB(y_image, uv_image, rgb_image, src_width, src_height, des_width, des_height, rgb_type);

    return TS_SUCCESS;
}

static int vp_algorithm_detect_process(uint8_t idx, uint8_t chn, vp_algorithm_ivs_args_t *args,
                                       vp_video_source_t *frame) {
    (void) args;
    // vp_track_opt_t *track = &g_context.track;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info;
    int ret, need_jpeg = 0;
    static int box_flag[VP_SENSOR_NUM] = {0};
    static int box_count[VP_SENSOR_NUM] = {0};
    vp_human_detect_result_t *human_result = NULL;
    uint8_t* frame_data;

    if (frame == NULL || channel == NULL) return -1;

    if (frame->width != 640 || frame->height != 360) {
        vp_error("human detect and motion detect only support 640 * 360.\n");
        return -1;
    } else {
        ALG_IMAGE_S* pimage = (ALG_IMAGE_S*)args->user_data;
        if (pimage == NULL || pimage->pData == NULL || frame->frame_data == NULL) {
            vp_error("Invalid params: pimage-%p, frame_data:%p.\n", pimage, frame->frame_data);
            return -1;
        }

        vp_algorithm_yuv2rgb(frame->frame_data, frame->frame_data + frame->width * frame->height, 
            ((TS_U8*)(uintptr_t)pimage->pData) + 4 * 12 * pimage->s32W, frame->width, frame->height, 
            frame->width, frame->height, ALG_RGB_TYPE_RGBA32);
        TS_MPI_SYS_MmzFlushCache(pimage->pDataPhy, pimage->pData, pimage->s32W * pimage->s32H * pimage->s32C);
        frame_data = frame->frame_data;
        frame->frame_data = pimage;
    }

    for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
        info = &channel->infos[i];

        if ((info->enable == 0 || info->handle == NULL || info->pause)) {
            if (i == VP_ALGORITHM_TYPE_HUMAN_DETECT) {
                if (box_flag[idx] == 1) {
                    box_flag[idx] = 0;
                    box_count[idx] = 0;
                    vp_algorithm_human_clear_boxes(idx, 0);
                    vp_algorithm_human_clear_boxes(idx, 1);
                }
            }
            continue;
        }

        info->result.state = 0;
        info->result.width = frame->width;
        info->result.height = frame->height;
        switch (i) {
#if VP_ALGORITHM_FUSION && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                if (frame->timestamp - channel->human_timestamp < 5000000) {
                    // 人形触发,忽略移动检测
                    continue;
                }
                if (track->motor_state && track->motor_state() == 1) continue;
#if (VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23DL || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_3)
                status_param_t *status_param = vp_status_param_get_handle();
                if (status_param->factory_mode.mode != 0 || status_param->speak.status != 0 ||
                    status_param->record_play_status.status > 0 || status_param->record_play_status.download_status > 0) {
                    continue;
                }
#endif
                ret = vp_fusion_detect_process(info->handle, frame);
                if (ret > 0) {
                    vp_lock(&channel->lock);
                    vp_fusion_detect_motion_result(info->handle, &info->result.motion);
                    if (info->result.motion.state) {
                        info->result.state = 1;
                        info->result.timestamp = frame->timestamp;
                    } else {
                        ret = 0;
                    }
                    vp_unlock(&channel->lock);
                }
                break;
            }
            case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                if (track->motor_state && track->motor_state() == 1) {
                    if (box_flag[idx] == 1) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                        box_count[idx] = 0;
                    }
                    continue;
                }
#if (VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23DL || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_3)
                status_param_t *status_param = vp_status_param_get_handle();
                if (status_param->factory_mode.mode != 0 || status_param->speak.status != 0 ||
                    status_param->record_play_status.status > 0) {
                    if (box_flag[idx] == 1) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                        box_count[idx] = 0;
                    }
                    continue;
                }
#endif
                ret = vp_fusion_detect_process(info->handle, frame);
                if (ret > 0) {
                    vp_lock(&channel->lock);
                    vp_fusion_detect_human_result(info->handle, &info->result.human);
                    if (info->result.human.count > 0) {
                        info->result.state = 1;
                        info->result.timestamp = frame->timestamp;
                        channel->human_timestamp = frame->timestamp;
                        human_result = &info->result.human;
                    } else {
                        ret = 0;
                    }
                    vp_unlock(&channel->lock);

                    if (info->result.state == 1) {
                        if (info->param.human.draw_box) {
                            if (box_flag[idx] == 1) {
                                vp_algorithm_human_clear_boxes(idx, 0);
                                vp_algorithm_human_clear_boxes(idx, 1);
                            }
                            box_flag[idx] = 1;
                            box_count[idx] = 0;
                            vp_algorithm_human_draw_boxes(idx, 0, frame->width, frame->height, &info->result.human);
                            vp_algorithm_human_draw_boxes(idx, 1, frame->width, frame->height, &info->result.human);
                        }
                        if (info->param.human.enable_track && track->track_target) {
                            for (int j = 0; j < info->result.human.count; ++j) {
                                g_context.track_objs[j].idx = idx;
                                g_context.track_objs[j].track_id = info->result.human.objs[j].track_id;
                                g_context.track_objs[j].score = info->result.human.objs[j].score;
                                g_context.track_objs[j].x = info->result.human.objs[j].rect.x;
                                g_context.track_objs[j].y = info->result.human.objs[j].rect.y;
                                g_context.track_objs[j].w = info->result.human.objs[j].rect.w;
                                g_context.track_objs[j].h = info->result.human.objs[j].rect.h;
                                g_context.track_objs[j].width = frame->width;
                                g_context.track_objs[j].height = frame->height;
                            }
                            track->track_target(g_context.track_objs, info->result.human.count);
                        }
                    } else {
                        if (box_flag[idx] == 1 && box_count[idx]++ >= 3) {
                            vp_algorithm_human_clear_boxes(idx, 0);
                            vp_algorithm_human_clear_boxes(idx, 1);
                            box_flag[idx] = 0;
                        }

                        if (info->param.human.enable_track && track->track_target) {
                            track->track_target(g_context.track_objs, 0);
                        }
                    }
                } else {
                    if (box_flag[idx] == 1 && box_count[idx]++ >= 3) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                    }

                    if (info->param.human.enable_track && track->track_target) {
                        track->track_target(g_context.track_objs, 0);
                    }
                }
                break;
            }
#elif VP_ALGORITHM_MULTIOBJECT && VP_ALGORITHM_HUMAN
            case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                if (frame->timestamp - channel->human_timestamp < 5000000) {
                    // 人形触发,忽略移动检测
                    continue;
                }
                if (track->motor_state && track->motor_state() == 1) continue;
                status_param_t *status_param = vp_status_param_get_handle();
                if (status_param->factory_mode.mode != 0){
                    continue;
                }
                ret = vp_multiobject_detect_process(info->handle, frame);
                if (ret > 0) {
                    vp_lock(&channel->lock);
                    vp_multiobject_detect_motion_result(info->handle, &info->result.motion);
                    if (info->result.motion.state) {
                        info->result.state = 1;
                        info->result.timestamp = frame->timestamp;
                    } else {
                        ret = 0;
                    }
                    vp_unlock(&channel->lock);
                }
                break;
            }
            case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                if (track->motor_state && track->motor_state() == 1) {
                    if (box_flag[idx] == 1) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                        box_count[idx] = 0;
                    }
                    continue;
                }
                status_param_t *status_param = vp_status_param_get_handle();
                if (status_param->factory_mode.mode != 0) {
                    if (box_flag[idx] == 1) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                        box_count[idx] = 0;
                    }
                    continue;
                }
                ret = vp_multiobject_detect_process(info->handle, frame);
                if (ret > 0) {
                    vp_lock(&channel->lock);
                    vp_multiobject_detect_human_result(info->handle, &info->result.human);
                    if (info->result.human.count > 0) {
                        info->result.state = 1;
                        info->result.timestamp = frame->timestamp;
                        channel->human_timestamp = frame->timestamp;
                        human_result = &info->result.human;
                    } else {
                        ret = 0;
                    }
                    vp_unlock(&channel->lock);

                    if (info->result.state == 1) {
                        if (info->param.human.draw_box) {
                            if (box_flag[idx] == 1) {
                                vp_algorithm_human_clear_boxes(idx, 0);
                                vp_algorithm_human_clear_boxes(idx, 1);
                            }
                            box_flag[idx] = 1;
                            box_count[idx] = 0;

                            vp_algorithm_human_draw_boxes(idx, 0, frame->width, frame->height, &info->result.human);
                            vp_algorithm_human_draw_boxes(idx, 1, frame->width, frame->height, &info->result.human);
                        }
                        if (info->param.human.enable_track && track->track_target) {
                            for (int j = 0; j < info->result.human.count; ++j) {
                                g_context.track_objs[j].idx = idx;
                                g_context.track_objs[j].track_id = info->result.human.objs[j].track_id;
                                g_context.track_objs[j].score = info->result.human.objs[j].score;
                                g_context.track_objs[j].x = info->result.human.objs[j].rect.x;
                                g_context.track_objs[j].y = info->result.human.objs[j].rect.y;
                                g_context.track_objs[j].w = info->result.human.objs[j].rect.w;
                                g_context.track_objs[j].h = info->result.human.objs[j].rect.h;
                                g_context.track_objs[j].width = frame->width;
                                g_context.track_objs[j].height = frame->height;
                            }
                            track->track_target(g_context.track_objs, info->result.human.count);
                        }
                    } else {
                        if (box_flag[idx] == 1 && box_count[idx]++ >= 3) {
                            vp_algorithm_human_clear_boxes(idx, 0);
                            vp_algorithm_human_clear_boxes(idx, 1);
                            box_flag[idx] = 0;
                        }

                        if (info->param.human.enable_track && track->track_target) {
                            track->track_target(g_context.track_objs, 0);
                        }
                    }
                } else {
                    if (box_flag[idx] == 1 && box_count[idx]++ >= 3) {
                        vp_algorithm_human_clear_boxes(idx, 0);
                        vp_algorithm_human_clear_boxes(idx, 1);
                        box_flag[idx] = 0;
                    }

                    if (info->param.human.enable_track && track->track_target) {
                        track->track_target(g_context.track_objs, 0);
                    }
                }
                break;
            }
#else
                case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                    if (frame->timestamp - channel->human_timestamp < 5000000) {
                        // 人形触发,忽略移动检测
                        continue;
                    }
                    // if (track->motor_state && track->motor_state() == 1) continue;
#if (VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23DL || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_3)
                    status_param_t *status_param = vp_status_param_get_handle();
                    if (status_param->factory_mode.mode != 0 || status_param->speak.status != 0 ||
                        status_param->record_play_status.status > 0) {
                        continue;
                    }
#endif
                    ret = vp_motion_detect_process(info->handle, frame);
                    if (ret > 0) {
                        vp_lock(&channel->lock);
                        info->result.state = 1;
                        vp_motion_detect_result(info->handle, &info->result.motion);
                        info->result.timestamp = frame->timestamp;
                        vp_unlock(&channel->lock);
                    }
                    break;
                }
                case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                    // if (track->motor_state && track->motor_state() == 1) {
                    //     if (box_flag[idx] == 1) {
                    //         vp_algorithm_human_clear_boxes(idx, 0);
                    //         vp_algorithm_human_clear_boxes(idx, 1);
                    //         box_flag[idx] = 0;
                    //         box_count[idx] = 0;
                    //     }
                    //     continue;
                    // }
#if (VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23DL || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_3)
                    status_param_t *status_param = vp_status_param_get_handle();
                    if (status_param->factory_mode.mode != 0 || status_param->speak.status != 0 ||
                        status_param->record_play_status.status > 0) {
                        if (box_flag[idx] == 1) {
                            vp_algorithm_human_clear_boxes(idx, 0);
                            vp_algorithm_human_clear_boxes(idx, 1);
                            box_flag[idx] = 0;
                            box_count[idx] = 0;
                        }
                        continue;
                    }
#endif
                    ret = vp_human_detect_process(info->handle, frame);
                    if (ret > 0) {
                        channel->human_timestamp = frame->timestamp;
                        vp_lock(&channel->lock);
                        info->result.state = 1;
                        vp_human_detect_result(info->handle, &info->result.human);
                        info->result.timestamp = frame->timestamp;
                        human_result = &info->result.human;
                        vp_unlock(&channel->lock);
                        if (info->param.human.draw_box) {
                            if (box_flag[idx] == 1) {
                                vp_algorithm_human_clear_boxes(idx, 0);
                                vp_algorithm_human_clear_boxes(idx, 1);
                            }
                            box_flag[idx] = 1;
                            box_count[idx] = 0;
                            vp_algorithm_human_draw_boxes(idx, 0, frame->width, frame->height, &info->result.human);
                            vp_algorithm_human_draw_boxes(idx, 1, frame->width, frame->height, &info->result.human);
                        } else {
                            if (box_flag[idx] == 1) {
                                vp_algorithm_human_clear_boxes(idx, 0);
                                vp_algorithm_human_clear_boxes(idx, 1);
                                box_flag[idx] = 0;
                                box_count[idx] = 0;
                            }
                        }

                        // if (info->param.human.enable_track && g_singletrack_handle) {
                        //     ALG_SINGLE_TARGET_INPUT_BOX_S tarck_input_box = {0};
                        //     ALG_SINGLE_TARGET_TRACK_RESULTS_S track_result = {0};

                        //     for (int j = 0; j < info->result.human.count; ++j) {
                        //         tarck_input_box.detBox[i].f32Score = info->result.human.objs[j].score;
                        //         tarck_input_box.detBox[i].u32X = info->result.human.objs[j].rect.x;
                        //         tarck_input_box.detBox[i].u32Y = info->result.human.objs[j].rect.y;
                        //         tarck_input_box.detBox[i].u32Width = info->result.human.objs[j].rect.w;
                        //         tarck_input_box.detBox[i].u32Height = info->result.human.objs[j].rect.h;
                        //     }

                        //     tarck_input_box.u32ObjNum = info->result.human.count;
                        //     TS_ALG_SingleTgtTrack_Process(g_singletrack_handle, &tarck_input_box, &track_result);

                        //     // if (track_result.u32TrackNum && track->track_target) {
                        //     //     g_context.track_objs[0].idx = idx;
                        //     //     g_context.track_objs[0].track_id = 0;
                        //     //     g_context.track_objs[0].score =  track_result.stBox.f32Score;
                        //     //     g_context.track_objs[0].x = track_result.stBox.u32X;
                        //     //     g_context.track_objs[0].y = track_result.stBox.u32Y;
                        //     //     g_context.track_objs[0].w = track_result.stBox.u32Width
                        //     //     g_context.track_objs[0].h = track_result.stBox.u32Height;
                        //     //     g_context.track_objs[0].width = frame->width;
                        //     //     g_context.track_objs[0].height = frame->height;

                        //     //     track->track_target(g_context.track_objs, info->result.human.count);

                        //     // }
                        // }
                    } else {
                        if (box_flag[idx] == 1) {
                            vp_algorithm_human_clear_boxes(idx, 0);
                            vp_algorithm_human_clear_boxes(idx, 1);
                            box_flag[idx] = 0;
                        }

                        // if (info->param.human.enable_track && track->track_target) {
                        //     track->track_target(g_context.track_objs, 0);
                        // }
                    }
                    break;
                }
#endif
            case VP_ALGORITHM_TYPE_CONVERGENCE_DETECT: {
                // ret = vp_convergence_detect_process(info->handle, channel->human_timestamp, human_result, frame);
                // if (ret > 0) {
                //     vp_lock(&channel->lock);
                //     info->result.state = 1;
                //     vp_convergence_detect_result(info->handle, &info->result.convergence);
                //     info->result.timestamp = frame->timestamp;
                //     vp_unlock(&channel->lock);
                // }
            }
            case VP_ALGORITHM_TYPE_MAX:
            default:
                break;
        }
        info->detect_timestamp = frame->timestamp;
        vp_events_send(channel->events, VP_EVENT_BIT(i));

        if (ret > 0 && info->param.notify && info->notify == 0) {
            if (info->last_notify == 0 || (frame->timestamp - info->last_notify) > (info->param.interval * 1000000)) {
                if (info->param.jpeg) need_jpeg = 1;
                info->has_notify = 1;
                info->last_notify = frame->timestamp;
            }
        }
    }
    if (need_jpeg && channel->jpeg_data) {
        for (int j = 0; j < VP_VIDEO_OSD_MAX; ++j) {
            // vp_video_osd_overlay(idx, chn, j, frame_data,
            //                      frame_data + frame->width * frame->height,
            //                      frame->width, frame->height);
        }

        vp_lock(&channel->lock);
        if (channel->jpeg_data) {
#if VP_SDK_PLATFORM == VP_SDK_PLATFORM_T41
            uint32_t height = (frame->height + 15) & (~15);
            uint32_t width = (frame->width + 15) & (~15);

            memmove(frame->frame_data + height * width, frame->frame_data + frame->width * frame->height,
                    frame->width * frame->height / 2);

#endif
            channel->jpeg_size = channel->data_size;
#if (VP_SDK_PLATFORM == VP_SDK_PLATFORM_T31 || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23DL || \
     VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_V9 || VP_SOC_MODEL == VP_SOC_MODEL_JZ_T23ZN_3)
            ret = vp_video_encoder_jpeg(idx, 1, NULL, channel->jpeg_data, &channel->jpeg_size);
#else
            ret = vp_video_encoder_jpeg(idx, 1, frame, channel->jpeg_data, &channel->jpeg_size);
#endif
            if (ret) {
                channel->jpeg_size = 0;
            }
        }
        vp_unlock(&channel->lock);
    }

    uint8_t need_notify = 0;
    for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
        info = &channel->infos[i];
        if (info->has_notify) {
            need_notify = 1;
            info->has_notify = 0;
            info->notify = 1;
        }
    }
    if (need_notify) vp_events_send(g_context.events, VP_EVENT_BIT0);
    return 0;
}

static void vp_algorithm_detect_destroy(vp_algorithm_type_t type, vp_algorithm_info_t *info) {
    switch (type) {
#if VP_ALGORITHM_FUSION && VP_ALGORITHM_HUMAN
        case VP_ALGORITHM_TYPE_MOTION_DETECT:
        case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
            vp_fusion_detect_destroy(info->handle);
            break;
        }
#elif VP_ALGORITHM_MULTIOBJECT && VP_ALGORITHM_HUMAN
        case VP_ALGORITHM_TYPE_MOTION_DETECT:
        case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
            vp_multiobject_detect_destroy(info->handle);
            break;
        }
#else
        case VP_ALGORITHM_TYPE_MOTION_DETECT: {
            vp_motion_detect_destroy(info->handle);
            break;
        }
        case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
            vp_human_detect_destroy(info->handle);
            break;
        }
#endif
        case VP_ALGORITHM_TYPE_CONVERGENCE_DETECT: {
            // vp_convergence_detect_destroy(info->handle);
            break;
        }
        case VP_ALGORITHM_TYPE_MAX:
        default:
            break;
    }
    vp_stack("destroy algorithm:%d handle:%p", type, info->handle);
    info->handle = NULL;
}


static int vp_algorithm_detect_after(uint8_t idx, uint8_t chn, vp_algorithm_ivs_args_t *args,
                                     vp_video_source_t *frame) {
    (void) args;
    (void) frame;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_events_send(channel->events, VP_EVENT_BIT(VP_ALGORITHM_TYPE_MAX));
    vp_algorithm_info_t *info;
    uint8_t has_jpeg = 0;
    for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
        info = &channel->infos[i];
        if (info->enable) {
            if (info->param.jpeg) has_jpeg = 1;
            if (info->pause && info->paused == 0) {
                vp_algorithm_detect_pause(i, info);
            }
            continue;
        }
        if (info->handle == NULL) continue;
        vp_algorithm_detect_destroy(i, info);
    }
    if (has_jpeg == 0 && channel->jpeg_data) {
        vp_debug("free jpeg buffer:%p", channel->jpeg_data);
        vp_lock(&channel->lock);
        if (channel->jpeg_data) {
            free(channel->jpeg_data);
            channel->jpeg_data = NULL;
        }
        vp_unlock(&channel->lock);
    }
    return 0;
}

static int vp_algorithm_detect_deinit(vp_algorithm_ivs_args_t *args) {
    (void) args;
    vp_algorithm_info_t *info;
    for (int i = 0; i < VP_SENSOR_NUM; ++i) {
        for (int j = 0; j < vp_video_chn_max; ++j) {
            vp_algorithm_channel_t *channel = &g_context.channels[i][j];
            for (int k = 0; k < VP_ALGORITHM_TYPE_MAX; ++k) {
                info = &channel->infos[k];
                if (info->handle == NULL) continue;
                vp_algorithm_detect_destroy(k, info);
            }
            vp_lock(&channel->lock);
            if (channel->jpeg_data) {
                free(channel->jpeg_data);
                channel->jpeg_data = NULL;
            }
            vp_unlock(&channel->lock);
        }
    }
#if VP_ALGORITHM_TYPE == VP_ALGORITHM_TYPE_YTLD
    vp_algorithm_ytld_unload();
#endif

    ALG_IMAGE_S* pimage = args->user_data;
    if (pimage != NULL) {
        if (pimage->pData != NULL) {
            TS_MPI_SYS_MmzFree(pimage->pDataPhy, pimage->pData);
        }

        free(pimage);
    }
    args->user_data = NULL;

    // if (g_singletrack_handle) {
    //     TS_ALG_SingleTgtTrack_Exit(g_singletrack_handle);
    //     g_singletrack_handle = NULL;
    // }

    return 0;
}

void *vp_algorithm_notify_thread(void *args) {
    (void) args;
    vp_pthread_name("algorithm_notify");
    pthread_detach(pthread_self());
    vp_debug("start");
    uint64_t bits;
    int ret;
    uint8_t *jpeg_data = malloc(128 * 1024);
    vp_algorithm_notify_t notify;
    while (g_context.state != VP_ALGORITHM_STATE_IDLE) {
        bits = VP_EVENTS_ALL;
        ret = vp_events_wait(g_context.events, &bits, VP_EVENTS_FLAG_OR_CLEAR, 1000);
        if (ret) {
            if (ret == -2) continue;
            break;
        }
        vp_algorithm_info_t *info;
        for (int i = 0; i < VP_SENSOR_NUM; ++i) {
            for (int j = 0; j < vp_video_chn_max; ++j) {
                vp_algorithm_channel_t *channel = &g_context.channels[i][j];
                for (int k = 0; k < VP_ALGORITHM_TYPE_MAX; ++k) {
                    info = &channel->infos[k];
                    if (info->enable == 0 || info->notify == 0) continue;
                    vp_lock(&channel->lock);
                    memcpy(&notify.result, &info->result, sizeof(vp_algorithm_result_t));
                    vp_unlock(&channel->lock);
                    vp_stack("rect notify: idx:%d chn:%d type:%d last timestamp:%lld jpeg:%d width:%d height:%d", i, j,
                             k, info->last_notify, info->param.jpeg, channel->frame_width, channel->frame_height);

                    if (info->param.jpeg) {
                        vp_lock(&channel->lock);
                        if (channel->jpeg_data && channel->jpeg_size < 128 * 1024) {
                            memcpy(jpeg_data, channel->jpeg_data, channel->jpeg_size);
                            notify.jpeg_data = jpeg_data;
                            notify.jpeg_size = channel->jpeg_size;
                        } else {
                            if (channel->jpeg_size >= 128 * 1024) {
                                vp_error("jpeg buffer too small: %d", channel->jpeg_size);
                            }
                            notify.jpeg_data = 0;
                            notify.jpeg_size = 0;
                        }
                        vp_unlock(&channel->lock);
                    } else {
                        notify.jpeg_data = 0;
                        notify.jpeg_size = 0;
                    }

                    notify.idx = i;
                    notify.chn = j;
                    notify.type = k;
                    notify.width = channel->frame_width;
                    notify.height = channel->frame_height;
                    switch (k) {
                        case VP_ALGORITHM_TYPE_MOTION_DETECT: {
                            vp_observers_post(VP_OBS_TYPE_ALARM, VP_OBS_EVENT_ALARM_MOTION, &notify);
                            break;
                        }
                        case VP_ALGORITHM_TYPE_HUMAN_DETECT: {
                            vp_observers_post(VP_OBS_TYPE_ALARM, VP_OBS_EVENT_ALARM_PERSON, &notify);
                            break;
                        }
                        case VP_ALGORITHM_TYPE_MAX:
                        default:
                            break;
                    }

                    info->notify = 0;
                }
            }
        }
    }
    if (jpeg_data) {
        free(jpeg_data);
        jpeg_data = NULL;
    }
    vp_debug("exit");
    return NULL;
}

/**
 * 初始化AI算法模块
 *
 * @return 错误码
 * @retval 0  成功
 * @retval <0 失败
 */
int vp_algorithm_init() {
    if (g_context.state != VP_ALGORITHM_STATE_IDLE) return -1;
    g_context.state = VP_ALGORITHM_STATE_STARTING;

    memset(&g_context.channels, 0, sizeof(g_context.channels));
    g_context.events = vp_events_create();
    for (int i = 0; i < VP_SENSOR_NUM; ++i) {
        for (int j = 0; j < vp_video_chn_max; ++j) {
            g_context.channels[i][j].events = vp_events_create();
            vp_lock_init(&g_context.channels[i][j].lock);
        }
    }
    // memset(&g_context.track, 0, sizeof(g_context.track));
    // vp_track_opt(&g_context.track);
    vp_algorithm_ivs_args_t args = {
            .init = vp_algorithm_detect_init,
            .before = vp_algorithm_detect_before,
            .process = vp_algorithm_detect_process,
            .after = vp_algorithm_detect_after,
            .deinit = vp_algorithm_detect_deinit,
            .user_data = NULL,
    };
    vp_algorithm_ivs_init(&args);
    for (int i = 0; i < VP_SENSOR_NUM; ++i) {
        int ret = vp_algorithm_ivs_bind(i, vp_video_chn_sec);
        vp_debug("vp_algorithm_ivs_bind:%d", ret);
    }
    pthread_t pthread;
    vp_pthread_create(&pthread, 64 * 1024, vp_algorithm_notify_thread, NULL);
    return 0;
}

/**
 * 获取AI算法模块鉴权文件路径
 * @return 算法鉴权文件路径
 */
char *vp_algorithm_auth_file() {
#if VP_PLATFORM_IS_DEVICE(VP_SDK_PLATFORM)
    return "/tmp/ai_auth.lic";
#else
    return "./tmp/ai_auth.lic";
#endif
}

/**
 * 获取AI算法模块模型文件路径
 * @return 算法模型文件路径
 */
char *vp_algorithm_model_file() {
#if VP_PLATFORM_IS_DEVICE(VP_SDK_PLATFORM)
    return "/model";
#else
    return "./model";
#endif
}

/**
 * 获取AI算法模块配置文件路径
 * @return 算法配置文件路径
 */
char *vp_algorithm_config_file() {
#if VP_PLATFORM_IS_DEVICE(VP_SDK_PLATFORM)
    return "/model/config/config.bin";
#else
    return "./model/config/config.bin";
#endif
}

/**
 * 启动AI算法模块
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @param param [in]    算法参数
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_start(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, vp_algorithm_param_t *param) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX) return -1;
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    if (param) memcpy(&info->param, param, sizeof(vp_algorithm_param_t));
    else param = &info->param;
    vp_stack("start idx:%d chn:%d detect type:%d notify:%d interval:%d jpeg:%d enable:%d notify:%d", idx, chn, type,
             param->notify, param->interval, param->jpeg, info->enable, info->notify);
    if (info->enable) {
        info->update = 1;
        return 0;
    }

    info->pause = 0;
    info->enable = 1;
    info->notify = 0;
    return 0;
}

/**
 * 获取AI算法模块状态
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @param status [out] 算法状态
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_state(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, vp_algorithm_status_t *status) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX || status == NULL) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    status->enable = info->enable;
    status->paused = info->pause;
    return 0;
}

/**
 * 获取AI算法模块是否启用
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @return 0 禁用,1 启用
 */
int vp_algorithm_enable_state(uint8_t idx, uint8_t chn, vp_algorithm_type_t type) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX) return 0;
    return g_context.channels[idx][chn].infos[type].enable;
}

/**
 * 获取AI算法模块是否暂停
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @return 0 暂停,1 运行
 */
int vp_algorithm_pause_state(uint8_t idx, uint8_t chn, vp_algorithm_type_t type) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX) return 0;
    return g_context.channels[idx][chn].infos[type].pause;
}

/**
 * 启用禁用AI算法模块通知
 *
 * @param idx       [in]    视频传感器索引
 * @param chn       [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type      [in]    算法类型
 * @param enable    [in]    是否启用通知
 * @param jpeg      [in]    是否推送图片
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_enable_notify(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, uint8_t enable, uint8_t jpeg) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX) return -1;
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return -1;
    g_context.channels[idx][chn].infos[type].param.notify = enable;
    g_context.channels[idx][chn].infos[type].param.jpeg = jpeg;
    return 0;
}

/**
 * 更新AI算法模块参数
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @param param [out]   算法参数
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_get_param(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, vp_algorithm_param_t *param) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX || param == NULL) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    memcpy(param, &info->param, sizeof(vp_algorithm_param_t));
    return 0;
}

/**
 * 更新AI算法模块参数
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 * @param param [in]    算法参数
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_set_param(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, vp_algorithm_param_t *param) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX || param == NULL) return -1;
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    memcpy(&info->param, param, sizeof(vp_algorithm_param_t));
    if (info->enable) info->update = 1;
    info->notify = 0;
    vp_stack("update idx:%d chn:%d detect type:%d notify:%d interval:%d jpeg:%d enable:%d notify:%d", idx, chn, type,
             param->notify, param->interval, param->jpeg, info->enable, info->notify);
    return 0;
}

/**
 * 暂停AI算法模块
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 *
 * @return 错误码
 * @retval =0 成功
 * @retval <0 失败
 */
int vp_algorithm_pause(uint8_t idx, uint8_t chn, vp_algorithm_type_t type) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max) return -1;
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    if (type == VP_ALGORITHM_TYPE_MAX) {
        for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
            channel->infos[i].pause = 1;
        }
    } else {
        channel->infos[type].pause = 1;
    }
    return 0;
}

/**
 * 恢复AI算法模块
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 *
 * @return 错误码
 * @retval =0 成功
 * @retval <0 失败
 */
int vp_algorithm_resume(uint8_t idx, uint8_t chn, vp_algorithm_type_t type) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max) return -1;
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    if (type == VP_ALGORITHM_TYPE_MAX) {
        for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
            channel->infos[i].pause = 0;
        }
    } else {
        channel->infos[type].pause = 0;
    }
    return 0;
}

/**
 * 等待AI算法模块结果
 *
 * @param idx       [in]    视频传感器索引
 * @param chn       [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type      [in]    算法类型
 * @param timeout   [in]    等待超时时间,单位毫秒(ms)
 * @param result    [out]  算法结果
 *
 * @return 错误码
 * @retval =0 成功
 * @retval <0 失败
 */
int vp_algorithm_wait_result(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, uint32_t timeout,
                             vp_algorithm_result_t *result) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type > VP_ALGORITHM_TYPE_MAX) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    if (info->enable == 0) return -2;
    uint64_t bits = (1 << type);
    uint64_t timestamp = vp_system_time_us();
    if (type == VP_ALGORITHM_TYPE_MAX) {
        return vp_events_wait(channel->events, &bits, VP_EVENTS_FLAG_OR_CLEAR, timeout);
    } else {
        do {
            int ret = vp_events_wait(channel->events, &bits, VP_EVENTS_FLAG_OR_CLEAR, timeout);
            if (ret < 0) return ret;
        } while (info->detect_timestamp < timestamp);
    }

    if (result == NULL) return 0;

    vp_lock(&channel->lock);
    memcpy(result, &info->result, sizeof(vp_algorithm_result_t));
    vp_unlock(&channel->lock);
    return 0;
}

/**
 * 获取AI算法模块结果
 *
 * @param idx       [in]    视频传感器索引
 * @param chn       [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type      [in]    算法类型
 * @param result    [out]   算法结果
 *
 * @return 错误码
 * @retval =0 成功
 * @retval <0 失败
 */
int vp_algorithm_get_result(uint8_t idx, uint8_t chn, vp_algorithm_type_t type, vp_algorithm_result_t *result) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max || type >= VP_ALGORITHM_TYPE_MAX) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    vp_algorithm_info_t *info = &channel->infos[type];
    if (info->enable == 0) return -2;
    vp_lock(&channel->lock);
    memcpy(result, &info->result, sizeof(vp_algorithm_result_t));
    vp_unlock(&channel->lock);
    return 0;
}

/**
 * 停止AI算法模块
 *
 * @param idx   [in]    视频传感器索引
 * @param chn   [in]    视频流通道索引,0 主码流通道,1 子码流通道
 * @param type  [in]    算法类型
 *
 * @return 错误码
 * @retval =0 成功
 * @retval !0 失败
 */
int vp_algorithm_stop(uint8_t idx, uint8_t chn, vp_algorithm_type_t type) {
    if (idx >= VP_SENSOR_NUM || chn >= vp_video_chn_max) return -1;
    vp_algorithm_channel_t *channel = &g_context.channels[idx][chn];
    if (type == VP_ALGORITHM_TYPE_MAX) {
        for (int i = 0; i < VP_ALGORITHM_TYPE_MAX; ++i) {
            channel->infos[i].enable = 0;
        }
    } else {
        channel->infos[type].enable = 0;
    }
    vp_stack("stop idx:%d chn:%d detect type:%d", idx, chn, type);
    return 0;
}

/**
 * 重新加载AI算法模块
 */
void vp_algorithm_reload() {
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return;
    vp_algorithm_ivs_reload();
}

/**
 * 释放AI算法模块资源
 */
void vp_algorithm_deinit() {
    if (g_context.state == VP_ALGORITHM_STATE_IDLE) return;
    g_context.state = VP_ALGORITHM_STATE_IDLE;
    for (int i = 0; i < VP_SENSOR_NUM; ++i) {
        for (int j = 0; j < vp_video_chn_max; ++j) {
            vp_algorithm_stop(i, j, VP_ALGORITHM_TYPE_MAX);
        }
    }
    vp_algorithm_ivs_deinit();
}