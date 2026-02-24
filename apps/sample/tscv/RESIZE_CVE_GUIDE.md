# TSCV resizeCve 缩放 Demo 使用指南

## 概述

本文档介绍了 `sample_tscv_cve.cpp` 中新增的 `resizeCve` 缩放功能，该功能使用 CVE (Computer Vision Engine) 硬件加速进行图像缩放。

## 功能说明

### resizeCve 接口

```cpp
CV_EXPORTS void resizeCve(void* cveHandle, 
                        cv::Mat &src, 
                        cv::Mat &dst, 
                        cv::Size dsize, 
                        tsImgType type = TS_IMGTYPE_YUV420SP_NV12,
                        int interpolation = CV_INTER_LINEAR);
```

**参数说明：**
- `cveHandle`: CVE 句柄，通过 `tscv::createCve()` 创建
- `src`: 源图像 Mat 对象
- `dst`: 目标图像 Mat 对象
- `dsize`: 目标尺寸 (cv::Size)
- `type`: 输入图像格式，默认为 NV12
  - `TS_IMGTYPE_YUV420SP_NV12`: YUV420 半平面 NV12 格式
  - `TS_IMGTYPE_YUV400`: YUV400 灰度格式
  - `TS_IMGTYPE_ARGB8888`: ARGB 8888 格式
  - `TS_IMGTYPE_ABGR8888`: ABGR 8888 格式
  - `TS_IMGTYPE_BGRA8888`: BGRA 8888 格式
  - `TS_IMGTYPE_RGBA8888`: RGBA 8888 格式
- `interpolation`: 插值方式，暂不支持

**注意：**
- 目前该接口为临时接口，仅支持 4 通道格式
- NV12 格式需要特别注意 Mat 的高度是 `height * 3 / 2`

## Demo 代码分析

### ResizeCveCore 函数

```cpp
static TS_S32 ResizeCveCore()
{
    TS_S32 ret = 0;
    
    // 源图像尺寸
    int src_w = 640;
    int src_h = 480;
    
    // 目标图像尺寸
    int dst_w = 320;
    int dst_h = 240;
    
    // 创建 NV12 格式的 Mat 对象
    // NV12 格式：Y 分量 + UV 分量，高度为 height * 3 / 2
    Mat matIn(src_h * 3 / 2, src_w, CV_8UC1);
    Mat matOut(dst_h * 3 / 2, dst_w, CV_8UC1);
    
    // 计算内存大小
    TS_S32 insize = src_w * src_h * 3 / 2;
    TS_S32 outsize = dst_w * dst_h * 3 / 2;
    
    // 分配 MMZ 内存（源图像）
    ret = TS_MPI_SYS_MmzAlloc(&matIn.mpi_phyaddr, 
                            (void**)&matIn.mpi_viraddr, 
                            NULL, NULL, insize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        return -1;
    }

    // 分配 MMZ 内存（目标图像）
    ret = TS_MPI_SYS_MmzAlloc(&matOut.mpi_phyaddr, 
                            (void**)&matOut.mpi_viraddr, 
                            NULL, NULL, outsize);
    if(ret != 0){
        printf("vb create failed , ret=%d, %s %d\n", ret, __func__, __LINE__);
        TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
        return -1;
    }

    // 设置 Mat 数据指针
    matIn.data = (uchar*)matIn.mpi_viraddr;
    matOut.data = (uchar*)matOut.mpi_viraddr;

    // 初始化源图像为灰色 (128)
    matIn.setTo(cv::Scalar(128));

    // 创建 CVE 句柄
    void *pHandleCve = tscv::createCve();
    if(pHandleCve == NULL) {
        SAMPLE_PRT("error, create cve handle failed!\n");
        TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
        TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
        return -1;
    }

    // 打印缩放信息
    SAMPLE_PRT("resizeCve: %dx%d -> %dx%d (NV12 format)\n", 
               src_w, src_h, dst_w, dst_h);
    
    // 执行 10 次缩放操作并测量时间
    for(int i=0; i<10; i++) {
        long t0 = SAMPLE_TSCV_GetTime();
        
        // 调用 resizeCve 进行缩放
        tscv::resizeCve(pHandleCve, 
                       matIn, 
                       matOut, 
                       cv::Size(dst_w, dst_h), 
                       tscv::TS_IMGTYPE_YUV420SP_NV12);
        
        long t1 = SAMPLE_TSCV_GetTime();
        SAMPLE_PRT("================resizeCve time:%ldus\n", t1 - t0);
    }
    
    // 销毁 CVE 句柄
    tscv::exitCve(pHandleCve);

    // 保存结果到文件
    TS_CHAR szOutFileNameWithPid[128];
    sprintf(szOutFileNameWithPid, "%s_%d_%dx%d_to_%dx%d.nv12", 
            "out/ResizeResult", getpid(), src_w, src_h, dst_w, dst_h);
    FILE *txt_fd = fopen(szOutFileNameWithPid, "w");
    fwrite(matOut.data, 1, outsize, txt_fd);
    fflush(txt_fd);
    fclose(txt_fd);
    SAMPLE_PRT("resizeCve result saved to: %s\n", szOutFileNameWithPid);

    // 释放 MMZ 内存
    TS_MPI_SYS_MmzFree(matIn.mpi_phyaddr,(TS_VOID *)matIn.mpi_viraddr);
    TS_MPI_SYS_MmzFree(matOut.mpi_phyaddr,(TS_VOID *)matOut.mpi_viraddr);
    
    return 0;
}
```

