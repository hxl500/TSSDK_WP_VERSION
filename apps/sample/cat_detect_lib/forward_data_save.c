// #include "forward_data_save.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>



// int save_blob_info_text(RNE_BLOBS_S* outputBlobs, const char* filepath) {
//     if (!outputBlobs || !outputBlobs->stpBlob || outputBlobs->u32NBlob<= 0) {
//         printf("Error: Invalid outputBlobs\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "w");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     fprintf(fp, "=== Blob Information ===\n\n");
//     fprintf(fp, "Total Blobs: %d\n\n", outputBlobs->u32NBlob);

//     for (int i = 0; i < outputBlobs->u32NBlob; i++) {
//         int c_align = outputBlobs->stpBlob[i].s32C;
        
//         fprintf(fp, "u32NBlob:%d H: %d, W:%d, C:%d, Cstride:%d, coeff: %f\n", 
//                i, 
//                outputBlobs->stpBlob[i].s32H, 
//                outputBlobs->stpBlob[i].s32W, 
//                outputBlobs->stpBlob[i].s32C, 
//                c_align, 
//                outputBlobs->stpBlob[i].fCoeff[0]);
//     }

//     fclose(fp);
//     printf("Saved blob info to %s\n", filepath);
//     return 0;
// }

// int save_blob_info_csv(RNE_BLOBS_S* outputBlobs, const char* filepath) {
//     if (!outputBlobs || !outputBlobs->stpBlob || outputBlobs->u32NBlob <= 0) {
//         printf("Error: Invalid outputBlobs\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "w");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     fprintf(fp, "u32NBlob,H,W,C,Cstride,coeff\n");

//     for (int i = 0; i < outputBlobs->u32NBlob; i++) {
//         int c_align = outputBlobs->stpBlob[i].s32C;
        
//         fprintf(fp, "%d,%d,%d,%d,%d,%f\n", 
//                i, 
//                outputBlobs->stpBlob[i].s32H, 
//                outputBlobs->stpBlob[i].s32W, 
//                outputBlobs->stpBlob[i].s32C, 
//                c_align, 
//                outputBlobs->stpBlob[i].fCoeff[0]);
//     }

//     fclose(fp);
//     printf("Saved blob info CSV to %s\n", filepath);
//     return 0;
// }

// int save_blob_data_binary(RNE_BLOBS_S* outputBlobs, const char* output_dir) {
//     if (!outputBlobs || !outputBlobs->stpBlob || outputBlobs->u32NBlob <= 0) {
//         printf("Error: Invalid outputBlobs\n");
//         return -1;
//     }

//     for (int i = 0; i < outputBlobs->u32NBlob; i++) {
//         char filepath[512];
//         snprintf(filepath, sizeof(filepath), "%s/blob%d_data.bin", output_dir, i);

//         FILE* fp = fopen(filepath, "wb");
//         if (!fp) {
//             printf("Error: Failed to open %s\n", filepath);
//             continue;
//         }

//         size_t written = fwrite(outputBlobs->stpBlob[i].vpAddr, 
//                               1, 
//                               outputBlobs->stpBlob[i].u32Size, 
//                               fp);
//         fclose(fp);

//         printf("Saved blob %d data to %s (%zu bytes)\n", i, filepath, written);
//     }

//     return 0;
// }

// int save_blob_data_text(RNE_BLOBS_S* outputBlobs, const char* output_dir) {
//     if (!outputBlobs || !outputBlobs->stpBlob || outputBlobs->u32NBlob <= 0) {
//         printf("Error: Invalid outputBlobs\n");
//         return -1;
//     }

//     for (int i = 0; i < outputBlobs->u32NBlob; i++) {
//         char filepath[512];
//         snprintf(filepath, sizeof(filepath), "%s/blob%d_data.txt", output_dir, i);

//         FILE* fp = fopen(filepath, "w");
//         if (!fp) {
//             printf("Error: Failed to open %s\n", filepath);
//             continue;
//         }

//         fprintf(fp, "Blob %d Data\n", i);
//         fprintf(fp, "H: %d, W: %d, C: %d\n", 
//                outputBlobs->stpBlob[i].s32H,
//                outputBlobs->stpBlob[i].s32W,
//                outputBlobs->stpBlob[i].s32C);
//         fprintf(fp, "Size: %d bytes\n\n", outputBlobs->stpBlob[i].u32Size);

