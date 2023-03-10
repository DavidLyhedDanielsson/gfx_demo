#pragma once

#include <graphics/api/i_rasterizer_state.hpp>

#include <d3d12.h>

struct RasterizerState: public IRasterizerState<RasterizerState, D3D12_RASTERIZER_DESC>
{
    friend IRasterizerState;

  private:
    static constexpr D3D12_RASTERIZER_DESC normal() noexcept
    {
        return {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_NONE,
            .FrontCounterClockwise = false,
            .DepthBias = 0,
            .DepthBiasClamp = 0.0f,
            .SlopeScaledDepthBias = 0.0f,
            .DepthClipEnable = true,
            .MultisampleEnable = false,
            .AntialiasedLineEnable = false,
            .ForcedSampleCount = 0,
            .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        };
    };
};