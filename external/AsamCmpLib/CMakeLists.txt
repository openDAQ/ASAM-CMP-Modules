list(APPEND CMAKE_MESSAGE_CONTEXT AsamCmpLib)

set(ASAM_CMP_LIB_ENABLE_TESTS OFF CACHE BOOL "Enable Asam CMP Lib unit tests" )
message("ASAM CMP MODULE HAS ASAM_CMP_LIB_ENABLE_TESTS " ${ASAM_CMP_LIB_ENABLE_TESTS})
FetchContent_Declare(
    AsamCmpLib
    GIT_REPOSITORY git@github.com:openDAQ/ASAM-CMP-Library.git
    GIT_TAG        4345e5863d73368b82df1f9ceacc890504e3e996
)
FetchContent_MakeAvailable(AsamCmpLib)
