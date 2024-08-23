list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        940e09d85723b992c1f9f2d12731d60f0a9f225e
)
FetchContent_MakeAvailable(AsamCmpLib)