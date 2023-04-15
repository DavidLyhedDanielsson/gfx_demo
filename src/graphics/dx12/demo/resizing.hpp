#pragma once

#include <array>
#include <cstdint>
#include <numeric>

#include <graphics/dx12/versioning.hpp>
#include <util/align.hpp>
#include <util/offset_counter.hpp>

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <d3d12.h>

namespace dx12_demo
{
namespace DEMO_NAME
{
    constexpr uint32_t BACKBUFFER_COUNT = 3;
    constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;
    constexpr uint32_t MSAA_COUNT = -1; // Highest will be picked at runtime
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

    struct State
    {
        ID3D12DeviceS device;
        IDXGISwapChainS swapChain;
        ID3D12CommandQueueS commandQueue;
        ID3D12CommandAllocatorS commandAllocator;
        ID3D12GraphicsCommandListS commandList;
        ID3D12RootSignatureS rootSignature;
        ID3D12PipelineStateS pipelineState;
        uint32_t msaaCount;

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
            ID3D12DescriptorHeapS dsv;
        } heaps;

        struct
        {
            std::array<ID3D12ResourceS, BACKBUFFER_COUNT> swapChainBuffers;
            ID3D12ResourceS renderTargetBuffer;
            ID3D12ResourceS depthStencilBuffer; // TODO: Not really a buffer
            ID3D12ResourceS uploadBuffer;
            ID3D12ResourceS vertexPositionBuffer;
            ID3D12ResourceS vertexUvBuffer;
            ID3D12ResourceS vertexNormalBuffer;
            ID3D12ResourceS vertexTangentBuffer;
            ID3D12ResourceS indexBuffer;
            ID3D12ResourceS textureAlbedo;
            ID3D12ResourceS textureAmbient;
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
            uint32_t CBV_TRANSFORM_0_OFFSET = -1;
            uint32_t CBV_TRANSFORM_0_SIZE = -1;
            uint32_t CBV_TRANSFORM_1_OFFSET = -1;
            uint32_t CBV_TRANSFORM_1_SIZE = -1;
            uint32_t CBV_VIEWPROJ_OFFSET = -1;
            uint32_t CBV_VIEWPROJ_SIZE = -1;
            uint32_t TEXTURE_ALBEDO_OFFSET = -1;
            uint32_t TEXTURE_ALBEDO_SIZE = -1;
            uint32_t TEXTURE_AMBIENT_OFFSET = -1;
            uint32_t TEXTURE_AMBIENT_SIZE = -1;
            uint32_t TEXTURE_NORMAL_OFFSET = -1;
            uint32_t TEXTURE_NORMAL_SIZE = -1;
            uint32_t UPLOAD_BUFFER_SIZE = -1;
        } constants;

        std::vector<uint32_t> indexData;
        std::vector<Vertex> vertexData;
    };

    void init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void render(uint32_t windowWidth, uint32_t windowHeight);
    void resize(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
    void destroy();
}
}