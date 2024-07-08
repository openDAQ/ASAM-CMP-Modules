cmake_minimum_required(VERSION 3.2)
set(REPO_NAME asam_cmp_capture_module)
set(REPO_OPTION_PREFIX ASAM_CMP_CAPTURE_MODULE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(AsamCmpCaptureModule
        LANGUAGES CXX
        VERSION 1.0.0
)

include(CommonUtils)
setup_repo(${REPO_OPTION_PREFIX})

add_subdirectory(external)
add_subdirectory(${REPO_NAME})