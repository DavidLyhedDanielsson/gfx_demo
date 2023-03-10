#pragma once

#include <graphics/api/i_blend_state.hpp>

#include <d3d12.h>

struct BlendState: public IBlendState<BlendState, D3D12_BLEND_DESC>
{
    friend IBlendState;

  private:
    static constexpr D3D12_BLEND_DESC disabled() noexcept
    {
        return {
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
};