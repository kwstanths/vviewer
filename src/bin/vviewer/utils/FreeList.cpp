#include "FreeList.hpp"

#include <stdexcept>

FreeList::FreeList(size_t nElements)
{
	m_end = 0;
	m_nElements = nElements;
}

size_t FreeList::getFree()
{
	if (m_freeElements.size() != 0) {
		size_t free = m_freeElements.front();
		m_freeElements.pop_front();
		return free;
	}
	else if (m_end == m_nElements) {
		throw std::runtime_error("Free list is full");
	}
	else {
		return m_end++;
	}
}

void FreeList::remove(size_t index)
{
	m_freeElements.push_back(index);
}

bool FreeList::isFull() const
{
	return m_freeElements.empty() && (m_end == m_nElements);
}
