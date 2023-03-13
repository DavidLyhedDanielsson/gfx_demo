#pragma once

#include <cstdint>

/**
 * @brief Wrapper around primitive data type.
 *
 * Only used for type-safety.
 */
struct AssetHandle
{
    uint32_t handle;

    AssetHandle(): handle(0) {}
    AssetHandle(uint32_t handle): handle(handle) {}

    operator uint32_t() const
    {
        return handle;
    }
    uint32_t operator*() const
    {
        return handle;
    }
};
