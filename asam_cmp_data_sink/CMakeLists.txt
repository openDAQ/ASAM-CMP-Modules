get_current_folder_name(TARGET_FOLDER_NAME)
add_subdirectory(src)

message("SINK " ${PROJECT_VERSION_MAJOR})
project(AsamCmpDataSink LANGUAGES CXX VERSION 1.0.0)
message("SINK 2 " ${PROJECT_VERSION_MAJOR})

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
