#ifndef __Mesh_hpp__
#define __Mesh_hpp__

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "vengine/math/AABB.hpp"
#include "Asset.hpp"

namespace vengine
{

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    Vertex()
    {
        position = glm::vec3(0, 0, 0);
        uv = glm::vec2(0, 0);
        normal = glm::vec3(0, 0, 1);
        color = glm::vec3(1, 0, 0);
        tangent = glm::vec3(1, 0, 0);
        bitangent = glm::vec3(0, 1, 0);
    };

    Vertex(glm::vec3 _position, glm::vec2 _uv, glm::vec3 _normal, glm::vec3 _color, glm::vec3 _tangent, glm::vec3 _bitangent)
        : position(_position)
        , uv(_uv)
        , normal(_normal)
        , color(_color)
        , tangent(_tangent)
        , bitangent(_bitangent){};
};

class Model3D;

class Mesh : public Asset
{
public:
    Mesh(const AssetInfo &info);
    Mesh(const AssetInfo &info,
         const std::vector<Vertex> &vertices,
         const std::vector<uint32_t> &indices,
         bool hasNormals = false,
         bool hasUVs = false);

    const std::vector<Vertex> &vertices() const;
    const std::vector<uint32_t> &indices() const;

    uint32_t nTriangles() const;

    bool hasNormals() const;
    bool hasUVs() const;
    bool hasTangents() const;

    const AABB3 &aabb() const;

    const Model3D *m_model = nullptr;

protected:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_nTriangles = 0;

    bool m_hasNormals = false;
    bool m_hasUVs = false;

    AABB3 m_aabb;

    void computeNormals();
    void computeAABB();
};

}  // namespace vengine

#endif