//         float* data = (float*)outputBlobs->stpBlob[i].vpAddr;
//         int total_elements = outputBlobs->stpBlob[i].u32Size / sizeof(float);
//         int display_count = total_elements > 1000 ? 1000 : total_elements;

//         fprintf(fp, "Data (first %d values):\n", display_count);
//         for (int j = 0; j < display_count; j++) {
//             fprintf(fp, "[%d]: %.6f\n", j, data[j]);
//         }

//         fclose(fp);
//         printf("Saved blob %d text to %s\n", i, filepath);
//     }

//     return 0;
// }

// int save_blob_complete(RNE_BLOBS_S* outputBlobs, const char* output_dir) {
//     if (!outputBlobs || !outputBlobs->stpBlob || outputBlobs->u32NBlob <= 0) {
//         printf("Error: Invalid outputBlobs\n");
//         return -1;
//     }

//     printf("\n=== Saving Complete Blob Information ===\n");
//     printf("Total Blobs: %d\n", outputBlobs->u32NBlob);
//     printf("Output Directory: %s\n\n", outputBlobs);

//     char filepath[512];

//     snprintf(filepath, sizeof(filepath), "%s/blob_info.txt", output_dir);
//     save_blob_info_text(outputBlobs, filepath);

//     snprintf(filepath, sizeof(filepath), "%s/blob_info.csv", output_dir);
//     save_blob_info_csv(outputBlobs, filepath);

//     save_blob_data_binary(outputBlobs, output_dir);
//     save_blob_data_text(outputBlobs, output_dir);

//     printf("\n=== Save Complete ===\n");
//     return 0;
// }

// // int main() {
// //     printf("=== Blob Save Format Implementation ===\n\n");

// //     int blob_count = 3;
// //     RNE_BLOB_S blobs[3];

// //     blobs[0].s32W = 640;
// //     blobs[0].s32H = 720;
// //     blobs[0].s32C = 3;
// //     blobs[0].s32Stride = 640 * 720 * sizeof(float);
// //     blobs[0].u32Size = 640 * 720 * 3 * sizeof(float);
// //     blobs[0].fCoeff[0] = 0.5f;
// //     blobs[0].virAddr = malloc(blobs[0].u32Size);

// //     blobs[1].s32W = 320;
// //     blobs[1].s32H = 360;
// //     blobs[1].s32C = 64;
// //     blobs[1].s32Stride = 320 * 360 * sizeof(float);
// //     blobs[1].u32Size = 320 * 360 * 64 * sizeof(float);
// //     blobs[1].fCoeff[0] = 0.25f;
// //     blobs[1].virAddr = malloc(blobs[1].u32Size);

// //     blobs[2].s32W = 160;
// //     blobs[2].s32H = 180;
// //     blobs[2].s32C = 128;
// //     blobs[2].s32Stride = 160 * 180 * sizeof(float);
// //     blobs[2].u32Size = 160 * 180 * 128 * sizeof(float);
// //     blobs[2].fCoeff[0] = 0.125f;
// //     blobs[2].virAddr = malloc(blobs[2].u32Size);

// //     for (int i = 0; i < blob_count; i++) {
// //         float* data = (float*)blobs[i].virAddr;
// //         int elements = blobs[i].u32Size / sizeof(float);
// //         for (int j = 0; j < elements; j++) {
// //             data[j] = (float)j / 10000.0f;
// //         }
// //     }

// //     RNE_BLOBS_S outputBlobs;
// //     outputBlobs.stpBlob = blobs;
// //     outputBlobs.s32BlobNum = blob_count;

// //     printf("=== Print Format Example ===\n\n");

// //     for (int i = 0; i < outputBlobs.s32BlobNum; i++) {
// //         int c_align = outputBlobs.stpBlob[i].s32C;
// //         printf("u32NBlob:%d H: %d, W:%d, C:%d, Cstride:%d, coeff: %f\n", 
// //                i, 
// //                outputBlobs.stpBlob[i].s32H, 
// //                outputBlobs.stpBlob[i].s32W, 
// //                outputBlobs.stpBlob[i].s32C, 
// //                c_align, 
// //                outputBlobs.stpBlob[i].fCoeff[0]);
// //     }

