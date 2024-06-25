set(LIB_NAME asam_cmp_capture_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})
set(MODULE_BINARY_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                asam_cmp_capture_module_impl.h
)

set(SRC_Srcs module_dll.cpp
             asam_cmp_capture_module_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

set(MODULE_FILES ${MODULE_HEADERS_DIR}/module_dll.h
                 ${MODULE_HEADERS_DIR}/asam_cmp_capture_module_impl.h
                 module_dll.cpp
                 asam_cmp_capture_module_impl.cpp
)

source_group("asam_cmp_capture_module" FILES ${MODULE_FILES})

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
                               
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)


target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
