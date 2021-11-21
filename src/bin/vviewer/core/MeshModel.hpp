#ifndef __MeshModel_hpp__
#define __MeshModel_hpp__

#include <string>

#include "Mesh.hpp"

class MeshModel {
public:
    MeshModel() {};
    MeshModel(std::vector<Mesh *>& meshes);

    std::vector<Mesh *> getMeshes() const;

    void setName(std::string name);
    std::string getName() const;

protected:
    std::string m_name;
    std::vector<Mesh *> m_meshes;
};

#endif