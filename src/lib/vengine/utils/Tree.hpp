#ifndef __Tree_hpp__
#define __Tree_hpp__

#include <cstdint>
#include <vector>

namespace vengine
{

template <typename T>
class Tree
{
public:
    Tree(){};
    Tree(const T &data)
        : m_data(data){};

    inline T &data() { return m_data; }
    inline const T &data() const { return m_data; }

    inline uint32_t size() const { return static_cast<uint32_t>(m_children.size()); }

    inline Tree<T> &add()
    {
        m_children.push_back(Tree());
        return m_children.back();
    }

    inline Tree<T> &add(const T &data)
    {
        m_children.emplace_back(data);
        return m_children.back();
    }

    inline void add(const Tree<T> &child) { m_children.emplace_back(child); }

    inline Tree<T> &child(uint32_t i)
    {
        assert(i < size());
        return m_children[i];
    }

    inline const Tree<T> &child(uint32_t i) const
    {
        assert(i < size());
        return m_children[i];
    }

private:
    std::vector<Tree<T>> m_children;
    T m_data;
};

}  // namespace vengine

#endif