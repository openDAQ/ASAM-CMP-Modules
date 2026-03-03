set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS AND NOT TARGET gtest)
    set(BUILD_GMOCK ON)
    opendaq_module_setup_gtest(1.12.1)
endif()

add_subdirectory(PcapPlusPlus)
add_subdirectory(AsamCmpLib)
