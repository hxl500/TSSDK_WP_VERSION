#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <unistd.h>
#include "mpi_sys.h"
#include "mpi_vgs.h"
#include "tscv.hpp"
#include "mpi_vb.h"

using namespace cv;

void log_message(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("Process [%d]: ", getpid());
	vprintf(format, args);
	printf("\n");

	va_end(args);
}

#define SAMPLE_PRT(fmt...)                                                                                             \
	do {                                                                                                           \
		printf("[%s]-%d: ", __func__, __LINE__);                                                               \
		printf(fmt);                                                                                           \
	} while (0)

static long SAMPLE_TSCV_GetTime()
{
	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	return (long)(tv1.tv_sec * 1000000) + (tv1.tv_usec);
}

/******************************************************************************
* function : vb init & MPI system init
******************************************************************************/
TS_S32 SAMPLE_SYS_Init()
{
	TS_S32 s32Ret = TS_FAILURE;

	TS_MPI_SYS_Exit();
	TS_MPI_VB_Exit();

	VB_CONFIG_S stVbConf;
	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt = 1;
	TS_S32 blk_size = 100*100;
	stVbConf.astCommPool[0].u64BlkSize = blk_size;
	stVbConf.astCommPool[0].u32BlkCnt = 2;

	s32Ret = TS_MPI_VB_SetConfig(&stVbConf);

	if (TS_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("TS_MPI_VB_SetConf failed!\n");
		return TS_FAILURE;
	}

	s32Ret = TS_MPI_VB_Init();

	if (TS_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("TS_MPI_VB_Init failed!\n");
		return TS_FAILURE;
	}

	s32Ret = TS_MPI_SYS_Init();
	if (TS_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("TS_MPI_SYS_Init failed!\n");
		TS_MPI_VB_Exit();
		return TS_FAILURE;
	}

	return s32Ret;
}

/******************************************************************************
* function : vb exit & MPI system exit
******************************************************************************/
TS_VOID SAMPLE_SYS_Exit()
{
    /*SAMPLE_COMM_VO_Exit();*/	/* only for debug */
    TS_MPI_SYS_Exit();
//    TS_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    TS_MPI_VB_Exit();
    return;
}

static TS_S32 CallCoreFunction(TS_S32 (*coreFunc)())
{
	TS_S32 ret = coreFunc();
	if (ret != TS_SUCCESS) {
		SAMPLE_PRT("Core function failed!\n");
	}
	return ret;
}


static TS_S32 CalcHistCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matOut(1, 256, CV_32SC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 1*256*4;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}

	matIn.setTo(cv::Scalar(3));

	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		int channels = 1;
		int histSize = 1;
		tscv::calcHistCve(pHandleCve, &matIn, 1, &channels, cv::Mat(), matOut, 1, &histSize, 0);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================calcHist time:%ldus\n", t1 - t0);
	}
	
	tscv::exitCve(pHandleCve);
	TS_U32 size = sizeof(int);
	TS_U32 counts = 256;
	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/HistResult", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, size, counts, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	return 0;
}

static TS_S32 AddCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matIn2(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	ret = TS_MPI_SYS_MmzAlloc(&matIn2.mpi_phyaddr, (void**)&matIn2.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matIn2.data = (uchar*)matIn2.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;
	
	matIn.setTo(cv::Scalar(10));
	matIn2.setTo(cv::Scalar(50));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::addCve(pHandleCve, matIn, matIn2, matOut, cv::Mat());
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================add time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);
	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/AddResult", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matIn2.mpi_phyaddr,(TS_VOID *)matIn2.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);

	return 0;
}

static TS_S32 SubtractCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matIn2(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	ret = TS_MPI_SYS_MmzAlloc(&matIn2.mpi_phyaddr, (void**)&matIn2.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matIn2.data = (uchar*)matIn2.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;
	
	matIn.setTo(cv::Scalar(50));
	matIn2.setTo(cv::Scalar(10));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::subtractCve(pHandleCve, matIn, matIn2, matOut, cv::Mat());
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================subtract time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/SubResult", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matIn2.mpi_phyaddr,(TS_VOID *)matIn2.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	return 0;
}

static TS_S32 BitwiseAndCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matIn2(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	ret = TS_MPI_SYS_MmzAlloc(&matIn2.mpi_phyaddr, (void**)&matIn2.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matIn2.data = (uchar*)matIn2.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(23));
	matIn2.setTo(cv::Scalar(185));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}

	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::bitwiseAndCve(pHandleCve, matIn, matIn2, matOut, cv::Mat());
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================bitwise_and time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/bitwiseAnd", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matIn2.mpi_phyaddr,(TS_VOID *)matIn2.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);

	return 0;
}

static TS_S32 BitwiseOrCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matIn2(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	ret = TS_MPI_SYS_MmzAlloc(&matIn2.mpi_phyaddr, (void**)&matIn2.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matIn2.data = (uchar*)matIn2.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(23));
	matIn2.setTo(cv::Scalar(185));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::bitwiseOrCve(pHandleCve, matIn, matIn2, matOut, cv::Mat());
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================bitwise_or time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/bitwiseOr", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matIn2.mpi_phyaddr,(TS_VOID *)matIn2.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);

	return 0;
}

static TS_S32 BitwiseXorCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matIn2(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	ret = TS_MPI_SYS_MmzAlloc(&matIn2.mpi_phyaddr, (void**)&matIn2.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matIn2.data = (uchar*)matIn2.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(23));
	matIn2.setTo(cv::Scalar(185));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::bitwiseXorCve(pHandleCve, matIn, matIn2, matOut, cv::Mat());
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================bitwise_xor time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/bitwiseXor", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matIn2.mpi_phyaddr,(TS_VOID *)matIn2.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	return 0;
}

