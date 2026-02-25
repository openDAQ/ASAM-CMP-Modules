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
                      publisher.h
                      data_sink_module_fb.h
                      asam_cmp_packets_subscriber.h
                      data_packets_publisher.h
                      capture_packets_publisher.h
                      status_handler.h
                      status_fb_impl.h
                      data_sink_fb.h
                      capture_fb.h
                      interface_fb.h
                      stream_fb.h
)

set(SRC_PrivateHeaders
)

source_group("module" FILES
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

if (MSVC)
    add_compile_options(/bigobj)
endif()

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

opendaq_set_module_properties(${PROJECT_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
        COMPONENT RUNTIME
)

if (${REPO_OPTION_PREFIX}_ENABLE_TESTS)

    set(SRC_Lib_Cpp data_sink_module.cpp
                data_sink_module_fb.cpp
                status_fb_impl.cpp
                data_sink_fb.cpp
                capture_fb.cpp
                interface_fb.cpp
                stream_fb.cpp
    )

    set(SRC_Lib_PublicHeaders common.h
                          data_sink_module.h
                          data_sink_module_fb.h
                          status_fb_impl.h
                          status_handler.h
                          data_sink_fb.h
                          capture_fb.h
                          interface_fb.h
                          stream_fb.h
    )

    set(SRC_Lib_PrivateHeaders
    )

    prepend_include(${TARGET_FOLDER_NAME} SRC_Lib_PrivateHeaders)
    prepend_include(${TARGET_FOLDER_NAME} SRC_Lib_PublicHeaders)

    add_library(${PROJECT_NAME}_lib STATIC
        ${SRC_Lib_Cpp}
        ${SRC_Lib_PublicHeaders}
        ${SRC_Lib_PrivateHeaders}
    )

    target_link_libraries(${PROJECT_NAME}_lib PUBLIC daq::opendaq
                                                     Pcap++
                                                     asam_cmp
                                                     asam_cmp_common_lib
    )

    target_include_directories(${PROJECT_NAME}_lib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                                          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                                          $<INSTALL_INTERFACE:include>
    )

endif()
