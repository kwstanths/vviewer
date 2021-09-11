#include "MeshModel.hpp"

MeshModel::MeshModel(std::vector<Mesh *>& meshes)
{
    m_meshes = meshes;
}

std::vector<Mesh*> MeshModel::getMeshes() const
{
    return m_meshes;
}
