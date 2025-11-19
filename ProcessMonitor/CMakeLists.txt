project(ProcessMonitor)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/3rd-party/FindWDK/cmake")
find_package(WDK REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Debug library postfix.")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /WX /EHsc")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT /WX /EHsc")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT /WX /EHsc")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /EHsc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP /EHsc -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=NTDDI_WIN8")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /WX")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MT /WX")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MT /WX")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /MP -DUNICODE -D_UNICODE -D_WIN32_WINNT=NTDDI_WIN8")

wdk_add_driver(${PROJECT_NAME} STL src/main.cpp ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Common/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/3rd-party/kf/include")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Activate precompiled headers, unfortunately `target_precompile_headers` doesn't work with `wdk_add_driver`
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/Yupch.h")
set_source_files_properties(src/pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")

target_compile_definitions(${PROJECT_NAME} PRIVATE _ITERATOR_DEBUG_LEVEL=0)