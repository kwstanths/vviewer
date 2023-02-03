#ifndef __MeshModel_hpp__
#define __MeshModel_hpp__

#include <string>
#include <memory>

#include "Mesh.hpp"

class MeshModel {
public:
    MeshModel() {};
    MeshModel(const std::vector<std::shared_ptr<Mesh>>& meshes);

    std::vector<std::shared_ptr<Mesh>> getMeshes() const;

    void setName(std::string name);
    std::string getName() const;

protected:
    std::string m_name;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
};

#endif