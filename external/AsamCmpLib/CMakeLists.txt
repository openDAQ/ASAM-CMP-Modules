list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

set(ASAM_CMP_LIB_BUILD_EXAMPLE OFF)
set(ASAM_CMP_LIB_ENABLE_TESTS OFF)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY https://github.com/openDAQ/ASAM-CMP-Library.git
    GIT_TAG        8e0cd696baf3b84db4b93d9ec005964b0a670400
)
FetchContent_MakeAvailable(AsamCmpLib)
