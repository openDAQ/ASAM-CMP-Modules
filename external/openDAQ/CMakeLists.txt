set(OPENDAQ_ENABLE_TESTS false)

FetchContent_Declare(
    openDAQ
    GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
    GIT_TAG        4ad72a2c8e0cece80a9898dd844dff3f53f43f2f
    GIT_PROGRESS   ON
    SYSTEM
    FIND_PACKAGE_ARGS 3.0.0 GLOBAL
)

FetchContent_MakeAvailable(openDAQ)
