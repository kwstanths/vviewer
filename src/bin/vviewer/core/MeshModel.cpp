#include "MeshModel.hpp"

MeshModel::MeshModel(const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    m_meshes = meshes;
}

std::vector<std::shared_ptr<Mesh>> MeshModel::getMeshes() const
{
    return m_meshes;
}

void MeshModel::setName(std::string name)
{
    m_name = name;
}

std::string MeshModel::getName() const
{
    return m_name;
}