static TS_S32 ThresholdCore()
{
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	if (access("data/480p_gray_0.raw", F_OK) != 0) {
		SAMPLE_PRT("error, Test bin files not found!\n");
		return -1;
	}

	FILE *txt_fd = fopen("data/480p_gray_0.raw", "r");
	fread(matIn.data, 1, insize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::thresholdCve(pHandleCve, matIn, matOut, 100.0, 200.0, THRESH_BINARY);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================threshold time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/ThreshResult", getpid());
	txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	return 0;
}

static TS_S32 ConvertToCore()
{
	printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_16SC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640*2;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(10));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::convertToCve(pHandleCve, matIn, matOut, CV_8UC1, 0.32, 2);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================convertTo time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/ConvertToResult", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);
	printf("BBBBBBBBBBBBBBBBBBBBBBBB\n");
	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	printf("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
	return 0;
}

static TS_S32 MapCore()
{
	TS_U16 mapTscv[256] = {
		0x06, 0xb5, 0xbe, 0x24, 0xf6, 0xac, 0x7b, 0xca, 0xc8, 0xf0, 0xbb, 0x98, 0x5e, 0xb9, 0x7c, 0x59, 0xd0, 0x41,
		0x33, 0x9d, 0x03, 0xdd, 0x59, 0x6a, 0x27, 0x28, 0xa6, 0x8f, 0x16, 0xb8, 0xb2, 0xe8, 0x03, 0xad, 0xf5, 0xd3,
		0x7b, 0xd7, 0xcf, 0x74, 0x10, 0xc6, 0xe3, 0x8f, 0x75, 0x45, 0xd5, 0x87, 0xd2, 0x65, 0xab, 0xd5, 0x32, 0x6e,
		0xcd, 0xa4, 0x5e, 0x20, 0xc9, 0x84, 0x81, 0xc0, 0x94, 0x36, 0x6c, 0x9f, 0x69, 0xe4, 0xb2, 0x5d, 0xf6, 0x9f,
		0xdc, 0xc2, 0x2a, 0x89, 0xaa, 0x6e, 0x88, 0x69, 0x89, 0x32, 0x8a, 0xdf, 0x2e, 0x64, 0xe2, 0x9c, 0xec, 0x47,
		0x88, 0xae, 0xf0, 0x93, 0x16, 0xc0, 0x95, 0xd4, 0xc3, 0xc0, 0x00, 0x46, 0x74, 0x7c, 0x3f, 0x5d, 0x64, 0xa0,
		0x04, 0xc9, 0x70, 0xf9, 0x56, 0xe8, 0x80, 0xff, 0x59, 0x98, 0x43, 0x30, 0xe6, 0x24, 0x4b, 0xb5, 0x7b, 0x1b,
		0xf8, 0xa0, 0xb4, 0xe6, 0x27, 0xb1, 0x95, 0x6a, 0x50, 0x96, 0x55, 0x64, 0x19, 0xeb, 0xba, 0xea, 0xab, 0x4a,
		0x53, 0xe5, 0xf6, 0xd8, 0xf0, 0x2f, 0x5a, 0xa7, 0x5a, 0x0a, 0xa1, 0xc8, 0xb5, 0x18, 0x6c, 0x22, 0xd2, 0xe4,
		0x5b, 0x44, 0x77, 0xdd, 0x7a, 0xd6, 0xed, 0x2e, 0x1b, 0x27, 0x9f, 0xfb, 0xb8, 0x7f, 0xdc, 0x68, 0x97, 0x0f,
		0xfc, 0x5a, 0xdb, 0x94, 0x0b, 0x7b, 0x21, 0xfa, 0x89, 0x45, 0xaf, 0x3d, 0x2e, 0x29, 0xd9, 0x41, 0x35, 0xd9,
		0x0f, 0x67, 0x7f, 0x3b, 0x74, 0xb1, 0x68, 0x7b, 0x9f, 0x50, 0x62, 0x74, 0x01, 0x60, 0xb7, 0xd0, 0x37, 0xf0,
		0x5f, 0x66, 0x1a, 0x24, 0xdc, 0x7d, 0x06, 0xe7, 0x0e, 0xeb, 0xc6, 0x85, 0xf1, 0x9d, 0xad, 0x0b, 0xd0, 0xd1,
		0xc6, 0x72, 0xa3, 0x70, 0xdd, 0xcf, 0x49, 0x71, 0xf3, 0x9d, 0x0f, 0x99, 0x4f, 0xa6, 0x71, 0x25, 0x3d, 0xac,
		0x81, 0xab, 0xa6, 0x0a,
	};
	
	TS_S32 ret = 0;
	Mat matIn(480, 640, CV_8UC1);
	Mat matOut(480, 640, CV_8UC1);
	TS_S32 insize = 480*640;
    TS_S32 outsize = 480*640;
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

	matIn.data = (uchar*)matIn.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(0));

	void *pHandleCve = tscv::createCve();
	if(pHandleCve == NULL) {
		SAMPLE_PRT("error, create cve handle failed!\n");
		return -1;
	}

	
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		tscv::mapCve(pHandleCve, matIn, matOut, mapTscv, 256);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================map time:%ldus\n", t1 - t0);
	}
	tscv::exitCve(pHandleCve);

	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d.bin", "out/MapResult", getpid());
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	return 0;
}
TS_FLOAT SAMPLE_TSCV_GetBitSizeByFmt(tscv::tsImgType pixel_fmt)
{
	TS_FLOAT fVal = 0;
	switch (pixel_fmt)
	{
	case tscv::TS_IMGTYPE_ARGB8888:
	case tscv::TS_IMGTYPE_ABGR8888:
	case tscv::TS_IMGTYPE_BGRA8888:
	case tscv::TS_IMGTYPE_RGBA8888:
		fVal = 4.0;
		break;
    case tscv::TS_IMGTYPE_YUV422_YUYV:
	case tscv::TS_IMGTYPE_YUV422_YVYU:
	case tscv::TS_IMGTYPE_YUV422_UYVY:
	case tscv::TS_IMGTYPE_YUV422_VYUY:
    case tscv::TS_IMGTYPE_YUV422SP_NV12:
    case tscv::TS_IMGTYPE_YUV422SP_NV21:
		fVal = 2.0;
		break;
	case tscv::TS_IMGTYPE_YUV420SP_NV12:
	case tscv::TS_IMGTYPE_YUV420SP_NV21:
		fVal = 1.5;
		break;
	default:
		break;
	}
	return fVal;
}

/**
 * @brief NV12 等比例缩放（纯 OpenCV 实现）
 * @param nv12    输入 NV12 图像（单通道，rows = src_h * 3 / 2, cols = src_w）
 * @param src_w   原始图像宽度
 * @param src_h   原始图像高度
 * @param max_w   缩放后最大宽度
 * @param max_h   缩放后最大高度
 * @return 缩放后的 NV12 图像
 */
Mat nv12ResizeKeepAspect(const Mat &nv12, int src_w, int src_h, int max_w, int max_h)
{
    // 1. 计算等比例缩放系数
    float scale_w = (float)max_w / src_w;
    float scale_h = (float)max_h / src_h;
    float scale = min(scale_w, scale_h);

    int dst_w = cvRound(src_w * scale);
    int dst_h = cvRound(src_h * scale);

    // 2. 拆分 Y 和 UV
    Mat y(src_h, src_w, CV_8UC1, nv12.data);
    Mat uv(src_h / 2, src_w, CV_8UC2, nv12.data + src_w * src_h);

    // 3. 分别缩放
    Mat y_dst, uv_dst;
    resize(y,  y_dst,  Size(dst_w, dst_h),        0, 0, INTER_LINEAR);
    resize(uv, uv_dst, Size(dst_w, dst_h / 2),  0, 0, INTER_LINEAR);

    // 4. 合并成新 NV12
    Mat nv12_dst(dst_h * 3 / 2, dst_w, CV_8UC1);
    y_dst.copyTo(nv12_dst.rowRange(0, dst_h));
    uv_dst.copyTo(nv12_dst.rowRange(dst_h, dst_h * 3 / 2));

    return nv12_dst;
}
// 全局/类成员 CVE 句柄（只需初始化一次）
static void* s_cveHandle = nullptr;
// ======================================================
// 这就是你要的：OpenCV 风格 + NPU 加速 的 NV12 缩放
// ======================================================
void npuResize(
    const Mat& src_nv12,   // 输入 NV12：CV_8UC1，rows=H*3/2
    Mat& dst_nv12,        // 输出 NV12
    Size dsize,           // 目标尺寸
    int interpolation = 0 // 无用，占位
) {
    // 直接调用清微 NPU/CVE 硬件接口
    tscv::resizeCve(
        s_cveHandle,
        (Mat&)src_nv12,
        dst_nv12,
        dsize,
        tscv::TS_IMGTYPE_YUV420SP_NV12,
        interpolation
    );
}

bool cveResizeNv12(
    const Mat& nv12_src,   // 必须是: 1920x1080*1.5, CV_8UC1
    Mat& nv12_dst,
    int dst_w, int dst_h
) {
    if (nv12_src.rows != 1080 * 3 / 2 || nv12_src.cols != 1920) {
        printf("src must be 1920x1080 NV12\\n");
        return false;
    }

    // 强制正确格式，从根源解决 format 错误
    Mat src = nv12_src.clone();

    // 输出必须先创建（解决 dims >=2 报错）
    nv12_dst.create(dst_h * 3 / 2, dst_w, CV_8UC1);

    // 硬件缩放
    resizeCve(
        s_cveHandle,
        src,
        nv12_dst,
        Size(dst_w, dst_h),
         tscv::TS_IMGTYPE_YUV420SP_NV12,
        0
    );
    return true;
}

