#include "Model3D.hpp"

namespace vengine
{

Model3D::Model3D(const std::string &name)
    : Asset(name)
{
}

Tree<Model3D::Model3DNode> &Model3D::data()
{
    return m_data;
}

std::shared_ptr<Mesh> Model3D::mesh(std::string name) const
{
    auto itr = m_meshes.find(name);
    if (itr == m_meshes.end()) {
        return nullptr;
    }
    return itr->second;
}

std::vector<std::shared_ptr<Mesh>> Model3D::meshes() const
{
    std::vector<std::shared_ptr<Mesh>> temp;
    for (auto i : m_meshes) {
        temp.push_back(i.second);
    }
    return temp;
}

}  // namespace vengine