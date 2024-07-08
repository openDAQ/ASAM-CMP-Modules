set(CMAKE_FOLDER external)
list(APPEND CMAKE_MESSAGE_CONTEXT external)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source build is not supported! Please choose a separate build directory e.g.: /build/x64/msvc")
endif()

include(FetchContent)

add_subdirectory(PcapPlusPlus)
add_subdirectory(openDAQ)
add_subdirectory(AsamCmpLib)