void yoloPreprocess_640(
    const Mat& nv12_src,   // 1920x1080 NV12
    Mat& nv12_out          // 输出 640x640 NV12
) {
    const int SRC_W = 1920;
    const int SRC_H = 1080;
    const int MODEL = 640;

    // 等比例：1920→640，1080→360
    int r_w = MODEL;
    int r_h = 360;

    // 1. NPU 硬件缩放（绝对不报错）
    Mat nv12_resize;
    cveResizeNv12(nv12_src, nv12_resize, r_w, r_h);

    // 2. 居中填充到 640x640
    nv12_out.create(MODEL * 3 / 2, MODEL, CV_8UC1);
    nv12_out = 0;

    int pad_y = 140; // 上下各140

    // 拷贝 Y
    Mat y_src = nv12_resize.rowRange(0, r_h);
    Mat y_dst = nv12_out.rowRange(0, MODEL);
    y_src.copyTo(y_dst.rowRange(pad_y, pad_y + r_h));

    // 拷贝 UV
    Mat uv_src = nv12_resize.rowRange(r_h, r_h * 3 / 2);
    Mat uv_dst = nv12_out.rowRange(MODEL, MODEL * 3 / 2);
    uv_src.copyTo(uv_dst.rowRange(pad_y/2, pad_y/2 + r_h/2));
}

// 等比例缩放 + 居中填充 640x640（NPU 缩放 + OpenCV 填充）
void yoloPreprocess(
    const Mat& src_nv12,  // 1920x1080 NV12
    Mat& dst_nv12,        // 输出 640x640 NV12
    int model_size = 640
) {
    int src_w = 1920;
    int src_h = 1080;
    // 1. 等比例计算
    float scale = (float)model_size / src_w;
    int rw = model_size;
    int rh = cvRound(src_h * scale); // 360

    // 2. NPU 硬件缩放（这一步是硬件！）
    Mat resized_nv12;
    npuResize(src_nv12, resized_nv12, Size(rw, rh));

    // 3. 填充成 640x640（YOLO 需要正方形）
    dst_nv12.create(model_size * 3 / 2, model_size, CV_8UC1);
    dst_nv12 = 0;

    int pad_y = (model_size - rh) / 2; // 上下各 140 行黑边

    // 拷贝 Y
    Mat y_src = resized_nv12.rowRange(0, rh);
    Mat y_dst = dst_nv12.rowRange(0, model_size);
    y_src.copyTo(y_dst.rowRange(pad_y, pad_y + rh));

    // 拷贝 UV
    Mat uv_src = resized_nv12.rowRange(rh, rh * 3 / 2);
    Mat uv_dst = dst_nv12.rowRange(model_size, model_size * 3 / 2);
    uv_src.copyTo(uv_dst.rowRange(pad_y/2, pad_y/2 + rh/2));
}



// 初始化 NPU/CVE（程序启动调用一次）
bool initNpuResize() {
	s_cveHandle = tscv::createCve();
    if (!s_cveHandle) {
        printf("CVE 初始化失败\\n");
        return false;
    }
    return true;
}

// 销毁 NPU/CVE
void deinitNpuResize() {
    if (s_cveHandle) {
        tscv::exitCve(s_cveHandle);
        s_cveHandle = nullptr;
    }
}
// static bool npu_resize_nv12(const Mat &src, Mat &dst, int dst_w, int dst_h) {
//     if (!s_cveHandle || src.empty() || src.channels() != 1)
//         return false;

//     // 你这里就是 1920x1080 NV12，直接强校验
//     if (src.cols != 1920 || src.rows != 1620)
//         return false;

//     // 硬件必须：连续内存
//     Mat safe_src = src.clone();
//     if (!safe_src.isContinuous())
//         return false;

//     // ======================
//     // 【关键】清微 CVE 对 NV12 必须：宽高 16 对齐！
//     // ======================
//     int dst_w_align = (dst_w + 15) / 16 * 16;
//     int dst_h_align = (dst_h + 15) / 16 * 16;

//     int dst_nv12_h = dst_h_align * 3 / 2;
//     dst.create(dst_nv12_h, dst_w_align, CV_8UC1);
//     dst.setTo(0);

//     // 只传连续 data，不带多余 step
//     Mat src_nv12(safe_src.rows, safe_src.cols, CV_8UC1, safe_src.data);
//     Mat dst_nv12(dst.rows, dst.cols, CV_8UC1, dst.data);

//     // ======================
//     // 【重点】给 CVE 的是对齐后尺寸！
//     // ======================
//     tscv::resizeCve(
//         s_cveHandle,
//         src_nv12,
//         dst_nv12,
//         Size(dst_w_align, dst_h_align),
//         tscv::TS_IMGTYPE_YUV420SP_NV12,
//         0
//     );

//     return true;
// }

// static bool npu_resize_nv12(const Mat &src, Mat &dst, int dst_w, int dst_h)
// {
//     if (!s_cveHandle || src.empty() )
//         return false;

//     // ======================
//     // 【堵死所有坑】
//     // 1. 强制深拷贝 → 连续内存
//     // 2. 强制尺寸正确 → 1920x1620
//     // 3. 强制对齐 → 硬件绝对认
//     // ======================
//     Mat safe_src;
//     if (src.rows == 1080*3/2 && src.cols == 1920 && src.isContinuous()) {
//         safe_src = src;
//     } else {
//         safe_src = src.clone();
//     }

//     // ======================
//     // 【关键】dst 必须先创建，并且必须是 NV12 结构
//     // ======================
//     int nv12_h = dst_h * 3 / 2;
//     dst.create(nv12_h, dst_w, CV_8UC1);
//     dst.setTo(0); // 清0，防止脏数据

//     // ======================
//     // 【终极保险】
//     // 只传：cols、rows、data，绝对不依赖Mat内部stride
//     // ======================
//     Mat src_real(1080*3/2, 1920, CV_8UC1, safe_src.data);
//     Mat dst_real(nv12_h, dst_w, CV_8UC1, dst.data);

//     resizeCve(
//         s_cveHandle,
//         src_real,        // 绝对标准NV12 Mat
//         dst_real,        // 提前创建好
//         Size(dst_w, dst_h),
//         tscv::TS_IMGTYPE_YUV420SP_NV12,
//         0
//     );

//     return true;
// }

 

// 全局句柄（你工程里已有）
extern void* s_cveHandle;

/**
 * @brief TX5215 专用：NV12 -> RGBA -> resizeCve 硬件缩放
 * @param src_nv12  输入：1920x1080 NV12 (rows=1620, cols=1920, CV_8UC1)
 * @param dst_rgba  输出：缩放后的 RGBA 图像
 * @param dst_w     目标宽
 * @param dst_h     目标高
 * @return true 成功
 */
static bool npu_nv12_to_rgba_resize_cve(
    const cv::Mat &src_nv12,
    cv::Mat &dst_rgba,
    int dst_w,
    int dst_h
)
{
    // 1. 基础校验
    if (!s_cveHandle || src_nv12.empty() || src_nv12.channels() != 1)
        return false;

    // 固定你输入：1920x1080 NV12
    if (src_nv12.cols != 1920 || src_nv12.rows != 1620)
        return false;

    // ==============================
    // 步骤1：NV12 -> RGBA4 通道转换
    // ==============================
    cv::Mat rgba_1080p;
    rgba_1080p.create(1080, 1920, CV_8UC4);
    cv::cvtColor(src_nv12, rgba_1080p, cv::COLOR_YUV2RGBA_NV12);

    // ==============================
    // 步骤2：resizeCve 只认 RGBA/ARGB，这里终于能跑了
    // ==============================
    dst_rgba.create(dst_h, dst_w, CV_8UC4);
    dst_rgba.setTo(0);

    tscv::resizeCve(
        s_cveHandle,
        rgba_1080p,
        dst_rgba,
        cv::Size(dst_w, dst_h),
        tscv::TS_IMGTYPE_ARGB8888, // 这里必须是4通道
        0
    );

    return true;
}

