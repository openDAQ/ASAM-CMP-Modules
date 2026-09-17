set(TEST_APP test_common)

set(TEST_SOURCES test_app.cpp
                 test_unit_converter.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE ${OPENDAQ_SDK_TARGET_NAMESPACE}::opendaq_test_utils
                                          gtest
                                          gmock
                                          asam_cmp_data_sink
)

opendaq_target_pch(${TEST_APP} <opendaq/opendaq.h> <gtest/gtest.h>)

# test_app.cpp defines main().
set_source_files_properties(test_app.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
opendaq_target_unity_tests(${TEST_APP} BATCH_SIZE 4)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE:${TEST_APP}>
)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (COMMAND setup_target_for_coverage AND OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
