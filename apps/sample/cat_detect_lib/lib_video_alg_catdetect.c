#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#include "video_alg_catdetect.h"
#include "soft_line.h"
#include "tsalg_alg_lib.h"
#include "ts_alg_body_detect_v2.h"
#include "cJSON.h"
#include "arrr_diff.h"
#include "file_sync.h"
#include "qi_nv12_yuv.h"
#define TEST_MODE1 0 //打开这个 则保存640码流
#define TEST_MODE2 0 //打开这个 则读取输入文件为视频流
#define TEST_MODE3 0 //打开这个 则不允许猫脸检测 只保存640码流
#define TEST_MODE4 0 //打开这个 保存输入文件
static int is_food_on = 0;

#if TEST_MODE1
#define MODE1_NAME_PRE "/media/mmcblk0p1/testfile/test_mode1/test_mode1_%d.rgb"
char mode1_name[128] = {0};
#define MODE1_NAME_PRE_YUV "/media/mmcblk0p1/testfile/test_mode1/test_mode1_%d.yuv"
char mode1_name_yuv[128] = {0};

#endif

#if TEST_MODE2
// char mode2_name[128] = {0};
// #define MODE2_NAME_PRE "/media/mmcblk0p1/testfile/test_mode2/input_mode2.rgba"
// char mode2_name_yuv[128] = {0};
// #define MODE2_NAME_PRE_YUV "/media/mmcblk0p1/testfile/test_mode2/input_mode2.yuv"

char mode2_name[128] = {0};
#define MODE2_NAME_PRE "/mnt/sda0/weipai/merge_img.rgba"
char mode2_name_yuv[128] = {0};
#define MODE2_NAME_PRE_YUV "/mnt/sda0/weipai/input_mode2.yuv"

#endif
#if 1
//#define LOCAL_MODERL "/media/mmcblk0p1/bin/yolov5_relu_quantize_r.cfg"
//#define LOCAL_MODERL_WEIGHT "/media/mmcblk0p1/bin/yolov5_relu_quantize_r.weight"

#define LOCAL_MODERL "/home/ipc_pro/bin/yolov5_relu_quantize_r.cfg"
#define LOCAL_MODERL_WEIGHT "/home/ipc_pro/bin/yolov5_relu_quantize_r.weight"

#define LOCAL_MODERL_TT_1 "/tmp/aicfg/yolov5_relu_quantize_r.cfg"
#define LOCAL_MODERL_WEIGHT_TT_1 "/tmp/aicfg/yolov5_relu_quantize_r.weight"

#define RSN_LOCAL_TT_1 "/tmp/aicfg/resnet18_quantize_r.weight"
#define RSN_LOCAL_CFG_TT_1  "/tmp/aicfg/resnet18_quantize_r.cfg"

#define MODEL_VERSION_TT_1  "/tmp/aicfg/version"

static TS_U32 getFileSize(FILE *pf)
{
	TS_U32 fileSize = 0;
	if (NULL != pf) {
		if (0 == fseek(pf, 0, SEEK_END)) {
			fileSize = ftell(pf);
		}
		rewind(pf);
	}
	return fileSize;
}
#ifdef OVERLAY_RGN
static TS_OVERLAY_RGN_PARAM_S overlay_rgn_param[TS_RNG_HANDLE_BUTT];
#endif
#define TS_MAX_OVERLAY_NUM	20
//static int nArraySize[TS_MAX_OVERLAY_NUM] = {0};
//static TS_BITMAP_CHAT_S *bitmapArray[TS_MAX_OVERLAY_NUM] = {NULL};

//libs add
static ALG_CatDetect_DET_PARAM_S all_det_param;
#define CATVERSION "1.0.2"
TS_CHAR cat_version_buf[64] = {0};
TS_CHAR yolo_version_buf[64] = {0};
TS_CHAR emb_version_buf[64] = {0};
ALG_CAT_MODEL_INIT_S *model_info = NULL;

extern TS_VOID  VIDEO_ALG_CatDetect_ResultProc(TS_U8 *pYuvBuf,  TS_U32 width, TS_U32 height, TS_U32 u32ImageRatio, TS_VOID *pAlgResult);