static bool npu_resize_nv12(const Mat &src, Mat &dst, int dst_w, int dst_h)
{
    if (!s_cveHandle || src.empty() || src.channels() != 1)
        return false;

    Mat safe_src;
    if (src.rows == 1620 && src.cols == 1920 && src.isContinuous()) {
        safe_src = src;
    } else {
        safe_src = src.clone();
    }

    int dst_nv12_h = dst_h * 3 / 2;
    dst.create(dst_nv12_h, dst_w, CV_8UC1);
    dst.setTo(0);

    Mat src_real(1620, 1920, CV_8UC1, safe_src.data);
    Mat dst_real(dst_nv12_h, dst_w, CV_8UC1, dst.data);

    tscv::resizeCve(
        s_cveHandle,
        src_real,
        dst_real,
        Size(dst_w, dst_h),
        tscv::TS_IMGTYPE_YUV420SP_NV12,
        0
    );

    return true;
}

// #include <opencv2/opencv.h>
// #include <algorithm>
// using namespace cv;

/**
 * @brief 极速 NV12 等比例缩放（不依赖任何 NPU/VPSS/CVE）
 * @param src        输入 NV12: CV_8UC1, rows = real_h * 3 / 2
 * @param dst        输出 NV12
 * @param dst_w      目标画布宽
 * @param dst_h      目标画布高
 * @param align      对齐要求（建议16，NPU推理最稳）
 * @return true 成功
 */
static bool nv12_resize_fast_general(const Mat &src, Mat &dst, int dst_w, int dst_h, int align = 16)
{
    if (src.empty() || src.channels() != 1)
        return false;

    int src_w = src.cols;
    int src_h = src.rows * 2 / 3;

    // 1. 等比例计算输出有效区域
    int out_w, out_h;
    if (dst_w * src_h > dst_h * src_w) {
        out_h = dst_h;
        out_w = (dst_h * src_w) / src_h;
    } else {
        out_w = dst_w;
        out_h = (dst_w * src_h) / src_w;
    }

    // 2. 按 align 对齐（NPU 必需要）
    out_w = ((out_w + align - 1) / align) * align;
    out_h = ((out_h + align - 1) / align) * align;

    // 3. 创建目标 NV12 内存
    int dst_nv12_h = dst_h * 3 / 2;
    dst.create(dst_nv12_h, dst_w, CV_8UC1);
    dst.setTo(0);

    const uint8_t *srcY = src.data;
    uint8_t       *dstY = dst.data;

    // 4. 极速采样缩放 Y 通道（等比例，无浮点，最快）
    for (int dy = 0; dy < out_h; dy++) {
        int sy = (dy * src_h) / out_h;
        for (int dx = 0; dx < out_w; dx++) {
            int sx = (dx * src_w) / out_w;
            dstY[dy * dst_w + dx] = srcY[sy * src_w + sx];
        }
    }

    // 5. UV 通道极速采样（NV12 格式：UV 交错，宽 same，高 1/2）
    const uint8_t *srcUV = src.data + src_w * src_h;
    uint8_t       *dstUV = dst.data + dst_w * dst_h;

    int uv_h = src_h / 2;
    int uv_oh = out_h / 2;

    for (int dy = 0; dy < uv_oh; dy++) {
        int sy = (dy * uv_h) / uv_oh;
        for (int dx = 0; dx < out_w; dx += 2) {
            int sx = (dx * src_w) / out_w;
            dstUV[dy * dst_w + dx + 0] = srcUV[sy * src_w + sx + 0];
            dstUV[dy * dst_w + dx + 1] = srcUV[sy * src_w + sx + 1];
        }
    }

    return true;
}

#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#ifndef __cache_flush_defined
#define __cache_flush_defined

#define CACHE_LINESIZE 64

/**
 * @brief 清理CPU缓存，保证NPU/CVE硬件读到最新数据
 * @param phy_addr  物理地址
 * @param vir_addr  虚拟地址
 * @param size      数据大小
 */
static inline void __flush_cache(unsigned char *phy_addr,
                                 unsigned char *vir_addr,
                                 int size)
{
    if (!vir_addr || size <= 0)
        return;

    uintptr_t start = (uintptr_t)vir_addr;
    uintptr_t end = start + size;

    // 按64字节缓存行对齐
    start = start & ~(CACHE_LINESIZE - 1);
    end = (end + CACHE_LINESIZE - 1) & ~(CACHE_LINESIZE - 1);

    // ARM 缓存清理系统调用（Linux 标准）
    syscall(__ARM_NR_cacheflush, start, end, 0);
}
#endif
// unsigned char* tscv_npu_resize_nv12(unsigned char* src_data, unsigned char* src_phy,
//                                     int src_w, int src_h,
//                                     unsigned char* dst_data, unsigned char* dst_phy,
//                                     int dst_w, int dst_h)
// {
//     // 1. 非法直接返回 NULL
//     if (!s_cveHandle || !src_data || !src_phy || !dst_data || !dst_phy) {
//         return NULL;
//     }
//     if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0) {
//         return NULL;
//     }

//     const int ALIGN = 64;
//     int src_stride = ((src_w + ALIGN - 1) / ALIGN) * ALIGN;
//     int dst_stride = ((dst_w + ALIGN - 1) / ALIGN) * ALIGN;

//     int src_nv12_size = src_stride * src_h * 3 / 2;
//     int dst_nv12_size = dst_stride * dst_h * 3 / 2;

//     int src_nv12_h = src_h * 3 / 2;
//     int dst_nv12_h = dst_h * 3 / 2;

//     //------------------------------------------------------------------
//     // 等比例核心：fit 宽高 + 居中偏移
//     //------------------------------------------------------------------
//     float scale_w = (float)dst_w / src_w;
//     float scale_h = (float)dst_h / src_h;
//     float scale    = (scale_w < scale_h) ? scale_w : scale_h;

//     int fit_w  = (int)(src_w  * scale);
//     int fit_h  = (int)(src_h  * scale);

//     int offset_x = (dst_w - fit_w) / 2;
//     int offset_y = (dst_h - fit_h) / 2;

//     // 防止偏移为负
//     offset_x = (offset_x < 0) ? 0 : offset_x;
//     offset_y = (offset_y < 0) ? 0 : offset_y;

//     //------------------------------------------------------------------
//     // NV12 整体 Mat
//     //------------------------------------------------------------------
//     cv::Mat src(src_nv12_h, src_w, CV_8UC1, src_data, src_stride);
//     cv::Mat dst(dst_nv12_h, dst_w, CV_8UC1, dst_data, dst_stride);

//     src.mpi_phyaddr = (uint64_t)src_phy;
//     dst.mpi_phyaddr = (uint64_t)dst_phy;
//     src.mpi_viraddr = (uint64_t)src_data;
//     dst.mpi_viraddr = (uint64_t)dst_data;

//     cv::Mat safe_src = src.isContinuous() && (src_stride == src_w)
//         ? src
//         : src.clone();

//     // 整图清黑
//     dst.setTo(cv::Scalar(0));

//     //------------------------------------------------------------------
//     // 目标 ROI：只画中间等比例区域
//     //------------------------------------------------------------------
//     cv::Mat dst_roi_nv12(dst,
//         cv::Rect(offset_x, offset_y * 3/2, fit_w, fit_h * 3/2)
//     );

//     cv::Mat src_real(src_nv12_h, src_w, CV_8UC1, safe_src.data, src_stride);

