#ifndef __Mesh_hpp__
#define __Mesh_hpp__

#include <vector>
#include <string>

#include <glm/glm.hpp>

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

    Vertex(glm::vec3 _position, glm::vec2 _uv, glm::vec3 _normal, glm::vec3 _color, glm::vec3 _tangent, glm::vec3 _bitangent) :
        position(_position), uv(_uv), normal(_normal), color(_color), tangent(_tangent), bitangent(_bitangent) {};
};

class MeshModel;

class Mesh {
public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, bool hasNormals = false, bool hasUVs = false);

    const std::vector<Vertex>& getVertices() const;
    const std::vector<uint16_t>& getIndices() const;

    bool hasNormals() const;
    bool hasUVs() const;
    bool hasTangents() const;

    void computeNormals();

    const MeshModel* m_meshModel;
    std::string m_name;
private:
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
    bool m_hasNormals = false;
    bool m_hasUVs = false;

};

#endif