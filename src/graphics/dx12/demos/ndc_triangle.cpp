#include "ndc_triangle.hpp"

#include <array>
#include <iostream>

#include <comdef.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include <graphics/dx12/blend_state.hpp>
#include <graphics/dx12/depth_stencil_state.hpp>
#include <graphics/dx12/rasterizer_state.hpp>
#include <graphics/dx12/versioning.hpp>

#include <asset/path.hpp>
#include <util/file_util.hpp>

static DX12Demo::NDCTriangle::State state;

namespace DX12Demo
{
namespace NDCTriangle
{

    void init(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight)
    {
        State state{};

        IDXGIFactoryS dxgiFactory;

        UINT factoryFlags = 0;
#ifndef NDEBUG
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        Die(CreateDXGIFactory2(factoryFlags, Out(dxgiFactory)));

#ifndef NDEBUG
        ID3D12DebugS debug;
        Die(D3D12GetDebugInterface(Out(debug)));
        debug->EnableDebugLayer();
#endif

        IDXGIAdapterS adapter;
        Die(dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, Out(adapter)));
        Die(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, Out(state.device)));

        auto& device = state.device;

        {
            state.sync.fenceCounter = 0;
            Die(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, Out(state.sync.flushFence)));
            state.sync.fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
            if(!state.sync.fenceEventHandle)
                Die(HRESULT_FROM_WIN32(GetLastError()));
        }

        state.descriptorSizes = {
            .rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
            .dsv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
            .cbvSrvUav = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        };

        {
            Die(device->CreateCommandQueue(
                as_lvalue(D3D12_COMMAND_QUEUE_DESC{
                    .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                    .Priority = 0,
                    .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                    .NodeMask = 0,

                }),
                Out(state.commandQueue)));
        }
        auto& commandQueue = state.commandQueue;

        {
            DXGI_SWAP_CHAIN_DESC1 desc;
            ComPtr<IDXGISwapChain1> swapChain1;

            Die(dxgiFactory->CreateSwapChainForHwnd(
                commandQueue.Get(),
                hWnd,
                as_lvalue(DXGI_SWAP_CHAIN_DESC1{
                    .Width = windowWidth,
                    .Height = windowHeight,
                    .Format = BACKBUFFER_FORMAT,
                    .Stereo = FALSE,
                    .SampleDesc = {.Count = MSAA_COUNT, .Quality = MSAA_QUALITY},
                    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                    .BufferCount = BACKBUFFER_COUNT,
                    .Scaling = DXGI_SCALING_STRETCH,
                    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
                    .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
                    .Flags = 0,

                }),
                nullptr,
                nullptr,
                swapChain1.GetAddressOf()));

            swapChain1.As(&state.swapChain);
        }
        auto& swapChain = state.swapChain;

        {
            Die(device->CreateDescriptorHeap(
                as_lvalue(D3D12_DESCRIPTOR_HEAP_DESC{
                    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                    .NumDescriptors = BACKBUFFER_COUNT,
                    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                    .NodeMask = 0,
                }),
                Out(state.heaps.rtv)));
        }

        auto& descriptorHeapRTV = state.heaps.rtv;
        {
            auto heapHandle = descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();
            for(uint32_t i{0}; i < BACKBUFFER_COUNT; ++i)
            {
                Die(swapChain->GetBuffer(i, Out(state.resources.swapChainBuffers[i])));
                device->CreateRenderTargetView(state.resources.swapChainBuffers[i].Get(), nullptr, heapHandle);
                heapHandle.ptr += state.descriptorSizes.rtv;
            }
        }

        {
            Die(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, Out(state.commandAllocator)));
            Die(device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                state.commandAllocator.Get(),
                nullptr,
                Out(state.commandList)));
        }

        {
            ID3DBlobS serialized;
            ID3DBlobS error;
            Die(D3D12SerializeVersionedRootSignature(
                as_lvalue(D3D12_VERSIONED_ROOT_SIGNATURE_DESC{
                    .Version = D3D_ROOT_SIGNATURE_VERSION_1,
                    .Desc_1_0 =
                        D3D12_ROOT_SIGNATURE_DESC{
                            .NumParameters = 0,
                            .pParameters = nullptr,
                            .NumStaticSamplers = 0,
                            .pStaticSamplers = nullptr,
                            .Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE,
                        },
                }),
                serialized.GetAddressOf(),
                error.GetAddressOf()));

            Die(device->CreateRootSignature(
                0,
                serialized->GetBufferPointer(),
                serialized->GetBufferSize(),
                Out(state.rootSignature)));
        }

        {
            std::vector vertexShaderCode = FileUtil::readFile(Path::getShaderPath("vs/ndc_triangle.bin")).value();
            Die(D3DCreateBlob(vertexShaderCode.size(), state.shaders.vertexBlob.GetAddressOf()));
            std::memcpy(state.shaders.vertexBlob->GetBufferPointer(), vertexShaderCode.data(), vertexShaderCode.size());
        }

        {
            std::vector pixelShaderCode = FileUtil::readFile(Path::getShaderPath("ps/white.bin")).value();
            Die(D3DCreateBlob(pixelShaderCode.size(), state.shaders.pixelBlob.GetAddressOf()));
            std::memcpy(state.shaders.pixelBlob->GetBufferPointer(), pixelShaderCode.data(), pixelShaderCode.size());
        }

        Die(device->CreateGraphicsPipelineState(
            as_lvalue(D3D12_GRAPHICS_PIPELINE_STATE_DESC{
                .pRootSignature = state.rootSignature.Get(),
                .VS =
                    {
                        .pShaderBytecode = state.shaders.vertexBlob->GetBufferPointer(),
                        .BytecodeLength = state.shaders.vertexBlob->GetBufferSize(),
                    },
                .PS =
                    {
                        .pShaderBytecode = state.shaders.pixelBlob->GetBufferPointer(),
                        .BytecodeLength = state.shaders.pixelBlob->GetBufferSize(),
                    },
                .DS = {},
                .HS = {},
                .GS = {},
                .StreamOutput = {},
                .BlendState = BlendState::Disabled,
                .SampleMask = UINT_MAX,
                .RasterizerState = RasterizerState::Normal,
                .DepthStencilState = DepthStencilState::Disabled,
                .InputLayout =
                    {
                        .pInputElementDescs = nullptr,
                        .NumElements = 0,
                    },
                .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = {BACKBUFFER_FORMAT},
                .DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
                .SampleDesc =
                    {
                        .Count = MSAA_COUNT,
                        .Quality = MSAA_QUALITY,
                    },
                .NodeMask = 0,
                .CachedPSO = {},
                .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
            }),
            Out(state.pipelineState)));

        state.commandList->Close();

        ::state = std::move(state);
    }

    void render(uint32_t windowWidth, uint32_t windowHeight)
    {
        auto& device = state.device;

        uint32_t currentFrame = state.swapChain->GetCurrentBackBufferIndex();

        state.commandAllocator->Reset();
        state.commandList->Reset(state.commandAllocator.Get(), state.pipelineState.Get());

        state.commandList->SetGraphicsRootSignature(state.rootSignature.Get());
        state.commandList->RSSetViewports(
            1,
            as_lvalue(D3D12_VIEWPORT{
                .TopLeftX = 0.0f,
                .TopLeftY = 0.0f,
                .Width = (FLOAT)windowWidth,
                .Height = (FLOAT)windowHeight,
                .MinDepth = 0.0f,
                .MaxDepth = 0.0f,
            }));
        state.commandList->RSSetScissorRects(
            1,
            as_lvalue(D3D12_RECT{
                .left = 0,
                .top = 0,
                .right = (LONG)windowWidth,
                .bottom = (LONG)windowHeight,
            }));

        // Implicit?
        state.commandList->ResourceBarrier(
            1,
            as_lvalue(D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition =
                    {
                        .pResource = state.resources.swapChainBuffers[currentFrame].Get(),
                        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                        .StateBefore = D3D12_RESOURCE_STATE_COMMON,
                        .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
                    },
            }));

        auto backBufferHandle = state.heaps.rtv->GetCPUDescriptorHandleForHeapStart();
        backBufferHandle.ptr += state.descriptorSizes.rtv * currentFrame;

        float clearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        state.commandList->ClearRenderTargetView(backBufferHandle, clearColor, 0, nullptr);
        state.commandList->OMSetRenderTargets(1, &backBufferHandle, true, nullptr);
        state.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        state.commandList->DrawInstanced(3, 1, 0, 0);

        state.commandList->ResourceBarrier(
            1,
            as_lvalue(D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition =
                    {
                        .pResource = state.resources.swapChainBuffers[currentFrame].Get(),
                        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                        .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
                        .StateAfter = D3D12_RESOURCE_STATE_COMMON,
                    },
            }));
        state.commandList->Close();

        ID3D12CommandList* commandList = state.commandList.Get();
        state.commandQueue->ExecuteCommandLists(1, &commandList);
        state.swapChain->Present(1, 0);

        const UINT64 fence = ++state.sync.fenceCounter;
        state.commandQueue->Signal(state.sync.flushFence.Get(), fence);

        if(state.sync.flushFence->GetCompletedValue() < fence)
        {
            state.sync.flushFence->SetEventOnCompletion(fence, state.sync.fenceEventHandle);
            WaitForSingleObject(state.sync.fenceEventHandle, INFINITE);
        }
    }
}
}