// //     printf("\n=== Usage Example ===\n\n");

// //     printf("1. Save blob info to text file:\n");
// //     printf("   save_blob_info_text(&outputBlobs, \"blob_info.txt\");\n\n");

// //     printf("2. Save blob info to CSV file:\n");
// //     printf("   save_blob_info_csv(&outputBlobs, \"blob_info.csv\");\n\n");

// //     printf("3. Save blob data to binary files:\n");
// //     printf("   save_blob_data_binary(&outputBlobs, \"./output\");\n\n");

// //     printf("4. Save blob data to text files:\n");
// //     printf("   save_blob_data_text(&outputBlobs, \"./output\");\n\n");

// //     printf("5. Save complete blob information:\n");
// //     printf("   save_blob_complete(&outputBlobs, \"./output\");\n\n");

// //     save_blob_complete(&outputBlobs, "./output");

// //     for (int i = 0; i < blob_count; i++) {
// //         free(blobs[i].virAddr);
// //     }

// //     return 0;
// // }

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <time.h>
// #include <sys/stat.h>

// typedef int TS_S32;
// typedef char TS_CHAR;

// // typedef struct {
// //     void* virAddr;
// //     void* phyAddr;
// //     int s32W;
// //     int s32H;
// //     int s32C;
// //     int s32Stride;
// //     int u32Size;
// //     float fCoeff[16];
// // } RNE_BLOB_S;

// // typedef struct {
// //     RNE_BLOB_S* stpBlob;
// //     int s32BlobNum;
// // } RNE_BLOBS_S;

// typedef enum {
//     SAVE_FORMAT_BINARY = 0,
//     SAVE_FORMAT_TEXT = 1,
//     SAVE_FORMAT_CSV = 2,
//     SAVE_FORMAT_HEADER = 3
// } SaveFormat;

// typedef struct {
//     TS_CHAR* debugLayerName;
//     TS_CHAR* outputPath;
//     TS_S32 frameIndex;
//     TS_S32 frameCount;
//     SaveFormat format;
//     int verbose;
// } SaveConfig;

// TS_S32 create_directory(const TS_CHAR* path) {
//     struct stat st = {0};
//     if (stat(path, &st) == -1) {
// #ifdef _WIN32
//         _mkdir(path);
// #else
//         mkdir(path, 0755);
// #endif
//         return 0;
//     }
//     return 0;
// }

// TS_S32 get_file_path(TS_CHAR* filepath, int size, const TS_CHAR* outputPath, 
//                      const TS_CHAR* debugLayerName, TS_S32 frameIndex, 
//                      const TS_CHAR* extension) {
//     if (outputPath && strlen(outputPath) > 0) {
//         snprintf(filepath, size, "%s/%s_frame%d.%s", 
//                 outputPath, debugLayerName, frameIndex, extension);
//     } else {
//         snprintf(filepath, size, "%s_frame%d.%s", 
//                 debugLayerName, frameIndex, extension);
//     }
//     return 0;
// }

// TS_S32 save_blob_binary(RNE_BLOB_S* blob, const TS_CHAR* filepath) {
//     if (!blob || !blob->vpAddr || blob->u32Size <= 0) {
//         printf("Error: Invalid blob\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "wb");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     size_t written = fwrite(blob->vpAddr, 1, blob->u32Size, fp);
//     fclose(fp);

//     if (written != blob->u32Size) {
//         printf("Warning: Only wrote %zu bytes, expected %d bytes\n", 
//                written, blob->u32Size);
//         return -1;
//     }

//     return 0;
// }

// TS_S32 save_blob_text(RNE_BLOB_S* blob, const TS_CHAR* filepath) {
//     if (!blob || !blob->vpAddr || blob->u32Size <= 0) {
//         printf("Error: Invalid blob\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "w");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     fprintf(fp, "=== Blob Information ===\n");
//     fprintf(fp, "Width: %d\n", blob->s32W);
//     fprintf(fp, "Height: %d\n", blob->s32H);
//     fprintf(fp, "Channels: %d\n", blob->s32C);
//     //fprintf(fp, "Stride: %d\n", blob->s32Stride);
//     fprintf(fp, "Size: %d bytes\n\n", blob->u32Size);

