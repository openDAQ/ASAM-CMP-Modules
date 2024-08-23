list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        921a32e30e4614525ea1604deb8beedf420c4251
)
FetchContent_MakeAvailable(AsamCmpLib)