//
#if 0
static TS_OVERLAY_YUV_PARAM_S overlay_yuv_param[TS_MAX_OVERLAY_NUM];
static char overlay_yuv_param_set[TS_MAX_OVERLAY_NUM][128];
TS_OVERLAY_YUV_PARAM_S * overlay_yuv_param_get(const char*name){
	int i;
	printf("overlay_yuv_param_get name=%s\n",name);
	TS_S32 s32Ret;
	for(i=0;i<TS_MAX_OVERLAY_NUM;i++){
		if(strlen(overlay_yuv_param_set[i]) == 0){
			printf("to set===%d\n",i);
			strcpy(overlay_yuv_param_set[i],name);
			snprintf(overlay_yuv_param[i].show_content_buffer, sizeof(overlay_yuv_param[i].show_content_buffer), "%s", overlay_yuv_param_set[i]);
			s32Ret = SAMPLE_COMM_REGION_GetOverlayYuvData(&overlay_yuv_param[i]);
			printf("to set===ret=%d\n",s32Ret);
                	if (0 != s32Ret) {
                        	ALG_LOGE("SAMPLE_COMM_REGION_GetOverlayYuvData error\n");
				overlay_yuv_param[i].show_content_buffer[0] = 0;
                        	return NULL;
                	}
			return &overlay_yuv_param[i];

		}
		if(strcmp(overlay_yuv_param[i].show_content_buffer,name) == 0){
			return &overlay_yuv_param[i];

		}
	}
	return NULL;

}
#endif
TS_S32 TS_ALG_CatDetect_Init(TS_VOID **pHandle, ALG_CAT_MODEL_INIT_S *param)
{
	TS_S32 s32Ret = TS_SUCCESS;

	ALG_MODEL_INIT_S alg_param;
	//ts_alg_log_set_level(ALG_LOG_WARN);
	ALG_CAT_MODEL_INIT_S *mode_param = param;
    SAMPLE_ALG_INSTANCE_S* pInst = *((SAMPLE_ALG_INSTANCE_S**)pHandle);

    FILE *pf_cfg = NULL;
	FILE *pf_weight = NULL;
	if(!model_info)
	{
		model_info = malloc(sizeof(ALG_CAT_MODEL_INIT_S));
		model_info->embedding_model_cfg = malloc(128);
		model_info->embedding_model_weight = malloc(128);
		model_info->food_model_cfg = malloc(128);
		model_info->food_model_weight = malloc(128);
		model_info->yolo_model_cfg = malloc(128);
		model_info->yolo_model_weight = malloc(128);
		model_info->model_version_file = malloc(128);
	}
	memset(model_info->embedding_model_cfg,0,128);
	memset(model_info->embedding_model_weight,0,128);
	memset(model_info->food_model_cfg,0,128);
	memset(model_info->food_model_weight,0,128);
	memset(model_info->yolo_model_cfg,0,128);
	memset(model_info->yolo_model_weight,0,128);
	memset(model_info->model_version_file,0,128);
	if(mode_param->food_model_cfg != NULL && mode_param->food_model_weight != NULL){
		memcpy(model_info->food_model_cfg ,mode_param->food_model_cfg,strlen((char *)mode_param->food_model_cfg));
		memcpy(model_info->food_model_weight ,mode_param->food_model_weight,strlen((char *)mode_param->food_model_weight));
		is_food_on = 1;
	}	
	#if 1
	memcpy(model_info->embedding_model_cfg ,mode_param->embedding_model_cfg,strlen((char *)mode_param->embedding_model_cfg));
	memcpy(model_info->embedding_model_weight ,mode_param->embedding_model_weight,strlen((char *)mode_param->embedding_model_weight));

	memcpy(model_info->yolo_model_cfg ,mode_param->yolo_model_cfg,strlen((char *)mode_param->yolo_model_cfg));
	memcpy(model_info->yolo_model_weight ,mode_param->yolo_model_weight,strlen((char *)mode_param->yolo_model_weight));
	memcpy(model_info->model_version_file ,mode_param->model_version_file,strlen((char *)mode_param->model_version_file));
	#else
	memcpy(model_info->embedding_model_cfg ,RSN_LOCAL_CFG_TT_1,strlen(RSN_LOCAL_CFG_TT_1));
	memcpy(model_info->embedding_model_weight ,RSN_LOCAL_TT_1,strlen(RSN_LOCAL_TT_1));

	memcpy(model_info->yolo_model_cfg ,LOCAL_MODERL_TT_1,strlen(LOCAL_MODERL_TT_1));
	memcpy(model_info->yolo_model_weight ,LOCAL_MODERL_WEIGHT_TT_1,strlen(LOCAL_MODERL_WEIGHT_TT_1));
	memcpy(model_info->model_version_file ,MODEL_VERSION_TT_1,strlen(MODEL_VERSION_TT_1));
	#endif
	model_info->cat_callback = mode_param->cat_callback;
	model_info->userdata = mode_param->userdata;



	TS_U8 *pu8_model_cfg = model_info->yolo_model_cfg;
    TS_U8 *pu8_model_weight = model_info->yolo_model_weight;
    //TS_U8 pu8_model_cfg[] = model_info->embedding_model_cfg;
    //TS_U8 pu8_model_weight[] = model_info->embedding_model_weight;
    //TS_U8 pu8_model_cfg[] = "./firesmokedetect_cfg.cfg";
    //TS_U8 pu8_model_weight[] = "./firesmokedetect_weight.weight";
	SAMPLE_PRT("pu8_model_cfg %s !!!\n",pu8_model_cfg);
	SAMPLE_PRT("pu8_model_weight %s !!!\n",pu8_model_weight);
	//SAMPLE_PRT("FireSmokeDetect model file fopen failed !!!\n");
	//SAMPLE_PRT("FireSmokeDetect model file fopen failed !!!\n");
    pf_cfg = fopen((const char*)pu8_model_cfg, "rb");
	pf_weight = fopen((const char*)pu8_model_weight, "rb");
	if((NULL == pf_cfg) || (NULL == pf_weight)){
		SAMPLE_PRT("FireSmokeDetect model file fopen failed !!!\n");
        SAMPLE_PRT("model_cfg %s, model_weight:%s\n", pu8_model_cfg,  pu8_model_weight);
		return TS_FAILURE;
	}

	TS_U32 u32WeightLen = getFileSize(pf_weight);
    TS_U8 *pWeightBuf = (TS_U8 *)malloc(u32WeightLen);
	fread(pWeightBuf, u32WeightLen, 1, pf_weight);
	fclose(pf_weight);

	TS_U32 u32CfgLen = getFileSize(pf_cfg);
	TS_U8 *pCfgBuf = (TS_U8 *)malloc(u32CfgLen);
	fread(pCfgBuf, u32CfgLen, 1, pf_cfg);
	fclose(pf_cfg);

    memset(&alg_param, 0, sizeof(ALG_MODEL_INIT_S));
    alg_param.pGraph = pCfgBuf;
	alg_param.pWeight = pWeightBuf;
	alg_param.u32GraphSize = u32CfgLen;
	alg_param.u32WeightSize = u32WeightLen;
	alg_param.eImageType = ALG_IMAGE_TYPE_INT_HWC_RGB0;//ALG_IMAGE_TYPE_FORMAT_0RGB;
	alg_param.bRneOff = 0;

	//libs add
	//all_det_param.DetectionConfThres = 0.4;//0.7
	//all_det_param.SimilarityThres_Night = 0.4;//0.45
	//all_det_param.SimilarityThres = 0.4;//0.45
	//
	printf("TS_ALG_BodyDetect_Init begin\n");
	s32Ret = TS_ALG_BodyDetect_Init(&(pInst->pHandle), &alg_param);
	if (0 != s32Ret) {
		ALG_LOGE("TS_ALG_BodyDetect_Init error\n");
		goto init_exit;
	}

	SAMPLE_PRT("TS_ALG_BodyDetect_GetVersion:%s\n", TS_ALG_BodyDetect_GetVersion());
    s32Ret = TS_SUCCESS;
    ALG_RSN_Detect_Init();
	if(is_food_on){
		ALG_RSN_Detect_FOOD_Init();
	}
	SAMPLE_PRT("ALG_RSN_Detect_Init sucess!\n");
init_exit:
	free(pCfgBuf);
	free(pWeightBuf);
    return s32Ret;
}

