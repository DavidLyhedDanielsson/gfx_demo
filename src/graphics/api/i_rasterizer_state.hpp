#pragma once

template<typename Impl, typename Type>
struct IRasterizerState
{
    static constexpr Type Normal = Impl::normal();
};
