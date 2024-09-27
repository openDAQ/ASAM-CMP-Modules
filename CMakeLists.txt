cmake_minimum_required(VERSION 3.25)
set(REPO_NAME asam_cmp_modules)
set(REPO_OPTION_PREFIX ASAM_CMP)

project(${REPO_NAME} VERSION 1.0.0)

option(${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE "Enable ASAM CMP Capture Module" ON)
option(${REPO_OPTION_PREFIX}_BUILD_DATA_SINK "Enable ASAM CMP Data Sink" ON)
option(${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE "Enable Example" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AsamCommonUtils)
setup_repo(${REPO_OPTION_PREFIX})

if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE)
        set(DAQMODULES_REF_DEVICE_MODULE ON)
        set(DAQMODULES_REF_FB_MODULE ON)

        add_subdirectory(asam_cmp_example)
endif()

add_subdirectory(external)
add_subdirectory(asam_cmp_common_lib)

if (${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE)
        message("Add capture module!!!")
        add_subdirectory(asam_cmp_capture_module)
endif()

if (${REPO_OPTION_PREFIX}_BUILD_DATA_SINK)
        message("Add data sink!!!")
        add_subdirectory(asam_cmp_data_sink)
endif()