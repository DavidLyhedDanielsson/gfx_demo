#include "cubed_cat.hpp"

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

#include <util/file_util.hpp>
#include <util/path.hpp>
#include <util/stbi.hpp>

static DX12Demo::CubedCat::State state;

namespace SimpleMath = DirectX::SimpleMath;

namespace DX12Demo
{
namespace CubedCat
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
        debug->SetEnableGPUBasedValidation(true);
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

            Die(device->CreateDescriptorHeap(
                as_lvalue(D3D12_DESCRIPTOR_HEAP_DESC{
                    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                    .NumDescriptors = 1,
                    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                    .NodeMask = 0,
                }),
                Out(state.heaps.srv)));
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

        uint32_t textureRowPitch;
        std::vector<char> textureData;
        {
            auto catPath = Path::getRandomCat();

            // Immediately invoked function expressions (IIFE)
            const auto stbiData = [&]() {
                // This scope prevents usage of `width`, `height`, and `channels`
                int width;
                int height;
                int channels;

                // Windows uses wchar in std::filesystem::path >:(
#ifdef _WIN32
                char stbiBuf[512];
                stbi_convert_wchar_to_utf8(stbiBuf, sizeof(stbiBuf), catPath.c_str());

                auto stbiData = std::unique_ptr<stbi_uc, void (*)(void*)>(
                    stbi_load(stbiBuf, &width, &height, &channels, STBI_rgb_alpha), // everything will break if this is
                                                                                    // changed from STBI_rgb_alpha :)
                    stbi_image_free);
#else
                auto stbiData = std::unique_ptr<stbi_uc, void (*)(void*)>(
                    stbi_load(catPath.c_str(), &width, &textureHeight, &channels, STBI_rgb_alpha),
                    stbi_image_free);
#endif
                assert(width == TEXTURE_WIDTH);
                assert(height == TEXTURE_HEIGHT);

                return stbiData;
            }();

            textureRowPitch = AlignTo(TEXTURE_WIDTH * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

            textureData.resize(textureRowPitch * TEXTURE_HEIGHT);
            for(uint32_t i = 0; i < TEXTURE_HEIGHT; ++i)
                std::memcpy(
                    textureData.data() + textureRowPitch * i,
                    stbiData.get() + TEXTURE_WIDTH * 4 * i,
                    TEXTURE_HEIGHT * 4);
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
                    .Width = state.constants.UPLOAD_BUFFER_SIZE,
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
                    .Width = sizeof(DirectX::XMFLOAT3) * vertices.size(),
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
                Out(state.resources.vertexPositionBuffer));
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
                    .Width = sizeof(DirectX::XMFLOAT2) * vertices.size(),
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
                Out(state.resources.vertexUvBuffer));

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
                    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                    .Alignment =
                        0, // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
                    .Width = 512,
                    .Height = 512,
                    .DepthOrArraySize = 1, // Mandatory
                    .MipLevels = 1,
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .SampleDesc =
                        {
                            .Count = 1, // Mandatory
                            .Quality = 0, // Mandatory
                        },
                    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN, // Mandatory
                    .Flags = D3D12_RESOURCE_FLAG_NONE,
                }),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                Out(state.resources.texture));
        }

        {
            device->CreateShaderResourceView(
                state.resources.texture.Get(),
                nullptr,
                state.heaps.srv->GetCPUDescriptorHandleForHeapStart());
        }

        {
            void* uploadBufferDataPointer;
            state.resources.uploadBuffer->Map(0, nullptr, &uploadBufferDataPointer);

            uint32_t i = 0;
            for(const auto [position, uv] : vertices)
            {
                std::memcpy(
                    (char*)uploadBufferDataPointer + state.constants.UPLOAD_VERTEX_POSITION_OFFSET
                        + sizeof(position) * i,
                    &position,
                    sizeof(position));
                std::memcpy(
                    (char*)uploadBufferDataPointer + state.constants.UPLOAD_VERTEX_UV_OFFSET + sizeof(uv) * i,
                    &uv,
                    sizeof(uv));

                ++i;
            }
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_INDEX_OFFSET,
                indexData.data(),
                sizeof(indexData));

            SimpleMath::Matrix transformMatrix = SimpleMath::Matrix::CreateRotationZ(0.0f).Transpose();
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_CBV_TRANSFORM_OFFSET,
                &transformMatrix,
                sizeof(transformMatrix));
            // Note the transpose!
            SimpleMath::Matrix viewProjectionMatrix =
                (SimpleMath::Matrix::CreateLookAt(CAMERA_POSITION, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f})
                 * SimpleMath::Matrix::CreatePerspectiveFieldOfView(
                     DirectX::XMConvertToRadians(59.0f), // 59.0f = 90 deg horizontal FoV at 16:9
                     windowWidth / (float)windowHeight,
                     1.0f,
                     10.0f))
                    .Transpose();
            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_CBV_VIEWPROJ_OFFSET,
                &viewProjectionMatrix,
                sizeof(viewProjectionMatrix));

            std::memcpy(
                (char*)uploadBufferDataPointer + state.constants.UPLOAD_TEXTURE_OFFSET,
                textureData.data(),
                textureData.size());
            state.resources.uploadBuffer->Unmap(0, nullptr);

            // TODO: Implicit transition?
            // state.commandList->ResourceBarrier(
            //     1,
            //     as_lvalue(D3D12_RESOURCE_BARRIER{
            //         .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            //         .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            //         .Transition =
            //             {
            //                 .pResource = state.resources.vertexBuffer.Get(),
            //                 .Subresource = 0,
            //                 .StateBefore = D3D12_RESOURCE_STATE_COMMON,
            //                 .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
            //             },
            //     }));

            state.commandList->CopyBufferRegion(
                state.resources.vertexPositionBuffer.Get(),
                0,
                state.resources.uploadBuffer.Get(),
                state.constants.UPLOAD_VERTEX_POSITION_OFFSET,
                sizeof(DirectX::XMFLOAT3) * vertices.size());
            state.commandList->CopyBufferRegion(
                state.resources.vertexUvBuffer.Get(),
                0,
                state.resources.uploadBuffer.Get(),
                state.constants.UPLOAD_VERTEX_UV_OFFSET,
                sizeof(DirectX::XMFLOAT2) * vertices.size());
            state.commandList->CopyBufferRegion(
                state.resources.indexBuffer.Get(),
                0,
                state.resources.uploadBuffer.Get(),
                state.constants.UPLOAD_INDEX_OFFSET,
                sizeof(indexData));
            state.commandList->CopyTextureRegion(
                as_lvalue(D3D12_TEXTURE_COPY_LOCATION{
                    .pResource = state.resources.texture.Get(),
                    .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                    .SubresourceIndex = 0,
                }),
                0,
                0,
                0,
                as_lvalue(D3D12_TEXTURE_COPY_LOCATION{
                    .pResource = state.resources.uploadBuffer.Get(),
                    .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                    .PlacedFootprint =
                        D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
                            .Offset = state.constants.UPLOAD_TEXTURE_OFFSET,
                            .Footprint =
                                D3D12_SUBRESOURCE_FOOTPRINT{
                                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                    .Width = 512,
                                    .Height = 512,
                                    .Depth = 1,
                                    .RowPitch = textureRowPitch,
                                },
                        }}),
                nullptr);

            // Note: resource has been promoted into COPY_DEST
            state.commandList->ResourceBarrier(
                1,
                as_lvalue(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition =
                        {
                            .pResource = state.resources.texture.Get(),
                            .Subresource = 0,
                            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
                            .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                        },
                }));
        }

        {
            std::array rootParameters = std::to_array({
                D3D12_ROOT_PARAMETER{
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor =
                        D3D12_ROOT_DESCRIPTOR{
                            .ShaderRegister = 0,
                            .RegisterSpace = 0,
                        },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
                },
                D3D12_ROOT_PARAMETER{
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor =
                        D3D12_ROOT_DESCRIPTOR{
                            .ShaderRegister = 1,
                            .RegisterSpace = 0,
                        },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
                },
                D3D12_ROOT_PARAMETER{
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                    .DescriptorTable =
                        D3D12_ROOT_DESCRIPTOR_TABLE{
                            .NumDescriptorRanges = 1,
                            .pDescriptorRanges = as_lvalue(D3D12_DESCRIPTOR_RANGE{
                                .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                                .NumDescriptors = 1,
                                .BaseShaderRegister = 0,
                                .RegisterSpace = 0,
                                .OffsetInDescriptorsFromTableStart = 0,
                            })},
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
                },
            });

            std::array samplers = std::to_array({D3D12_STATIC_SAMPLER_DESC{
                .Filter = D3D12_FILTER_ANISOTROPIC,
                .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                .MipLODBias = 0.0f,
                .MaxAnisotropy = 16,
                .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
                .MinLOD = 0.0f,
                .MaxLOD = 0.0,
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
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
                            .NumStaticSamplers = samplers.size(),
                            .pStaticSamplers = samplers.data(),
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
            std::vector vertexShaderCode = FileUtil::readFile(Path::getShaderPath("vs/cubed_cat.bin")).value();
            Die(D3DCreateBlob(vertexShaderCode.size(), state.shaders.vertexBlob.GetAddressOf()));
            std::memcpy(state.shaders.vertexBlob->GetBufferPointer(), vertexShaderCode.data(), vertexShaderCode.size());
        }

        {
            std::vector pixelShaderCode = FileUtil::readFile(Path::getShaderPath("ps/spinning_cat.bin")).value();
            Die(D3DCreateBlob(pixelShaderCode.size(), state.shaders.pixelBlob.GetAddressOf()));
            std::memcpy(state.shaders.pixelBlob->GetBufferPointer(), pixelShaderCode.data(), pixelShaderCode.size());
        }

        {
            std::array inputLayout = std::to_array({
                D3D12_INPUT_ELEMENT_DESC{
                    .SemanticName = "POSITION",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                    .InputSlot = 0,
                    .AlignedByteOffset = 0,
                    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0, // Mandatory
                },
                D3D12_INPUT_ELEMENT_DESC{
                    .SemanticName = "UV",
                    .SemanticIndex = 0,
                    .Format = DXGI_FORMAT_R32G32_FLOAT,
                    .InputSlot = 1,
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
        SimpleMath::Matrix transform =
            (SimpleMath::Matrix::CreateRotationZ(time) * SimpleMath::Matrix::CreateRotationY(time * 0.5f)).Transpose();
        std::memcpy(
            (char*)uploadBufferDataPointer + state.constants.UPLOAD_CBV_TRANSFORM_OFFSET,
            &transform,
            sizeof(transform));
        state.resources.uploadBuffer->Unmap(0, nullptr);

        auto backBufferHandle = state.heaps.rtv->GetCPUDescriptorHandleForHeapStart();
        backBufferHandle.ptr += state.descriptorSizes.rtv * currentFrame;

        float clearColor[4] = {69.0f / 255.0f, 133.0f / 255.0f, 136.0f / 255.0f, 1.0f};
        state.commandList->ClearRenderTargetView(backBufferHandle, clearColor, 0, nullptr);
        state.commandList->OMSetRenderTargets(1, &backBufferHandle, true, nullptr);
        state.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        std::array bufferViews{
            D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = state.resources.vertexPositionBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = sizeof(DirectX::XMFLOAT3) * vertices.size(),
                .StrideInBytes = sizeof(float) * 3,
            },
            D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = state.resources.vertexUvBuffer->GetGPUVirtualAddress(),
                .SizeInBytes = sizeof(DirectX::XMFLOAT2) * vertices.size(),
                .StrideInBytes = sizeof(float) * 2,
            },
        };
        state.commandList->IASetVertexBuffers(0, bufferViews.size(), bufferViews.data());
        state.commandList->IASetIndexBuffer(as_lvalue(D3D12_INDEX_BUFFER_VIEW{
            .BufferLocation = state.resources.indexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = sizeof(indexData),
            .Format = DXGI_FORMAT_R32_UINT,
        }));
        state.commandList->SetGraphicsRootConstantBufferView(
            0,
            state.resources.uploadBuffer->GetGPUVirtualAddress() + state.constants.UPLOAD_CBV_TRANSFORM_OFFSET);
        state.commandList->SetGraphicsRootConstantBufferView(
            1,
            state.resources.uploadBuffer->GetGPUVirtualAddress() + state.constants.UPLOAD_CBV_VIEWPROJ_OFFSET);

        state.commandList->SetDescriptorHeaps(1, state.heaps.srv.GetAddressOf());
        state.commandList->SetGraphicsRootDescriptorTable(2, state.heaps.srv->GetGPUDescriptorHandleForHeapStart());
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