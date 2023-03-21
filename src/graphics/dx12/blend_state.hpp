#pragma once

#include <d3d12.h>

struct BlendState
{
    static constexpr D3D12_BLEND_DESC Disabled{
        .AlphaToCoverageEnable = false,
        .IndependentBlendEnable = false,
        .RenderTarget = {{
            .BlendEnable = false,
            .LogicOpEnable = false,
            .SrcBlend = D3D12_BLEND_ONE,
            .DestBlend = D3D12_BLEND_ZERO,
            .BlendOp = D3D12_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D12_BLEND_ONE,
            .DestBlendAlpha = D3D12_BLEND_ZERO,
            .BlendOpAlpha = D3D12_BLEND_OP_ADD,
            .LogicOp = D3D12_LOGIC_OP_NOOP,
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        }},
    };
};