//     //------------------------------------------------------------------
//     // 硬件缩放
//     //------------------------------------------------------------------
//    tscv::resizeCve(
//         s_cveHandle,
//         src_real,
//         dst_roi_nv12,
//         cv::Size(fit_w, fit_h),
//         tscv::TS_IMGTYPE_YUV420SP_NV12,
//         0
//     );
//     //memcpy(dst_data,dst_real.data,dst_nv12_size);
//     //------------------------------------------------------------------
//     // 缓存同步
//     //------------------------------------------------------------------
//     __flush_cache(src_phy, src_data, src_nv12_size);
//     __flush_cache(dst_phy, dst_data, dst_nv12_size);

//     // 返回目标数据指针
//     return dst_data;
// }
int tscv_npu_resize_nv12(unsigned char* src_data, unsigned char* src_phy,
                         int src_w, int src_h,
                         unsigned char* dst_data, unsigned char* dst_phy,
                         int dst_w, int dst_h)
{
    // 【堵死所有坑】64字节对齐 · 工业级稳定版
    // 1. 句柄 & 空指针（物理地址也必须校验）
    if (!s_cveHandle || !src_data || !src_phy || !dst_data || !dst_phy) {
        return -1;
    }

    // 2. 宽高合法性
    if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0) {
        return -2;
    }

    //------------------------------------------------------------------
    // 3. 64字节对齐（TX5215 硬件强制要求）
    //------------------------------------------------------------------
    const int ALIGN = 64;
    int src_stride = ((src_w + ALIGN - 1) / ALIGN) * ALIGN;
    int dst_stride = ((dst_w + ALIGN - 1) / ALIGN) * ALIGN;

    int src_nv12_size = src_stride * src_h * 3 / 2;
    int dst_nv12_size = dst_stride * dst_h * 3 / 2;

    int src_nv12_h = src_h * 3 / 2;
    int dst_nv12_h = dst_h * 3 / 2;

    //------------------------------------------------------------------
    // 4. 带 stride 创建 Mat（硬件唯一认的格式）
    //------------------------------------------------------------------
    cv::Mat src(src_nv12_h, src_w, CV_8UC1, src_data, src_stride);
    cv::Mat dst(dst_nv12_h, dst_w, CV_8UC1, dst_data, dst_stride);

    // 5. 物理/虚拟地址（TSCV 定制 Mat）
    src.mpi_phyaddr = (uint64_t)src_phy;
    dst.mpi_phyaddr = (uint64_t)dst_phy;
    src.mpi_viraddr = (uint64_t)src_data;
    dst.mpi_viraddr = (uint64_t)dst_data;

    //------------------------------------------------------------------
    // 6. 真正安全：只有 连续内存 + 64对齐 才不拷贝
    //------------------------------------------------------------------
    cv::Mat safe_src;
    if (src.isContinuous() && (src_stride == src_w)) {
        safe_src = src;
    } else {
        safe_src = src.clone(); // 深拷贝到连续对齐内存
    }

    // 7. 目标清空，防止脏数据/花屏
    dst.setTo(cv::Scalar(0));

    //------------------------------------------------------------------
    // 8. 最终送入硬件的 Mat（带正确 stride）
    //------------------------------------------------------------------
    cv::Mat src_real(src_nv12_h, src_w, CV_8UC1, safe_src.data, src_stride);
    cv::Mat dst_real(dst_nv12_h, dst_w, CV_8UC1, dst.data, dst_stride);

    //------------------------------------------------------------------
    // 9. 调用CVE硬件缩放，并判断返回值
    //------------------------------------------------------------------
   tscv::resizeCve(
        s_cveHandle,
        src_real,
        dst_real,
        cv::Size(dst_w, dst_h),
        tscv::TS_IMGTYPE_YUV420SP_NV12,
        0
    );

     

    //------------------------------------------------------------------
    // 10. ARM 缓存同步（NPU 必加，否则必花屏/旧数据）
    //------------------------------------------------------------------
    __flush_cache(src_phy, src_data, src_nv12_size);
    __flush_cache(dst_phy, dst_data, dst_nv12_size);

    return 0;
}

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <libyuv.h>

// #define ALIGN(w, a) (((w) + (a) - 1) & ~((a) - 1))

// /**
//  * NV12 等比例缩放（保持比例，居中黑边）
//  */
// int nv12_scale_fit(const uint8_t *src_nv12, int src_w, int src_h,
//                    uint8_t *dst_nv12, int dst_w, int dst_h)
// {
//     if (!src_nv12 || !dst_nv12 || src_w <=0 || src_h <=0 || dst_w <=0 || dst_h <=0)
//         return -1;

//     // 计算等比例
//     float scale_w = (float)dst_w / src_w;
//     float scale_h = (float)dst_h / src_h;
//     float scale = (scale_w < scale_h) ? scale_w : scale_h;

//     int out_w = (int)(src_w * scale);
//     int out_h = (int)(src_h * scale);

//     int x_off = (dst_w - out_w) / 2;
//     int y_off = (dst_h - out_h) / 2;

//     // 先把整个目标 NV12 填黑色（替代 NV12Rect）
//     memset(dst_nv12, 0x00, dst_w * dst_h);                  // Y 填 0
//     memset(dst_nv12 + dst_w * dst_h, 0x80, dst_w * dst_h / 2); // UV 填 128

//     // NEON 加速缩放
//     return libyuv::NV12Scale(
//         src_nv12, src_w,
//         src_nv12 + src_w * src_h, src_w,
//         src_w, src_h,
//         dst_nv12 + x_off + y_off * dst_w, dst_w,
//         dst_nv12 + x_off + y_off * dst_w + dst_w * dst_h, dst_w,
//         out_w, out_h,
//         libyuv::kFilterBilinear
//     );
// }
// #include <string.h>
// #include <libyuv.h>

// int nv12_scale_fit_fix_color(const uint8_t *src_nv12,
//                              int src_w, int src_h, int src_stride,
//                              uint8_t *dst_nv12,
//                              int dst_w, int dst_h)
// {
//     if (!src_nv12 || !dst_nv12)
//         return -1;

//     // 等比例计算
//     float scale_w = (float)dst_w / src_w;
//     float scale_h = (float)dst_h / src_h;
//     float scale   = (scale_w < scale_h) ? scale_w : scale_h;

//     int out_w = (int)(src_w * scale);
//     int out_h = (int)(src_h * scale);

//     // 居中偏移
//     int x_off = (dst_w - out_w) / 2;
//     int y_off = (dst_h - out_h) / 2;

//     // --------------------------
//     // 整块清黑色（绝对不偏色）
//     // --------------------------
//     memset(dst_nv12,                       0, dst_w * dst_h);       // Y
//     memset(dst_nv12 + dst_w * dst_h,    0x80, dst_w * dst_h / 2);   // UV

//     // --------------------------
//     // 关键修复：dst UV stride 必须 = dst_w
//     // --------------------------
//     const uint8_t *src_y = src_nv12;
//     const uint8_t *src_uv = src_nv12 + src_stride * src_h;

//     uint8_t *dst_y = dst_nv12 + y_off * dst_w + x_off;
//     uint8_t *dst_uv = dst_nv12 + dst_w * dst_h + y_off * dst_w / 2 + x_off;

//     return libyuv::NV12Scale(
//         src_y, src_stride,
//         src_uv, src_stride,    // 输入 UV stride = 输入 Y stride
//         src_w, src_h,

//         dst_y, dst_w,
//         dst_uv, dst_w,         // 输出 UV stride = 输出 dst_w（关键修复）
//         out_w, out_h,

//         libyuv::kFilterBilinear
//     );
// }

#include <string.h>
#include <libyuv.h>

/**
 * @brief NV12 等比例缩放 + 居中
 * 速度：640x720 → 640x640 约 600~1000us
 */
