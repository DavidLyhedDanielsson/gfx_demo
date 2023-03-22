cmake_minimum_required(VERSION 3.26) # This is what I have so I'll just roll with it
project(gfx_demo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/$<$<CONFIG:RELEASE>:release>$<$<CONFIG:DEBUG>:debug>)
set(ROOT_DIR_ASSET ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(ROOT_DIR_SHADER ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)
# ROOT_DIR_ASSET_STR is needed to make it work as expected and not break the
# syntax highlighting in CMakeLists files
set(ROOT_DIR_ASSET_STR "\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
set(ROOT_DIR_SHADER_STR "\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader\"")

# Why why why why why why why why
if(WIN32)
    add_compile_definitions(-DNOMINMAX -DSTBI_NO_SIMD)
endif()

# Runtime
find_package(SDL2 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(entt CONFIG REQUIRED)
# Graphics
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)

add_subdirectory(src)