cmake_minimum_required(VERSION 3.2.0...3.17.0)
project(lafplay CXX)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

find_library(AVFORMAT_LIBRARY avformat)
find_library(AVCODEC_LIBRARY avcodec)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)

if(NOT AVFORMAT_LIBRARY OR NOT AVCODEC_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY)
    set(HAS_FFMPEG OFF)
    message("ffmpeg is not found. we will disable ffmpeg controls.")
else()
    set(HAS_FFMPEG ON)
    message("ffmpeg is found at ${AVFORMAT_LIBRARY}.")
endif()

option(LAFPLAY_BUILD_TESTS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(LAFPLAY_INCLUDES
    blocking_queue.h
    image_viewer.h
    image_viewer_p.h
    animation_viewer.h
    animation_viewer_p.h
    waitingspinnerwidget.h
    scanning_widget.h
    flow_view.h
    lafplay_utils.h
    vertical_navigator.h
    styled_decoration.h
    graphics_glow_effect.h
    word_item.h
    bubble_tips.h
    tool_tip_widget.h
    navigate_title.h
    image_label.h
    animate_stacked_widget.h
    qprocessindicator.h
    range_calendar_widget.h
    image_selector.h
)

set(LAFPLAY_SOURCES
    blocking_queue.cpp
    image_viewer.cpp
    waitingspinnerwidget.cpp
    scanning_widget.cpp
    flow_view.cpp
    lafplay_utils.cpp
    vertical_navigator.cpp
    bubble_tips.cpp
    word_item.cpp
    graphics_glow_effect.cpp
    styled_decoration.cpp
    tool_tip_widget.cpp
    navigate_title.cpp
    image_label.cpp
    animate_stacked_widget.cpp
    qprocessindicator.cpp
    range_calendar_widget.cpp
    image_selector.cpp
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(LAFPLAY_INCLUDES ${LAFPLAY_INCLUDES} win_hotkey.h)
    set(LAFPLAY_SOURCES ${LAFPLAY_SOURCES} win_hotkey.cpp)
endif()

add_library(lafplay ${LAFPLAY_INCLUDES} ${LAFPLAY_SOURCES})
target_include_directories(lafplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lafplay PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)

if (HAS_FFMPEG)
    target_sources(lafplay PRIVATE animation_viewer.h animation_viewer.cpp)
    # 这里要怎么写比较好？在 linux 下找到 ffmpeg 是很容易的，
    # 但是在 windows 下没有统一的位置，所以即使使用 FindFFMPEG 也很难找到库的位置。
    target_include_directories(lafplay PRIVATE "/usr/include/ffmpeg/" "/usr/local/inclue/ffmpeg/")
    target_link_libraries(lafplay PRIVATE ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
endif()

if(LAFPLAY_BUILD_TESTS)
    add_executable(lafplay_test main.cpp)
    target_link_libraries(lafplay_test lafplay)
endif()
