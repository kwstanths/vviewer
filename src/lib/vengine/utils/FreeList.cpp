#include "FreeList.hpp"

#include <stdexcept>
#include <cassert>

#include "Console.hpp"

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
    } else if (m_end < m_nElements) {
        return m_end++;
    }

    debug_tools::ConsoleCritical("FreeList::getFree(): FreeList is empty");
    return m_nElements;
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

size_t FreeList::size() const
{
    return m_nElements;
}

}  // namespace vengine
