#include "Model3D.hpp"

namespace vengine
{

Model3D::Model3D(const AssetInfo &info)
    : Asset(info)
{
}

Tree<Model3D::Model3DNode> &Model3D::data()
{
    return m_data;
}

Mesh *Model3D::mesh(std::string name) const
{
    auto itr = m_meshes.find(name);
    if (itr == m_meshes.end()) {
        return nullptr;
    }
    return itr->second;
}

std::vector<Mesh *> Model3D::meshes() const
{
    std::vector<Mesh *> temp;
    for (auto i : m_meshes) {
        temp.push_back(i.second);
    }
    return temp;
}

}  // namespace vengine