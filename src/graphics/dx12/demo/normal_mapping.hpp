#pragma once

#include <array>
#include <cstdint>
#include <numeric>

#include <graphics/dx12/versioning.hpp>

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <d3d12.h>

namespace dx12_demo
{
namespace DEMO_NAME
{
    constexpr uint32_t BACKBUFFER_COUNT = 3;
    constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr uint32_t MSAA_COUNT = 1;
    constexpr uint32_t MSAA_QUALITY = 0;

    constexpr uint32_t TEXTURE_WIDTH = 512;
    constexpr uint32_t TEXTURE_HEIGHT = 512;
    constexpr uint32_t TEXTURE_CHANNELS = 4;

    constexpr DirectX::SimpleMath::Vector3 CAMERA_POSITION{0.0f, 0.0f, -3.0f};

    struct Vertex
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector2 uv;
        DirectX::SimpleMath::Vector3 normal;
        DirectX::SimpleMath::Vector3 tangent;
    };

    inline uint32_t AlignTo(uint32_t val, uint32_t alignment)
    {
        assert(std::popcount(alignment) == 1); // Must be power of two
        return (val + alignment - 1) / alignment * alignment;
    }

    inline uint32_t AlignTo256(uint32_t val)
    {
        return AlignTo(val, 256);
    }

    struct OffsetCounter
    {
        uint32_t offset = 0;

        std::tuple<uint32_t, uint32_t> append(uint32_t size)
        {
            auto offsetBefore = std::exchange(offset, offset + size);
            return std::make_tuple(offsetBefore, size);
        }

        template<typename T>
        std::tuple<uint32_t, uint32_t> append(uint32_t count)
        {
            auto size = sizeof(T) * count;
            auto offsetBefore = std::exchange(offset, offset + size);
            return std::make_tuple(offsetBefore, size);
        }

        template<typename T>
        std::tuple<uint32_t, uint32_t> appendAligned(uint32_t count, uint32_t alignment)
        {
            auto size = sizeof(T) * count;
            auto alignedOffset = AlignTo(std::exchange(offset, AlignTo(offset, alignment) + size), alignment);
            return std::make_tuple(alignedOffset, size);
        }

        std::tuple<uint32_t, uint32_t> appendAligned(uint32_t size, uint32_t alignment)
        {
            auto alignedOffset = AlignTo(std::exchange(offset, AlignTo(offset, alignment) + size), alignment);
            return std::make_tuple(alignedOffset, size);
        }
    };

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
            ID3D12ResourceS vertexTangentBuffer;
            ID3D12ResourceS indexBuffer;
            ID3D12ResourceS textureDiffuse;
            ID3D12ResourceS textureNormal;
        } resources;

        struct
        {
            ID3DBlobS vertexBlob;
            ID3DBlobS pixelBlob;
        } shaders;

        struct
        {
            uint32_t VERTEX_POSITION_OFFSET = -1;
            uint32_t VERTEX_POSITION_SIZE = -1;
            uint32_t VERTEX_UV_OFFSET = -1;
            uint32_t VERTEX_UV_SIZE = -1;
            uint32_t VERTEX_NORMAL_OFFSET = -1;
            uint32_t VERTEX_NORMAL_SIZE = -1;
            uint32_t VERTEX_TANGENT_OFFSET = -1;
            uint32_t VERTEX_TANGENT_SIZE = -1;
            uint32_t INDEX_OFFSET = -1;
            uint32_t INDEX_SIZE = -1;
            uint32_t CBV_TRANSFORM_OFFSET = -1;
            uint32_t CBV_TRANSFORM_SIZE = -1;
            uint32_t CBV_VIEWPROJ_OFFSET = -1;
            uint32_t CBV_VIEWPROJ_SIZE = -1;
            uint32_t TEXTURE_DIFFUSE_OFFSET = -1;
            uint32_t TEXTURE_DIFFUSE_SIZE = -1;
            uint32_t TEXTURE_NORMAL_OFFSET = -1;
            uint32_t TEXTURE_NORMAL_SIZE = -1;
            uint32_t UPLOAD_BUFFER_SIZE = -1;
        } constants;

        std::vector<uint32_t> indexData;
        std::vector<Vertex> vertexData;
    };

    void init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void render(uint32_t windowWidth, uint32_t windowHeight);
}
}