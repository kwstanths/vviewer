#ifndef __MathUtils_hpp__
#define __MathUtils_hpp__

#include <cstdint>

#include <glm/glm.hpp>

namespace vengine
{

/**
 * @brief Get the next power of 2
 *
 * @param v
 * @return uint32_t
 */
uint32_t roundPow2(uint32_t v);

/**
 * @brief Get the translation from a mat4
 *
 * @param m
 * @return glm::vec3
 */
inline glm::vec3 getTranslation(const glm::mat4 &m)
{
    return glm::vec3(m[3][0], m[3][1], m[3][2]);
}

inline bool isBlack(const glm::vec3 &a, const float &threshold = std::numeric_limits<float>::epsilon())
{
    return (std::abs(a.r) < threshold) && (std::abs(a.g) < threshold) && (std::abs(a.b) < threshold);
}

inline bool isBlack(const glm::vec4 &a, const float &threshold = std::numeric_limits<float>::epsilon())
{
    return (std::abs(a.r) < threshold) && (std::abs(a.g) < threshold) && (std::abs(a.b) < threshold) && (std::abs(a.a) < threshold);
}

}  // namespace vengine

#endif  // !__MathUtils_hpp__
