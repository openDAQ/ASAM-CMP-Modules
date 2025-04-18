project(asam_cmp_common_lib LANGUAGES CXX VERSION 1.0.0)

set(SRC_Cpp interface_common_fb.cpp
            ethernet_pcpp_impl.cpp
            network_manager_fb.cpp
            unit_converter.cpp
)

set(SRC_PublicHeaders common.h
                      id_manager.h
                      capture_common_fb.h
                      interface_common_fb.h
                      stream_common_fb_impl.h
                      ethernet_pcpp_impl.h
                      ethernet_pcpp_itf.h
                      ethernet_pcpp_mock.h
                      ethernet_itf.h
                      network_manager_fb.h
                      unit_converter.h
)

set(SRC_PrivateHeaders
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

add_library(${PROJECT_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

if(UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC daq::opendaq
                                          Pcap++
                                          asam_cmp
                                          daq::test_utils
)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                               $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")
