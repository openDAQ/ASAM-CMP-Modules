list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        f0798b1cb60c41be5f3cfa79adab17e2fe49f131
)
FetchContent_MakeAvailable(AsamCmpLib)