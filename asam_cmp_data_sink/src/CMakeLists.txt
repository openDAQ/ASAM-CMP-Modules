
message("SINK " ${PROJECT_VERSION_MAJOR})
project(AsamCmpDataSink LANGUAGES CXX VERSION 1.0.0)
message("SINK 2 " ${PROJECT_VERSION_MAJOR})

set(SRC_Cpp module_dll.cpp
            asam_cmp_data_sink_module_impl.cpp
            asam_cmp_data_sink_module_fb_impl.cpp
            asam_cmp_status_fb_impl.cpp
            asam_cmp_data_sink_fb_impl.cpp
            capture_module_impl.cpp
)

set(SRC_PublicHeaders module_dll.h
                      asam_cmp_data_sink_module_impl.h
                      asam_cmp_data_sink_module_fb_impl.h
                      asam_cmp_status_fb_impl.h
                      asam_cmp_status_handler.h
                      asam_cmp_data_sink_fb_impl.h
                      capture_module_impl.h
)

set(SRC_PrivateHeaders
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

set(LIB_NAME asam_cmp_data_sink)

add_library(${LIB_NAME} SHARED
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
                                          Pcap++
                                          asam_cmp
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                               $<INSTALL_INTERFACE:include>
)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})

install(TARGETS ${LIB_NAME}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        PUBLIC_HEADER DESTINATION include/${TARGET_FOLDER_NAME}
)
