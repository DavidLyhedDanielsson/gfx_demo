#pragma once

template<typename Impl, typename Type>
struct IBlendState
{
    static constexpr Type Disabled = Impl::disabled();
};
