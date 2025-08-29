cmake_minimum_required(VERSION 3.20)
project(launch)

set(CMAKE_CXX_STANDARD 17)

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(FATAL_ERROR "本项目在Windows上不支持使用 MinGW 编译，请使用 MSVC 编译器！")
endif()

if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
endif()

add_executable(launch main.cpp system_info.cpp crash_log.cpp)

if(WIN32)
    # Windows 平台链接库
    target_link_libraries(launch
            wbemuuid
            iphlpapi
            ole32
            oleaut32
            advapi32
    )

    if(MSVC)
        target_compile_options(launch PRIVATE /EHsc /W4)
        target_compile_options(launch PRIVATE "/utf-8")
        target_link_libraries(launch comsuppw)
    endif()
    
elseif(APPLE)
    find_library(IOKIT IOKit)
    find_library(COREFOUNDATION CoreFoundation)
    target_link_libraries(launch ${IOKIT} ${COREFOUNDATION})
else()
    find_package(Threads REQUIRED)
    target_link_libraries(launch Threads::Threads)
endif()