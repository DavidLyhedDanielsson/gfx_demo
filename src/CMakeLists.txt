# Shaders
add_custom_target(shader)
set(SHADER_SRC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/shader/)
set(SHADER_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)

set(SHADER_PS
    "white"
    "spinning_cat"
    "normal_mapping"
)

set(SHADER_VS
    "ndc_triangle"
    "vertex_triangle_ia"
    "vertex_triangle_pull"
    "vertex_triangle_spinning"
    "spinning_cat"
    "perspective_cat"
    "cubed_cat"
    "normal_mapping"
)

add_custom_command(
        TARGET shader
        MAIN_DEPENDENCY ${SHADER_OUTPUT_DIR}
        COMMENT "Creating shader output directories"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}/vs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}/ps
        VERBATIM)

function(CompileShader TYPE FILE_NAME)
    string(TOLOWER ${TYPE} TYPE)
    add_custom_command(
            TARGET shader
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
    path.cpp path.hpp
    stbi.cpp stbi.hpp
)
list(TRANSFORM SRC_UTIL PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/util/)

set(SRC_DX
    blend_state.hpp
    depth_stencil_state.hpp
    rasterizer_state.hpp
    versioning.hpp
)
list(TRANSFORM SRC_DX PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/)

function(create_demo DEMO_NAME)
    if(NOT ARGN)
        set("NONE" ARGN)
    endif()

    foreach(VARIANT ${ARGN})
        set(SOURCE_FILE_NAME ${DEMO_NAME})
        if(VARIANT EQUAL "NONE")
            set(EXECUTABLE_NAME ${DEMO_NAME})
        else()
            string(TOLOWER ${DEMO_NAME}_${VARIANT} EXECUTABLE_NAME)
        endif()

        add_executable(${EXECUTABLE_NAME}
            ${SRC_UTIL}
            ${SRC_DX}
            main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/demo/${SOURCE_FILE_NAME}.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/graphics/dx12/demo/${SOURCE_FILE_NAME}.hpp
        )
        add_dependencies(${EXECUTABLE_NAME}
            shader
        )

        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
            ROOT_DIR_ASSET=${ROOT_DIR_ASSET_STR}
            ROOT_DIR_SHADER=${ROOT_DIR_SHADER_STR}
            DEMO_NAME=${DEMO_NAME}
            DEMO_VARIANT_${VARIANT}
        )
        target_include_directories(${EXECUTABLE_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE
            SDL2::SDL2 SDL2::SDL2main
            EnTT::EnTT
            assimp::assimp
            d3d12.lib dxgi.lib d3dcompiler.lib
            Microsoft::DirectX-Guids Microsoft::DirectX-Headers Microsoft::DirectXTK12
        )
    endforeach()
endfunction()

create_demo(ndc_triangle)
create_demo(vertex_triangle_ia)
create_demo(vertex_triangle_pull)
create_demo(spinning_triangle)
create_demo(spinning_quad)
create_demo(spinning_cat)
create_demo(perspective_cat)
create_demo(cubed_cat)
create_demo(placed_cat)
create_demo(normal_mapping)