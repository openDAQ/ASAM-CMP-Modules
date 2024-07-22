project(AsamCmpCaptureModule
        VERSION 1.0.0
        LANGUAGES CXX)

set(LIB_NAME asam_cmp_capture_module)

set(SRC_Cpp module_dll.cpp
    capture_module_fb.cpp
    capture_module.cpp
    interface_fb.cpp
    stream_fb.cpp
    capture_fb.cpp
)

set(SRC_PublicHeaders module_dll.h
    capture_module_fb.h
    capture_module.h
    interface_fb.h
    stream_fb.h
    capture_fb.h
)

set(SRC_PrivateHeaders
    encoder_bank.h
)

prepend_include(${TARGET_FOLDER_NAME} SRC_PrivateHeaders)
prepend_include(${TARGET_FOLDER_NAME} SRC_PublicHeaders)

add_library(${LIB_NAME} SHARED
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
)

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
                                          Pcap++
                                          asam_cmp
                                          asam_cmp_common_lib
)

# if (MSVC)
#     target_compile_definitions(${LIB_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
#     target_compile_options(${LIB_NAME} PRIVATE /wd4100)
# endif()

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