int Alarm_time_static11()
{
	struct tm *tm_ptr;
	struct timeval tv;
	char str[25] = { 0 };
	;
	char str_dada[100] = { 0 };
	gettimeofday(&tv, NULL);

	time_t now = time(NULL);
	tm_ptr = localtime(&now);
	//printf("Year: %d\n", tm_ptr->tm_year + 1900);
	// printf("Month: %d\n", tm_ptr->tm_mon + 1);
	// printf("Day: %d\n", tm_ptr->tm_mday);
	// printf("Hour: %d\n", tm_ptr->tm_hour);
	// printf("Minute: %d\n", tm_ptr->tm_min);
	// printf("Second: %d\n", tm_ptr->tm_sec);
	// printf("Millisecond: %d\n", tv.tv_usec / 1000);
	// strcat(timedata,itoa(tm_ptr->tm_year + 1900));
	// strcat(timedata,itoa(tm_ptr->tm_mon + 1));
	// strcat(timedata,itoa(tm_ptr->tm_mday));
	// strcat(timedata,itoa(tm_ptr->tm_hour));
	// strcat(timedata,itoa(tm_ptr->tm_min));
	// strcat(timedata,itoa(tm_ptr->tm_sec));
	// strcat(timedata,itoa(tv.tv_usec / 1000));

	sprintf(str, "%d", tm_ptr->tm_year + 1900);

	strcat(str_dada, str);
	strcat(str_dada, "年");
	sprintf(str, "%d", tm_ptr->tm_mon + 1);

	strcat(str_dada, str);
	strcat(str_dada, "月");

	sprintf(str, "%d", tm_ptr->tm_mday);

	strcat(str_dada, str);
	strcat(str_dada, "日");
	sprintf(str, "%d", tm_ptr->tm_hour);
	strcat(str_dada, str);
	strcat(str_dada, "时");

	sprintf(str, "%d", tm_ptr->tm_min);

	strcat(str_dada, str);
	strcat(str_dada, "分");

	sprintf(str, "%d", tm_ptr->tm_sec);

	strcat(str_dada, str);
	strcat(str_dada, "秒");

	sprintf(str, "%d", tv.tv_usec / 1000);

	strcat(str_dada, str);
	strcat(str_dada, "毫秒");

	//memcpy(timedata,str_dada,sizeof(str_dada));
	//strcpy(timedata,str_dada);
	printf("===VIDEO_ALG_CatDetect_Proc str_dada====:%s\n", str_dada);
	//timedata = str_dada;
	return 0;
}

unsigned long long getSystemTime1()
{
	//struct timeval tv1;
	//gettimeofday(&tv1, NULL);
	//return (unsigned long long)(tv1.tv_sec * 1000LL) + (tv1.tv_usec / 1000);
	unsigned long long int pts = 0;
	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	pts = (unsigned long long int)tv1.tv_sec * 1000 + (tv1.tv_usec / 1000);
	return (long)pts;
}



