#pragma once

#include <cassert>
#include <cstring>
#include <memory>

/**
 * @brief Custom std::vector-like implementation that allows taking ownership of
 * its data pointer.
 *
 * Not meant to be a replacement for std::vector so functionality will be added
 * when needed.
 *
 * @tparam T
 */
template<typename T>
class FixedVector
{
    std::unique_ptr<uint8_t[]> mdata;
    size_t currentIndex;
    size_t maxIndex;

  public:
    FixedVector(): currentIndex(0), maxIndex(0) {}

    FixedVector(size_t maxItems)
        : mdata(new uint8_t[sizeof(T) * maxItems])
        , currentIndex(0)
        , maxIndex(maxItems)
    {
    }

    void push_back(const T& element)
    {
        assert(currentIndex < maxIndex);
        std::memcpy((void*)&mdata[sizeof(T) * currentIndex], &element, sizeof(T));
        currentIndex++;
    }

    void push_back(T&& data)
    {
        assert(currentIndex < maxIndex);
        ((T*)mdata.get())[currentIndex] = std::move(data);
        currentIndex++;
    }

    std::unique_ptr<uint8_t[]> take()
    {
        return std::move(mdata);
    }

    size_t size() const
    {
        return currentIndex;
    }

    size_t byte_size() const
    {
        return currentIndex * sizeof(T);
    }

    uint8_t* data()
    {
        return mdata.get();
    }

    void resize(size_t newSize)
    {
        this->mdata.reset(new uint8_t[sizeof(T) * newSize]);
        this->currentIndex = 0;
        this->maxIndex = newSize;
    }

    void set_data(void* src, size_t size)
    {
        assert(size <= this->maxIndex);
        assert(size % sizeof(T) == 0);

        memcpy(mdata.get(), src, size);
        currentIndex = size / sizeof(T);
    }

    const T& operator[](size_t i) const
    {
        assert(i < this->maxIndex);
        return ((T*)this->mdata.get())[i];
    }

    T& operator[](size_t i)
    {
        assert(i + 1 <= this->maxIndex);
        this->currentIndex = std::max(this->currentIndex, i + 1);
        return ((T*)this->mdata.get())[i];
    }
};
