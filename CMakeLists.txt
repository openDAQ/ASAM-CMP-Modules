cmake_minimum_required(VERSION 3.2)
set(REPO_NAME asam_cmp_modules)
set(REPO_OPTION_PREFIX ASAM_CMP)

project(${REPO_NAME} VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CommonUtils)
setup_repo(${REPO_OPTION_PREFIX})

add_subdirectory(external)
add_subdirectory(asam_cmp_common_lib)

if (${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE)
        add_subdirectory(asam_cmp_capture_module)
endif()

if (${REPO_OPTION_PREFIX}_BUILD_DATA_SINK)
        add_subdirectory(asam_cmp_data_sink)
endif()