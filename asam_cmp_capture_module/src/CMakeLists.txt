set(SRC_Cpp module_dll.cpp
    asam_cmp_capture_impl.cpp
    asam_cmp_capture_module_impl.cpp
    asam_cmp_interface_fb_impl.cpp
    asam_cmp_stream_fb_impl.cpp
    capture_module_impl.cpp
)

set(SRC_PublicHeaders module_dll.h
    asam_cmp_capture_impl.h
    asam_cmp_capture_module_impl.h
    asam_cmp_interface_fb_impl.h
    asam_cmp_stream_fb_impl.h
    capture_module_impl.h
)

set(SRC_PrivateHeaders
    asam_cmp_id_manager.h
    asam_cmp_encoder_bank.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

add_library(${REPO_NAME} SHARED
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

target_link_libraries(${REPO_NAME} PUBLIC daq::opendaq
                                          Pcap++
                                          asam_cmp
)

if (MSVC)
    target_compile_definitions(${REPO_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${REPO_NAME} PRIVATE /wd4100)
endif()

target_include_directories(${REPO_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                               $<INSTALL_INTERFACE:include>
)

set_target_properties(${REPO_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_module_properties(${REPO_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${REPO_NAME})

install(TARGETS ${REPO_NAME}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        PUBLIC_HEADER DESTINATION include/${TARGET_FOLDER_NAME}
)
