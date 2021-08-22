#ifndef __Mesh_hpp__
#define __Mesh_hpp__

#include <vector>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 color;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    const std::vector<Vertex>& getVertices() const;
    const std::vector<uint16_t>& getIndices() const;

    void computeNormals();

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
};

#endif