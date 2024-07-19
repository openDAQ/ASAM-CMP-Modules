project(asam_cmp_data_sink LANGUAGES CXX VERSION 1.0.0)

set(SRC_Cpp module_dll.cpp
            data_sink_module_impl.cpp
            data_sink_module_fb_impl.cpp
            status_fb_impl.cpp
            data_sink_fb_impl.cpp
            capture_module_common_impl.cpp
            capture_module_impl.cpp
            interface_common_fb_impl.cpp
            interface_fb_impl.cpp
            stream_common_fb_impl.cpp
            stream_fb_impl.cpp
)

set(SRC_PublicHeaders module_dll.h
                      common.h
                      data_sink_module_impl.h
                      data_sink_module_fb_impl.h
                      status_fb_impl.h
                      status_handler.h
                      data_sink_fb_impl.h
                      id_manager.h
                      capture_module_common_impl.h
                      capture_module_impl.h
                      interface_common_fb_impl.h
                      interface_fb_impl.h
                      stream_fb_impl.h
                      stream_common_fb_impl.h
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
