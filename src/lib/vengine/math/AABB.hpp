#ifndef __AABB_hpp__
#define __AABB_hpp__

#include "glm/glm.hpp"

namespace vengine
{

/* 3D Axis Aligned Bounding Box */
class AABB3
{
public:
    AABB3();

    static AABB3 fromPoint(const glm::vec3 &p);
    static AABB3 fromMinAndMax(const glm::vec3 &min, const glm::vec3 &max);
    static AABB3 fromCenterAndLength(const glm::vec3 &center, const glm::vec3 &length);

    static bool overlaps(const AABB3 &a, const AABB3 &b);
    bool overlaps(const AABB3 &other) const;

    const glm::vec3 &min() const;
    glm::vec3 &min();

    const glm::vec3 &max() const;
    glm::vec3 &max();

    const glm::vec3 &operator[](uint32_t i) const;
    glm::vec3 &operator[](uint32_t i);

    glm::vec3 corner(uint32_t i) const;

    bool isInside(const glm::vec3 &p) const;

    float diagonal() const;

    void add(const glm::vec3 p);

    void translate(const glm::vec3 &translation);

private:
    glm::vec3 m_min;
    glm::vec3 m_max;
};

};  // namespace vengine

#endif /* __AABB_hpp__ */
