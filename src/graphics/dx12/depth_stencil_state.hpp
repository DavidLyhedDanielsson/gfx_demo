#pragma once

#include <d3d12.h>

struct DepthStencilState
{
    static constexpr D3D12_DEPTH_STENCIL_DESC Enabled{
        .DepthEnable = true,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = false,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace =
            {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
        .BackFace =
            {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
    };

    static constexpr D3D12_DEPTH_STENCIL_DESC Disabled{
        .DepthEnable = false,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = D3D12_COMPARISON_FUNC_NEVER,
        .StencilEnable = false,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace =
            {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
        .BackFace =
            {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
    };
};