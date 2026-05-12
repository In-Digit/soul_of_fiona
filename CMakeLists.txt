cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ./components
    ./components/gui
    ./components/fiona_core
    ./components/system_actions
    ./components/fiona_soul
    ./components/config_manager
    ./components/fiona_brain
    )

set(EXCLUDE_COMPONENTS
    esp_http_server
    esp_https_ota
    )

idf_build_set_property(MINIMAL_BUILD ON)

add_compile_options(-Wno-format)
add_compile_options(-Wno-ignored-qualifiers)
add_compile_options(-Wno-attributes)
add_compile_options(-Wno-unused-function)
project(lvgl_demo_v9)