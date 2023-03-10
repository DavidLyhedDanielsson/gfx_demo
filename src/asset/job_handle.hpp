#pragma once

#include <cstdint>

/**
 * @brief Wrapper around primitive data type.
 *
 * Only used for type-safety.
 */
struct JobHandle
{
    uint32_t handle;

    JobHandle(): handle(0) {}
    JobHandle(uint32_t handle): handle(handle) {}

    operator uint32_t() const
    {
        return handle;
    }
    uint32_t operator*() const
    {
        return handle;
    }
};