TS_S32 VIDEO_ALG_CatDetect_Proc(TS_VOID *pHandle, ALG_IMAGE_S *pImage, ALG_IMAGE_S *pImageDet, ALG_CatDetect_DET_RESULT_S *pResult)
{
	//printf("######VIDEO_ALG_CatDetect_Proc start#######\n");
	TS_S32 s32Ret = TS_SUCCESS;
	long t0 ,t1,t2,t3,t4,t5,t6;
	//t5 = getSystemTime1();
    SAMPLE_ALG_INSTANCE_S* pInst = (SAMPLE_ALG_INSTANCE_S*)pHandle;
	if (!pHandle|| !pImageDet || !pResult) {
		ALG_LOGE("VIDEO_ALG_CatDetect_Proc param is null\n");
		return TS_FAILURE;
	}

	
	//后续修改输入 从这里修改输入
	#if TEST_MODE2
		//int file_num = atoi(out_file_num);
		static int file_count = 0;
		static int set_name_flag = 1;
		long ret = 0;
		long ret_yuv = 0;
		if(set_name_flag)
		{
			sprintf(mode2_name,"%s",MODE2_NAME_PRE);
			//sprintf(mode2_name_yuv,"%s",MODE2_NAME_PRE_YUV);
			set_name_flag = 0;
		}
		FILE* pOut;
		FILE* pOut_yuv;
        pOut = fopen(mode2_name, "r");
		//pOut_yuv = fopen(mode2_name_yuv, "r");
		// 改变文件指针位置
	    if (fseek(pOut, file_count*pImageDet->s32W*4*pImageDet->s32H, SEEK_SET) != 0) {
	        perror("Failed to seek in file");
			fseek(pOut, 0, SEEK_SET);
	    }
		//if (fseek(pOut_yuv, file_count*pImage->s32W*1.5*pImage->s32H, SEEK_SET) != 0) {
	        //perror("Failed to seek in file");
			//fseek(pOut_yuv, 0, SEEK_SET);
			//file_count = 0;
	       	//fclose(pOut);
			//fclose(pOut_yuv);
	        //return TS_FAILURE;
	    //}
		file_count++;
        //fwrite(pdstData, dst_w*4, dst_h, pOut);//buf max imgae = W*H*4
		ret = fread(pImageDet->pData,pImageDet->s32W*4, pImageDet->s32H, pOut);
		if(ret < pImageDet->s32H)//pImageDet->s32W*4*
		{
			printf("I4S_TEST_MODE2 %s ret %ld over!!\n",mode2_name,ret);

		}
		// ret_yuv = fread(pImage->pData,pImage->s32W*1.5, pImage->s32H, pOut_yuv);
		// if(ret < pImageDet->s32H  || ret_yuv < pImage->s32H)
		// {
		// 	fclose(pOut);
		// 	fclose(pOut_yuv);
		// 	file_count = 0;
		// 	//return TS_FAILURE;
		// }else{
        	fclose(pOut);
		// 	fclose(pOut_yuv);
		// }
		printf("I4S_TEST_MODE2 %s imgDet w:%d h:%d object\n",
            mode2_name, pImageDet->s32W, pImageDet->s32H);

	#endif
	#if TEST_MODE4
		
		char *dir_path = "/mnt/sda0/weipai_1";
            char filepath[200] = {0};
            snprintf(filepath, sizeof(filepath) - 1, "%s/%d.rgba", dir_path, Frame_count);
            printf("***filepath***:%s\n", filepath);
            // if ((pFile = fopen(filepath, "rb")) == NULL) {
            FILE *fp1 = NULL;
            fp1 = fopen(filepath, "w+");
            fwrite(pImageDet->pData, 1, pImageDet->s32W * (pImageDet->s32H) * 4, fp1);
            fclose(fp1);
            Frame_count++;
            snprintf(filepath, sizeof(filepath) - 1, "%s/%d.img", dir_path, Frame_count);
            printf("***filepath***:%s\n", filepath);
            // if ((pFile = fopen(filepath, "rb")) == NULL) {
            FILE *fp11 = NULL;
            fp11 = fopen(filepath, "w+");
            fwrite(pImage->pData, 1, pImage->s32W * (pImage->s32H) * 3, fp11);
            fclose(fp11);
#endif
	//

	//ALG_CatDetect_DET_RESULT_S
	
	ALG_CatDetect_DET_RESULT_S *pTmpResult = (ALG_CatDetect_DET_RESULT_S *)pResult;
	//SAMPLE_ALG_RESULT_S *pCurResult = (SAMPLE_ALG_RESULT_S *)pResult;
    	//ALG_CatDetect_DET_RESULT_S *pTmpResult = &(pCurResult->gstAlgCatdetResult);
	
	s32Ret = TS_ALG_BodyDetect_Process(pInst->pHandle, pImageDet, pTmpResult);
	if (0 != s32Ret) {
		ALG_LOGE("TS_ALG_BodyDetect_Process error\n");
	}
	//pImageDet 已经是图片了，从这里保存
#if TEST_MODE1
		static int set_name_flag_mode1 = 1;
		if(set_name_flag_mode1)
		{
			// 使用当前时间作为随机数生成器的种子
		    srand(time(0));

		    // 生成一个0到99之间的随机数
		    int random_number = rand() % 100;
			sprintf(mode1_name,MODE1_NAME_PRE, random_number);
			sprintf(mode1_name_yuv,MODE1_NAME_PRE_YUV, random_number);
			set_name_flag_mode1 = 0;
		}
		FILE* pOut_mode1;
		FILE* pOut_mode1_yuv;
        pOut_mode1 = fopen(mode1_name, "ab+");
		pOut_mode1_yuv = fopen(mode1_name_yuv, "ab+");
        fwrite(pImageDet->pData,pImageDet->s32W*4, pImageDet->s32H, pOut);//buf max imgae = W*H*4
		fwrite(pImage->pData, pImage->s32W*1.5,pImage->s32H, pOut_mode1_yuv);
        fclose(pOut_mode1);
		fclose(pOut_mode1_yuv);
		printf("I4S_TEST_MODE1 %s imgDet w:%d h:%d object\n",
            mode1_name, pImageDet->s32W, pImageDet->s32H);

#endif
	//

	extern float get_simi();
    RECT rect;
    	char ids[128];
	if(is_food_on){
		my_crop_resize_detect_food(pImageDet, &rect,ids);
	}
    	//int tmpid;
	for (int i = 0; i < pTmpResult->u32ObjNum; i++)
	{
		if(pTmpResult->stBox[i].f32Xmin > 1.0){
			pTmpResult->stBox[i].f32Xmin = 1.0;
		}else if(pTmpResult->stBox[i].f32Xmin < 0){
			pTmpResult->stBox[i].f32Xmin = 0;
		}
		if(pTmpResult->stBox[i].f32Ymin > 1.0){
			pTmpResult->stBox[i].f32Ymin = 1.0;
		}else if(pTmpResult->stBox[i].f32Ymin < 0){
			pTmpResult->stBox[i].f32Ymin = 0;
		}
		if(pTmpResult->stBox[i].f32Xmax > 1.0){
			pTmpResult->stBox[i].f32Xmax = 1.0;
		}else if(pTmpResult->stBox[i].f32Xmax < 0){
			pTmpResult->stBox[i].f32Xmax = 0;
		}
		if(pTmpResult->stBox[i].f32Ymax > 1.0){
			pTmpResult->stBox[i].f32Ymax = 1.0;
		}else if(pTmpResult->stBox[i].f32Ymax < 0){
			pTmpResult->stBox[i].f32Ymax = 0;
		}

        rect.left = pTmpResult->stBox[i].f32Xmin * pImage->s32W;
        rect.top = pTmpResult->stBox[i].f32Ymin * pImage->s32H;
        rect.right = pTmpResult->stBox[i].f32Xmax * pImage->s32W;
        rect.bottom = pTmpResult->stBox[i].f32Ymax * pImage->s32H;
// 		if(pTmpResult->stBox[i].class_id != ALG_CAT_CLASS_ID_FOOD){
// 			strcpy(pTmpResult->stBox[i].nameid,"0");
// 			char ids[64];
// 			//printf("resize2 srcImg->s32W=%d,srcImg->s32H=%d\n",pImage->s32W,pImage->s32H);
// 			printf("=======取消识别 my_crop_resize_detect========\n");
// 			//my_crop_resize_detect(pImage, &rect,ids);//识别  取消识别测试检测时间hxl
// 			strncpy(pTmpResult->stBox[i].nameid,ids,sizeof(pTmpResult->stBox[i].nameid));
// #if 0			
// 			tmpid = my_crop_resize_detect(pImage, &rect,pTmpResult->stBox[i].id);
// 			if(tmpid == 17){
// 				strcpy(pTmpResult->stBox[i].id,"");
// 			}else{
// 				sprintf(pTmpResult->stBox[i].id,"%d",tmpid);
// 			}
// #endif
// 			//pTmpResult->stBox[i].id = my_crop_resize_detect(pImage, &rect);//+1
// 			//pTmpResult->stBox[i].DetectionConf = test_conf_get();
	
// 			pTmpResult->stBox[i].MaxSimilarity = get_simi();
// 		}else{
// 			 //strcpy(pTmpResult->stBox[i].id,"no id");
// 			 //pTmpResult->stBox[i].MaxSimilarity = 0;
// 			 //pTmpResult->stBox[i].act = 1;
// 		}
	}
	//int cat_num;
	//cat_num = pTmpResult->u32ObjNum;	//检测到食物取消食物检测
	// for(int i=0;i<cat_num;i++)
	// {
	// 	if(pTmpResult->stBox[i].class_id == ALG_CAT_CLASS_ID_FOOD)
	// 	{
	// 		printf("delete food\n");
	// 		if(i < cat_num - 1){
	// 			memcpy(&pTmpResult->stBox[i],&pTmpResult->stBox[i+1],(cat_num - 1 -i)*sizeof(ALG_CatDetect_DET_BOX_S));			
	// 		}
	// 		i--;
	// 		cat_num -= 1;	
	// 	}		
	// }
	 
	//pTmpResult->u32ObjNum = cat_num;
	//t3 = getSystemTime1();
	//printf("=======================TS_ALG_BodyDetect_Process t3===============================%llu\n",t3);
	//printf("=======================TS_ALG_BodyDetect_Process t3-t0===============================%d\n",t3-t0);
	//printf("=======================TS_ALG_BodyDetect_Process t3-t1===============================%d\n",t3-t1);
	
	set_result(pTmpResult);
	//  t4 = getSystemTime1();

	//  printf("=======================TS_ALG_BodyDetect_Process t4===============================%llu\n",t4);
	// // printf("=======================TS_ALG_BodyDetect_Process t4-t3===============================%d\n",t4-t3);
	//  printf("=======================TS_ALG_BodyDetect_Process t4-t5===============================%d\n",t4-t5);
	//  printf("=======================TS_ALG_BodyDetect_Process 4-t0===============================%d\n",t4-t0);

    return s32Ret;
}

