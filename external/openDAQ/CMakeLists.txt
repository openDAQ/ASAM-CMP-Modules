set(OPENDAQ_ENABLE_TESTS false)

FetchContent_Declare(
    openDAQ
    GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
    GIT_TAG        61128f6b9f6f851376b7250267f217e631c5e167
    GIT_PROGRESS   ON
    GIT_SHALLOW    ON
    SYSTEM
    FIND_PACKAGE_ARGS 3.0.0 GLOBAL
)

FetchContent_MakeAvailable(openDAQ)
