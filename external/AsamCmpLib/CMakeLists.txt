list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        e8efc9a2ac68fd4042176509900945643937d839
)
FetchContent_MakeAvailable(AsamCmpLib)