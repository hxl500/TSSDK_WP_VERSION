set (SAMPLE_COMMON_DIR      ${APP_SRC_DIR}/common)
set (VP_VIDEO_CODER_DIR     ${APP_SRC_DIR}/vp_video_coder)
set (VP_COMM_DIR            ${APP_SRC_DIR}/vp_comm)

add_definitions(-DSENSOR0_TYPE=EVB_SENSOR)

set(VP_VIDEO_CODER_TEST vp_video_coder_test)

# set(ALGO_LIB 
# 	trp
# 	algmotiondetect
# 	algpcppdetect
# 	cat_detect_lib
# )

include_directories(
    ${VP_VIDEO_CODER_DIR}
    ${VP_VIDEO_CODER_DIR}/vp_osd
    ${VP_VIDEO_CODER_DIR}/qi_alg
    ${VP_COMM_DIR}/inc/vp_tools
    ${VP_COMM_DIR}/inc/vp_printf
    ${VP_COMM_DIR}/inc/vp_events
    ${VP_COMM_DIR}/inc/vp_context
    ${VP_COMM_DIR}/inc/vp_observers
)

link_directories(
    ${VP_COMM_DIR}/lib/vp_tools
    ${VP_COMM_DIR}/lib/vp_printf
    ${VP_COMM_DIR}/lib/vp_events
    ${VP_COMM_DIR}/lib/vp_context
    ${VP_COMM_DIR}/lib/vp_observers
)

add_executable(
    ${VP_VIDEO_CODER_TEST}
    ${SAMPLE_COMMON_DIR}/sample_comm_isp.c
    ${SAMPLE_COMMON_DIR}/sample_comm_sys.c
    ${SAMPLE_COMMON_DIR}/sample_comm_vi.c
    ${SAMPLE_COMMON_DIR}/sample_comm_venc.c
    ${SAMPLE_COMMON_DIR}/sample_comm_vpss.c
    ${SAMPLE_COMMON_DIR}/sample_comm_cpm.c
    ${SAMPLE_COMMON_DIR}/soft_line.c
    ${VP_VIDEO_CODER_DIR}/vp_video_encoder.c
    ${VP_VIDEO_CODER_DIR}/vp_video_osd.c
    ${VP_VIDEO_CODER_DIR}/soft_alphabet_overlay.c
    ${VP_VIDEO_CODER_DIR}/test.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/lv_font_fmt_txt.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/lv_font.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/lv_txt.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/lv_utils.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_8.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_12.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_20.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_24.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_32.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_40.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_64.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font_80.c
    ${VP_VIDEO_CODER_DIR}/vp_osd/vp_font.c
    ${VP_VIDEO_CODER_DIR}/qi_alg/sample_alg_cpm.c
    ${VP_VIDEO_CODER_DIR}/qi_alg/video_alg_catdetect.c
    # ${VP_VIDEO_CODER_DIR}/vp_isp_ts.c
)

TARGET_LINK_LIBRARIES(${VP_VIDEO_CODER_TEST} -Wl,--start-group dl pthread stdc++ 
    libvp_printf.a
    libvp_events.a
    libvp_tools.a
    libvp_observers.a
    libtrp.a 
    libcat_detect_lib.a
     libalgcommon.a
     libalgutil_cv.a
	#libplat_rne_linux_a53.a libchip_sdk.a 
	 librne_rt_g3.a
	 librne_pal_linux.a
	 librne_common.a
    ${MPI_BASE_LIB} -Wl,--end-group)
