#ifndef __FreeList__
#define __FreeList__

#include <iostream>
#include <atomic>
#include <vector>

#include "Stack.hpp"

namespace vengine
{

class FreeList
{
public:
    FreeList(uint32_t N);

    uint32_t getFree();

    void setFree(uint32_t index);

    bool empty() const;

    void reset();

    uint32_t size() const;

    void resize(uint32_t N);

private:
    uint32_t m_end;
    uint32_t m_nElements;
    Stack<uint32_t> m_freeElements;
};

template <typename T>
class FreeBlockList : private FreeList
{
public:
    FreeBlockList(uint32_t N)
        : FreeList(N)
        , m_valid(N, false)
    {
        m_blocks.resize(N);
    }

    T *get()
    {
        uint32_t index = getFree();
        if (index == size()) {
            return nullptr;
        }

        m_valid[index] = true;
        return &m_blocks[index];
    }

    void remove(T *t)
    {
        uint32_t index = getIndex(t);
        m_valid[index] = false;
        setFree(index);
    }

    void reset() { FreeList::reset(); }

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T *;
        using reference = T &;
        using index = uint32_t;

        Iterator(std::vector<T> &blocks, const std::vector<bool> &valid, index i)
            : m_blocks(blocks)
            , m_valid(valid)
            , m_index(i)
        {
            if (!m_valid[m_index]) {
                findNextValid();
            }
        };

        Iterator(std::vector<T> &blocks, const std::vector<bool> &valid)
            : m_blocks(blocks)
            , m_valid(valid)
            , m_index(blocks.size()){};

        reference operator*() const { return m_blocks[m_index]; }
        pointer operator->() { return &m_blocks[m_index]; }

        // Prefix increment
        Iterator &operator++()
        {
            findNextValid();
            return *this;
        }

        // Postfix increment
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator &a, const Iterator &b) { return a.m_index == b.m_index; };
        friend bool operator!=(const Iterator &a, const Iterator &b) { return a.m_index != b.m_index; };

    private:
        index m_index;
        std::vector<T> &m_blocks;
        const std::vector<bool> &m_valid;

        void findNextValid()
        {
            m_index++;
            while (m_index < m_blocks.size() && !m_valid[m_index]) {
                m_index++;
            }
        }
    };

    Iterator begin() { return Iterator(m_blocks, m_valid, 0); }
    Iterator end() { return Iterator(m_blocks, m_valid); }

private:
    std::vector<T> m_blocks;
    std::vector<bool> m_valid;

    uint32_t getIndex(T *t) { return static_cast<uint32_t>(t - &m_blocks[0]); }
};

}  // namespace vengine

#endif