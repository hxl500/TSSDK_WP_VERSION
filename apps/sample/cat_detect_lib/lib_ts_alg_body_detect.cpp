#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "ts_rne_c_api.h"
#include "ts_rne_log.h"
#include "ts_rne_version.h"
#include "ts_rne_time.h"

#include "ts_alg_log.h"
#include "ts_alg_body_detect_v2.h"
#include "ts_rne_nn_input.h"
#include "arrr_diff.h"
//#define TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM (4)	//56 de sdk内部会对齐
// #define TIME_CONSUME_LAYER

static TS_BOOL gRneOff = TS_FALSE;
static TS_FLOAT gThreshold = 0.25;//0.25;
static TS_FLOAT *gPostProcBuf = NULL;

#ifdef TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM
	static TS_U8 *gParamStride = NULL;
#endif
ALG_CatDetect_DET_PARAM_S det_param_cpp;

static int nms(float *torchCat, int len1, int len2, int *nmsOut, float thresh, int maxDet)
{
    float tmpX, tmpY, tmpW, tmpH;

    float* pScore = (float *)(nmsOut + len1);
    int* box_idx = (int *)(pScore + len1);
    float* areas = (float*)(box_idx + len1);
    for (int i = 0; i < len1; i++)
    {
        box_idx[i] = i;
        pScore[i] = torchCat[len2 * i + 4];
        tmpX = torchCat[len2 * i + 0];
        tmpY = torchCat[len2 * i + 1];
        tmpW = torchCat[len2 * i + 2];
        tmpH = torchCat[len2 * i + 3];
        areas[i] = tmpW * tmpH;
        torchCat[len2 * i + 0] = tmpX - tmpW / 2;
        torchCat[len2 * i + 1] = tmpY - tmpH / 2;
        torchCat[len2 * i + 2] = tmpX + tmpW / 2;
        torchCat[len2 * i + 3] = tmpY + tmpH / 2;
    }

    for (int i = 0; i < len1; i++) {
        float max = pScore[i];
        int idx_t = i;
        for (int j = i + 1; j < len1; j++) {
            if (max < pScore[j]) {
                max = pScore[j];
                idx_t = j;
            }
        }
        int tt = box_idx[i];
        box_idx[i] = box_idx[idx_t];
        box_idx[idx_t] = tt;
        float tt1 = pScore[i];
        pScore[i] = pScore[idx_t];
        pScore[idx_t] = tt1;
    }

    int	cnt = 0;
    int sSzie = len1;
    while (sSzie > 0) {// torch.numel()返回张量元素个数
        if (sSzie == 1) {//    保留框只剩一个
            nmsOut[cnt] = box_idx[0];
            cnt++;
            break;
        }
        else {
            nmsOut[cnt] = box_idx[0];
            cnt++;
        }
        if (cnt >= maxDet) {
            break;
        }

		//计算box[i]与其余各框的IOU(思路很好)
		//auto orderMask = order.narrow(1, order.m_size - 1, 0);
		//x1.index({ orderMask });
		//x1.index({ orderMask }).clamp(x1[keep.back()].item().toFloat(), 1e10);
		//auto xx1 = x1.index({ orderMask }).clamp(x1.m_data[keep.back()],1e10);// [N - 1, ]
		//auto yy1 = y1.index({ orderMask }).clamp(y1.m_data[keep.back()], 1e10);
		//auto xx2 = x2.index({ orderMask }).clamp(0, x2.m_data[keep.back()]);
		//auto yy2 = y2.index({ orderMask }).clamp(0, y2.m_data[keep.back()]);
        float *xx1 = (float *)(areas + len1);
        float *yy1 = (float *)(xx1 + sSzie);
        float *xx2 = (float *)(yy1 + sSzie);
        float *yy2 = (float *)(xx2 + sSzie);
        for (int i=1; i<sSzie; i++){
            xx1[i-1] = MIN(MAX(torchCat[len2*box_idx[0] + 0], torchCat[len2*box_idx[i] + 0]), 1e10);
            yy1[i-1] = MIN(MAX(torchCat[len2*box_idx[0] + 1], torchCat[len2*box_idx[i] + 1]), 1e10);
            xx2[i-1] = MIN(MAX(0, torchCat[len2*box_idx[i] + 2]), torchCat[len2*box_idx[0] + 2]);
            yy2[i-1] = MIN(MAX(0, torchCat[len2*box_idx[i] + 3]), torchCat[len2*box_idx[0] + 3]);
        }

		// auto selWidth = xx2 - xx1;
        // auto selHeight = yy2 - yy1;
        // auto selWidth_ = selWidth.clamp(0, 1e10);
        // auto selHeight_= selHeight.clamp(0, 1e10);
		// auto inter = selWidth_ * selHeight_;// [N - 1, ]
        // xx2 = inter
        for (int i=0; i<sSzie-1; i++){
            xx1[i] = MIN(MAX(0, xx2[i] - xx1[i]), 1e10);
            yy1[i] = MIN(MAX(0, yy2[i] - yy1[i]), 1e10);
            xx2[i] = xx1[i] * yy1[i];
        }

        // auto tmpNarrow = order.narrow(1,order.m_size - 1, 0) ;
        // auto merge = areas.index(tmpNarrow) + areas.m_data[keep.back()] - inter;
		// auto iou = inter / (merge);//[N - 1, ]
        //xx1 = merge
        //yy1 = iou
        //yy2 = idx
        // auto idx = cmpnonzero(iou, thresh);
        int idx_len = 0;
        int *newbox_idx = (int *)yy2;
        //int* idx = (int*)yy2;
        for (int i = 1; i < sSzie; i++) {
            xx1[i - 1] = areas[box_idx[i]] + areas[box_idx[0]] - xx2[i - 1];
            yy1[i - 1] = xx2[i - 1] / xx1[i - 1];

            if (yy1[i - 1] < thresh) {
                newbox_idx[idx_len] = box_idx[i];
                idx_len++;
            }
        }

        if (idx_len == 0) {
            break;
        }

        // auto tmpIndex = idx + 1;
        // tmpIndex.m_type = 2;
        // order = order.index(tmpIndex); //修补索引之间的差值
        memcpy(box_idx, newbox_idx, idx_len * sizeof(int));
        //sSzie = idx_len - 1;
        sSzie = idx_len;
    }
    return cnt;
}
//lx test
float test_conf = 0;
int test_conf_set(float conf)
{
	test_conf = conf;
	return 0;
}
int test_conf_get()
{
	int ret = test_conf*100;
	return ret;
}
//
int TS_ALG_PcppDetV12_PostProcess(unsigned char **blob, unsigned int *cstride, unsigned int *s32C, float *fcoeff, float* dataVec, TS_U16* dataidx)
{
	int nblob = 3;
	//int imShape[4] = {1, 3, 640, 480}; // = srcShape
	int shapeVec2[3][5] = {{1, 3, 48, 80, 6}, {1, 3, 24, 40, 6}, {1, 3, 12, 20, 6}};//0,1,2

    float scale_output[] = { fcoeff[0], fcoeff[1], fcoeff[2] }; //1,2,3

    int stride[3] = { 8,16,32 };
    //unsigned int time1 = TIME_CACL_GET();
	//初始化先验眶
    // int anch[3][6] = {{6,9, 32,25, 22,59}, {73,53, 55,144, 108,291}, {201,172, 336,402, 432,439}};
    int anch[3][6] = {{10,13, 16,30, 33,23}, {30,61, 62,45, 59,119}, {116,90, 156,198, 373,326}};
    //int anch[3][6] = { {5,5, 11,12, 15,19}, {22,27, 40,53, 54,41}, {61,79, 115,159, 252,217} };

    float value_det = 0.5f; //_makeGrid : float value = 0.5;

    int len = 0;
    int idxtmp = 0;
    float petThres = 0.7;//0.45; //confidence
    	TS_ALG_CatDetect_GetParam(&det_param_cpp);
	petThres = det_param_cpp.DetectionConfThres;
    int maxNms = 1024;  // maximum number of boxes into torchvision.ops.nms()
    for (int i = 0; i < nblob; i++)
    {
        int no_yolo = s32C[i] / 3;
        shapeVec2[i][4] = no_yolo;
        for (int j = 0; j < shapeVec2[i][1]; j++) {
            int* anchorgrid = &(anch[i][j * 2]);
            for (int k = 0; k < shapeVec2[i][2]; k++) {
                for (int m = 0; m < shapeVec2[i][3]; m++) {
                    unsigned char *dataf = blob[i] + j * no_yolo + k * shapeVec2[i][3] * cstride[i] + m * cstride[i];
                    //int8_t *sigmoid_in = (int8_t *)dataf;
                    //Feature_gtf(pred.m_data, mask, pred.m_shape[0] * pred.m_shape[1], confThres, pred.m_shape[2], 4);
                    float data = (dataf[4] * scale_output[i]);
                    if (data > petThres) {
                        if(len < maxNms){
                            unsigned char max_data = 0;
                            unsigned char max_idx = 5;
                            for(int n = 5; n < no_yolo; n++){
                                if(max_data < dataf[n]){
                                    max_data = dataf[n];
                                    max_idx = n;
                                }
                            }

                            float score_tmp = dataf[max_idx] * scale_output[i] * data;
                            float tmpThres = petThres;
                            if(score_tmp > tmpThres){
                                float griddataf0 = (float)m - value_det;
                                float griddataf1 = (float)k - value_det;

                                //y_tmp = ((y.index("...", sha3) * 2.0) + this->grid[i]) * this->stride[i];
                                dataVec[idxtmp++] = (dataf[0] * scale_output[i] * 2.0f + griddataf0) * stride[i];
                                dataVec[idxtmp++] = (dataf[1] * scale_output[i] * 2.0f + griddataf1) * stride[i];

                                // y_tmp = (y.index("...", sha4) * 2).pow(2) * this->anchorGrid[i];
                                float value = (dataf[2] * scale_output[i] * 2.0f);
                                dataVec[idxtmp++] = value * value * anchorgrid[0];
                                value = (dataf[3] * scale_output[i] * 2.0f);
                                dataVec[idxtmp++] = value * value * anchorgrid[1];

                                dataVec[idxtmp++] = score_tmp;
                                //printf("score_tmp:%f\n",score_tmp);
                                // dataVec[idxtmp++] = dataf[5] * scale_output[i];
                                dataidx[len++] = max_idx;
                            }
                        }
						test_conf_set(data);
                    }
               }
           }
       }
    }

    return len;
}

