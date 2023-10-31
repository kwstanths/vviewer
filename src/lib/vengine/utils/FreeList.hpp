#ifndef __FreeList__
#define __FreeList__

#include <iostream>
#include <atomic>
#include <vector>

#include "Stack.hpp"

namespace vengine
{

/* A thread safe class that manages free indices */
class FreeList
{
public:
    FreeList(size_t N);

    size_t getFree();

    void setFree(size_t index);

    bool empty() const;

    void reset();

private:
    size_t m_end;
    size_t m_nElements;
    Stack<size_t> m_freeElements;
};

/* A thread safe class that manages free blocks */
template <typename T>
class FreeBlockList : public FreeList
{
public:
    FreeBlockList(size_t N)
        : FreeList(N)
    {
        m_blocks.resize(N);
    }

    T *get()
    {
        size_t index = getFree();
        return &m_blocks[index];
    }

    void remove(T *t)
    {
        size_t index = getIndex(t);
        setFree(index);
    }

    size_t getIndex(T *t) { return (t - &m_blocks[0]); }

private:
    std::vector<T> m_blocks;
};

}  // namespace vengine

#endif