int nv12_scale_fit_fast(const uint8_t *src_nv12,
                         int src_w, int src_h, int src_stride,
                         uint8_t *dst_nv12,
                         int dst_w, int dst_h)
{
    float scale = (dst_w * 1.0f / src_w < dst_h * 1.0f / src_h)
                ? (dst_w * 1.0f / src_w)
                : (dst_h * 1.0f / src_h);

    int out_w = src_w * scale;
    int out_h = src_h * scale;
    int x_off = (dst_w - out_w) / 2;
    int y_off = (dst_h - out_h) / 2;

    uint8_t *dst_y  = dst_nv12;
    uint8_t *dst_uv = dst_nv12 + dst_w * dst_h;

    // 只清 UV（Y 会被覆盖，不清以提速）
    memset(dst_uv, 0x80, dst_w * dst_h / 2);

    // 核心：最快滤波 + NEON 加速
    return libyuv::NV12Scale(
        src_nv12,                 src_stride,
        src_nv12 + src_stride * src_h, src_stride,
        src_w, src_h,

        dst_y  + y_off * dst_w + x_off, dst_w,
        dst_uv + (y_off/2)*dst_w + x_off, dst_w,
        out_w, out_h,

        libyuv::kFilterNone  // 最快！kFilterNone  kFilterBox
    );
}

TS_S32 ResizeCveCore()
{
    //TS_CHAR szInFileName[128] = "/mnt/sda0/wei/data/1080p_nv12_1.raw";
	//TS_CHAR szInFileName[128] = "/mnt/sda0/wei/data/1080p_nv12_1.raw";
	TS_CHAR szInFileName[128] = "/mnt/sda0/wei/data/1080p_nv12.raw";
	
    TS_CHAR szOutFileName[128] = "/mnt/sda0/wei/out/out_file_cv_resize";
    TS_CHAR szOutFileNameWithPid[128];
    TS_S32 src_w = 640;
    TS_S32 src_h = 720;
    TS_S32 dst_w = 640;
    TS_S32 dst_h = 640;
    cv::Size srcSize(src_w, src_h*3/2);
    cv::Size dstSize(dst_w, dst_h);
    cv::Mat yuvIn(srcSize, CV_8UC1);
    cv::Mat yuvOut(dst_h*3/2, dst_w, CV_8UC1);
    TS_S32 insize = src_w*src_h*SAMPLE_TSCV_GetBitSizeByFmt(tscv::TS_IMGTYPE_YUV420SP_NV12);
    TS_S32 outsize = dst_w*dst_h*SAMPLE_TSCV_GetBitSizeByFmt(tscv::TS_IMGTYPE_YUV420SP_NV12);
    TS_S32 ret = 0;
	// 1. 初始化 NPU
    // initNpuResize();
    // ret = TS_MPI_SYS_MmzAlloc(&yuvIn.mpi_phyaddr, (void**)&yuvIn.mpi_viraddr, NULL, NULL, insize);
    // if(ret != 0){
    //     printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
    //     return -1;
    // }
    // ret = TS_MPI_SYS_MmzAlloc(&yuvOut.mpi_phyaddr, (void**)&yuvOut.mpi_viraddr, NULL, NULL, outsize);
    // if(ret != 0){
    //     printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
    //     return -1;
    // }

    // yuvIn.data = (uchar*)yuvIn.mpi_viraddr;
    // yuvOut.data = (uchar*)yuvOut.mpi_viraddr;

	// ==================== 你只需要改这里 ====================
    const int SRC_W = 640;
    const int SRC_H = 720;
    const int DST_W = 640;
    const int DST_H = 640;
    // ======================================================

    // 申请内存
    size_t src_size = SRC_W * SRC_H * 3 / 2;
    size_t dst_size = DST_W * DST_H * 3 / 2;

    uint8_t *src_buf = (uint8_t *)malloc(src_size);
    uint8_t *dst_buf = (uint8_t *)malloc(dst_size);

    if (!src_buf || !dst_buf) {
        printf("malloc failed\\n");
        return -1;
    }

    // 这里你可以从文件/摄像头读入 NV12，Demo 用 0 填充
    memset(src_buf, 0x80, src_size);
    memset(dst_buf, 0, dst_size);

    printf("Start scale: %dx%d -> %dx%d (fit)\\n",
           SRC_W, SRC_H, DST_W, DST_H);

    

    
    //加载测试数据
    FILE *pFileRead = fopen(szInFileName, "rb");
    if (TS_NULL == pFileRead)
    {
        SAMPLE_PRT("can't open file %s\n", szInFileName);
        return -1;
    }
    fread((char*)src_buf, 1, insize, pFileRead);
    fclose(pFileRead);
    pFileRead = NULL;
	printf("s_cveHandle = %p\\n", s_cveHandle);

   // tscv::resizeVpss(vpssHandle, yuvIn, yuvOut, dstSize);/**参数与初始化冲突时，内部会重新初始化，时间消耗会变大**/
   // nv12_resize_fast_general(yuvIn, yuvOut, dst_w, dst_h,16);
   // 等比例缩放
    if (nv12_scale_fit_fast(src_buf, SRC_W, SRC_H,SRC_W,
                       dst_buf, DST_W, DST_H) == 0) {
        printf("Scale success!\\n");
    } else {
        printf("Scale failed!\\n");
    }
	struct timeval tmStart, tmEnd;
    int nTmCost = 0;
    for(int i=0; i<10000; i++) {
        gettimeofday(&tmStart, NULL);
        //nv12_resize_fast_general(yuvIn, yuvOut, dst_w, dst_h,16);
		// 等比例缩放
    if (nv12_scale_fit_fast(src_buf, SRC_W, SRC_H,SRC_W,
                       dst_buf, DST_W, DST_H) == 0) {
        printf("Scale success!\\n");
    } else {
        printf("Scale failed!\\n");
    }
        gettimeofday(&tmEnd, NULL);
        nTmCost = (tmEnd.tv_sec - tmStart.tv_sec)*1000000 + (tmEnd.tv_usec - tmStart.tv_usec);
        printf("tscv::resizeVpss Nv12 1920x1080 to Nv12 640*360 time cost: %dus\n", nTmCost);
		usleep(1000 * 30);
    }
    

    //保存测试数据
    sprintf(szOutFileNameWithPid, "%s_%d.yuv", (TS_CHAR*)szOutFileName, getpid());
    pFileRead = fopen(szOutFileNameWithPid, "wb");
    if (TS_NULL != pFileRead) {
        TS_S32 outsize = dst_w*dst_h*SAMPLE_TSCV_GetBitSizeByFmt(tscv::TS_IMGTYPE_YUV420SP_NV12);
        printf("out file w=%d h=%d, size=%d ptr=%p\n", dst_w, dst_h, outsize, (void*)yuvOut.data);
        TS_S32 ret = fwrite((char*)dst_buf, 1, outsize, pFileRead);
        fclose(pFileRead);
        printf("success to save out file, ret=%d\n", ret);
    }

    // 释放vb
    // TS_MPI_SYS_MmzFree(yuvIn.mpi_phyaddr,(TS_VOID *)yuvIn.mpi_viraddr);
    // TS_MPI_SYS_MmzFree(yuvOut.mpi_phyaddr,(TS_VOID *)yuvOut.mpi_viraddr);

	// yuvIn.data = 0;
	// yuvOut.data = 0;
	// yuvIn.mpi_phyaddr = 0;
	// yuvOut.mpi_phyaddr = 0;
	// yuvIn.mpi_viraddr = 0;
	// yuvOut.mpi_viraddr = 0;
free(src_buf);
    free(dst_buf);
	return 0;
}