void overlay_bitmap_on_nv12(Uint8 *nv12_y, int width, int height, TS_BITMAP_CHAT_S *bitmapArray, int arraySize, int posX, int posY)
{
	if (NULL == bitmapArray){
		printf("bitmapArray is null!\n");
		return;
	}
	if (0== arraySize){
		printf("arraySize is 0!\n");
		return;
	}
    for (int i = 0; i < arraySize; i++) {
        int x = posX + bitmapArray[i].x;
        int y = posY + bitmapArray[i].y;
        if (x >= 0 && x < width && y >= 0 && y < height) {
            Uint8 *pixelY = nv12_y + y * width + x;
            Uint8 alpha = bitmapArray[i].alpha;
            *pixelY = (*pixelY * (255 - alpha) + 255 * alpha) / 255; // 混合Y平面亮度
        }
    }
}

void overlay_with_shadow_bitmap_on_nv12(Uint8 *nv12_y, int width, int height, TS_BITMAP_CHAT_S *bitmapArray, int arraySize, int posX, int posY)
{
    int shadow_offset = 2;			// offset
    Uint8 shadow_intensity = 50;	// 阴影亮度较低

    // shadow
    for (int i = 0; i < arraySize; i++) {
        int x = posX + bitmapArray[i].x + shadow_offset;
        int y = posY + bitmapArray[i].y + shadow_offset;
        if (x >= 0 && x < width && y >= 0 && y < height) {
            Uint8 *pixelY = nv12_y + y * width + x;
            Uint8 alpha = bitmapArray[i].alpha;
            *pixelY = (*pixelY * (255 - alpha) + shadow_intensity * alpha) / 255;
        }
    }

    // text
    for (int i = 0; i < arraySize; i++) {
        int x = posX + bitmapArray[i].x;
        int y = posY + bitmapArray[i].y;
        if (x >= 0 && x < width && y >= 0 && y < height) {
            Uint8 *pixelY = nv12_y + y * width + x;
            Uint8 alpha = bitmapArray[i].alpha;
            *pixelY = (*pixelY * (255 - alpha) + 255 * alpha) / 255;
        }
    }
}
#if 0
TS_VOID  VIDEO_ALG_CatDetect_ResultProc(TS_U8 *pYuvBuf,  TS_U32 width, TS_U32 height, TS_U32 u32ImageRatio, TS_VOID *pAlgResult)
{
    RECT rect;
    //TS_U32 i;
    //float fThres = 0.0001f;
    //enYuvColorIdx enColor = YUV_RED;

    SAMPLE_ALG_RESULT_S *pCurResult = (SAMPLE_ALG_RESULT_S *)pAlgResult;
	ALG_BODYDET_RESULT_S *pResult = &(pCurResult->gstAlgCatdetResult);
	//测试时需要退出
	#if TEST_MODE3
		return;
	#endif
	//
    //printf("-algo body target num: %d w:%d h:%d\n",
    //        pResult->u32BodyNum, width, height);
#if 1
    for(TS_U32 j = 0; j < pResult->u32BodyNum; j++) {
        //printf("body:%d --score:%f xmin:%f ymin:%f xmax:%f ymax:%f\n", j, pResult->stBox[j].f32Score,
        //        pResult->stBox[j].f32Xmin * image.s32W, pResult->stBox[j].f32Ymin * image.s32H,
        //        pResult->stBox[j].f32Xmax * image.s32W, pResult->stBox[j].f32Ymax * image.s32H);
    }
	for (int i = 0; i < pResult->u32BodyNum; i++)
	{
        rect.left = u32ImageRatio * pResult->stBox[i].f32Xmin  * width;
        rect.top = u32ImageRatio * pResult->stBox[i].f32Ymin *  height;
        rect.right = u32ImageRatio * pResult->stBox[i].f32Xmax * width;
        rect.bottom = u32ImageRatio *pResult->stBox[i].f32Ymax * height;
		//lx test
		int test_conf = test_conf_get();
		YUY_Draw_num(pYuvBuf, width, height, 12,(rect.left+rect.right)/2, rect.top+i+20, test_conf/10, YUV_BLUE);
		YUY_Draw_num(pYuvBuf, width, height, 12,(rect.left+rect.right)/2+12, rect.top+i+20+12, test_conf%10, YUV_BLUE);
		//
		YUV_Draw_Rect(pYuvBuf, width, height, 12, &rect, YUV_BLUE, 5);
		TS_OVERLAY_YUV_PARAM_S * overlay_yuv_param_p;
		char name[128];
#if 1
		extern int get_catName_name(int id, char*name);
		if((pResult->stBox[i].id == 0) || get_catName_name(pResult->stBox[i].id-1, name) < 0){
			overlay_yuv_param_p =overlay_yuv_param_get(" ");//no id
		}else{
			printf("uname=%s\n",name);
			overlay_yuv_param_p =overlay_yuv_param_get(name);
		}
#endif
		#if 1
		//overlay_yuv_param_p =overlay_yuv_param_get(name);
		int newTmpLocaX;
		int newTmpLocaY;
		newTmpLocaX = abs((rect.left + (rect.right - rect.left)/2)-(overlay_yuv_param_p->sDis_w/2));
		newTmpLocaY = (rect.top - (overlay_yuv_param_p->sDis_h))<0?0:(rect.top - (overlay_yuv_param_p->sDis_h));
		overlay_yuv_param_p->sDis_x = newTmpLocaX;
		overlay_yuv_param_p->sDis_y = newTmpLocaY;
		#if 0
		overlay_bitmap_on_nv12(pYuvBuf, width, height, overlay_yuv_param[i].pBitmapByte, overlay_yuv_param[i].sArraySize, overlay_yuv_param[i].sDis_x, overlay_yuv_param[i].sDis_y);
		#else
		overlay_with_shadow_bitmap_on_nv12(pYuvBuf, width, height, overlay_yuv_param_p->pBitmapByte, overlay_yuv_param_p->sArraySize, overlay_yuv_param_p->sDis_x, overlay_yuv_param_p->sDis_y);
		#endif
		#endif
#ifdef OVERLAY_RGN
		//font osd
		int newTmpLocaX = abs((rect.left + (rect.right - rect.left)/2)-(overlay_rgn_param[i].sDis_w/2));
		int newTmpLocaY = (rect.top - (overlay_rgn_param[i].sDis_h))<0?0:(rect.top - (overlay_rgn_param[i].sDis_h));
		int LocaChX = abs(overlay_rgn_param[i].sDis_x - newTmpLocaX);//居中显示
		int LocaChY = abs(overlay_rgn_param[i].sDis_y - newTmpLocaY);//画框之上
		if (LocaChX>5 || LocaChY>5){
			overlay_rgn_param[i].sDis_x = newTmpLocaX;
			overlay_rgn_param[i].sDis_y = newTmpLocaY;
			overlay_rgn_param[i].rgnUpdate = 1;
		}
		afafafafa
#endif
	}

#ifdef OVERLAY_RGN
	if (pResult->u32BodyNum==0){	//侦测不到，撤回所有显示
		for (int i = 0; i < TS_RNG_HANDLE_BUTT; i++){
			if(overlay_rgn_param[i].AttachState){
				SAMPLE_COMM_REGION_Detach(&overlay_rgn_param[i]);
				overlay_rgn_param[i].AttachState = 0;
				overlay_rgn_param[i].sDis_x = 0;
				overlay_rgn_param[i].sDis_y = 0;
			}
		}
	}
	else{
		for (int i = 0; i < pResult->u32BodyNum; i++){
			if(overlay_rgn_param[i].AttachState && overlay_rgn_param[i].rgnUpdate){
				SAMPLE_COMM_REGION_Detach(&overlay_rgn_param[i]);
				overlay_rgn_param[i].AttachState = 0;
			}
			if(!overlay_rgn_param[i].AttachState&& overlay_rgn_param[i].rgnUpdate){
				SAMPLE_COMM_REGION_Attach(&overlay_rgn_param[i]);
				overlay_rgn_param[i].AttachState = 1;
				overlay_rgn_param[i].rgnUpdate = 0;
			}
		}
		for (int i = pResult->u32BodyNum; i < TS_RNG_HANDLE_BUTT; i++){
			if(overlay_rgn_param[i].AttachState){
				SAMPLE_COMM_REGION_Detach(&overlay_rgn_param[i]);
				overlay_rgn_param[i].AttachState = 0;
			}
		}
	}
#endif
#endif
}
#endif
TS_S32 VIDEO_ALG_CatDetect_Exit(TS_VOID *pHandle)
{
	TS_S32 s32Ret = TS_SUCCESS;
	if (!pHandle) {
		SAMPLE_PRT("VIDEO_ALG_CatDetect_Exit param is null\n");
		s32Ret = TS_FAILURE;
		return s32Ret;
	}

	SAMPLE_ALG_INSTANCE_S* pInst = (SAMPLE_ALG_INSTANCE_S*)pHandle;
	s32Ret = TS_ALG_BodyDetect_Exit(pInst->pHandle);
	
	return s32Ret;
}


