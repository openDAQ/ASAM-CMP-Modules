opendaq_get_cmake_mode(_ASAM_CMP_CMAKE_MODERN_MODE_SAVED)
opendaq_set_cmake_mode(ANCIENT)

set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS AND NOT TARGET gtest)
    set(BUILD_GMOCK ON)
    opendaq_module_setup_gtest(1.12.1)

    get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(gmock PRIVATE ${GTEST_INCLUDE_DIRS})
    target_include_directories(gmock_main PRIVATE ${GTEST_INCLUDE_DIRS})
endif()

add_subdirectory(PcapPlusPlus)
add_subdirectory(AsamCmpLib)

opendaq_set_cmake_mode(${_ASAM_CMP_CMAKE_MODERN_MODE_SAVED})