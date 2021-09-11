#ifndef __MeshModel_hpp__
#define __MeshModel_hpp__

#include "Mesh.hpp"

class MeshModel {
public:
    MeshModel() {};
    MeshModel(std::vector<Mesh *>& meshes);

    std::vector<Mesh *> getMeshes() const;

protected:
    std::vector<Mesh *> m_meshes;
};

#endif