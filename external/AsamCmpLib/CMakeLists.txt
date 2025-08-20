list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

set(ASAM_CMP_LIB_BUILD_EXAMPLE OFF)
set(ASAM_CMP_LIB_ENABLE_TESTS OFF)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY https://github.com/openDAQ/ASAM-CMP-Library.git
    GIT_TAG        261cd2c03b4207f33dd2c2be545bc2d27e0f40cb
)
FetchContent_MakeAvailable(AsamCmpLib)
