cmake_minimum_required(VERSION 3.18.0)
project(FileDisguise LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Win MSVC下需要手动指定源码编码类型
if (MSVC)
    add_compile_options("/source-charset:utf-8")
endif()

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "设置构建类型为Debug")
endif()
# if("${CMAKE_BUILD_TYPE}" STREQUAL "")
#     set(CMAKE_BUILD_TYPE Debug)
#     message(STATUS "设置构建类型为Debug")
# endif()
message(STATUS "构建类型：${CMAKE_BUILD_TYPE}")

# 初始化OpenCV文件夹
if(NOT ZWN_OPENCV_DIR)
    set(ZWN_OPENCV_DIR "D:\\ProgramFiles\\Dev\\OpenCV\\opencv_v4.7.0\\build")
    message(STATUS "没有指定OpenCV文件夹-使用默认OpenCV文件夹")
endif()
message(STATUS "OpenCV文件夹 ${ZWN_OPENCV_DIR}")

# 初始化FMT文件夹
if(NOT ZWN_FMT_ROOT)
    set(ZWN_FMT_ROOT "D:/ProgramFiles/Dev/FMT/fmt_v9.1.0")
    message(STATUS "没有指定FMT文件夹-使用默认FMT文件夹")
endif()
set(ZWN_FMT_DIR "${ZWN_FMT_ROOT}/${CMAKE_BUILD_TYPE}")
set(ZWN_FMT_INC_DIR "${ZWN_FMT_DIR}/include")
message(STATUS "FMT文件夹 ${ZWN_FMT_DIR}")
message(STATUS "FMT头文件夹 ${ZWN_FMT_INC_DIR}")

# 设置OpenCV和FMT的
set(CMAKE_PREFIX_PATH "${ZWN_OPENCV_DIR};${CMAKE_PREFIX_PATH}")
set(CMAKE_PREFIX_PATH "${ZWN_FMT_DIR}/lib/cmake;${CMAKE_PREFIX_PATH}")

# 查找OpenCV
find_package(OpenCV REQUIRED core imgcodecs)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "未发现OpenCV")
else()
    message(STATUS "OpenCV库 ${OpenCV_LIBS}")
    message(STATUS "OpenCV头文件夹 ${OpenCV_INCLUDE_DIRS}")
endif()

# 查找FMT
find_package(fmt REQUIRED)
if(NOT fmt_FOUND)
    message(FATAL_ERROR "未发现fmt")
else()
    message(STATUS "发现fmt")
endif()

add_executable(
    ${PROJECT_NAME}
    main.cpp
    FileCamouflage.hpp
    ThreadPool.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS} ${ZWN_FMT_INC_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} fmt::fmt)

