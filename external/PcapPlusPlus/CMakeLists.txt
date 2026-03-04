if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/W1>)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/wd4068>)
endif()

if(WIN32)
    set(OPENDAQ_ALWAYS_FETCH_NPCAPSDK ON CACHE BOOL "" FORCE)
    opendaq_dependency(
        NAME                npcapsdk
        URL                 https://npcap.com/dist/npcap-sdk-1.13.zip
    )
    
    set(PCAP_ROOT ${npcapsdk_SOURCE_DIR})
    set(PCAP_LINKS_SOLO ON)
endif()

set(_OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(_OLD_CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4505 /wd26444")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /wd4505 /wd26444")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough -Wno-unused-function")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-fallthrough -Wno-unused-function")
endif()

set(OPENDAQ_ALWAYS_FETCH_PCAPPLUSPLUS ON CACHE BOOL "" FORCE)
opendaq_dependency(
    NAME                PcapPlusPlus
    GIT_REPOSITORY      https://github.com/seladb/PcapPlusPlus.git
    GIT_REF             v25.05
    EXPECT_TARGET       Pcap++
)
 
set(CMAKE_CXX_FLAGS "${_OLD_CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${_OLD_CMAKE_C_FLAGS}")