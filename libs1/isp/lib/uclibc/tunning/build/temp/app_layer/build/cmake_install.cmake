# Install script for directory: /home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/app_layer/build

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/prebuilts/host/gcc/gcc-ts-10.3-2023.10-x86_64-arm-none-linux-uclibcgnueabihf/bin/arm-ts-linux-uclibcgnueabihf-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/lib/libapp_isp.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/lib" TYPE STATIC_LIBRARY FILES "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/out/usr/lib/libapp_isp.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/app_layer/build/CMakeFiles/app_isp.dir/install-cxx-module-bmi-noconfig.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/out/data/config.txt;/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/out/data/config_dualcam_preview.txt")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/out/data" TYPE FILE FILES
    "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/src//app_layer/config.txt"
    "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/src//app_layer/config_dualcam_preview.txt"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/app_layer/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
