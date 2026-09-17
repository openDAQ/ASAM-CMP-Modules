set(TEST_APP test_data_sink)

set(TEST_SOURCES test_app.cpp
                 test_data_sink_module_fb.cpp
                 test_status_fb.cpp
                 test_data_sink_fb.cpp
                 test_capture_fb.cpp
                 test_interface_fb.cpp
                 test_stream_fb.cpp
                 test_data_packets_publisher.cpp
)

if (MSVC)
    add_compile_options(/bigobj)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4459>)
endif()

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE ${OPENDAQ_SDK_TARGET_NAMESPACE}::opendaq_test_utils
                                          gtest
                                          gmock
                                          asam_cmp_data_sink_lib
)

opendaq_target_pch(${TEST_APP} <opendaq/opendaq.h> <gtest/gtest.h>)

# test_app.cpp defines main().
set_source_files_properties(test_app.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
opendaq_target_unity_tests(${TEST_APP} BATCH_SIZE 4)

if (MSVC)
    target_link_options(${TEST_APP} PRIVATE /DELAYLOAD:wpcap.dll /DELAYLOAD:packet.dll)
    target_link_libraries(${TEST_APP} PRIVATE delayimp)
endif()


add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE:${TEST_APP}>
)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (COMMAND setup_target_for_coverage AND OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
