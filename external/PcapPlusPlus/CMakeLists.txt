list(APPEND CMAKE_MESSAGE_CONTEXT PcapPlusPlus)


if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/W1>)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4068>)
endif()

if(WIN32)
    FetchContent_Declare(
        npcapsdk
        URL https://npcap.com/dist/npcap-sdk-1.13.zip
    )
    FetchContent_MakeAvailable(npcapsdk)
    SET(PCAP_ROOT ${npcapsdk_SOURCE_DIR})
endif()

set(_OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(_OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-fallthrough -Wno-unused-function")

FetchContent_Declare(
    PcapPlusPlus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG v25.05
)
FetchContent_MakeAvailable(PcapPlusPlus)

set(CMAKE_CXX_FLAGS "${_OLD_CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${_OLD_CMAKE_C_FLAGS}")