//     fprintf(fp, "=== Data (first 1000 values) ===\n");
//     float* data = (float*)blob->vpAddr;
//     int total_elements = blob->u32Size / sizeof(float);
//     int display_count = total_elements > 1000 ? 1000 : total_elements;

//     for (int i = 0; i < display_count; i++) {
//         fprintf(fp, "[%d]: %.6f\n", i, data[i]);
//     }

//     fclose(fp);
//     return 0;
// }

// TS_S32 save_blob_csv(RNE_BLOB_S* blob, const TS_CHAR* filepath) {
//     if (!blob || !blob->vpAddr || blob->u32Size <= 0) {
//         printf("Error: Invalid blob\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "w");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     fprintf(fp, "Width,Height,Channels,Size\n");
//     fprintf(fp, "%d,%d,%d,%d\n\n", 
//            blob->s32W, blob->s32H, blob->s32C, blob->u32Size);

//     fprintf(fp, "Index,Value\n");
//     float* data = (float*)blob->vpAddr;
//     int total_elements = blob->u32Size / sizeof(float);
//     int display_count = total_elements > 1000 ? 1000 : total_elements;

//     for (int i = 0; i < display_count; i++) {
//         fprintf(fp, "%d,%.6f\n", i, data[i]);
//     }

//     fclose(fp);
//     return 0;
// }

// TS_S32 save_blob_header(RNE_BLOB_S* blob, const TS_CHAR* filepath) {
//     if (!blob || !blob->vpAddr || blob->u32Size <= 0) {
//         printf("Error: Invalid blob\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "wb");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     int header[4];
//     header[0] = blob->s32W;
//     header[1] = blob->s32H;
//     header[2] = blob->s32C;
//     //header[3] = blob->s32Stride;
//     header[3] = blob->u32Size;

//     fwrite(header, sizeof(int), 4, fp);
//     fwrite(blob->vpAddr, 1, blob->u32Size, fp);

//     fclose(fp);
//     return 0;
// }

// TS_S32 save_blobs_list(RNE_BLOBS_S* blobs, const TS_CHAR* filepath) {
//     if (!blobs || !blobs->stpBlob || blobs->u32NBlob <= 0) {
//         printf("Error: Invalid blobs\n");
//         return -1;
//     }

//     FILE* fp = fopen(filepath, "wb");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     fwrite(&blobs->u32NBlob, sizeof(int), 1, fp);

//     for (int i = 0; i < blobs->u32NBlob; i++) {
//         RNE_BLOB_S* blob = &blobs->stpBlob[i];
//         int header[4];
//         header[0] = blob->s32W;
//         header[1] = blob->s32H;
//         header[2] = blob->s32C;
//         //header[3] = blob->s32Stride;
//         header[3] = blob->u32Size;

//         fwrite(header, sizeof(int), 4, fp);
//         fwrite(blob->vpAddr, 1, blob->u32Size, fp);
//     }

//     fclose(fp);
//     return 0;
// }

// TS_S32 TS_ALG_RNE_RecordFile(RNE_BLOBS_S *blobs, TS_CHAR *debugLayerName, 
//                               TS_CHAR *outputPath, TS_S32 frameIndex, 
//                               TS_S32 frameCount) {
//     if (!blobs || !blobs->stpBlob || blobs->u32NBlob <= 0) {
//         printf("Error: Invalid blobs parameter\n");
//         return -1;
//     }

//     if (!debugLayerName || strlen(debugLayerName) == 0) {
//         printf("Error: Invalid debugLayerName\n");
//         return -1;
//     }

//     if (outputPath && strlen(outputPath) > 0) {
//         create_directory(outputPath);
//     }

//     TS_CHAR filepath[512];
//     get_file_path(filepath, sizeof(filepath), outputPath, debugLayerName, 
//                 frameIndex, "bin");

//     FILE* fp = fopen(filepath, "wb");
//     if (!fp) {
//         printf("Error: Failed to open %s\n", filepath);
//         return -1;
//     }

