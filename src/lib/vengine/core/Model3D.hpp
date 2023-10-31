#ifndef __MeshModel_hpp__
#define __MeshModel_hpp__

#include <string>
#include <memory>

#include "Asset.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "math/Transform.hpp"
#include "utils/Tree.hpp"

namespace vengine
{

class Model3D : public Asset
{
public:
    struct Model3DNode {
        std::string name;
        std::vector<Mesh *> meshes;
        std::vector<Material *> materials;
        Transform transform;
    };

    Model3D(const std::string &name);

    Tree<Model3DNode> &data();

    Mesh *mesh(std::string name) const;

    std::vector<Mesh *> meshes() const;

protected:
    Tree<Model3DNode> m_data;
    std::unordered_map<std::string, Mesh *> m_meshes;
};

}  // namespace vengine

#endif