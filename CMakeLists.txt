cmake_minimum_required(VERSION 3.22)
project(gstreamer)

add_executable(app main.cpp)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(facedetection REQUIRED)
pkg_check_modules(gtk3 REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
pkg_search_module(gstreamer-base REQUIRED IMPORTED_TARGET gstreamer-base-1.0>=1.4)

target_link_libraries(app
PkgConfig::gtk3
PkgConfig::gstreamer
PkgConfig::gstreamer-sdp
PkgConfig::gstreamer-app
PkgConfig::gstreamer-video
PkgConfig::gstreamer-base
gstfacesticker
)

get_target_property(facedetection-includes facedetection INTERFACE_INCLUDE_DIRECTORIES)

add_library(gstfacesticker SHARED ${CMAKE_SOURCE_DIR}/face-sticker-plugin/gstfacesticker.cpp)
target_include_directories(gstfacesticker PRIVATE ${CMAKE_SOURCE_DIR}/face-sticker-plugin ${OpenCV_INCLUDE_DIRS} ${facedetection-includes}/facedetection)

target_link_libraries(gstfacesticker 
    PkgConfig::gstreamer
    PkgConfig::gstreamer-sdp
    PkgConfig::gstreamer-base
    PkgConfig::gstreamer-video 
    facedetection
    ${OpenCV_LIBS})

set_target_properties(app PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

install(FILES ${CMAKE_SOURCE_DIR}/libgstfacesticker.so DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