## 使用方法

### 编译

```bash
cd /home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/apps/sample/tscv
make sample_tscv_cve
```

### 运行

```bash
# 简单模式
./sample_tscv_cve 9

# 多任务模式
./sample_tscv_cve multi 9
```

### 输出

程序会：
1. 创建 CVE 句柄
2. 分配 MMZ 内存
3. 执行 10 次缩放操作
4. 打印每次操作的时间
5. 保存结果到 `out/ResizeResult_<pid>_<src_w>x<src_h>_to_<dst_w>x<dst_h>.nv12`
6. 释放内存

## NV12 格式说明

### NV12 格式结构

NV12 是一种 YUV420 半平面格式，布局如下：

```
+-------------------+
|                   |
|      Y 分量       |  height
|                   |
+-------------------+
|                   |
|      UV 分量      |  height / 2
|                   |
+-------------------+
```

### NV12 内存计算

- **Y 分量大小**: `width * height`
- **UV 分量大小**: `width * height / 2`
- **总大小**: `width * height * 3 / 2`

### NV12 Mat 创建

```cpp
// 正确的 NV12 Mat 创建方式
Mat nv12_mat(height * 3 / 2, width, CV_8UC1);

// 错误的方式（会导致数据错乱）
Mat nv12_mat(height, width, CV_8UC1);
```

## 支持的图像格式

| 格式 | tsImgType | Mat 高度 | 说明 |
|------|-----------|-----------|------|
| YUV420 NV12 | TS_IMGTYPE_YUV420SP_NV12 | height * 3 / 2 | 最常用的视频格式 |
| YUV400 | TS_IMGTYPE_YUV400 | height | 灰度格式 |
| ARGB8888 | TS_IMGTYPE_ARGB8888 | height | 32 位 ARGB |
| ABGR8888 | TS_IMGTYPE_ABGR8888 | height | 32 位 ABGR |
| BGRA8888 | TS_IMGTYPE_BGRA8888 | height | 32 位 BGRA |
| RGBA8888 | TS_IMGTYPE_RGBA8888 | height | 32 位 RGBA |

## 性能优化建议

### 1. 复用 CVE 句柄

```cpp
// ❌ 不好的方式：每次都创建和销毁
for (int i = 0; i < 1000; i++) {
    void *pHandleCve = tscv::createCve();
    tscv::resizeCve(pHandleCve, src, dst, dsize, type);
    tscv::exitCve(pHandleCve);
}

// ✅ 好的方式：复用 CVE 句柄
void *pHandleCve = tscv::createCve();
for (int i = 0; i < 1000; i++) {
    tscv::resizeCve(pHandleCve, src, dst, dsize, type);
}
tscv::exitCve(pHandleCve);
```

