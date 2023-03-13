#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>

#include <stl_util/index_iterator.hpp>
#include <util/offset_counter.hpp>

/**
 * @brief A circular buffer which lets the user control when the next sections
 * are available.
 *
 * The buffer does *not* own its memory, allowing it to be used with
 * memory mapped pointers.
 *
 * @tparam BufferCount the number of buffers the circular buffer should contain
 */
template<uint32_t BufferCount>
class CircularBuffer
{
    void* memory;
    uint32_t bufferSize;
    std::array<OffsetCounter, BufferCount> counters;

    uint32_t lastFreedIndex;
    // If the buffer is full, there is no section to write to
    std::optional<uint32_t> writeSectionOpt;
    bool dataAvailable;

    // Request a new section immediately?
    bool currentSectionInvalidated;

    uint32_t nextSection(uint32_t val)
    {
        return (val + 1) % BufferCount;
    }

    uint32_t sectionOffset(uint32_t section)
    {
        return bufferSize / BufferCount * section;
    }

  public:
    CircularBuffer()
        : memory(nullptr)
        , bufferSize(0)
        , lastFreedIndex(0)
        , writeSectionOpt(0)
        , dataAvailable(false)
        , currentSectionInvalidated(false)
    {
    }

    CircularBuffer(void* memory, uint32_t bufferSize)
        : memory(memory)
        , bufferSize(bufferSize)
        , lastFreedIndex(BufferCount - 1)
        , writeSectionOpt(0)
        , dataAvailable(false)
        , currentSectionInvalidated(false)
    {
        assert(bufferSize / BufferCount * BufferCount == bufferSize);

        for(auto [counter, i] : Index(counters))
            counter = OffsetCounter(bufferSize / BufferCount);
    }

    /**
     * @brief Copy data into the circular buffer.
     *
     * @tparam Func a function with the signature bool(uint32_t section)
     * Should return "true" if the given section is available for writing
     * @tparam Func2 a function with the signature void(uint32_t section,
     * uint32_t offset). `section` is the section to which data was added, and
     * `offset` is at which offset the data was inserted
     * @param source data source
     * @param size byte size of the data
     * @param indexReady a function which will be called when the current buffer
     * section is full and another section is required
     * @param dataPlaced a function which will be called if data is added into
     * the buffer by this function call
     * @return true if there is more room in the buffer
     * @return false if the buffer is full
     */
    template<typename Func, typename Func2>
    bool memcpy(void* source, uint32_t size, const Func& indexReady, const Func2& dataPlaced)
    {
        assert(size <= bufferSize / BufferCount);

        auto goToNextSection = [&] {
            this->currentSectionInvalidated = false;

            uint32_t nextIndex;
            if(writeSectionOpt)
                nextIndex = nextSection(*writeSectionOpt);
            else
                nextIndex = nextSection(lastFreedIndex);

            if(indexReady(nextIndex))
            {
                counters[nextIndex].reset();
                writeSectionOpt = nextIndex;
                return true;
            }
            else
            {
                writeSectionOpt = std::nullopt;
                return false;
            }
        };

        if(!writeSectionOpt && !goToNextSection())
            return false;
        if(size > counters[*writeSectionOpt].getAvailable() && !goToNextSection())
            return false;
        if(currentSectionInvalidated && !goToNextSection())
            return false;

        dataAvailable = true;

        const auto writeSection = *writeSectionOpt;
        const auto offset = counters[writeSection].getAndAdd(size);
        std::memcpy((char*)memory + offset + sectionOffset(writeSection), source, size);

        dataPlaced(writeSection, offset);

        return true;
    }

    /**
     * @brief Calls a function for every written section.
     *
     * If no data has been written, the function returns early.
     *
     * @tparam Func a function with the signature void(uint32_t section)
     * @param func a function that will be called for each section that data has
     * been written to since this function was last called
     * @param invalidateSection causes the next memcpy to request a new section
     */

    template<typename Func>
    void forEachWritten(const Func& func, bool invalidateSection = true)
    {
        if(!dataAvailable)
            return;

        bool firstIteration = true;

        auto end = writeSectionOpt ? *writeSectionOpt : lastFreedIndex;
        for(uint32_t i = nextSection(lastFreedIndex); firstIteration || i <= end; i++)
        {
            firstIteration = false;
            func(i);
        }
        lastFreedIndex = end;

        if(invalidateSection)
            currentSectionInvalidated = true;
    }
};