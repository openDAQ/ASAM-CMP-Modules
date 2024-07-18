list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        03119ad158592b7f675d5bc0ec48f894ed8c99f4
)
FetchContent_MakeAvailable(AsamCmpLib)