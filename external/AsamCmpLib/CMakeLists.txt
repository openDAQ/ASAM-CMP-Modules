list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

set(ASAM_CMP_LIB_BUILD_EXAMPLE OFF)
set(ASAM_CMP_LIB_ENABLE_TESTS OFF)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY https://github.com/openDAQ/ASAM-CMP-Library.git
    GIT_TAG        af640aa454d63c22c03d141cf92cf5f15e8c5a48
)
FetchContent_MakeAvailable(AsamCmpLib)
