##
## CPack configuration for the ASAM CMP module
##

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "openDAQ d.o.o.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "openDAQ ASAM CMP module")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://opendaq.io/")
set(CPACK_PACKAGE_CONTACT "info@opendaq.io")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_THREADS 0)

set(CPACK_GENERATOR TGZ)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

if(NOT CPACK_PACKAGE_DIRECTORY)
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/build/_packages")
endif()

##
## Package filename and staging metadata
##
## Format: asam-cmp-module-<version>[-<sha>]-<arch>-<os>-<compiler>-<compiler-version>-<build-type>
## Composed by opendaq-cmake-utils, the same way core names its packages.
##

set(CPACK_OPENDAQ_META_PACKAGE_NAME "asam-cmp-module")

execute_process(COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short=7 HEAD
                OUTPUT_VARIABLE _PACKING_SHORT_SHA
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
)
if(_PACKING_SHORT_SHA)
    set(CPACK_OPENDAQ_META_PACKAGE_VERSION_SUFFIX "-${_PACKING_SHORT_SHA}")
endif()

opendaq_detect_settings()
opendaq_detect_package()
opendaq_compose_package_triplet()
opendaq_compose_package_file_name()

# Let each cpack run name the package it produces (-D CPACK_OPENDAQ_META_PACKAGE_NAME=...).
set(CPACK_PROJECT_CONFIG_FILE "${CPACK_OPENDAQ_PROJECT_CONFIG}")

include(CPack)