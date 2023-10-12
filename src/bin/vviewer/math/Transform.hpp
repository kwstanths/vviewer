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

    static constexpr glm::vec3 X = glm::vec3(1, 0, 0);
    static constexpr glm::vec3 Y = glm::vec3(0, 1, 0);
    static constexpr glm::vec3 Z = glm::vec3(0, 0, 1);

    void setPosition(const glm::vec3 &newPosition);
    void setPosition(float x, float y, float z);
    glm::vec3 getPosition() const;

    void setRotation(const glm::quat &newRotation);
    /* Values in radians */
    void setRotationEuler(float x, float y, float z);
    void setRotation(glm::vec3 forward, glm::vec3 up);
    glm::quat getRotation() const;

    void setScale(const glm::vec3 &newScale);
    void setScale(float x, float y, float z);
    glm::vec3 getScale() const;

    inline glm::vec3 getX() const { return m_x; }
    inline glm::vec3 getY() const { return m_y; }
    inline glm::vec3 getZ() const { return m_z; }
    inline glm::vec3 getForward() const { return -m_z; }
    inline glm::vec3 getUp() const { return m_y; }
    inline glm::vec3 getRight() const { return m_x; }

    glm::mat4 getModelMatrix() const;

    void rotate(const glm::vec3 &axis, const float angle);

    inline bool operator==(const Transform &o)
    {
        return (m_position == o.m_position) && (m_rotation == o.m_rotation) && (m_scale == o.m_scale);
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