#include "FreeList.hpp"

#include <stdexcept>
#include <cassert>

namespace vengine
{

FreeList::FreeList(size_t N)
    : m_freeElements(N)
{
    m_end = 0;
    m_nElements = N;
}

size_t FreeList::getFree()
{
    if (!m_freeElements.empty()) {
        return m_freeElements.pop();
    } else if (m_end == m_nElements) {
        throw std::runtime_error("FreeList is empty");
    } else {
        return m_end++;
    }
}

void FreeList::setFree(size_t index)
{
    assert(index < m_end);
    m_freeElements.push(index);
}

bool FreeList::empty() const
{
    return m_freeElements.empty() && (m_end == m_nElements);
}

void FreeList::reset()
{
    m_end = 0;
    m_freeElements.clear();
}

}  // namespace vengine