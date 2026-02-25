if (${REPO_OPTION_PREFIX}_BUILD_CAPTURE_MODULE)
    add_subdirectory(asam_cmp_capture_module)
endif()

if (${REPO_OPTION_PREFIX}_BUILD_DATA_SINK)
    add_subdirectory(asam_cmp_data_sink)
endif()
