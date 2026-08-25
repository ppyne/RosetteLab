cmake_minimum_required(VERSION 3.20)

project(RosetteLab VERSION 0.1.0 LANGUAGES CXX)

option(ROSETTELAB_BUILD_GUI "Build the Qt 6 desktop application" ON)
option(ROSETTELAB_BUILD_TESTS "Build RosetteLab tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rosettelab_core
    src/curves/polar_rose.cpp
    src/document/document.cpp
)

target_include_directories(rosettelab_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(rosettelab_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(rosettelab_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(rosettelab_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ROSETTELAB_BUILD_GUI)
    find_package(Qt6 6.5 QUIET COMPONENTS Widgets)

    if(Qt6_FOUND)
        add_executable(rosettelab
            src/app/main.cpp
            src/ui/color_editor_dialog.cpp
            src/ui/main_window.cpp
            src/ui/layer_list_item_widget.cpp
            src/ui/preview_widget.cpp
        )
        target_include_directories(rosettelab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(rosettelab PRIVATE rosettelab_core Qt6::Widgets)
        install(TARGETS rosettelab RUNTIME DESTINATION bin)
    else()
        message(WARNING "Qt 6 Widgets was not found; building the core without the GUI")
    endif()
endif()

if(ROSETTELAB_BUILD_TESTS)
    enable_testing()
    add_executable(rosettelab_core_tests tests/test_polar_rose.cpp)
    target_link_libraries(rosettelab_core_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_core_tests COMMAND rosettelab_core_tests)

    add_executable(rosettelab_document_tests tests/test_document.cpp)
    target_link_libraries(rosettelab_document_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_document_tests COMMAND rosettelab_document_tests)
endif()
