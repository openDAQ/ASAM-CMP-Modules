project(asam_cmp_data_sink LANGUAGES CXX VERSION 1.0.0)

set(SRC_Cpp module_dll.cpp
            data_sink_module.cpp
            data_sink_module_fb.cpp
            status_fb_impl.cpp
            data_sink_fb.cpp
            capture_fb.cpp
            interface_fb.cpp
            stream_fb.cpp
)

set(SRC_PublicHeaders module_dll.h
                      common.h
                      data_sink_module.h
                      data_sink_module_fb.h
                      status_fb_impl.h
                      status_handler.h
                      data_sink_fb.h
                      capture_fb.h
                      interface_fb.h
                      stream_fb.h
)

set(SRC_PrivateHeaders
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

add_library(${PROJECT_NAME} SHARED
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

target_link_libraries(${PROJECT_NAME} PUBLIC daq::opendaq
                                          Pcap++
                                          asam_cmp
                                          asam_cmp_common_lib
)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                               $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_module_properties(${PROJECT_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        PUBLIC_HEADER DESTINATION include/${TARGET_FOLDER_NAME}
)
