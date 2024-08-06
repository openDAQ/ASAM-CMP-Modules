set(OPENDAQ_ENABLE_TESTS false)

FetchContent_Declare(
    openDAQ
    GIT_REPOSITORY ssh://git@gitlab.dewesoft.com:2345/Blueberry/openDAQ.git
    GIT_TAG        v3.0.0-dev16
    GIT_PROGRESS   ON
    GIT_SHALLOW    ON
    SYSTEM
    FIND_PACKAGE_ARGS 3.0.0 GLOBAL
)

FetchContent_MakeAvailable(openDAQ)
