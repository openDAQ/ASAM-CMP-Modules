list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        40b4d54421b0a3328f750224066c16050e5ec3ab
)
FetchContent_MakeAvailable(AsamCmpLib)