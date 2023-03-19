# Shaders
add_custom_target(shaders)
set(SHADER_SRC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/shaders/)
set(SHADER_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

set(SHADER_PS
    "white"
    "spinning_cat"
)

set(SHADER_VS
    "ndc_triangle"
    "vertex_triangle_ia"
    "vertex_triangle_pull"
    "vertex_triangle_spinning"
    "spinning_cat"
)

add_custom_command(
        TARGET shaders
        MAIN_DEPENDENCY ${SHADER_OUTPUT_DIR}
        COMMENT "Creating shader output directories"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}/vs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}/ps
        VERBATIM)

function(CompileShader TYPE FILE_NAME)
    string(TOLOWER ${TYPE} TYPE)
    add_custom_command(
            TARGET shaders
            MAIN_DEPENDENCY ${SHADER_OUTPUT_DIR}/${TYPE}/${FILE_NAME}.bin
            COMMENT "Compiling ${TYPE} at ${FILE_NAME}"
            COMMAND dxc -O0 -E main -T ${TYPE}_6_5 -Zi ${SHADER_SRC_ROOT_DIR}/${TYPE}/${FILE_NAME}.hlsl -Fo ${SHADER_OUTPUT_DIR}/${TYPE}/${FILE_NAME}.bin -Fd ${SHADER_OUTPUT_DIR}/${TYPE}/${FILE_NAME}.pdb
            VERBATIM)
endfunction(CompileShader)

function(CompileVertexShader FILE_NAME)
    CompileShader(vs ${FILE_NAME})
endfunction(CompileVertexShader)

function(CompilePixelShader FILE_NAME)
    CompileShader(ps ${FILE_NAME})
endfunction(CompilePixelShader)

foreach(SHADER IN ITEMS ${SHADER_PS})
    CompilePixelShader(${SHADER})
endforeach()
foreach(SHADER IN ITEMS ${SHADER_VS})
    CompileVertexShader(${SHADER})
endforeach()

# Source code
set(SRC_UTIL
    file_util.cpp file_util.hpp
)
list(TRANSFORM SRC_UTIL PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/util/)

set(SRC_ASSET
    asset_handle.hpp
    asset_loader.cpp asset_loader.hpp
    asset.hpp
    io_job.hpp
    job_handle.hpp
    job.hpp
    loader.hpp
    path.cpp path.hpp
)
list(TRANSFORM SRC_ASSET PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/asset/)

set(SRC_DX
    blend_state.hpp
    versioning.hpp
    demos/ndc_triangle.cpp demos/ndc_triangle.hpp
    demos/vertex_triangle_ia.cpp demos/vertex_triangle_ia.hpp
    demos/vertex_triangle_pull.cpp demos/vertex_triangle_pull.hpp
    demos/spinning_triangle.cpp demos/spinning_triangle.hpp
    demos/spinning_quad.cpp demos/spinning_quad.hpp
    demos/spinning_cat.cpp demos/spinning_cat.hpp
)
list(TRANSFORM SRC_DX PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/)

add_executable(gfx_demo
    ${SRC_UTIL}
    ${SRC_ASSET}
    ${SRC_DX}
    main.cpp
)
add_dependencies(gfx_demo
    shaders
)

target_compile_definitions(gfx_demo PRIVATE
    ROOT_DIR_ASSETS=${ROOT_DIR_ASSETS_STR}
    ROOT_DIR_SHADERS=${ROOT_DIR_SHADERS_STR}
)
target_include_directories(gfx_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(gfx_demo PRIVATE
    SDL2::SDL2 SDL2::SDL2main
    EnTT::EnTT
    ${ASSIMP_LIBRARIES}
    d3d12.lib dxgi.lib d3dcompiler.lib
    Microsoft::DirectX-Guids Microsoft::DirectX-Headers Microsoft::DirectXTK12
)