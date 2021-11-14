#include "MeshModel.hpp"

MeshModel::MeshModel(std::vector<Mesh *>& meshes)
{
    m_meshes = meshes;
}

std::vector<Mesh*> MeshModel::getMeshes() const
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
