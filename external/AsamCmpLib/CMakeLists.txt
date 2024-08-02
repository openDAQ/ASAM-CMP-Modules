list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        b71c73e7ca55d71f7452abd26a944a06ab20da9d
)
FetchContent_MakeAvailable(AsamCmpLib)