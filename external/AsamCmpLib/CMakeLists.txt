list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        3326c8ac5cdbd77bde2fb56519e46ff2ac87161c
)
FetchContent_MakeAvailable(AsamCmpLib)