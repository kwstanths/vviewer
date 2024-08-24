#include "AABB.hpp"

#include <stdexcept>

namespace vengine
{

AABB3::AABB3()
{
    m_min = glm::vec3(std::numeric_limits<float>::lowest());
    m_max = glm::vec3(std::numeric_limits<float>::max());
}

AABB3 AABB3::fromPoint(const glm::vec3 &p)
{
    AABB3 t;
    t.min() = p;
    t.max() = p;

    return t;
}

AABB3 AABB3::fromMinAndMax(const glm::vec3 &min, const glm::vec3 &max)
{
    AABB3 t;
    t.min() = min;
    t.max() = max;

    return t;
}

AABB3 AABB3::fromCenterAndLength(const glm::vec3 &center, const glm::vec3 &length)
{
    AABB3 t;
    for (uint32_t i = 0; i < 3; i++) {
        t.min()[i] = center[i] - length[i] / 2.F;
        t.max()[i] = center[i] + length[i] / 2.F;
    }

    return t;
}

bool AABB3::overlaps(const AABB3 &a, const AABB3 &b)
{
    bool ox = (a.max()[0] >= b.min()[0] && b.max()[0] >= a.min()[0]);
    bool oy = (a.max()[1] >= b.min()[1] && b.max()[1] >= a.min()[1]);
    bool oz = (a.max()[2] >= b.min()[2] && b.max()[2] >= a.min()[2]);

    if (ox && oy && oz)
        return true;

    return false;
}

bool AABB3::overlaps(const AABB3 &other) const
{
    return AABB3::overlaps(*this, other);
}

const glm::vec3 &AABB3::min() const
{
    return m_min;
}

glm::vec3 &AABB3::min()
{
    return m_min;
}

const glm::vec3 &AABB3::max() const
{
    return m_max;
}

glm::vec3 &AABB3::max()
{
    return m_max;
}

const glm::vec3 &AABB3::operator[](uint32_t i) const
{
    if (i == 0) {
        return m_min;
    } else if (i == 1) {
        return m_max;
    }
    throw std::runtime_error("Wront input argument");
}

glm::vec3 &AABB3::operator[](uint32_t i)
{
    if (i == 0) {
        return m_min;
    } else if (i == 1) {
        return m_max;
    }
    throw std::runtime_error("Wront input argument");
}

glm::vec3 AABB3::corner(uint32_t i) const
{
    return glm::vec3((*this)[(i & 1)].x, (*this)[(i & 2) ? 1 : 0].y, (*this)[(i & 4) ? 1 : 0].z);
}

void AABB3::add(const glm::vec3 p)
{
    min() = glm::vec3(std::min(min().x, p.x), std::min(min().y, p.y), std::min(min().z, p.z));
    max() = glm::vec3(std::max(max().x, p.x), std::max(max().y, p.y), std::max(max().z, p.z));
}

void AABB3::translate(const glm::vec3 &translation)
{
    for (uint32_t i = 0; i < 3; i++) {
        m_min[i] += translation[i];
        m_max[i] += translation[i];
    }
}

bool AABB3::isInside(const glm::vec3 &p) const
{
    bool inside = true;
    for (uint32_t i = 0; i < 3; i++)
        inside = inside && (min()[i] <= p[i]) && (p[i] <= max()[i]);
    return inside;
}

float AABB3::diagonal() const
{
    return glm::distance(min(), max());
}

}  // namespace vengine