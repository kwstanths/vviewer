#ifndef __Algorithms_hpp__
#define __Algorithms_hpp__

#include <vector>
#include <array>
#include <queue>
#include <functional>

namespace vengine
{

/**
 * @brief Returns N indices that correspond to the N smallest elements inside o based on the comparator c
 *
 * @tparam T
 * @tparam N
 * @param o
 * @param c
 * @return std::array<unsigned int, N>
 */
template <typename T, unsigned int N = 1>
std::array<unsigned int, N> findNSmallest(const std::vector<T> &o, std::function<bool(const T &, const T &)> c)
{
    struct Compare {
        std::function<bool(const T &, const T &)> &comp;
        bool operator()(const std::pair<T, int> &lhs, const std::pair<T, int> &rhs) const { return comp(lhs.first, rhs.first); }

        Compare(std::function<bool(const T &, const T &)> &comparator)
            : comp(comparator)
        {
        }
    } comp(c);

    std::priority_queue<std::pair<T, unsigned int>, std::vector<std::pair<T, unsigned int>>, decltype(comp)> q(comp);
    for (unsigned int i = 0; i < o.size(); i++) {
        q.push(std::make_pair(o[i], i));
        if (q.size() > N) {
            q.pop();
        }
    }

    unsigned int i = 0;
    std::array<unsigned int, N> out;
    while (!q.empty()) {
        auto e = q.top();
        out[i++] = e.second;
        q.pop();
    }

    return out;
}

}  // namespace vengine

#endif