### 2. 复用 MMZ 内存

```cpp
// ❌ 不好的方式：每次都分配和释放
for (int i = 0; i < 1000; i++) {
    TS_MPI_SYS_MmzAlloc(&phy, &vir, NULL, NULL, size);
    // 处理...
    TS_MPI_SYS_MmzFree(phy, vir);
}

// ✅ 好的方式：预先分配，复用内存
TS_MPI_SYS_MmzAlloc(&phy, &vir, NULL, NULL, size);
for (int i = 0; i < 1000; i++) {
    // 处理...
}
TS_MPI_SYS_MmzFree(phy, vir);
```

### 3. 批量处理

```cpp
// ✅ 批量处理减少函数调用开销
for (int i = 0; i < 1000; i += 10) {
    for (int j = 0; j < 10 && i + j < 1000; j++) {
        tscv::resizeCve(pHandleCve, src[i+j], dst[i+j], dsize, type);
    }
}
```

## 与其他缩放方式的对比

| 方式 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| resizeCve | 硬件加速，速度快 | 仅支持特定格式 | 需要高性能的视频处理 |
| resizeVpss | 功能全面，支持多种格式 | 软件实现，速度较慢 | 通用图像处理 |
| OpenCV resize | 功能最全面 | 软件实现，速度最慢 | 通用图像处理 |

## 常见问题

### 1. 内存分配失败

**问题：** `vb create failed`

**原因：** MMZ 内存不足

**解决：**
- 检查系统内存使用情况
- 减小图像尺寸
- 减少并发任务数量

### 2. CVE 句柄创建失败

**问题：** `error, create cve handle failed!`

**原因：** CVE 硬件资源不足

**解决：**
- 检查是否有其他进程占用 CVE
- 减少并发任务数量
- 重启系统

### 3. 缩放结果异常

**问题：** 输出图像颜色异常或错乱

**原因：** NV12 Mat 高度设置错误

**解决：**
```cpp
// ✅ 正确：NV12 格式高度为 height * 3 / 2
Mat nv12_mat(height * 3 / 2, width, CV_8UC1);

// ❌ 错误：高度为 height
Mat nv12_mat(height, width, CV_8UC1);
```

## 示例输出

```
[ResizeCveCore]-637: resizeCve: 640x480 -> 320x240 (NV12 format)
[ResizeCveCore]-681: ================resizeCve time:1234us
[ResizeCveCore]-681: ================resizeCve time:1245us
[ResizeCveCore]-681: ================resizeCve time:1238us
[ResizeCveCore]-681: ================resizeCve time:1241us
[ResizeCveCore]-681: ================resizeCve time:1239us
[ResizeCveCore]-681: ================resizeCve time:1243us
[ResizeCveCore]-681: ================resizeCve time:1240us
[ResizeCveCore]-681: ================resizeCve time:1242us
[ResizeCveCore]-681: ================resizeCve time:1237us
[ResizeCveCore]-681: ================resizeCve time:1244us
[ResizeCveCore]-690: resizeCve result saved to: out/ResizeResult_12345_640x480_to_320x240.nv12
[ResizeCveCore]-691: program exit normally!
```

## 总结

`resizeCve` 提供了基于 CVE 硬件加速的高性能图像缩放功能：

1. ✅ **硬件加速**：使用 CVE 硬件，性能优于软件实现
2. ✅ **支持 NV12**：支持常用的 NV12 视频格式
3. ✅ **易于使用**：接口简洁，参数清晰
4. ✅ **性能可测**：Demo 包含性能测试代码

**使用建议：**
- 优先使用 `resizeCve` 进行视频缩放
- 复用 CVE 句柄和 MMZ 内存以提升性能
- 注意 NV12 格式的 Mat 高度设置
- 根据实际需求选择合适的图像格式
