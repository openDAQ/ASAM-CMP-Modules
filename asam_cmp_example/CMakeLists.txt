add_executable(asam_cmp_modules_example main.cpp)

target_link_libraries(asam_cmp_modules_example PRIVATE
    daq::opendaq
)

add_dependencies(
    asam_cmp_modules_example
    daq::ref_device_module
    daq::ref_fb_module
    asam_cmp_capture_module
    asam_cmp_data_sink
)

set_target_properties(asam_cmp_modules_example
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:asam_cmp_modules_example>
)