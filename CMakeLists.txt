cmake_minimum_required(VERSION 3.2)
project(lafplay CXX)


# set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_library(AVFORMAT_LIBRARY avformat REQUIRED)
find_library(AVCODEC_LIBRARY avcodec REQUIRED)
find_library(AVUTILS_LIBRARY avutil REQUIRED)
find_library(SWSCALE_LIBRARY swscale REQUIRED)

set(CMAKE_AUTOMOC ON)

option(LAFPLAY_BUILD_TESTS OFF)

set(LAFPLAY_INCLUDES
    blocking_queue.h
    lafplay.h
)

set(LAFPLAY_SOURCES
    blocking_queue.cpp
    lafplay.cpp
)

add_library(lafplay ${LAFPLAY_SOURCES} ${LAFPLAY_INCLUDES})
target_include_directories(lafplay PRIVATE "/usr/include/ffmpeg/" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lafplay ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTILS_LIBRARY} ${SWSCALE_LIBRARY} Qt5::Core Qt5::Gui Qt5::Widgets)

if(LAFPLAY_BUILD_TESTS)
    add_executable(lafplay_test main.cpp)
    target_link_libraries(lafplay_test lafplay)
endif()