TS_S32 TS_ALG_CatDetect_SetParam(ALG_CatDetect_DET_PARAM_S *pParam)
{
	TS_S32 s32Ret = TS_SUCCESS;
	if(!pParam)
	{
		SAMPLE_PRT("ALG_CatDetect_DET_PARAM_S param is null\n");
		return TS_FAILURE;
	}
	ALG_CatDetect_DET_PARAM_S *det_param = pParam;
	if(det_param->DetectionConfThres<0||det_param->DetectionConfThres>1)
	{
		SAMPLE_PRT("DetectionConfThres is overload Please set 0.0 - 1.0\n");
		return TS_FAILURE;
	}
	if(det_param->SimilarityThres_Day<0||det_param->SimilarityThres_Day>1)
	{
		SAMPLE_PRT("SimilarityThres_Day is overload Please set 0.0 - 1.0\n");
		return TS_FAILURE;
	}
	if(det_param->SimilarityThres_Night<0||det_param->SimilarityThres_Night>1)
	{
		SAMPLE_PRT("SimilarityThres_Night is overload Please set 0.0 - 1.0\n");
		return TS_FAILURE;
	}
	all_det_param.DetectionConfThres = det_param->DetectionConfThres;
	all_det_param.SimilarityThres_Day = det_param->SimilarityThres_Day;
	all_det_param.SimilarityThres_Night = det_param->SimilarityThres_Night;
	set_eta_thres(det_param->EAT_Thres);
	set_out_times(det_param->OUT_times);
	set_eat_out_times(det_param->EAT_OUT_times);
//	set_eat_shake_out_times(det_param->EAT_OUT_SHAKE_times);

	return s32Ret;
}

