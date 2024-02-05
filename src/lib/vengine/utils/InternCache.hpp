#ifndef __InternCache_hpp__
#define __InternCache_hpp__

#include <cstdint>
#include <span>
#include <vector>
#include <unordered_map>

#include "FreeList.hpp"

namespace vengine
{

/**
 * @brief A class to manage a span of data by reusing memory for data with the same hash
 *
 * @tparam T Must be able to be used as a key in an unordered map
 */
template <typename T>
class InternCache
{
public:
    InternCache()
        : m_freeList(0)
    {
    }

    void setData(std::span<T> data)
    {
        m_data = data;
        m_freeList.resize(data.size());
    }

    bool lookup(const T &t)
    {
        auto itr = m_uniqueData.find(t);
        return itr != m_uniqueData.end();
    }

    uint32_t lookupAndAdd(const T &t)
    {
        auto itr = m_uniqueData.find(t);
        if (itr == m_uniqueData.end()) {
            /* Add new element */
            uint32_t index = static_cast<uint32_t>(m_freeList.getFree());
            T *p = &m_data[index];
            m_uniqueData[t] = {1, p};
            return index;
        }

        itr->second.n++;
        return getIndex(itr->second.p);
    }

    void remove(uint32_t index)
    {
        auto &t = m_data[index];

        auto itr = m_uniqueData.find(t);
        if (itr != m_uniqueData.end()) {
            itr->second.n--;
            if (itr->second.n == 0) {
                uint32_t index = getIndex(itr->second.p);
                m_uniqueData.erase(itr);

                m_freeList.setFree(index);
            }
        }
    }

private:
    std::span<T> m_data;

    struct Entry {
        uint32_t n;
        T *p;
    };
    std::unordered_map<T, Entry> m_uniqueData;

    FreeList m_freeList;

    uint32_t getIndex(T *t) { return (static_cast<uint32_t>(t - m_data.data())); };
};

}  // namespace vengine

#endif /*__InternCache_hpp__ */
