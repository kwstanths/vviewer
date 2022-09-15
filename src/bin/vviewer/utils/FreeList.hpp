#ifndef __FreeList__
#define __FreeList__

#include<list>

class FreeList {
public:
	FreeList(size_t nElements);
	
	size_t getFree();

	void remove(size_t index);

	bool isFull() const;

private:
	size_t m_end;
	size_t m_nElements;
	std::list<size_t> m_freeElements;
};

#endif