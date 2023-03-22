#pragma once

#include <array>
#include <cstdint>

#include <graphics/dx12/versioning.hpp>

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <d3d12.h>

namespace DX12Demo
{
namespace CubedCat
{
    constexpr uint32_t BACKBUFFER_COUNT = 3;
    constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr uint32_t MSAA_COUNT = 1;
    constexpr uint32_t MSAA_QUALITY = 0;

    constexpr uint32_t TEXTURE_WIDTH = 512;
    constexpr uint32_t TEXTURE_HEIGHT = 512;
    constexpr uint32_t TEXTURE_CHANNELS = 4;

    constexpr DirectX::SimpleMath::Vector3 CAMERA_POSITION{0.0f, 0.0f, 5.0f};

    struct Vertex
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector2 uv;
    };

    // clang-format off
    constexpr auto vertices = std::to_array<Vertex>({
        // Front
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        // Back
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        // Top
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
        // Bottom
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
        // Left
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
        // Right
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},
    });

    constexpr auto indexData = std::to_array<uint32_t>({
         0,  1,  2,    0,  2,  3,
         4,  6,  5,    4,  7,  6,
         8,  9, 10,    8, 10, 11,
        12, 14, 13,   12, 15, 14,
        16, 18, 17,   16, 19, 18,
        20, 21, 22,   20, 22, 23,
    });
    // clang-format on

    inline uint32_t AlignTo(uint32_t val, uint32_t alignment)
    {
        assert(std::popcount(alignment) == 1); // Must be power of two
        return (val + alignment - 1) / alignment * alignment;
    }

    inline uint32_t AlignTo256(uint32_t val)
    {
        return AlignTo(val, 256);
    }

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
            ID3D12DescriptorHeapS srv;
        } heaps;

        struct
        {
            std::array<ID3D12ResourceS, BACKBUFFER_COUNT> swapChainBuffers;
            ID3D12ResourceS uploadBuffer;
            ID3D12ResourceS vertexPositionBuffer;
            ID3D12ResourceS vertexUvBuffer;
            ID3D12ResourceS vertexNormalBuffer;
            ID3D12ResourceS indexBuffer;
            ID3D12ResourceS texture;
        } resources;

        struct
        {
            ID3DBlobS vertexBlob;
            ID3DBlobS pixelBlob;
        } shaders;

        struct
        {
            uint32_t UPLOAD_VERTEX_POSITION_OFFSET = 0;
            // clang-format off
            // Note: this approach is manual and error-prone and shouldn't be used in a real project
            uint32_t UPLOAD_VERTEX_UV_OFFSET =      AlignTo256(UPLOAD_VERTEX_POSITION_OFFSET + sizeof(DirectX::XMFLOAT3) * vertices.size());
            uint32_t UPLOAD_INDEX_OFFSET =          AlignTo256(UPLOAD_VERTEX_UV_OFFSET       + sizeof(DirectX::XMFLOAT2) * vertices.size());
            uint32_t UPLOAD_CBV_TRANSFORM_OFFSET =  AlignTo256(UPLOAD_INDEX_OFFSET           + sizeof(indexData));
            uint32_t UPLOAD_TEXTURE_OFFSET =        AlignTo(UPLOAD_CBV_TRANSFORM_OFFSET      + sizeof(DirectX::SimpleMath::Matrix), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
            uint32_t UPLOAD_CBV_VIEWPROJ_OFFSET =   AlignTo256(UPLOAD_TEXTURE_OFFSET         + AlignTo(TEXTURE_WIDTH * TEXTURE_CHANNELS, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * TEXTURE_HEIGHT);
            uint32_t UPLOAD_BUFFER_SIZE =           AlignTo256(UPLOAD_CBV_VIEWPROJ_OFFSET    + sizeof(DirectX::XMFLOAT4X4));
            // clang-format on
        } constants;
    };

    void init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void render(uint32_t windowWidth, uint32_t windowHeight);
}
}