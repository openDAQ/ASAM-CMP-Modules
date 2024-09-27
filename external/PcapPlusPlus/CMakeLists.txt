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

FetchContent_Declare(
    PcapPlusPlus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG v24.09
)
FetchContent_GetProperties(PcapPlusPlus)
if (NOT PcapPlusPlus_POPULATED)
    FetchContent_Populate(PcapPlusPlus)
    add_subdirectory(${pcapplusplus_SOURCE_DIR} ${pcapplusplus_BINARY_DIR})
endif()
