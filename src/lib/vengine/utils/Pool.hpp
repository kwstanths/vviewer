#ifndef _Pool_hpp__
#define _Pool_hpp__

#include <stdint.h>
#include <cassert>
#include <vector>

#include "FreeList.hpp"

namespace vengine
{

/* Thread safe */
template <typename T>
class GenerationalPool
{
public:
    struct Handle {
        const uint32_t index;
        const uint32_t generation;

        Handle(uint32_t i, uint32_t g)
            : index(i)
            , generation(g){};
    };

    GenerationalPool(uint32_t N)
        : m_data(N)
        , m_generation(N, 0)
        , m_freeIndices(N)
    {
    }

    Handle allocate()
    {
        assert(!m_freeIndices.empty());

        uint32_t freeIndex = m_freeIndices.getFree();
        return Handle(freeIndex, m_generation[freeIndex]);
    }

    void deallocate(const Handle &h)
    {
        m_freeIndices.setFree(h.index);
        m_generation[h.index]++;
    }

    T *get(const Handle &h)
    {
        if (h.generation != m_generation[h.index]) {
            return nullptr;
        }

        return &m_data[h.index];
    }

    void clear()
    {
        m_generation = std::vector<uint32_t>(0);
        m_freeIndices.reset();
    }

    bool full() const { return m_freeIndices.empty(); }

private:
    std::vector<T> m_data;
    std::vector<uint32_t> m_generation;

    FreeList m_freeIndices;
};

}  // namespace vengine

#endif