//     for (int i = 0; i < blobs->u32NBlob; i++) {
//         RNE_BLOB_S* blob = &blobs->stpBlob[i];
        
//         if (!blob->vpAddr || blob->u32Size <= 0) {
//             printf("Warning: Invalid blob at index %d\n", i);
//             continue;
//         }

//         size_t written = fwrite(blob->vpAddr, 1, blob->u32Size, fp);
//         if (written != blob->u32Size) {
//             printf("Warning: Only wrote %zu bytes, expected %d bytes for blob %d\n", 
//                    written, blob->u32Size, i);
//         }
//     }

//     fclose(fp);
//     printf("Successfully saved %d blobs to %s\n", blobs->u32NBlob, filepath);
    
//     return 0;
// }

// TS_S32 TS_ALG_RNE_RecordFileEx(RNE_BLOBS_S *blobs, TS_CHAR *debugLayerName, 
//                                 TS_CHAR *outputPath, TS_S32 frameIndex, 
//                                 TS_S32 frameCount, SaveFormat format) {
//     if (!blobs || !blobs->stpBlob || blobs->u32NBlob <= 0) {
//         printf("Error: Invalid blobs parameter\n");
//         return -1;
//     }

//     if (!debugLayerName || strlen(debugLayerName) == 0) {
//         printf("Error: Invalid debugLayerName\n");
//         return -1;
//     }

//     if (outputPath && strlen(outputPath) > 0) {
//         create_directory(outputPath);
//     }

//     TS_CHAR filepath[512];
//     const TS_CHAR* extension;

//     switch (format) {
//         case SAVE_FORMAT_TEXT:
//             extension = "txt";
//             break;
//         case SAVE_FORMAT_CSV:
//             extension = "csv";
//             break;
//         case SAVE_FORMAT_HEADER:
//             extension = "hdr";
//             break;
//         case SAVE_FORMAT_BINARY:
//         default:
//             extension = "bin";
//             break;
//     }

//     get_file_path(filepath, sizeof(filepath), outputPath, debugLayerName, 
//                 frameIndex, extension);

//     if (blobs->u32NBlob == 1) {
//         RNE_BLOB_S* blob = &blobs->stpBlob[0];
//         switch (format) {
//             case SAVE_FORMAT_TEXT:
//                 return save_blob_text(blob, filepath);
//             case SAVE_FORMAT_CSV:
//                 return save_blob_csv(blob, filepath);
//             case SAVE_FORMAT_HEADER:
//                 return save_blob_header(blob, filepath);
//             case SAVE_FORMAT_BINARY:
//             default:
//                 return save_blob_binary(blob, filepath);
//         }
//     } else {
//         return save_blobs_list(blobs, filepath);
//     }
// }

// TS_S32 TS_ALG_RNE_RecordFileWithConfig(RNE_BLOBS_S *blobs, SaveConfig* config) {
//     if (!config) {
//         return -1;
//     }
//     return TS_ALG_RNE_RecordFileEx(blobs, config->debugLayerName, config->outputPath,
//                                    config->frameIndex, config->frameCount, config->format);
// }

// // int main() {
// //     printf("=== RNE_RecordFile Open Source Implementation ===\n\n");

// //     int blob_count = 3;
// //     RNE_BLOB_S blobs[3];

// //     blobs[0].s32W = 640;
// //     blobs[0].s32H = 720;
// //     blobs[0].s32C = 3;
// //     blobs[0].s32Stride = 640 * 720 * sizeof(float);
// //     blobs[0].u32Size = 640 * 720 * 3 * sizeof(float);
// //     blobs[0].fCoeff[0] = 0.5f;
// //     blobs[0].virAddr = malloc(blobs[0].u32Size);

// //     blobs[1].s32W = 320;
// //     blobs[1].s32H = 360;
// //     blobs[1].s32C = 64;
// //     blobs[1].s32Stride = 320 * 360 * sizeof(float);
// //     blobs[1].u32Size = 320 * 360 * 64 * sizeof(float);
// //     blobs[1].fCoeff[0] = 0.25f;
// //     blobs[1].virAddr = malloc(blobs[1].u32Size);