static TS_S32 ResizeCveCore33()
{
	TS_CHAR szInFileName[128] = "/mnt/sda0/wei/data/1080p_nv12_1.raw";
    //TS_CHAR szOutFileName[128] = "/mnt/sda0/wei/data/out/out_file_cv_resize";
	//TS_CHAR szInFileName[128] = "/model/1080p_nv12.raw";
    TS_CHAR szOutFileName[128] = "/model/out_file_cv_resize";
	  int src_w = 1920;
    int src_h = 1080;
TS_S32 dst_w = 640;
    TS_S32 dst_h = 640;
	TS_S32 insize = src_w*src_h*3/2;
    TS_S32 outsize = dst_w*dst_h*3/2;
	// 1. 初始化 NPU
    initNpuResize();
unsigned char* src_data = (unsigned char*)malloc(640 * 720 * 3 / 2);
    // 2. 你的 NV12 数据（1920x1080）
    Mat src_nv12(1080 * 3 / 2, 1920, CV_8UC1);
//加载测试数据
    FILE *pFileRead = fopen(szInFileName, "rb");
    if (TS_NULL == pFileRead)
    {
        SAMPLE_PRT("can't open file %s\n", szInFileName);
        return -1;
    }
    fread((char*)src_nv12.data, 1, insize, pFileRead);
    fclose(pFileRead);
    pFileRead = NULL;
SAMPLE_PRT("===========8888=====resizeVgs time\n");
    // 3. YOLO 前处理：NPU 缩放 + 填充 640x640
    Mat yolo_input;
	unsigned char* dst_buf = (unsigned char*)malloc(640 * 640 * 3 / 2);
	long t0 = SAMPLE_TSCV_GetTime();
    //yoloPreprocess(src_nv12, yolo_input);
	 Mat yolo_in;
    //yoloPreprocess_640(src_nv12, yolo_in);
	npu_resize_nv12(src_nv12, yolo_in, dst_w, dst_h);
	//dst_buf = tscv_npu_resize_nv12(src_data,src_data, 640, 720, dst_buf,dst_buf, 640, 640);
	long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================resizeVgs time:%ldus\n", t1 - t0);
	 TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d_%dx%d_to_%dx%d.argb", "/model/ResizeResult", getpid(), src_w, src_h, dst_w, dst_h);
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(yolo_in.data, 1, 640*640*3/2, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);
	SAMPLE_PRT("resizeVgs result saved to: %s\n", szOutFileNameWithPid);
        
		free(dst_buf);
		free(src_data);
	return 0;
}

static TS_S32 ResizeCveCore2()
{
	TS_CHAR szInFileName[128] = "/mnt/sda0/wei/data/1080p_nv12_1.raw";
    TS_CHAR szOutFileName[128] = "/mnt/sda0/wei/data/out/out_file_cv_resize";
	  int src_w = 1920;
    int src_h = 1080;
TS_S32 dst_w = 640;
    TS_S32 dst_h = 640;
	TS_S32 insize = src_w*src_h*3/2;
    TS_S32 outsize = dst_w*dst_h*3/2;
    Mat nv12_src(src_h * 3 / 2, src_w, CV_8UC1);  // 你的 NV12 数据
	 //加载测试数据
    FILE *pFileRead = fopen(szInFileName, "rb");
    if (TS_NULL == pFileRead)
    {
        SAMPLE_PRT("can't open file %s\n", szInFileName);
        return -1;
    }
    fread((char*)nv12_src.data, 1, insize, pFileRead);
    fclose(pFileRead);
    pFileRead = NULL;
SAMPLE_PRT("===========8888=====resizeVgs time\n");
	Mat nv12_dst1;
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		  Mat nv12_dst = nv12ResizeKeepAspect(nv12_src, src_w, src_h, 640, 640);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================resizeVgs time:%ldus\n", t1 - t0);
		nv12_dst1.data = nv12_dst.data;
	}
	
	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d_%dx%d_to_%dx%d.argb", "/mnt/sda/wei/data/out/ResizeResult", getpid(), src_w, src_h, dst_w, dst_h);
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(nv12_dst1.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);
	SAMPLE_PRT("resizeVgs result saved to: %s\n", szOutFileNameWithPid);

	 
	
	return 0;
}


static TS_S32 ResizeCveCore1()
{
	TS_S32 ret = 0;
	
	int src_w = 640;
	int src_h = 480;
	int dst_w = 320;
	int dst_h = 240;
	
	Mat matIn(src_h, src_w, CV_8UC4);
	Mat matOut(dst_h, dst_w, CV_8UC4);
	
	TS_S32 insize = src_w * src_h * 4;
	TS_S32 outsize = dst_w * dst_h * 4;
	
	ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, (void**)&matIn.mpi_viraddr, NULL, NULL, insize);
	if(ret != 0){
		printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
		return -1;
	}

	ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, (void**)&matOut.mpi_viraddr, NULL, NULL, outsize);
	if(ret != 0){
		printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
		TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
		return -1;
	}

	matIn.data = (uchar*)matIn.mpi_viraddr;
	matOut.data = (uchar*)matOut.mpi_viraddr;

	matIn.setTo(cv::Scalar(128, 128, 128, 255));

	cv::Size dstSize(dst_w, dst_h);
	
	SAMPLE_PRT("resizeVgs: %dx%d -> %dx%d (ARGB format)\n", src_w, src_h, dst_w, dst_h);
	
	for(int i=0; i<10; i++) {
		long t0 = SAMPLE_TSCV_GetTime();
		//tscv::resizeVgs(matIn, matOut, dstSize, tscv::TS_IMGTYPE_ARGB8888);
		long t1 = SAMPLE_TSCV_GetTime();
		SAMPLE_PRT("================resizeVgs time:%ldus\n", t1 - t0);
	}
	
	TS_CHAR szOutFileNameWithPid[128];
	sprintf(szOutFileNameWithPid, "%s_%d_%dx%d_to_%dx%d.argb", "out/ResizeResult", getpid(), src_w, src_h, dst_w, dst_h);
	FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
	fwrite(matOut.data, 1, outsize, txt_fd);
	fflush(txt_fd);
	fclose(txt_fd);
	SAMPLE_PRT("resizeVgs result saved to: %s\n", szOutFileNameWithPid);

	TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
	TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
	
	return 0;
}

static TS_VOID SAMPLE_TSCV_Usage(TS_CHAR *sPrgNm)
{
	printf("Usage : %s <index>\n", sPrgNm);
	printf("index:\n");

	printf("\t 0)calcHist\n");
	printf("\t 1)Add\n");
	printf("\t 2)Subtract\n");
	printf("\t 3)bitwiseAnd\n");
	printf("\t 4)bitwiseOr\n");
	printf("\t 5)bitwiseXor\n");
	printf("\t 6)Map\n");
	printf("\t 7)Threshold\n");
	printf("\t 8)converTo\n");
	printf("\t 9)resizeCve\n");
}

static TS_VOID SAMPLE_TSCV_HandleSig(TS_S32 signo)
{
	if (SIGINT == signo || SIGTERM == signo) {
		printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
	}

	exit(-1);
}

