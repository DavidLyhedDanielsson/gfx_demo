#pragma once

#include <cassert>
#include <cstdint>

/**
 * @brief A counter which keeps track of a max value and its current value.
 *
 * Useful when keeping track of offsets into buffers, the getAndAdd function
 * makes sure incrementing the offset is not forgotten - and, of course, turns
 * it into a single line.
 */
class OffsetCounter
{
    uint32_t max;
    uint32_t offset;

  public:
    OffsetCounter(): max(0), offset(0) {}
    OffsetCounter(uint32_t max): max(max), offset(0) {}
    OffsetCounter(const OffsetCounter&) = default;
    OffsetCounter(OffsetCounter&&) = default;
    OffsetCounter& operator=(const OffsetCounter&) = default;
    OffsetCounter& operator=(OffsetCounter&&) = default;

    uint32_t getAndAdd(uint32_t size)
    {
        assert(offset + size <= max);

        auto previous = offset;
        offset += size;
        return previous;
    }

    uint32_t getAvailable() const
    {
        return max - offset;
    }

    uint32_t getOffset() const
    {
        return offset;
    }

    void reset()
    {
        offset = 0;
    }
};
