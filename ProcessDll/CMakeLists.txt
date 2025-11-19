project(ProcessDll)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" include/*.h)

add_library(ProcessDll SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Common/include" "include")
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)