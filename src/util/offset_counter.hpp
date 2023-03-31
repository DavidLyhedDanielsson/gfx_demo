#pragma once

#include <util/align.hpp>

#include <cstdint>
#include <tuple>
#include <utility>

struct OffsetCounter
{
    // Inherit from tuple to enable std::tie
    struct Ret: std::tuple<uint32_t, uint32_t>
    {
        uint32_t& offset = std::get<0>(*this);
        uint32_t& size = std::get<1>(*this);

        Ret(uint32_t offset, uint32_t size): std::tuple<uint32_t, uint32_t>{offset, size} {}
    };

    uint32_t offset = 0;

    Ret append(uint32_t size)
    {
        uint32_t offsetBefore = std::exchange(offset, offset + size);
        return Ret{offsetBefore, size};
    }

    template<typename T>
    Ret append(uint32_t count)
    {
        uint32_t size = sizeof(T) * count;
        uint32_t offsetBefore = std::exchange(offset, offset + size);
        return Ret{offsetBefore, size};
    }

    template<typename T>
    Ret appendAligned(uint32_t count, uint32_t alignment)
    {
        uint32_t size = sizeof(T) * count;
        uint32_t alignedOffset = AlignTo(std::exchange(offset, AlignTo(offset, alignment) + size), alignment);
        return Ret{alignedOffset, size};
    }

    Ret appendAligned(uint32_t size, uint32_t alignment)
    {
        uint32_t alignedOffset = AlignTo(std::exchange(offset, AlignTo(offset, alignment) + size), alignment);
        return Ret{alignedOffset, size};
    }
};
