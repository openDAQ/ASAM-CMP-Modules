set(OPENDAQ_ENABLE_TESTS false)

FetchContent_Declare(
    openDAQ
    GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
    GIT_TAG        cf643e83fcb3dc51bc491d2b0502022e5b5934b5
    GIT_PROGRESS   ON
    SYSTEM
    FIND_PACKAGE_ARGS 3.0.0 GLOBAL
)

FetchContent_MakeAvailable(openDAQ)
