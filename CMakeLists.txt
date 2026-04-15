set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.25)

set(REPO_NAME asam_cmp_modules)
set(REPO_OPTION_PREFIX ASAM_CMP)

list(APPEND CMAKE_MESSAGE_CONTEXT ${REPO_NAME})
add_subdirectory(cmake)

opendaq_read_file_contents("${CMAKE_CURRENT_LIST_DIR}/module_version" module_version)
opendaq_get_version_major_minor_patch("${module_version}" ${REPO_OPTION_PREFIX}_VERSION)

project(${REPO_NAME} VERSION ${${REPO_OPTION_PREFIX}_VERSION} LANGUAGES CXX)

opendaq_common_early_setup()

if (PROJECT_IS_TOP_LEVEL)
    message(STATUS "Building ${REPO_NAME} version ${${REPO_OPTION_PREFIX}_VERSION} standalone")
else()
    message(STATUS "Building ${REPO_NAME} version ${${REPO_OPTION_PREFIX}_VERSION} as submodule")
endif()

opendaq_setup_common_build_options()
opendaq_setup_project_specific_build_options(${REPO_OPTION_PREFIX})

option(${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE "Enable ASAM CMP Capture Module" ON)
option(${REPO_OPTION_PREFIX}_BUILD_DATA_SINK "Enable ASAM CMP Data Sink" ON)
option(${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE "Enable Example" ${PROJECT_IS_TOP_LEVEL})
option(${REPO_OPTION_PREFIX}_ENABLE_TESTS "Enable ${REPO_NAME} testing" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

opendaq_common_compile_targets_settings()
opendaq_setup_compiler_flags(${REPO_OPTION_PREFIX})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    add_compile_options(-Wno-unknown-warning-option)
endif()

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    message(STATUS "Unit tests in ${REPO_NAME} are ENABLED")
    set(OPENDAQ_ENABLE_TEST_UTILS ON CACHE BOOL "Enable testing utils library")
    enable_testing()
else()
    message(STATUS "Unit tests in ${REPO_NAME} are DISABLED")
endif()

if (MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/bigobj>)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4244>)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4100>)
endif()

add_compile_definitions(FMT_HEADER_ONLY)

if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE)
    set(DAQMODULES_REF_DEVICE_MODULE ON)
    set(DAQMODULES_REF_FB_MODULE ON)
endif()

if (NOT TARGET "${OPENDAQ_SDK_TARGET_NAMESPACE}::${OPENDAQ_SDK_TARGET_NAME}")
    if (PROJECT_IS_TOP_LEVEL)
        find_package(${OPENDAQ_SDK_NAME} GLOBAL)
    endif()
    if (NOT ${OPENDAQ_SDK_NAME}_FOUND)
        include(FetchContent)
        set(OPENDAQ_ENABLE_TESTS OFF CACHE BOOL "")

        opendaq_read_file_contents("${CMAKE_CURRENT_LIST_DIR}/opendaq_ref" OPENDAQ_REF)
        opendaq_get_custom_fetch_content_params(${OPENDAQ_SDK_NAME} FC_PARAMS)

        FetchContent_Declare(
            ${OPENDAQ_SDK_NAME}
            GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
            GIT_TAG ${OPENDAQ_REF}
            GIT_PROGRESS ON
            ${FC_PARAMS}
        )
        FetchContent_MakeAvailable(${OPENDAQ_SDK_NAME})
    else()
        message(STATUS "Found installed ${OPENDAQ_SDK_NAME} version: ${${OPENDAQ_SDK_NAME}_VERSION}")
        set(OPENDAQ_PACKAGE_VERSION "${${OPENDAQ_SDK_NAME}_VERSION}" CACHE INTERNAL "${OPENDAQ_SDK_NAME} SDK version")
    endif()
endif()

add_subdirectory(external)
add_subdirectory(shared)
add_subdirectory(modules)
if (${REPO_OPTION_PREFIX}_ENABLE_EXAMPLE)
    add_subdirectory(examples)
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
