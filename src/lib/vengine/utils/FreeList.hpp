#ifndef __FreeList__
#define __FreeList__

#include <iostream>
#include <atomic>
#include <vector>

#include <deque>

namespace vengine
{

/* A thread safe class that manages free indices */
class FreeList
{
public:
    FreeList(size_t nElements);

    size_t getFree();

    void remove(size_t index);

    bool isFull() const;

    void clear();

private:
    size_t m_end;
    size_t m_nElements;
    std::deque<size_t> m_freeElements;
};

/* A thread safe class that manages free blocks */
template <typename T>
class FreeBlockList : public FreeList
{
public:
    FreeBlockList(size_t nElements)
        : FreeList(nElements)
    {
        m_blocks.resize(nElements);
    }

    T *add(const T &t)
    {
        size_t index = getFree();
        m_blocks[index] = t;
        return &m_blocks[index];
    }

    size_t getIndex(T *t) { return (t - &m_blocks[0]); }

private:
    std::vector<T> m_blocks;
};

}  // namespace vengine

#endif