// //     blobs[2].s32W = 160;
// //     blobs[2].s32H = 180;
// //     blobs[2].s32C = 128;
// //     blobs[2].s32Stride = 160 * 180 * sizeof(float);
// //     blobs[2].u32Size = 160 * 180 * 128 * sizeof(float);
// //     blobs[2].fCoeff[0] = 0.125f;
// //     blobs[2].virAddr = malloc(blobs[2].u32Size);

// //     for (int i = 0; i < blob_count; i++) {
// //         float* data = (float*)blobs[i].virAddr;
// //         int elements = blobs[i].u32Size / sizeof(float);
// //         for (int j = 0; j < elements; j++) {
// //             data[j] = (float)j / 10000.0f;
// //         }
// //     }

// //     RNE_BLOBS_S outputBlobs;
// //     outputBlobs.stpBlob = blobs;
// //     outputBlobs.s32BlobNum = blob_count;

// //     printf("=== Basic Usage ===\n\n");
// //     printf("1. Save to binary format (default):\n");
// //     printf("   TS_ALG_RNE_RecordFile(&outputBlobs, \"output\", \"./output\", 0, 1);\n\n");

// //     TS_ALG_RNE_RecordFile(&outputBlobs, "output", "./output", 0, 1);

// //     printf("\n2. Save to text format:\n");
// //     printf("   TS_ALG_RNE_RecordFileEx(&outputBlobs, \"output\", \"./output\", 0, 1, SAVE_FORMAT_TEXT);\n\n");

// //     TS_ALG_RNE_RecordFileEx(&outputBlobs, "output", "./output", 1, 1, SAVE_FORMAT_TEXT);

// //     printf("\n3. Save to CSV format:\n");
// //     printf("   TS_ALG_RNE_RecordFileEx(&outputBlobs, \"output\", \"./output\", 2, 1, SAVE_FORMAT_CSV);\n\n");

// //     TS_ALG_RNE_RecordFileEx(&outputBlobs, "output", "./output", 2, 1, SAVE_FORMAT_CSV);

// //     printf("\n4. Save with header:\n");
// //     printf("   TS_ALG_RNE_RecordFileEx(&outputBlobs, \"output\", \"./output\", 3, 1, SAVE_FORMAT_HEADER);\n\n");

// //     TS_ALG_RNE_RecordFileEx(&outputBlobs, "output", "./output", 3, 1, SAVE_FORMAT_HEADER);

// //     printf("\n5. Save with config:\n");
// //     printf("   SaveConfig config;\n");
// //     printf("   config.debugLayerName = \"output\";\n");
// //     printf("   config.outputPath = \"./output\";\n");
// //     printf("   config.frameIndex = 4;\n");
// //     printf("   config.frameCount = 1;\n");
// //     printf("   config.format = SAVE_FORMAT_BINARY;\n");
// //     printf("   TS_ALG_RNE_RecordFileWithConfig(&outputBlobs, &config);\n\n");

// //     SaveConfig config;
// //     config.debugLayerName = "output";
// //     config.outputPath = "./output";
// //     config.frameIndex = 4;
// //     config.frameCount = 1;
// //     config.format = SAVE_FORMAT_BINARY;
// //     config.verbose = 1;

// //     TS_ALG_RNE_RecordFileWithConfig(&outputBlobs, &config);

// //     printf("\n6. Save multiple frames:\n");
// //     printf("   for (int i = 0; i < 10; i++) {\n");
// //     printf("       TS_ALG_RNE_RecordFile(&outputBlobs, \"frame\", \"./frames\", i, 10);\n");
// //     printf("   }\n\n");

// //     for (int i = 0; i < 5; i++) {
// //         TS_ALG_RNE_RecordFile(&outputBlobs, "frame", "./frames", i, 5);
// //     }

// //     for (int i = 0; i < blob_count; i++) {
// //         free(blobs[i].virAddr);
// //     }

// //     printf("\n=== Features ===\n");
// //     printf("- Support multiple formats: binary, text, CSV, header\n");
// //     printf("- Support single blob and multiple blobs\n");
// //     printf("- Support multiple frames\n");
// //     printf("- Automatic directory creation\n");
// //     printf("- Memory safety checks\n");
// //     printf("- Detailed error messages\n");

// //     return 0;
// // }

