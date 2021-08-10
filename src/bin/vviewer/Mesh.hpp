#ifndef __Mesh_hpp__
#define __Mesh_hpp__

#include <vector>

#include <glm/glm.hpp>

#include "Utils.hpp"

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    const std::vector<Vertex>& getVertices() const;
    const std::vector<uint16_t>& getIndices() const;

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
};

#endif