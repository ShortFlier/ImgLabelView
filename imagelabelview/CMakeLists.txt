cmake_minimum_required(VERSION 3.16)

project(imagelabelview)

add_library(imagelabelview STATIC
    imagelabelview.h  imagelabelview.cpp    imagelabelview.ui
    imagelabel.h    imagelabel.cpp
)


target_link_libraries(imagelabelview PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)