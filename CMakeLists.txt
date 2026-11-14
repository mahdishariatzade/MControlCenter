cmake_minimum_required(VERSION 3.25)

project(MControlCenter VERSION 0.5.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Widgets
    LinguistTools
    DBus
)

qt_standard_project_setup()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.ui
    src/mainwindow.cpp src/mainwindow.h
    src/operate.cpp src/operate.h
    src/powermonitor.cpp src/powermonitor.h
    src/helper.cpp src/helper.h
    src/msi-ec_helper.cpp src/msi-ec_helper.h
    src/settings.cpp src/settings.h
    src/dashboard.cpp src/dashboard.h src/theme.h
    src/sensors.cpp src/sensors.h
    src/pages.cpp src/pages.h src/uikit.h
    src/rgb.cpp src/rgb.h
    src/keyboard.cpp src/keyboard.h
    src/lighting.cpp
    src/ddc.cpp src/ddc.h src/displays.cpp src/displays.h
    src/appprofiles.cpp src/appprofiles.h
    src/bridge.cpp src/bridge.h
    src/resources.qrc
)

# Translations: the .ts sources live in src/i18n and resources.qrc embeds the compiled .qm files from the same folder, so compile them there
# (they are git-ignored build products). A clean checkout needs this to build at all.
set(TS_NAMES de_DE en es eu fi_FI fr hu it nb nl_NL pt_BR pt_PT ru tr vi zh_CN)
set(TS_FILES "")
foreach(n ${TS_NAMES})
    list(APPEND TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/MControlCenter_${n}.ts)
endforeach()
foreach(ts ${TS_FILES})
    set_source_files_properties(${ts} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n)
endforeach()
qt_add_translation(QM_FILES ${TS_FILES})
add_custom_target(mcc_translations DEPENDS ${QM_FILES})

qt_add_executable(mcontrolcenter
    ${PROJECT_SOURCES}
)
add_dependencies(mcontrolcenter mcc_translations)

add_subdirectory(src/helper ${CMAKE_BINARY_DIR}/helper)

target_link_libraries(mcontrolcenter PRIVATE
    Qt6::Widgets
    Qt6::DBus
)

target_compile_definitions(mcontrolcenter PRIVATE
    MControlCenter_VERSION="${PROJECT_VERSION}")
