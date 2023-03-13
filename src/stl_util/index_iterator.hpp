#pragma once

template<typename T>
class IndexIterator
{
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<typename T::reference, size_t>;
    using pointer = value_type*;
    using reference = value_type&;
    using iter = IndexIterator<T>;

    typename T::iterator lhs;
    size_t index;

  public:
    IndexIterator(typename T::iterator lhs, size_t index): lhs(lhs), index(index) {}

    bool operator==(const iter& other) const
    {
        return index == other.index;
    }
    bool operator!=(const iter& other) const
    {
        return !(*this == other);
    }

    value_type operator*()
    {
        return value_type(*lhs, index);
    }

    iter operator++()
    {
        lhs++;
        index++;
        return *this;
    }

    iter operator++(int)
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }
};

/**
 * @brief A function which takes a collection and returns an iterator of std::tuple<T, size_t> where
 * size_t corresponds to the current iteration index.
 *
 * For instance:
 * for(auto [val, index]: Index(collection)) {
 *     // index will be 0, 1, 2...
 * }
 *
 * @tparam T
 */
template<typename T>
struct Index
{
    // I'm not a huge fan of the name `Index` but it's alright for now
    explicit Index(T&& lhs): lhs(std::move(lhs)), lhsRef(this->lhs) {}
    explicit Index(T& lhs): lhsRef(lhs) {}

    auto begin()
    {
        return IndexIterator<T>(lhsRef.begin(), 0);
    }
    auto end()
    {
        return IndexIterator<T>(lhsRef.end(), lhsRef.size());
    }

  private:
    // When given a temporary, the iterator needs to take ownership of the
    // temporary. In that case, `lhs` will be that object
    T lhs;
    // This will either point to `lhs` or the ref given to `Index(T&)`. Either
    // way, lhsRef should always be used instead of lhs
    T& lhsRef;
};
