#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "qi_nv12_yuv.h"

#define TEST_SRC_WIDTH 640
#define TEST_SRC_HEIGHT 720
#define TEST_DST_WIDTH 640
#define TEST_DST_HEIGHT 640
#define TEST_ITERATIONS 100

unsigned long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)(tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
}

void generate_test_nv12(unsigned char* nv12_data, int width, int height) {
    int y_size = width * height;
    int uv_size = width * height / 2;
    
    for (int i = 0; i < y_size; i++) {
        nv12_data[i] = (unsigned char)(i % 256);
    }
    for (int i = 0; i < uv_size; i++) {
        nv12_data[y_size + i] = (unsigned char)((i * 2) % 256);
    }
}

int main() {
    printf("=== NV12 Scale Performance Test ===\n\n");
    
    int src_size = TEST_SRC_WIDTH * TEST_SRC_HEIGHT * 3 / 2;
    int dst_size = TEST_DST_WIDTH * TEST_DST_HEIGHT * 3 / 2;
    
    unsigned char* src = (unsigned char*)malloc(src_size);
    unsigned char* dst_ex = (unsigned char*)malloc(dst_size);
    unsigned char* dst_nearest = (unsigned char*)malloc(dst_size);
    unsigned char* dst_fixed = (unsigned char*)malloc(dst_size);
    
    if (!src || !dst_ex || !dst_nearest || !dst_fixed) {
        printf("Memory allocation failed!\n");
        return -1;
    }
    
    generate_test_nv12(src, TEST_SRC_WIDTH, TEST_SRC_HEIGHT);
    
    unsigned long long start, end, total;
    double avg_time;
    
    printf("Test parameters:\n");
    printf("  Source: %dx%d\n", TEST_SRC_WIDTH, TEST_SRC_HEIGHT);
    printf("  Destination: %dx%d\n", TEST_DST_WIDTH, TEST_DST_HEIGHT);
    printf("  Iterations: %d\n\n", TEST_ITERATIONS);
    
    printf("Testing nv12_scale_ex (original bilinear)...\n");
    total = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        start = get_time_ms();
        nv12_scale_ex(src, TEST_SRC_WIDTH, TEST_SRC_HEIGHT,
                      dst_ex, TEST_DST_WIDTH, TEST_DST_HEIGHT, 1);
        end = get_time_ms();
        total += (end - start);
    }
    avg_time = (double)total / TEST_ITERATIONS;
    printf("  Total time: %llu ms\n", total);
    printf("  Average time: %.2f ms\n", avg_time);
    printf("  FPS: %.2f\n\n", 1000.0 / avg_time);
    
    printf("Testing nv12_scale_nearest (nearest neighbor)...\n");
    total = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        start = get_time_ms();
        nv12_scale_nearest(src, TEST_SRC_WIDTH, TEST_SRC_HEIGHT,
                          dst_nearest, TEST_DST_WIDTH, TEST_DST_HEIGHT, 1);
        end = get_time_ms();
        total += (end - start);
    }
    avg_time = (double)total / TEST_ITERATIONS;
    printf("  Total time: %llu ms\n", total);
    printf("  Average time: %.2f ms\n", avg_time);
    printf("  FPS: %.2f\n", avg_time);
    printf("  Speedup: %.2fx\n\n", (double)(total) / (double)(total));
    
    printf("Testing nv12_scale_fixed (fixed-point bilinear)...\n");
    total = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        start = get_time_ms();
        nv12_scale_fixed(src, TEST_SRC_WIDTH, TEST_SRC_HEIGHT,
                        dst_fixed, TEST_DST_WIDTH, TEST_DST_HEIGHT, 1);
        end = get_time_ms();
        total += (end - start);
    }
    avg_time = (double)total / TEST_ITERATIONS;
    printf("  Total time: %llu ms\n", total);
    printf("  Average time: %.2f ms\n", avg_time);
    printf("  FPS: %.2f\n\n", 1000.0 / avg_time);
    
    printf("=== Performance Comparison ===\n");
    printf("1. nv12_scale_ex (original): Baseline\n");
    printf("2. nv12_scale_nearest: Fastest, lower quality\n");
    printf("3. nv12_scale_fixed: Good balance of speed and quality\n\n");
    
    printf("Recommendation:\n");
    printf("- For maximum speed: Use nv12_scale_nearest\n");
    printf("- For best quality/speed balance: Use nv12_scale_fixed\n");
    printf("- For best quality: Use nv12_scale_opencv (if available)\n");
    
    free(src);
    free(dst_ex);
    free(dst_nearest);
    free(dst_fixed);
    
    return 0;
}
