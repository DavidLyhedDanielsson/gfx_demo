#pragma once

#include <array>
#include <cstdint>

#include <graphics/dx12/versioning.hpp>

namespace dx12_demo
{
namespace DEMO_NAME
{
    constexpr uint32_t BACKBUFFER_COUNT = 3;
    constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr uint32_t MSAA_COUNT = 1;
    constexpr uint32_t MSAA_QUALITY = 0;

    struct State
    {
        ID3D12DeviceS device;
        IDXGISwapChainS swapChain;
        ID3D12CommandQueueS commandQueue;
        ID3D12CommandAllocatorS commandAllocator;
        ID3D12GraphicsCommandListS commandList;
        ID3D12RootSignatureS rootSignature;
        ID3D12PipelineStateS pipelineState;

        struct
        {
            ID3D12FenceS flushFence;
            uint64_t fenceCounter;
            HANDLE fenceEventHandle;
        } sync;

        struct
        {
            uint32_t rtv;
            uint32_t dsv;
            union
            {
                uint32_t cbvSrvUav;
                uint32_t cbv;
                uint32_t srv;
                uint32_t uav;
            };
        } descriptorSizes;

        struct
        {
            ID3D12DescriptorHeapS rtv;
        } heaps;

        struct
        {
            std::array<ID3D12ResourceS, BACKBUFFER_COUNT> swapChainBuffers;
        } resources;

        struct
        {
            ID3DBlobS vertexBlob;
            ID3DBlobS pixelBlob;
        } shaders;
    };

    void init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void render(uint32_t windowWidth, uint32_t windowHeight);
}
}