#pragma once

template<typename Impl, typename Type>
struct IDepthStencilState
{
    static constexpr Type Enabled = Impl::enabled();
    static constexpr Type Disabled = Impl::disabled();
};
