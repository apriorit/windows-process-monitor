cmake_minimum_required(VERSION 3.8)

# support VS2019 and higher
if(MSVC_VERSION LESS 1920)
    return()
endif()

project(Procmon VERSION 2.0
                  DESCRIPTION "Process monitoring project"
                  LANGUAGES CXX)

# Default to debug build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W3>)
add_compile_options($<$<C_COMPILER_ID:MSVC>:/W3>)

add_subdirectory(Common)
add_subdirectory(ProcessMonitor)
add_subdirectory(ProcessDll)
add_subdirectory(ProcMonGUI)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(Common ProcessMonitor ProcessDll ProcMonGUI
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)