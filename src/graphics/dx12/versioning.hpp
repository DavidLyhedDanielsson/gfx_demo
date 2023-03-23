#pragma once

#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define CREATE_BASE(OBJECT)   \
    using OBJECT##N = OBJECT; \
    using OBJECT##S = ComPtr<OBJECT##N>;
#define CREATE_VERSION(OBJECT, VERSION) \
    using OBJECT##N = OBJECT##VERSION;  \
    using OBJECT##S = ComPtr<OBJECT##N>;

CREATE_BASE(IDXGIAdapter);
CREATE_VERSION(IDXGIFactory, 6); // EnumAdapterByGpuPreference
CREATE_VERSION(IDXGISwapChain, 3); // CreateSwapChainForHwnd, GetCurrentBackBufferIndex
CREATE_VERSION(ID3D12Debug, 1); // SetEnableGPUBasedValidation

CREATE_BASE(ID3D12Device);
CREATE_BASE(ID3D12InfoQueue);
CREATE_BASE(ID3D12Fence);
CREATE_BASE(ID3D12CommandQueue);
CREATE_BASE(ID3D12CommandAllocator);
CREATE_BASE(ID3D12DescriptorHeap);
CREATE_BASE(ID3D12Resource);
CREATE_BASE(ID3D12GraphicsCommandList);
CREATE_BASE(ID3D12RootSignature);
CREATE_BASE(ID3DBlob);
CREATE_BASE(ID3D12PipelineState);
CREATE_BASE(ID3D12Heap);

#undef CREATE_DEFAULT
#undef CREATE_VERSION

// Not really related to versioning but common utils
// https://stackoverflow.com/questions/47458206/pass-pointer-to-temporary-in-c-11
template<typename T>
const T* as_lvalue(const T&& val)
{
    return &val;
}

#ifdef Die
    #error Die already defined
#endif
#define Die(x)                                                                 \
    {                                                                          \
        HRESULT hRes = (x);                                                    \
        if(FAILED(hRes))                                                       \
        {                                                                      \
            _com_error err(hRes);                                              \
            std::cerr << "DIRECTX ERROR: " << err.ErrorMessage() << std::endl; \
            assert(false);                                                     \
        }                                                                      \
    }

#ifdef Out
    #error Out already defined
#endif
#define Out(X) IID_PPV_ARGS(X.GetAddressOf())