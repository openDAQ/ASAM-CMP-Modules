set(TEST_APP test_data_sink)

set(TEST_SOURCES test_app.cpp
                 test_data_sink_module_fb.cpp
                 test_status_fb.cpp
                 test_data_sink_fb.cpp
                 test_capture_module.cpp
                 test_interface.cpp
                 test_stream.cpp
                 test_calls_multi_map.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          asam_cmp_data_sink_lib
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
