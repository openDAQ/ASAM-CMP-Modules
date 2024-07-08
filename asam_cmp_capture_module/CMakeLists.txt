get_current_folder_name(TARGET_FOLDER_NAME)
add_subdirectory(src)

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
