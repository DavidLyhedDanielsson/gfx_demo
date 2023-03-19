
#include "spinning_quad.hpp"

#include <DirectXMath.h>
#include <array>
#include <cstring>
#include <dxgiformat.h>
#include <iostream>

#include <SimpleMath.h>
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

static DX12Demo::SpinningQuad::State state;

namespace DX12Demo
{
namespace SpinningQuad
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
            device->CreateCommittedResource(
                as_lvalue(D3D12_HEAP_PROPERTIES{
                    .Type = D3D12_HEAP_TYPE_UPLOAD,
                    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // Mandatory
                    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN, // Mandatory
                    .CreationNodeMask = 0,
                    .VisibleNodeMask = 0,
                }),
                D3D12_HEAP_FLAG_NONE,
                as_lvalue(D3D12_RESOURCE_DESC{
                    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                    .Alignment =
                        0, // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
                    .Width = sizeof(vertexData) + sizeof(indexData) + (256 - sizeof(vertexData) - sizeof(indexData))
                             + sizeof(DirectX::SimpleMath::Matrix), // CBV must be 256-byte aligned
                    .Height = 1, // Mandatory
                    .DepthOrArraySize = 1, // Mandatory
                    .MipLevels = 1, // Mandatory
                    .Format = DXGI_FORMAT_UNKNOWN, // Mandatory
                    .SampleDesc =
                        {
                            .Count = 1, // Mandatory
                            .Quality = 0, // Mandatory
                        },
                    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR, // Mandatory
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                }),
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                nullptr,
                Out(state.resources.uploadBuffer));
        }

        {
            device->CreateCommittedResource(
                as_lvalue(D3D12_HEAP_PROPERTIES{
                    .Type = D3D12_HEAP_TYPE_DEFAULT,
                    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // Mandatory
                    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN, // Mandatory
                    .CreationNodeMask = 0,
                    .VisibleNodeMask = 0,
                }),
                D3D12_HEAP_FLAG_NONE,
                as_lvalue(D3D12_RESOURCE_DESC{
                    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                    .Alignment =
                        0, // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
                    .Width = sizeof(vertexData),
                    .Height = 1, // Mandatory
                    .DepthOrArraySize = 1, // Mandatory
                    .MipLevels = 1, // Mandatory
                    .Format = DXGI_FORMAT_UNKNOWN, // Mandatory
                    .SampleDesc =
                        {
                            .Count = 1, // Mandatory
                            .Quality = 0, // Mandatory
                        },
                    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR, // Mandatory
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                }),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                Out(state.resources.vertexBuffer));

            device->CreateCommittedResource(
                as_lvalue(D3D12_HEAP_PROPERTIES{
                    .Type = D3D12_HEAP_TYPE_DEFAULT,
                    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // Mandatory
                    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN, // Mandatory
                    .CreationNodeMask = 0,
                    .VisibleNodeMask = 0,
                }),
                D3D12_HEAP_FLAG_NONE,
                as_lvalue(D3D12_RESOURCE_DESC{
                    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                    .Alignment =
                        0, // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
                    .Width = sizeof(indexData),
                    .Height = 1, // Mandatory
                    .DepthOrArraySize = 1, // Mandatory
                    .MipLevels = 1, // Mandatory
                    .Format = DXGI_FORMAT_UNKNOWN, // Mandatory
                    .SampleDesc =
                        {
                            .Count = 1, // Mandatory
                            .Quality = 0, // Mandatory
                        },
                    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR, // Mandatory
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                }),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                Out(state.resources.indexBuffer));
        }

        {
            void* uploadBufferDataPointer;
            state.resources.uploadBuffer->Map(0, nullptr, &uploadBufferDataPointer);
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_VERTEX_OFFSET,
                vertexData.data(),
                sizeof(vertexData));
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_INDEX_OFFSET,
                indexData.data(),
                sizeof(indexData));

            DirectX::SimpleMath::Matrix transform = DirectX::SimpleMath::Matrix::CreateRotationZ(DirectX::XM_PIDIV2);
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_CBV_OFFSET,
                &transform,
                sizeof(transform));
            state.resources.uploadBuffer->Unmap(0, nullptr);

            // TODO: Implicit transition?
            state.commandList->ResourceBarrier(
                1,
                as_lvalue(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition =
                        {
                            .pResource = state.resources.vertexBuffer.Get(),
                            .Subresource = 0,
                            .StateBefore = D3D12_RESOURCE_STATE_COMMON,
                            .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
                        },
                }));

            state.commandList->CopyBufferRegion(
                state.resources.vertexBuffer.Get(),
                0,
                state.resources.uploadBuffer.Get(),
                state.constants.UPLOAD_VERTEX_OFFSET,
                sizeof(vertexData));
            state.commandList->CopyBufferRegion(
                state.resources.indexBuffer.Get(),
                0,
                state.resources.uploadBuffer.Get(),
                state.constants.UPLOAD_INDEX_OFFSET,
                sizeof(indexData));

            state.commandList->ResourceBarrier(
                1,
                as_lvalue(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition =
                        {
                            .pResource = state.resources.vertexBuffer.Get(),
                            .Subresource = 0,
                            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
                            .StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                        },
                }));
        }

        {
            std::array rootParameters = std::to_array({D3D12_ROOT_PARAMETER{
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                .Descriptor =
                    D3D12_ROOT_DESCRIPTOR{
                        .ShaderRegister = 0,
                        .RegisterSpace = 0,
                    },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
            }});

            ID3DBlobS serialized;
            ID3DBlobS error;
            Die(D3D12SerializeVersionedRootSignature(
                as_lvalue(D3D12_VERSIONED_ROOT_SIGNATURE_DESC{
                    .Version = D3D_ROOT_SIGNATURE_VERSION_1,
                    .Desc_1_0 =
                        D3D12_ROOT_SIGNATURE_DESC{
                            .NumParameters = rootParameters.size(),
                            .pParameters = rootParameters.data(),
                            .NumStaticSamplers = 0,
                            .pStaticSamplers = nullptr,
                            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
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
            std::vector vertexShaderCode =
                FileUtil::readFile(Path::getShaderPath("vs/vertex_triangle_spinning.bin")).value();
            Die(D3DCreateBlob(vertexShaderCode.size(), state.shaders.vertexBlob.GetAddressOf()));
            std::memcpy(state.shaders.vertexBlob->GetBufferPointer(), vertexShaderCode.data(), vertexShaderCode.size());
        }

        {
            std::vector pixelShaderCode = FileUtil::readFile(Path::getShaderPath("ps/white.bin")).value();
            Die(D3DCreateBlob(pixelShaderCode.size(), state.shaders.pixelBlob.GetAddressOf()));
            std::memcpy(state.shaders.pixelBlob->GetBufferPointer(), pixelShaderCode.data(), pixelShaderCode.size());
        }

        {
            std::array inputLayout = std::to_array({
                D3D12_INPUT_ELEMENT_DESC{
                    .SemanticName = "POSITION",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 0,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0, // Mandatory
                },
            });

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
                            .pInputElementDescs = inputLayout.data(),
                            .NumElements = inputLayout.size(),
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
        }

        state.commandList->Close();

        ID3D12CommandList* commandList = state.commandList.Get();
        state.commandQueue->ExecuteCommandLists(1, &commandList);

        const UINT64 fence = ++state.sync.fenceCounter;
        state.commandQueue->Signal(state.sync.flushFence.Get(), fence);

        if(state.sync.flushFence->GetCompletedValue() < fence)
        {
            state.sync.flushFence->SetEventOnCompletion(fence, state.sync.fenceEventHandle);
            WaitForSingleObject(state.sync.fenceEventHandle, INFINITE);
        }

        ::state = std::move(state);
    }

    float time = 0.0f;

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

        time += 1 / 60.0f; // vsync is on, so this should be fine

        void* uploadBufferDataPointer;
        state.resources.uploadBuffer->Map(0, nullptr, &uploadBufferDataPointer);
        DirectX::SimpleMath::Matrix transform = DirectX::SimpleMath::Matrix::CreateRotationZ(time);
        std::memcpy((char*)uploadBufferDataPointer + state.constants.UPLOAD_CBV_OFFSET, &transform, sizeof(transform));
        state.resources.uploadBuffer->Unmap(0, nullptr);

        auto backBufferHandle = state.heaps.rtv->GetCPUDescriptorHandleForHeapStart();
        backBufferHandle.ptr += state.descriptorSizes.rtv * currentFrame;

        float clearColor[4] = {69.0f / 255.0f, 133.0f / 255.0f, 136.0f / 255.0f, 1.0f};
        state.commandList->ClearRenderTargetView(backBufferHandle, clearColor, 0, nullptr);
        state.commandList->OMSetRenderTargets(1, &backBufferHandle, true, nullptr);
        state.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        state.commandList->IASetVertexBuffers(
            0,
            1,
            as_lvalue(D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = state.resources.vertexBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = sizeof(vertexData),
                .StrideInBytes = sizeof(float) * 2,
            }));
        state.commandList->IASetIndexBuffer(as_lvalue(D3D12_INDEX_BUFFER_VIEW{
            .BufferLocation = state.resources.indexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = sizeof(indexData),
            .Format = DXGI_FORMAT_R32_UINT,
        }));
        state.commandList->SetGraphicsRootConstantBufferView(
            0,
            state.resources.uploadBuffer->GetGPUVirtualAddress() + state.constants.UPLOAD_CBV_OFFSET);
        state.commandList->DrawIndexedInstanced(indexData.size(), 1, 0, 0, 0);

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