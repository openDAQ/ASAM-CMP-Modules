set(TEST_APP test_capture_module)

set(TEST_SOURCES
                 test_interface.cpp
                 test_stream.cpp
                 test_capture_fb.cpp
                 test_app.cpp
                 ref_can_channel_impl.cpp
)

set(TEST_HEADERS 
    include/ref_can_channel_impl.h 
)

if (MSVC)
    add_compile_options(/bigobj)
endif()


add_executable(${TEST_APP} ${TEST_SOURCES} ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP} PRIVATE daq::opendaq
                                          daq::test_utils
                                          asam_cmp_capture_module_lib
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
