#ifndef __Stack_hpp__
#define __Stack_hpp__

#include <stdint.h>
#include <exception>
#include <vector>
#include <atomic>
#include <iostream>
#include <cassert>

namespace vengine
{

/* Thread safe */
template <typename T>
class Stack
{
public:
    Stack(uint32_t N)
        : m_data(N)
    {
        m_top = -1;
    }

    void push(const T &t)
    {
        m_top++;

        assert(m_top < m_data.size());
        m_data[m_top] = t;
    }

    T pop()
    {
        assert(!empty());

        return m_data[m_top--];
    }

    bool empty() const { return m_top == -1; }

    void clear() { m_top = -1; }

    void resize(size_t N)
    {
        m_data = std::vector<T>(N);
        clear();
    }

private:
    std::vector<T> m_data;
    std::atomic<int> m_top;
};

}  // namespace vengine

#endif