TS_S32 TS_ALG_CatDetect_GetParam(ALG_CatDetect_DET_PARAM_S *pParam)
{
	TS_S32 s32Ret = TS_SUCCESS;
	if(!pParam)
	{
		SAMPLE_PRT("TS_ALG_CatDetect_GetParam param is null\n");
		return TS_FAILURE;
	}
	ALG_CatDetect_DET_PARAM_S *det_param = pParam;
	det_param->DetectionConfThres = all_det_param.DetectionConfThres;
	det_param->SimilarityThres_Day = all_det_param.SimilarityThres_Day;
	det_param->SimilarityThres_Night = all_det_param.SimilarityThres_Night;
	
	return s32Ret;
}

TS_CHAR* CatLibVerGet(void)
{
	//SAMPLE_PRT("VER %s %s %s\n",__DATE__,__TIME__,CATVERSION);
	sprintf(cat_version_buf,"%s%s%s",__DATE__,__TIME__,CATVERSION);
	return cat_version_buf;
}

TS_CHAR* CatYoloModelVerGet(void)
{
	sprintf(yolo_version_buf,"%s%s%s",__DATE__,__TIME__,CATVERSION);
	return yolo_version_buf;
}

TS_CHAR* CatEmbeddingModelVerGet(void)
{
	sprintf(emb_version_buf,"%s%s%s",__DATE__,__TIME__,CATVERSION);
	return emb_version_buf;
}

