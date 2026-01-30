# Install script for directory: /home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/src/custom/tuning/build

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
    set(CMAKE_INSTALL_CONFIG_NAME "debug")
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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/imx415/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/gc4023/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/sc200ai/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/sc2336p/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/gc2083/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/c3390/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/gc4653/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/gc5603/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/cv4002/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/k17/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/sc430ai/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/sc501ai/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/hi556/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/cv2003/cmake_install.cmake")
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/tuning/sc2337p/cmake_install.cmake")

endif()

