function(get_current_folder_name OUTFOLDER)
    get_filename_component(FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    set(${OUTFOLDER} ${FOLDER} PARENT_SCOPE)
endfunction()

function(set_cmake_folder_context OUTFOLDER)
    get_current_folder_name(TARGET_FOLDER_NAME)

    if (ARGC GREATER 1)
        list(APPEND CMAKE_MESSAGE_CONTEXT ${ARGV1})
    else()
        list(APPEND CMAKE_MESSAGE_CONTEXT ${TARGET_FOLDER_NAME})
    endif()

    set(CMAKE_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT} PARENT_SCOPE)
    if (ARGC GREATER 1)
        set(CMAKE_FOLDER "${CMAKE_FOLDER}/${ARGV1}" PARENT_SCOPE)
    else()
        set(CMAKE_FOLDER "${CMAKE_FOLDER}/${TARGET_FOLDER_NAME}" PARENT_SCOPE)
    endif()
    set(${OUTFOLDER} ${TARGET_FOLDER_NAME} PARENT_SCOPE)
endfunction()

cmake_minimum_required(VERSION 3.2)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(AsamCmpCaptureModule VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(OPENDAQ_ENABLE_TESTS false)
set(DAQMODULES_OPENDAQ_SERVER_MODULE false)
set(OPENDAQ_ENABLE_OPCUA false)
set(OPENDAQ_ENABLE_NATIVE_STREAMING false)
set(OPENDAQ_MODULE_SUFFIX ".module${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(DAQMODULES_REF_DEVICE_MODULE OFF CACHE BOOL "" FORCE)
set(DAQMODULES_REF_FB_MODULE OFF CACHE BOOL "" FORCE)
add_compile_definitions(OPENDAQ_ENABLE_PARAMETER_VALIDATION OPENDAQ_THREAD_SAFE)    

include(FetchContent)
FetchContent_Declare(
    openDAQ
    GIT_REPOSITORY https://github.com/opendaq/opendaq.git
    GIT_TAG        31fc089b51d2e6fed9cfb8a87975469c2e771ff1
)
FetchContent_MakeAvailable(openDAQ)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        1a1d3cc217fd49f6fe3cb6873047e78e597125c7
)
FetchContent_MakeAvailable(AsamCmpLib)

# FetchContent_GetProperties(counterlib)
# if (NOT counterlib_POPULATED)
#   FetchContent_Populate(counterlib)
#   add_subdirectory(${counterlib_SOURCE_DIR} ${counterlib_BINARY_DIR})
# endif()

set(TARGET_FOLDER_NAME asam_cmp_capture_module)
add_subdirectory(src)

if (ASAM_CMP_CAPTURE_MODULE_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
