cmake_minimum_required(VERSION 3.20)

project(RosetteLab VERSION 0.1.0 LANGUAGES CXX)

option(ROSETTELAB_BUILD_GUI "Build the Qt 6 desktop application" ON)
option(ROSETTELAB_BUILD_TESTS "Build RosetteLab tests" ON)
option(ROSETTELAB_REQUIRE_GUI
    "Fail configuration when the requested Qt GUI cannot be built" OFF)

if(ROSETTELAB_BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rosettelab_core
    src/curves/droplet_rosette.cpp
    src/curves/ellipse.cpp
    src/curves/harmonograph.cpp
    src/curves/lissajous.cpp
    src/curves/polar_rose.cpp
    src/curves/trochoid.cpp
    src/document/document.cpp
    src/pdf/pdf_serializer.cpp
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
        set(ROSETTELAB_APP_SOURCES
            src/app/main.cpp
            src/render/document_renderer.cpp
            src/ui/color_editor_dialog.cpp
            src/ui/color_preview_button.cpp
            src/ui/main_window.cpp
            src/ui/layer_list_item_widget.cpp
            src/ui/preview_widget.cpp
            src/svg/qt_svg_parser.cpp
            icons/rosettelab.qrc
        )

        if(WIN32)
            list(APPEND ROSETTELAB_APP_SOURCES icons/Windows/RosetteLab.rc)
        endif()

        if(APPLE)
            list(APPEND ROSETTELAB_APP_SOURCES
                icons/macOS/RosetteLab.icns
                LICENSE
            )
        endif()

        add_executable(rosettelab WIN32 MACOSX_BUNDLE ${ROSETTELAB_APP_SOURCES})
        target_include_directories(rosettelab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(rosettelab PRIVATE rosettelab_core Qt6::Widgets)
        target_compile_definitions(rosettelab PRIVATE
            ROSETTELAB_VERSION="${PROJECT_VERSION}"
        )

        set_target_properties(rosettelab PROPERTIES
            OUTPUT_NAME "RosetteLab"
            MACOSX_BUNDLE_BUNDLE_NAME "RosetteLab"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.rosettelab.RosetteLab"
            MACOSX_BUNDLE_ICON_FILE "RosetteLab.icns"
        )

        if(APPLE)
            set_target_properties(rosettelab PROPERTIES
                BUILD_RPATH "@executable_path/../Frameworks"
                INSTALL_RPATH "@executable_path/../Frameworks"
            )
        endif()

        if(APPLE)
            set_source_files_properties(
                icons/macOS/RosetteLab.icns
                LICENSE
                PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources"
            )
        endif()

        install(TARGETS rosettelab
            BUNDLE DESTINATION .
            RUNTIME DESTINATION bin
        )

        set(rosettelab_deploy_arguments)
        if(APPLE AND Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
            # Keep test bundles internally consistent after macdeployqt copies
            # the Qt frameworks and plugins. Public releases still require a
            # Developer ID signature and Apple notarization.
            list(APPEND rosettelab_deploy_arguments
                DEPLOY_TOOL_OPTIONS "-codesign=-"
            )
        endif()

        qt_generate_deploy_app_script(
            TARGET rosettelab
            OUTPUT_SCRIPT rosettelab_deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            ${rosettelab_deploy_arguments}
        )
        install(SCRIPT ${rosettelab_deploy_script})

        if(UNIX AND NOT APPLE)
            install(FILES packaging/linux/org.rosettelab.RosetteLab.desktop
                DESTINATION share/applications
            )
            install(FILES packaging/linux/org.rosettelab.RosetteLab.metainfo.xml
                DESTINATION share/metainfo
            )
            foreach(size 16 24 32 48 64 128 256 512 1024)
                install(FILES icons/Linux/rosettelab_${size}.png
                    DESTINATION share/icons/hicolor/${size}x${size}/apps
                    RENAME org.rosettelab.RosetteLab.png
                )
            endforeach()
        endif()

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

            add_executable(rosettelab_main_window_tests
                src/render/document_renderer.cpp
                src/ui/color_editor_dialog.cpp
                src/ui/color_preview_button.cpp
                src/ui/main_window.cpp
                src/ui/layer_list_item_widget.cpp
                src/ui/preview_widget.cpp
                src/svg/qt_svg_parser.cpp
                tests/test_main_window.cpp
            )
            target_include_directories(rosettelab_main_window_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
            target_link_libraries(rosettelab_main_window_tests PRIVATE rosettelab_core Qt6::Widgets)
            add_test(NAME rosettelab_main_window_tests COMMAND rosettelab_main_window_tests)
            set_tests_properties(rosettelab_main_window_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        endif()
    else()
        if(ROSETTELAB_REQUIRE_GUI)
            message(FATAL_ERROR "Qt 6.5 Widgets was not found")
        else()
            message(WARNING "Qt 6 Widgets was not found; building the core without the GUI")
        endif()
    endif()
endif()

set(CPACK_PACKAGE_NAME "RosetteLab")
set(CPACK_PACKAGE_VENDOR "RosetteLab contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Design, layer, and export editable mathematical curves")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RosetteLab")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "https://github.com/ppyne/RosetteLab")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "RosetteLab ${PROJECT_VERSION}")
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "RosetteLab")
    set(CPACK_NSIS_PACKAGE_NAME "RosetteLab")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/Windows/RosetteLab.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\RosetteLab.exe")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_MENU_LINKS "bin/RosetteLab.exe" "RosetteLab")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)

if(ROSETTELAB_BUILD_TESTS)
    add_executable(rosettelab_droplet_rosette_tests tests/test_droplet_rosette.cpp)
    target_link_libraries(rosettelab_droplet_rosette_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_droplet_rosette_tests COMMAND rosettelab_droplet_rosette_tests)

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

    add_executable(rosettelab_pdf_tests tests/test_pdf_serializer.cpp)
    target_link_libraries(rosettelab_pdf_tests PRIVATE rosettelab_core)
    add_test(NAME rosettelab_pdf_tests COMMAND rosettelab_pdf_tests)
endif()
