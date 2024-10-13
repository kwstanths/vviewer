#ifndef __Transform_hpp__
#define __Transform_hpp__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define EPSILON_EQUAL_GLM 0.0001

namespace vengine
{

class Transform
{
public:
    Transform();
    Transform(glm::vec3 pos);
    Transform(glm::vec3 pos, glm::vec3 scale);
    Transform(glm::vec3 pos, glm::quat rotation);
    Transform(glm::vec3 pos, glm::vec3 scale, glm::quat rotation);
    Transform(glm::vec3 pos, glm::vec3 scale, glm::vec3 eulerAngles);

    static constexpr glm::vec3 WORLD_X = glm::vec3(1, 0, 0);
    static constexpr glm::vec3 WORLD_Y = glm::vec3(0, 1, 0);
    static constexpr glm::vec3 WORLD_Z = glm::vec3(0, 0, 1);

    const glm::vec3 &position() const;
    glm::vec3 &position();

    const glm::quat &rotation() const;
    void setRotation(const glm::quat &newRotation);
    /* Values in radians */
    void setRotationEuler(float x, float y, float z);
    void setRotation(glm::vec3 forward, glm::vec3 up);

    const glm::vec3 &scale() const;
    glm::vec3 &scale();

    inline const glm::vec3 &X() const { return m_x; }
    inline const glm::vec3 &Y() const { return m_y; }
    inline const glm::vec3 &Z() const { return m_z; }

    inline glm::vec3 forward() const { return -m_z; }
    inline glm::vec3 up() const { return m_y; }
    inline glm::vec3 right() const { return m_x; }

    glm::mat4 getModelMatrix() const;

    void rotate(const glm::vec3 &axis, const float angle);

    inline bool operator==(const Transform &o)
    {
        return (m_position == o.m_position) && (m_rotation == o.m_rotation) && (m_scale == o.m_scale);
    }
    
    inline bool operator!=(const Transform &o)
    {
        return (m_position != o.m_position) || (m_rotation != o.m_rotation) || (m_scale != o.m_scale);
    }

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    /* Local basis vectors */
    glm::vec3 m_z;
    glm::vec3 m_x;
    glm::vec3 m_y;

    void computeBasisVectors();
};

}  // namespace vengine

#endif