cmake_minimum_required(VERSION 3.20)

project(RosetteLab VERSION 0.1.0 LANGUAGES CXX)

option(ROSETTELAB_BUILD_GUI "Build the Qt 6 desktop application" ON)
option(ROSETTELAB_BUILD_TESTS "Build RosetteLab tests" ON)

if(ROSETTELAB_BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rosettelab_core
    src/curves/ellipse.cpp
    src/curves/harmonograph.cpp
    src/curves/lissajous.cpp
    src/curves/polar_rose.cpp
    src/curves/trochoid.cpp
    src/document/document.cpp
    src/svg/svg_serializer.cpp
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
            src/render/document_renderer.cpp
            src/ui/color_editor_dialog.cpp
            src/ui/color_preview_button.cpp
            src/ui/main_window.cpp
            src/ui/layer_list_item_widget.cpp
            src/ui/preview_widget.cpp
            src/svg/qt_svg_parser.cpp
        )
        target_include_directories(rosettelab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(rosettelab PRIVATE rosettelab_core Qt6::Widgets)
        install(TARGETS rosettelab RUNTIME DESTINATION bin)

        if(ROSETTELAB_BUILD_TESTS)
            add_executable(rosettelab_renderer_tests
                src/render/document_renderer.cpp
                tests/test_document_renderer.cpp
            )
            target_include_directories(rosettelab_renderer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
            target_link_libraries(rosettelab_renderer_tests PRIVATE rosettelab_core Qt6::Widgets)
            add_test(NAME rosettelab_renderer_tests COMMAND rosettelab_renderer_tests)

            add_executable(rosettelab_qt_svg_tests
                src/svg/qt_svg_parser.cpp
                tests/test_qt_svg_parser.cpp
            )
            target_include_directories(rosettelab_qt_svg_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
            target_link_libraries(rosettelab_qt_svg_tests PRIVATE rosettelab_core Qt6::Core)
            add_test(NAME rosettelab_qt_svg_tests COMMAND rosettelab_qt_svg_tests)
        endif()
    else()
        message(WARNING "Qt 6 Widgets was not found; building the core without the GUI")
    endif()
endif()

if(ROSETTELAB_BUILD_TESTS)
    add_executable(rosettelab_ellipse_tests tests/test_ellipse.cpp)
    target_link_libraries(rosettelab_ellipse_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_ellipse_tests COMMAND rosettelab_ellipse_tests)

    add_executable(rosettelab_lissajous_tests tests/test_lissajous.cpp)
    target_link_libraries(rosettelab_lissajous_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_lissajous_tests COMMAND rosettelab_lissajous_tests)

    add_executable(rosettelab_harmonograph_tests tests/test_harmonograph.cpp)
    target_link_libraries(rosettelab_harmonograph_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_harmonograph_tests COMMAND rosettelab_harmonograph_tests)

    add_executable(rosettelab_trochoid_tests tests/test_trochoid.cpp)
    target_link_libraries(rosettelab_trochoid_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_trochoid_tests COMMAND rosettelab_trochoid_tests)

    add_executable(rosettelab_core_tests tests/test_polar_rose.cpp)
    target_link_libraries(rosettelab_core_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_core_tests COMMAND rosettelab_core_tests)

    add_executable(rosettelab_document_tests tests/test_document.cpp)
    target_link_libraries(rosettelab_document_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_document_tests COMMAND rosettelab_document_tests)

    add_executable(rosettelab_svg_tests tests/test_svg_serializer.cpp)
    target_link_libraries(rosettelab_svg_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_svg_tests COMMAND rosettelab_svg_tests)
endif()