TS_S32 TS_ALG_BodyDetect_Init(TS_VOID **handle, ALG_MODEL_INIT_S *param)
{
	printf("22222==============\n");
	TS_MPI_TRP_RNE_SetLogLevel(RNE_LOG_INFO);
    ALG_LOGI("rne log level : %d\n", TS_MPI_TRP_RNE_GetLogLevel());
    ALG_LOGI("rne lib version :%s\n", TS_MPI_TRP_RNE_GetSdkVersion());
	TS_S32 ret = 0;

	gRneOff = (TS_BOOL)param->bRneOff;
	if(TS_TRUE != gRneOff) {
	    // ret = TS_MPI_TRP_RNE_OpenDevice(NULL, rne_register_gp_layers);
	    ret = TS_MPI_TRP_RNE_OpenDevice(NULL, NULL);
	    if (0 != ret) {
	        ALG_LOGE("open device error!\n");
	        return -1;
	    }
		ALG_LOGD("open device success! \n");
	}

    RNE_NET_S *nModel = (RNE_NET_S *)malloc(sizeof(RNE_NET_S));
    if(NULL == nModel)
    {
		ALG_LOGE("malloc error!\n");
        return -1;
    }
	//ALG_LOGE("malloc error!\n");

	memset(nModel, 0, sizeof(RNE_NET_S));
    nModel->u8pGraph = param->pGraph;
    nModel->u8pParams = param->pWeight;
    nModel->eInputType = (RNE_NET_INPUT_TYPE_E)param->eImageType;
    /* 初始化多网络模型，并在每次初始化网络配置后，进行网络OnceLoad
     */
    /* 量化和权重数据需要4byte对齐
     * 如果未在头文件4byte对齐，可执行W_ALIGN_BYTES_NUM内代码，进行4字节对齐
     */

#ifdef TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM

	if(TS_NULL != gParamStride) {
		ALG_LOGE("Multi threading is not supported!\n");
        return -1;
	}

#ifdef LINUX_PAL
    gParamStride = (TS_U8 *)TS_MPI_TRP_RNE_AllocLinearMem(param->u32WeightSize + TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM);
#else
	gParamStride = (TS_U8 *)TS_MPI_TRP_RNE_Alloc(param->u32WeightSize + TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM);
#endif

    if (NULL == gParamStride) {
        ALG_LOGE("insufficient memory!\n");
        return -1;
    }
    TS_SIZE_T addr = (TS_SIZE_T)gParamStride;
    addr += TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM - 1;
    addr &= ~(TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM - 1);
    memcpy((TS_VOID *)addr, nModel->u8pParams, param->u32WeightSize);
    nModel->u8pParams = (TS_U8 *)addr;
#endif
	//ALG_LOGE("malloc error!\n");
    /* 初始化单个网路
        */
    ret = TS_MPI_TRP_RNE_LoadModel(nModel);
    if (ret) {
        ALG_LOGE("load model error!\n");
        return ret;
    }
    /* net once load
        * 仅有网络模型配置为once load情况下，内部才真正执行once load
        */
    ret = TS_MPI_TRP_RNE_OnceLoad(nModel);
    if (ret) {
        ALG_LOGE("once load error!\n");
        return ret;
    }

#ifdef TS_MPI_TRP_RNE_W_ALIGN_BYTES_NUM
	if(TS_NULL != gParamStride) {
#ifdef LINUX_PAL
		TS_MPI_TRP_RNE_FreeLinearMem(gParamStride);
#else
		TS_MPI_TRP_RNE_Free(gParamStride);
#endif
		gParamStride = TS_NULL;
	}
#endif
	//ALG_LOGE("malloc error!\n");
	TS_U32 heap_size = TS_MPI_TRP_RNE_GetBlobsBufSize(nModel);
	if(heap_size > 0){
		ALG_LOGI("model heap size is %d\n", heap_size);
	}
	else{
		ALG_LOGE("error, TS_MPI_TRP_RNE_GetBlobsBufSize error!\n");
		return -1;
	}
	//ALG_LOGE("malloc error!\n");
    int shapeVec[] = { 1, 3 * 48 * 80 + 3 * 24 * 40 + 3 * 12 * 20, 6};//0,1,2
    gPostProcBuf = (TS_FLOAT *)malloc(shapeVec[0] * shapeVec[1] * shapeVec[2] * sizeof(float));    //362,880
	if(NULL == gPostProcBuf){
		ALG_LOGE("malloc error !!!\n");
		return -1;
	}
	ALG_LOGI("algo heap size is %d\n", shapeVec[0] * shapeVec[1] * shapeVec[2] * sizeof(float));
	//ALG_LOGE("malloc error!\n");
    *handle = nModel;
    return 0;
}

