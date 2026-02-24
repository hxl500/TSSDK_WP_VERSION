# Install script for directory: /home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/src/custom/module/build

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/lib/libsen_modules.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/lib" TYPE STATIC_LIBRARY FILES "/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/out/usr/lib/libsen_modules.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/home/hxl/桌面/qi_code/weipai_2025_12_25/tssdk_wp_1225/libs/isp/lib/uclibc/tunning/build/temp/vendor/module/CMakeFiles/sen_modules.dir/install-cxx-module-bmi-debug.cmake" OPTIONAL)
endif()