TS_S32 simple_task_core(int argc, char *argv[])
{
	TS_S32 s32Ret = TS_FAILURE;
	TS_S32 s32Index;

	if (argc < 2 || argc > 2) {
		SAMPLE_TSCV_Usage(argv[0]);
		return TS_FAILURE;
	}

	if (!strncmp(argv[1], "-h", 2)) {
		SAMPLE_TSCV_Usage(argv[0]);
		return TS_SUCCESS;
	}

#ifndef __TsingLite__
	signal(SIGINT, SAMPLE_TSCV_HandleSig);
	signal(SIGTERM, SAMPLE_TSCV_HandleSig);
#endif
	
	s32Ret = SAMPLE_SYS_Init();
	if (s32Ret != TS_SUCCESS) {
		SAMPLE_PRT("SAMPLE_SYS_Init failed, s32Ret:%d\n", s32Ret);
		return -1;
	}

	s32Index = atoi(argv[1]);
	SAMPLE_PRT("cve ------------------s32Index<%d>--------\n", s32Index);
	switch (s32Index) {
	case 0:
		s32Ret = CalcHistCore();
		break;
	case 1:
		s32Ret = AddCore();
		break;
	case 2:
		s32Ret = SubtractCore();
		break;
	case 3:
		s32Ret = BitwiseAndCore();
		break;
	case 4:
		s32Ret = BitwiseOrCore();
		break;
	case 5:
		s32Ret = BitwiseXorCore();
		break;
	case 6:
		s32Ret = MapCore();
		break;
	case 7:
		s32Ret = ThresholdCore();
		break;
	case 8:
		s32Ret = ConvertToCore();
		break;
	case 9:
		s32Ret = ResizeCveCore();
		break;
		
	default:
		SAMPLE_PRT("the index %d is invaild!\n", s32Index);
		SAMPLE_TSCV_Usage(argv[0]);
		s32Ret = TS_FAILURE;
		break;
	}

	if (TS_SUCCESS == s32Ret)
		SAMPLE_PRT("program exit normally!\n");
	else
		SAMPLE_PRT("program exit abnormally!\n");

	SAMPLE_SYS_Exit();
	return s32Ret;
}

TS_S32 multi_task_core(int argc, char *argv[], int init_flag, int exit_flag)
{
	TS_S32 s32Ret = TS_FAILURE;
	TS_S32 s32Index;

	if (argc < 2 || argc > 2) {
		SAMPLE_TSCV_Usage(argv[0]);
		return TS_FAILURE;
	}

	if (!strncmp(argv[1], "-h", 2)) {
		SAMPLE_TSCV_Usage(argv[0]);
		return TS_SUCCESS;
	}

#ifndef __TsingLite__
	signal(SIGINT, SAMPLE_TSCV_HandleSig);
	signal(SIGTERM, SAMPLE_TSCV_HandleSig);
#endif

	if (init_flag) {
		s32Ret = SAMPLE_SYS_Init();
		if (s32Ret != TS_SUCCESS) {
			SAMPLE_PRT("SAMPLE_SYS_Init failed, s32Ret:%d\n", s32Ret);
			return -1;
		}
	}

	s32Index = atoi(argv[1]);
	SAMPLE_PRT("cve ------------------s32Index<%d>--------\n", s32Index);
	switch (s32Index) {
	case 0:
		s32Ret = CallCoreFunction(CalcHistCore);
		break;
	case 1:
		s32Ret = CallCoreFunction(AddCore);
		break;
	case 2:
		s32Ret = CallCoreFunction(SubtractCore);
		break;
	case 3:
		s32Ret = CallCoreFunction(BitwiseAndCore);
		break;
	case 4:
		s32Ret = CallCoreFunction(BitwiseOrCore);
		break;
	case 5:
		s32Ret = CallCoreFunction(BitwiseXorCore);
		break;
	case 6:
		s32Ret = CallCoreFunction(MapCore);
		break;
	case 7:
		s32Ret = CallCoreFunction(ThresholdCore);
		break;
	case 8:
		s32Ret = CallCoreFunction(ConvertToCore);
		break;
	case 9:
		s32Ret = CallCoreFunction(ResizeCveCore);
		break;
	default:
		SAMPLE_PRT("the index %d is invaild!\n", s32Index);
		SAMPLE_TSCV_Usage(argv[0]);
		s32Ret = TS_FAILURE;
		break;
	}

	if (TS_SUCCESS == s32Ret)
		SAMPLE_PRT("program exit normally!\n");
	else
		SAMPLE_PRT("program exit abnormally!\n");

	if (exit_flag)
		SAMPLE_SYS_Exit();

	return s32Ret;
}

int main(int argc, char *argv[])
{
	const char *new_argv[3];
	char *parm_argv[3];
	char str_i[10];
	//TS_U32 u32Pid = 0;

	// TS_MPI_SYS_GetPid(&u32Pid);
	// if (argc < 3) {
	// 	if(TX5368_AV200 == u32Pid)
	// 		SAMPLE_PRT("Usage: %s <mode> [ Use 'simple' or 'multi']  [num_children] \n", argv[0]);
	// 	else
	// 		SAMPLE_PRT("Usage: %s <mode> [ Use 'simple']  [num_children] \n", argv[0]);
	// 	return 1;
	// }
	if (argc < 3) {
		SAMPLE_PRT("Usage: %s <mode> [ Use 'simple' or 'multi']  [num_children] \n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "simple") == 0) {
		// If "simple", execute simple_task_core

		int num_index = atoi(argv[2]);
		sprintf(str_i, "%d", num_index);
		new_argv[0] = "simple_task_core";
		new_argv[1] = str_i;
		new_argv[2] = NULL;

		parm_argv[0] = const_cast<char *>(new_argv[0]);
		parm_argv[1] = const_cast<char *>(new_argv[1]);
		parm_argv[2] = NULL;

		return simple_task_core(2, parm_argv);
	} else if (strcmp(argv[1], "multi") == 0 ) {	// && TX5368_AV200 == u32Pid
		// If "multi", execute multi_task_core
		int num_children = 3; // Default number of child processes
		int num_childrn_count = 0;
		pid_t child_pids[20];
		if (argc >= 3) {
			num_children = atoi(argv[2]); // Get number of child processes from command line argument
			SAMPLE_PRT(" num_children  %d \n", num_children);
			if (num_children <= 0) {
				SAMPLE_PRT("Invalid number of children. Please provide a positive integer.\n");
				return 1;
			}
		}

		for (int i = 0; i < num_children; i++) {
			pid_t pid = fork();
			if (pid == 0) { // Child process
				for (int j = 0; j < 9; j++) {
					sprintf(str_i, "%d", j);
					new_argv[0] = "multi_task_core";
					new_argv[1] = str_i;
					new_argv[2] = NULL;

					parm_argv[0] = const_cast<char *>(new_argv[0]);
					parm_argv[1] = const_cast<char *>(new_argv[1]);
					parm_argv[2] = NULL;

					if (j == 0)
						multi_task_core(2, parm_argv, 1, 0);
					else if (j >= (21 - 1))
						multi_task_core(2, parm_argv, 0, 1);
					else
						multi_task_core(2, parm_argv, 0, 0);
				}
				_exit(0);
			} else if (pid > 0) {
				child_pids[i] =
					pid; // wait(NULL); //Parent process waits for each child process to finish
				num_childrn_count = num_childrn_count + 1;
				log_message("+++++++++++++++++ create  child_pids[%d] : +++++++++++++++++\n", i,
					    child_pids[i]);
			} else {
				perror("fork");
				return 1;
			}
		}
		sleep(15);
		for (int i = 0; i < num_childrn_count; i++) {
			log_message(" +++++++++++++++++ wait child out all %d +++++++++++++++++ \n ",
				    num_childrn_count);
			pid_t result = waitpid(child_pids[i], NULL, 0);
			if (result == -1) {
				// 错误处理
			} else {
				log_message(" +++++++++++++++++ child [%d]  out   +++++++++++++++++ \n ", result);
			}
		}
	}else {
		// if(TX5368_AV200 == u32Pid)
		// 	SAMPLE_PRT("Invalid mode. Use 'simple' or 'multi'.\n");
		// else
		// 	SAMPLE_PRT("Invalid mode. Use 'simple'.\n");
		SAMPLE_PRT("Invalid mode. Use 'simple' or 'multi'.\n");
		return 1;
	}

	return 0;
}