//json test


#if 0
static char *json_key_buf[] = {"pets","petId","images"};

// 读取文件内容到字符串
static char* read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("无法打开文件");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char*)malloc(length + 1);
    if (!content) {
        perror("malloc error");
        fclose(file);
        return NULL;
    }

    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);
    return content;
}

int catinfo_cjson_anylx(char * file_content)
{
	int cat_id = 0;
	// 解析JSON字符串
	cJSON *json = cJSON_Parse(file_content);
	if (!json) {
		printf("json anxlx error: %s\n", cJSON_GetErrorPtr());
		free(file_content);
		return 1;
	}

	// 示例：打印JSON对象的键值对
	cJSON *item = NULL;
	item = cJSON_GetObjectItem(json,json_key_buf[0]);
	if (!item) {
		printf("item anxlx error: %s\n", cJSON_GetErrorPtr());
		free(file_content);
		cJSON_Delete(json);
		return 1;
	}
	int num = cJSON_GetArraySize(item);
	cJSON *array_item = NULL;
	cJSON *id_item = NULL;
	cJSON *path_item = NULL;
	cJSON *test_item = NULL;
	int pic_num = 0;
	struct catPicInfo pic_data[25];
	memset(pic_data,0,sizeof(pic_data));
	for(int i=0 ;i < num ;i++)
	{
		array_item = cJSON_GetArrayItem(item,i);
		id_item = cJSON_GetObjectItem(array_item,json_key_buf[1]);
		cat_id = id_item->valueint;
		//printf("key: %s, value: %d\n", id_item->string, cat_id);
		path_item = cJSON_GetObjectItem(array_item,json_key_buf[2]);
		cJSON_ArrayForEach(test_item, path_item){
			//printf("key: %s, value[%d]: %s\n", test_item->string, strlen(cJSON_GetStringValue(test_item)),cJSON_GetStringValue(test_item));
			pic_data[pic_num].id = cat_id;
			pic_data[pic_num].size = strlen(cJSON_GetStringValue(test_item));
			memcpy(pic_data[pic_num].data,cJSON_GetStringValue(test_item),pic_data[pic_num].size);
			pic_data[pic_num].data[pic_data[pic_num].size+1] = '\0';
			pic_num++;
		}
	}
	change_catName_info2(pic_data,pic_num,NULL,0);
	// 清理
	cJSON_Delete(json);
	free(file_content);
	return 0;
}
#endif
//
char* extract_filepath(const char* full_path) {
    char* last_slash = strrchr(full_path, '/');     // 查找最后一个'/'位置‌:ml-citation{ref="4" data="citationList"}
    if (last_slash == NULL) return strdup("");      // 无路径时返回空字符串‌:ml-citation{ref="1" data="citationList"}
    
    size_t path_len = last_slash - full_path + 1;   // 计算路径长度（包含末尾斜杠）
    char* path = malloc(path_len + 1);              // 分配动态内存‌:ml-citation{ref="3" data="citationList"}
    if (!path) return NULL;
    
    strncpy(path, full_path, path_len);             // 复制路径部分‌:ml-citation{ref="4" data="citationList"}
    path[path_len] = '\0';                          // 添加终止符
    return path;
}
#if 0
TS_S32 CatConfigRenew(TS_CHAR*path)
{
    const char *filename = path;
    char *file_content = read_file(filename);
    if (!file_content) {
        return 1;
    }
	int ret = catinfo_cjson_anylx(file_content);
	char *pic_path_head = extract_filepath(filename);
   set_picpath_head(pic_path_head);
    return ret;

}
#endif
TS_S32 CatConfigRenew(TS_CHAR*url,TS_CHAR*sn)
{
	pic_renew(url,sn);
    return 0;

}
TS_S32 CatSetPicDir(TS_CHAR*path)
{
	printf("CatSetPicDir\n");
	set_pic_path(path);
	return 0;
}

TS_S32 TS_NV12_Vertical_Concat_Correct(const TS_U8 *Src_NV12_Top,
                                 const TS_U8 *Src_NV12_Bottom,
                                 TS_U8 *Dst_NV12,
                                 TS_S32 Width, TS_S32 Height,
                                 TS_S32 StrideY, TS_S32 StrideUV)
								 {
									return nv12_vertical_concat_correct(Src_NV12_Top,
                                 Src_NV12_Bottom,
                                 Dst_NV12,
                                 Width, Height,
                                 StrideY, StrideUV);
								 }

TS_S32 TS_NV12_Scale_Ex(TS_U8* Src, TS_S32 Src_Width, TS_S32 Src_Height,
                  TS_U8* Dst, TS_S32 Dst_Width, TS_S32 Dst_Height,
                  TS_S32 Keep_Aspect)
				  {
					return nv12_scale_ex(Src, Src_Width, Src_Height,
                  Dst, Dst_Width, Dst_Height,
                  Keep_Aspect);
				  }

TS_S32 TS_NV12_Scale_Fit_Fast(TS_U8 *Src_NV12, TS_S32 Src_Width, TS_S32 Src_Height,TS_S32 Src_Stride,
                   TS_U8* Dst_NV12, TS_S32 Dst_Width, TS_S32 Dst_Height)
				   {
					return nv12_scale_fit_fast(Src_NV12, Src_Width, Src_Height, Src_Stride,
                  Dst_NV12, Dst_Width, Dst_Height);
				   }
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
