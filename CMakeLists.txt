cmake_minimum_required(VERSION 3.25)
set(REPO_NAME asam_cmp_modules)
set(REPO_OPTION_PREFIX ASAM_CMP)

if (MSVC)
    add_compile_options(/bigobj)
endif()

project(${REPO_NAME} VERSION 1.0.0)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE "Enable ASAM CMP Capture Module" ON)
option(${REPO_OPTION_PREFIX}_BUILD_DATA_SINK "Enable ASAM CMP Data Sink" ON)
option(${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE "Enable Example" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_definitions(-DFMT_HEADER_ONLY)

include(AsamCommonUtils)
setup_repo(${REPO_OPTION_PREFIX})

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    enable_testing()
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE)
    set(DAQMODULES_REF_DEVICE_MODULE ON)
    set(DAQMODULES_REF_FB_MODULE ON)

    add_subdirectory(asam_cmp_example)
endif()

add_subdirectory(external)
add_subdirectory(asam_cmp_common_lib)

if (${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE)
    add_subdirectory(asam_cmp_capture_module)
endif()

if (${REPO_OPTION_PREFIX}_BUILD_DATA_SINK)
    add_subdirectory(asam_cmp_data_sink)
endif()

# Set CPack variables
set(CPACK_COMPONENTS_ALL RUNTIME)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/package")

# Set the CPack generator based on the platform
if (WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif (UNIX AND NOT APPLE)
    cmake_host_system_information(RESULT DISTRO_ID QUERY DISTRIB_ID)
    cmake_host_system_information(RESULT DISTRO_VERSION_ID QUERY DISTRIB_VERSION_ID)
    set(CPACK_SYSTEM_NAME "${DISTRO_ID}${DISTRO_VERSION_ID}")
    set(CPACK_GENERATOR "TGZ")
endif()

# Include CPack for packaging
include(CPack)