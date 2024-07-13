#include "Instances.hpp"

#include "SceneObject.hpp"

namespace vengine
{

InstancesManager::InstancesManager()
{
    m_isBuilt = false;
}

void InstancesManager::initResources(InstanceData *instancesBuffer, uint32_t instancesBufferSize)
{
    m_instancesBuffer = instancesBuffer;
    m_instancesBufferSize = instancesBufferSize;

    uint32_t nMaterialTypes = static_cast<int32_t>(MaterialType::MATERIAL_TOTAL_TYPES);
    m_instancesOpaque.resize(nMaterialTypes);
    for (uint32_t materialIndex = 0; materialIndex < nMaterialTypes; ++materialIndex) {
        m_instancesOpaque[materialIndex].materialType = static_cast<MaterialType>(materialIndex);
        m_instancesOpaque[materialIndex].meshGroups.clear();
    }
    m_isBuilt = false;
}

void InstancesManager::build(SceneGraph &sceneGraph)
{
    if (m_isBuilt) {
        return;
    }

    fillSceneObjectVectors(sceneGraph);

    buildInstanceData();

    m_isBuilt = true;
}

void InstancesManager::reset()
{
    uint32_t nMaterialTypes = static_cast<int32_t>(MaterialType::MATERIAL_TOTAL_TYPES);
    for (uint32_t materialIndex = 0; materialIndex < nMaterialTypes; ++materialIndex) {
        m_instancesOpaque[materialIndex].meshGroups.clear();
    }

    m_transparent.clear();
    m_lights.clear();
    m_meshLights.clear();

    m_sceneObjectMap.clear();

    m_isBuilt = false;
}

const InstancesManager::MaterialGroup &InstancesManager::materialGroup(MaterialType type) const
{
    size_t index = static_cast<size_t>(type);
    assert(index < m_instancesOpaque.size());

    return m_instancesOpaque[index];
}

InstanceData *InstancesManager::getInstanceData(SceneObject *so)
{
    auto itr = m_sceneObjectMap.find(so);
    if (itr == m_sceneObjectMap.end()) {
        return nullptr;
    }

    return itr->second;
}

uint32_t InstancesManager::getInstanceDataIndex(SceneObject *so)
{
    InstanceData *instanceData = getInstanceData(so);
    assert(instanceData);

    return instanceData - &m_instancesBuffer[0];
}

void InstancesManager::sortTransparent(const glm::vec3 &pos)
{
    std::sort(m_transparent.begin(), m_transparent.end(), [&](SceneObject *a, SceneObject *b) {
        float d1 = glm::distance(pos, a->worldPosition());
        float d2 = glm::distance(pos, b->worldPosition());
        return d1 > d2;
    });
}

void InstancesManager::setInstanceData(InstanceData *instanceData, SceneObject *so)
{
    if (so->has<ComponentMaterial>()) {
        instanceData->materialIndex = so->get<ComponentMaterial>().material->materialIndex();
    }
    instanceData->id = {so->getIDRGB(), so->selected()};
    instanceData->modelMatrix = so->modelMatrix();
}

void InstancesManager::fillSceneObjectVectors(SceneGraph &sceneGraph)
{
    for (SceneObject *sceneObject : sceneGraph) {
        if (!sceneObject->active())
            continue;

        if (sceneObject->has<ComponentMesh>() && sceneObject->has<ComponentMaterial>()) {
            sceneObject->computeAABB();

            Mesh *mesh = sceneObject->get<ComponentMesh>().mesh;
            Material *material = sceneObject->get<ComponentMaterial>().material;

            if (material->isEmissive()) {
                m_meshLights.push_back(sceneObject);
            }

            if (!material->isTransparent()) {
                MaterialType materialType = material->type();
                assert(materialType > MaterialType::MATERIAL_NOT_SET && materialType < MaterialType::MATERIAL_TOTAL_TYPES);

                MaterialGroup &materialGroup = m_instancesOpaque[static_cast<uint32_t>(materialType)];

                /* Search mesh in existing groups */
                auto itr = materialGroup.meshGroups.find(mesh);
                if (itr == materialGroup.meshGroups.end()) {
                    MeshGroup meshGroup;
                    meshGroup.sceneObjects.push_back(sceneObject);

                    materialGroup.meshGroups.insert({mesh, meshGroup});
                } else {
                    itr->second.sceneObjects.push_back(sceneObject);
                }
            } else {
                m_transparent.push_back(sceneObject);
            }

        } else if (sceneObject->has<ComponentLight>()) {
            ComponentLight &lightComponent = sceneObject->get<ComponentLight>();
            m_lights.push_back(sceneObject);
        }
    }
}

void InstancesManager::buildInstanceData()
{
    uint32_t currentIndex = 0;

    for (MaterialGroup &materialGroup : m_instancesOpaque) {
        for (auto &meshGroup : materialGroup.meshGroups) {
            meshGroup.second.startIndex = currentIndex;

            uint32_t nObjects = meshGroup.second.sceneObjects.size();
            assert(currentIndex + nObjects < m_instancesBufferSize);
            for (uint32_t index = 0; index < nObjects; ++index) {
                SceneObject *sceneObject = meshGroup.second.sceneObjects[index];

                setInstanceData(&m_instancesBuffer[currentIndex + index], sceneObject);
                m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex + index]});
            }

            currentIndex += nObjects;
        }
    }

    for (SceneObject *&sceneObject : m_transparent) {
        assert(currentIndex < m_instancesBufferSize);

        setInstanceData(&m_instancesBuffer[currentIndex], sceneObject);
        m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex]});

        currentIndex++;
    }

    for (SceneObject *&sceneObject : m_lights) {
        assert(currentIndex < m_instancesBufferSize);

        setInstanceData(&m_instancesBuffer[currentIndex], sceneObject);
        m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex]});

        currentIndex++;
    }
}

}  // namespace vengine