#include "Mesh.hpp"

#include <utils/Console.hpp>

#include <glm/glm.hpp>

Mesh::Mesh() : Component(ComponentType::MESH)
{
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool hasNormals, bool hasUVs) : Component(ComponentType::MESH)
{
    m_vertices = vertices;
    m_indices = indices;
    m_hasNormals = hasNormals;
    m_hasUVs = hasNormals;
}

const std::vector<Vertex>& Mesh::getVertices() const
{
    return m_vertices;
}

const std::vector<uint32_t>& Mesh::getIndices() const
{
    return m_indices;
}

bool Mesh::hasNormals() const
{
    return m_hasNormals;
}

bool Mesh::hasUVs() const
{
    return m_hasUVs;
}

bool Mesh::hasTangents() const
{
    /* If mesh doesn't have uvs, it doesn't have tangents either */
    return m_hasUVs;
}

void Mesh::computeNormals()
{
    /* Make all normals zero */
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].normal = glm::vec3(0, 0, 0);
    };

    /* Add contribution of each normal to the face's vertices */
    for (size_t i = 0; i < m_indices.size(); i += 3) {
        Vertex& v1 = m_vertices[m_indices[i]];
        Vertex& v2 = m_vertices[m_indices[i + 1]];
        Vertex& v3 = m_vertices[m_indices[i + 2]];

        glm::vec3 U = v2.position - v1.position, V = v3.position - v1.position;
        glm::vec3 normal = glm::vec3(U.y * V.z - U.z * V.y, U.z * V.x - U.x * V.z, U.x * V.y - U.y * V.x);
        v1.normal += normal;
        v2.normal += normal;
        v3.normal += normal;
    }

    /* Normalize */
    for (size_t i = 0; i < m_vertices.size(); i++) {
        m_vertices[i].normal = glm::normalize(m_vertices[i].normal);
    };
}