TS_S32 TS_ALG_BodyDetect_Exit(TS_VOID *handle)
{
    RNE_NET_S *nModel = (RNE_NET_S *)handle;

	TS_MPI_TRP_RNE_UnloadModel(nModel);

	if(TS_NULL != nModel) {
		free(nModel);
		nModel = TS_NULL;
	}

	if(TS_TRUE != gRneOff) {
		TS_MPI_TRP_RNE_CloseDevice();
	}

	if(TS_NULL != gPostProcBuf) {
		free(gPostProcBuf);
		gPostProcBuf = TS_NULL;
	}

    return 0;
}

TS_S32 strides[3] = {8, 16, 32};
std::unordered_map<std::string, std::vector<std::vector<TS_S32>>> anchors = {{"0", {{10, 13}, {16, 30}, {33, 23}}},
    {"1", {{30, 61}, {62, 45}, {59, 119}}},
    {"2", {{116, 90}, {156, 198}, {373, 326}}}
};

TS_S32 TS_ALG_BodyDetect_Process(TS_VOID *handle, ALG_IMAGE_S *image, ALG_CatDetect_DET_RESULT_S *result)
{
#if 1
	TS_U32 cstride[16];
	TS_FLOAT fcoeff[16];

    if(TS_NULL == image->pData || TS_NULL == result) {
		ALG_LOGE("error, Invalid parameter!\n");
		return -1;
	}
	//printf("%s,%d\n",__FUNCTION__,__LINE__);
	RNE_NET_S *nModel = (RNE_NET_S *)handle;



#if 1 //input image is 640*384
    if (0 != TS_MPI_TRP_RNE_SetInputBlobsAddr(nModel, (void *)(image->pData), (void *)(image->pDataPhy))) {
        ALG_LOGE("set inputBlobs error!\n");
        return -1;
    }
#else //input image is 640*360
	if(image->s32H != 360 || image->s32W != 640) {
		ALG_LOGE("error, Invalid image size need resize!\n");
		ALG_LOGE("image h:%d w:%d, blob h:%d w:%d\n", image->s32H, image->s32W,
								inputBlobs->stpBlob->s32H, inputBlobs->stpBlob->s32W);
		return -1;
	}

	//image pretreatment: 640*360->640*384, fill with 114(0x72) before image and end image
	TS_U32 inputSize = image->s32C*image->s32H*image->s32W;
	TS_U32 imageSize = image->s32C*640*384;

	memset(imageBuf, 114, imageSize);
	memcpy(&imageBuf[image->s32C*640*12], image->pData, inputSize);

	nModel->vpInput = imageBuf;
#endif

#ifdef TIME_CONSUME_LAYER
	 TS_MPI_TRP_RNE_StartSysTimer();
	 RNE_TIME_STATES_S time;
	 memset(&time, 0, sizeof(RNE_TIME_STATES_S));
	 TS_MPI_TRP_RNE_InitTimeState(DETECT_OUTPUT_GP_LAYER, MAX_LAYER_TYPE - SLICE_GP_LAYER, &time);
	 TS_MPI_TRP_RNE_NetBindTimeState(nModel, &time);
#endif

//	TS_U32 time0 = 0;//TIME_CACL_GET();
	//TIME_CACL_GET();
	RNE_BLOBS_S *outputBlobs = TS_MPI_TRP_RNE_Forward(nModel);
	if (outputBlobs == TS_NULL) {
		ALG_LOGE("net forward error!\n");
		#ifdef TIME_CONSUME_LAYER
		TS_MPI_TRP_RNE_ReleaseTimeState(&time);
		#endif
		return -1;
	}

//	TS_U32 time1 = 1;//TIME_CACL_GET();
	//ALG_LOGE("rne forward time:%d\n",time1-time0);
	//printf("%s,%d\n",__FUNCTION__,__LINE__);
#ifdef TIME_CONSUME_LAYER
	ALG_LOGD("total time:%lld us\n", TS_MPI_TRP_RNE_GetTotalTime(&time));
	ALG_LOGD("forward time:%lld us\n", TS_MPI_TRP_RNE_GetTimeOfForward(&time));
	ALG_LOGD("hw layer time:%lld us\n", TS_MPI_TRP_RNE_GetTimeOfHwLayer(&time));
	ALG_LOGD("gp layer time:%lld us\n", TS_MPI_TRP_RNE_GetTimeOfGpLayer(&time));
	TS_MPI_TRP_RNE_ReleaseTimeState(&time);
#endif

	//ALG_LOGI("net body detect forward done\n");
	//ALG_LOGI("outputBlobs->u32NBlob:%d\n", outputBlobs->u32NBlob);

	if(outputBlobs->u32NBlob <= 0 || TS_NULL == outputBlobs->stpBlob) {
		ALG_LOGE("net forward no result!\n");
		return -1;
	}

	//ALG_LOGD("totol u32NBlob:%d\n", outputBlobs->u32NBlob);
	// 检查是否有足够的blob数量
	if(outputBlobs->u32NBlob < 3) {
		ALG_LOGE("insufficient blobs: expected at least 3, got %d\n", outputBlobs->u32NBlob);
		return -1;
	}
	
	TS_U8 *resultAddr[3] = {TS_NULL, TS_NULL, TS_NULL};
    unsigned int s32C[16];
    
	// 检查每个vpAddr是否为空
	for(int i = 0; i < 3; i++) {
		if(TS_NULL == outputBlobs->stpBlob[i].vpAddr) {
			ALG_LOGE("blob[%d] vpAddr is NULL\n", i);
			return -1;
		}
		resultAddr[i] = (TS_U8 *)outputBlobs->stpBlob[i].vpAddr;
	}
	for(TS_U32 i = 0; i < outputBlobs->u32NBlob; i++) {

		const int c = outputBlobs->stpBlob[i].s32C;
		const int s32BitNum = outputBlobs->stpBlob[i].s32BitNum;
		const int c_align = TS_MPI_TRP_RNE_CStride(c, s32BitNum, outputBlobs->stpBlob[i].bIsJoined);

		cstride[i] = c_align;
		s32C[i] = c;
		//fcoeff[i] = outputBlobs->stpBlob[i].fCoeff[0];
		fcoeff[i] = *(outputBlobs->stpBlob[i].fCoeff);
		// ALG_LOGD("u32NBlob:%d H: %d, W:%d, C:%d, Cstride:%d, coeff: %f\n", i, outputBlobs->stpBlob[i].s32H, outputBlobs->stpBlob[i].s32W, outputBlobs->stpBlob[i].s32C, c_align, outputBlobs->stpBlob[i].fCoeff[0]);
	}

    float* dataVec = (float*)gPostProcBuf;    //362,880
    TS_U16* dataidx = (TS_U16 *)(gPostProcBuf + 15*1024);    //362,880
	int len = TS_ALG_PcppDetV12_PostProcess(resultAddr, cstride, s32C, fcoeff, dataVec, dataidx);
    const int maxDet = 100;       // maximum detections per image
    int *nmsOut = (int *)(gPostProcBuf + len * 5);
    // if (33 == frameNo1)
	// {
        // ALG_LOGD("before nms, len = %d\n", len);
    //     for(int i=0; i<len; i++){
    //         ALG_LOGD("%f, %f, %f, %f, %f, %f\n", dataVec[i*no_yolo + 0], dataVec[i*no_yolo + 1], dataVec[i*no_yolo + 2], dataVec[i*no_yolo + 3], dataVec[i*no_yolo + 4], dataVec[i*no_yolo + 5]);
    //     }
    // }

    //int outNum = nonMaxSuppression(dataVec, confThres, iouThres, agnosticNms, maxDet, false, len, no_yolo, nmsOut);
    int outNum = nms(gPostProcBuf, len, 5, nmsOut, 0.45, maxDet);  // NMS
	result->u32ObjNum = outNum;
	
    TS_ALG_CatDetect_GetParam(&det_param_cpp);
	
	//petThres = det_param_cpp.DetectionConfThres;
	int j = 0;
	
	for (int i = 0; i < outNum; i++) {
		//printf("outNum %d\n",outNum);
		if(gPostProcBuf[nmsOut[i] * 5 + 4] <= det_param_cpp.DetectionConfThres){
			continue;
		}
		result->stBox[j].f32Xmin = gPostProcBuf[nmsOut[i] * 5 + 0]/640.0;
		result->stBox[j].f32Ymin = MAX(0, (gPostProcBuf[nmsOut[i] * 5 + 1]-12.0) / 360.0);
		result->stBox[j].f32Xmax = gPostProcBuf[nmsOut[i] * 5 + 2]/640.0;
		result->stBox[j].f32Ymax = MAX(0, (gPostProcBuf[nmsOut[i] * 5 + 3]-12.0) / 360.0);

		//result->stBox[i].f32Xmin = gPostProcBuf[nmsOut[i] * 5 + 0];
		//result->stBox[i].f32Ymin = gPostProcBuf[nmsOut[i] * 5 + 1];
		//result->stBox[i].f32Xmax = gPostProcBuf[nmsOut[i] * 5 + 2];
		//result->stBox[i].f32Ymax = gPostProcBuf[nmsOut[i] * 5 + 3];
        result->stBox[j].DetectionConf = gPostProcBuf[nmsOut[i] * 5 + 4];
		 result->stBox[j].class_id = dataidx[nmsOut[i]] - 5;
		 j++;
		//printf("f32Score=%f\n",result->stBox[i].f32Score);
        //printf("f32Xmin %f,f32Ymin %f,f32Xmax %f,f32Ymax %f\n",gPostProcBuf[nmsOut[i] * 5 + 0],
        //    gPostProcBuf[nmsOut[i] * 5 + 1],gPostProcBuf[nmsOut[i] * 5 + 2],gPostProcBuf[nmsOut[i] * 5 + 3]);
	}
	result->u32ObjNum = j;
    //ALG_LOGE("rne postprocess time:%d\n",TIME_CACL_GET()-time1);

#endif

	return 0;
}

TS_S32 TS_ALG_BodyDetect_SetParam(TS_VOID *handle, ALG_BODYDET_PARAM_S *param)
{
	if(TS_NULL == param) {
		ALG_LOGE("error, Invalid parameter!\n");
		return -1;
	}

	gThreshold = param->f32Thresh;
	ALG_LOGD("body detect thresh:%f\n", gThreshold);
	return 0;
}

TS_S32 TS_ALG_BodyDetect_GetParam(TS_VOID *handle, ALG_BODYDET_PARAM_S *param)
{
	if(TS_NULL == param) {
		ALG_LOGE("error, Invalid parameter!\n");
		return -1;
	}

	param->f32Thresh = gThreshold;
	return 0;
}

const TS_CHAR* TS_ALG_BodyDetect_GetVersion(TS_VOID)
{
    static const TS_CHAR *ver = "bodyDetect_v1.5_model_0712";

    return (const TS_CHAR *)ver;
}
