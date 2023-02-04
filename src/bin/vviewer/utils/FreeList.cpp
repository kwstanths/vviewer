#include "FreeList.hpp"

#include <stdexcept>
#include <cassert>

FreeList::FreeList(size_t nElements)
{
	m_end = 0;
	m_nElements = nElements;
}

size_t FreeList::getFree()
{
	if (m_freeElements.size() != 0) {
		auto itr = m_freeElements.begin();
		size_t freeElement = *itr;
		m_freeElements.erase(itr);
		return freeElement;
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
	assert(index < m_end);
	m_freeElements.insert(index);
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

