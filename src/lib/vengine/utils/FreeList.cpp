#include "FreeList.hpp"

#include <stdexcept>
#include <cassert>

namespace vengine
{

FreeList::FreeList(size_t nElements)
{
    m_end = 0;
    m_nElements = nElements;
}

size_t FreeList::getFree()
{
    if (!m_freeElements.empty()) {
        size_t freeElement = m_freeElements.front();
        m_freeElements.pop_front();
        return freeElement;
    } else if (m_end == m_nElements) {
        throw std::runtime_error("FreeList is full");
    } else {
        return m_end++;
    }
}

void FreeList::remove(size_t index)
{
    assert(index < m_end);
    m_freeElements.push_front(index);
}

bool FreeList::isFull() const
{
    return m_freeElements.empty() && (m_end == m_nElements);
}

void FreeList::clear()
{
    m_end = 0;
    m_freeElements.clear();
}

}  // namespace vengine
