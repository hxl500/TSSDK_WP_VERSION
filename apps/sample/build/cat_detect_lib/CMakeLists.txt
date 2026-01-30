set(SAMPLE_LIB_CATDETECT cat_detect_lib)
set (SAMPLE_CATDETECT_DIR ${APP_SRC_DIR}/cat_detect_lib)

set(START_GRP "-Wl,--start-group")
set(END_GRP "-Wl,--end-group")

message("SOLIBPATH = $ENV{TARGET_STAGING_LIB}")
link_directories($ENV{TARGET_STAGING_LIB})

include_directories($ENV{TARGET_STAGING_INC}/opencv4)
link_directories($ENV{TARGET_STAGING_LIB}/opencv4/3rdparty)
 
include_directories(${APP_SRC_DIR}/../../libs/algstore/libalgutil_cv/include)
include_directories(${APP_SRC_DIR}/../../libs/algstore/libalgutil_cv/lib/uclibc)
 
include_directories(${APP_SRC_DIR}/../../libs/trp/rne/include)
include_directories(${APP_SRC_DIR}/../../trp/rne/lib/uclibc)

include_directories(${SAMPLE_CATDETECT_DIR})
include_directories(${SAMPLE_CATDETECT_DIR}/record_file)

#include_directories($ENV{TARGET_STAGING_INC}/opencv4)
#link_directories($ENV{TARGET_STAGING_LIB}/opencv4/3rdparty)
#include_directories(${APP_SRC_DIR}/tsalg/alg_bodydetect/include)
#include_directories(${APP_SRC_DIR}/tsalg/alg_bodydetect/model)
#include_directories(${APP_SRC_DIR}/../../libs/algstore/libalgbodydetect/include)
#include_directories(${APP_SRC_DIR}/../../libs/algstore/libalgcommon/include)
#include_directories(${APP_SRC_DIR}/tsalg_lib)
#link_directories(${APP_SRC_DIR}/temp/tsalg_lib)

add_library(${SAMPLE_LIB_CATDETECT} STATIC
	${SAMPLE_CATDETECT_DIR}/lib_ts_alg_body_detect.cpp
	${SAMPLE_CATDETECT_DIR}/lib_video_alg_catdetect.c 
	${SAMPLE_CATDETECT_DIR}/lib_tsalg_alg_lib.c
	${SAMPLE_CATDETECT_DIR}/lib_tsalg_alg_rsn.c
	${SAMPLE_CATDETECT_DIR}/lib_food_rsn.c
	${SAMPLE_CATDETECT_DIR}/lib_array_diff.c
	${SAMPLE_CATDETECT_DIR}/lib_act.c
	${SAMPLE_CATDETECT_DIR}/lib_dirsync.c
	${SAMPLE_CATDETECT_DIR}/lib_file_sync.c
	${SAMPLE_CATDETECT_DIR}/lib_iot_net.c
	${SAMPLE_CATDETECT_DIR}/forward_data_save.c
	${SAMPLE_CATDETECT_DIR}/qi_nv12_yuv.cpp
	${SAMPLE_CATDETECT_DIR}/record_file/ts_rne_record_file.c
	 
)

TARGET_LINK_LIBRARIES(${SAMPLE_LIB_CATDETECT} libalgcommon.a)
#TARGET_LINK_LIBRARIES(${SAMPLE_LIB_CATDETECT} libalgfiresmokedetect.a)
TARGET_LINK_LIBRARIES(${SAMPLE_LIB_CATDETECT} -Wl,--start-group dl pthread stdc++  libalgcommon.a
		mpi
		libalgutil_cv.a
		librne_rt_g3.a
	librne_pal_linux.a
	librne_common.a
	${ALGO_LIB}
		libtrp.a
 -Wl,--end-group)

#set_target_properties(cat_detect_lib PROPERTIES
#    ARCHIVE_OUTPUT_DIRECTORY "${APP_SRC_DIR}/tsalg_video"
#)
set_target_properties(cat_detect_lib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${APP_SRC_DIR}/../../libs/algstore/libalgmotiondetect/lib/uclibc"
)