#pragma once

#include <cassert>
#include <cstdint>
#include <numeric>

constexpr uint32_t AlignTo(uint32_t val, uint32_t alignment)
{
    assert(std::popcount(alignment) == 1); // Must be power of two
    return (val + alignment - 1) / alignment * alignment;
}

constexpr uint32_t AlignTo256(uint32_t val)
{
    return AlignTo(val, 256);
}