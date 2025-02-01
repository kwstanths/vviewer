#include "Instances.hpp"

#include "Scene.hpp"
#include "SceneObject.hpp"

namespace vengine
{

InstancesManager::InstancesManager(Scene *scene)
    : m_scene(scene)
{
    m_isBuilt = false;
}

void InstancesManager::initResources(InstanceData *instancesBuffer, uint32_t instancesBufferSize)
{
    m_instancesBuffer = instancesBuffer;
    m_instancesBufferSize = instancesBufferSize;

    m_isBuilt = false;
}

void InstancesManager::build()
{
    if (isBuilt()) {
        return;
    }

    SceneObjectVector sceneObjects = m_scene->getSceneObjectsFlat();
    fillSceneObjectVectors(sceneObjects);
    buildInstanceDataFromScratch();

    m_isBuilt = true;
}

void InstancesManager::invalidate()
{
    m_instancesOpaque.clear();
    m_transparent.clear();
    m_lights.clear();
    m_meshLights.clear();
    m_volumes.clear();

    m_sceneObjectMap.clear();

    m_isBuilt = false;
}

InstanceData *InstancesManager::findInstanceData(SceneObject *so) const
{
    auto itr = m_sceneObjectMap.find(so);
    if (itr == m_sceneObjectMap.end()) {
        return nullptr;
    }

    return itr->second;
}

uint32_t InstancesManager::findInstanceDataIndex(SceneObject *so) const
{
    InstanceData *instanceDataPtr = findInstanceData(so);
    assert(instanceDataPtr);

    return static_cast<uint32_t>(instanceDataPtr - &m_instancesBuffer[0]);
}

void InstancesManager::sortTransparent(const glm::vec3 &pos)
{
    std::sort(m_transparent.begin(), m_transparent.end(), [&](SceneObject *a, SceneObject *b) {
        float d1 = glm::distance(pos, a->worldPosition());
        float d2 = glm::distance(pos, b->worldPosition());
        return d1 > d2;
    });
}

void InstancesManager::initInstanceData(InstanceData *instanceData, SceneObject *so)
{
    /* Set object model matrix transformation */
    instanceData->modelMatrix = so->modelMatrix();
    /* Set object id */
    instanceData->id = glm::vec4(so->getID(), -1, -1, 0);

    /* Set material index  */
    if (so->has<ComponentMaterial>()) {
        Material *mat = so->get<ComponentMaterial>().material();
        instanceData->materialIndex = mat->materialIndex();
    }

    /* Set volumes */
    if (so->has<ComponentVolume>()) {
        Material *frontVol = so->get<ComponentVolume>().frontFacing();
        if (frontVol != nullptr)
            instanceData->id.g = frontVol->materialIndex();
        Material *backVol = so->get<ComponentVolume>().backFacing();
        if (backVol != nullptr)
            instanceData->id.b = backVol->materialIndex();
    }
}

void InstancesManager::fillSceneObjectVectors(SceneObjectVector &sceneGraph)
{
    for (SceneObject *sceneObject : sceneGraph) {
        if (!sceneObject->isActive())
            continue;

        if (sceneObject->has<ComponentMesh>()) {
            Mesh *mesh = sceneObject->get<ComponentMesh>().mesh();

            if (sceneObject->has<ComponentMaterial>()) {
                Material *material = sceneObject->get<ComponentMaterial>().material();

                if (material->isEmissive()) {
                    m_meshLights.push_back(sceneObject);
                }

                if (!material->isTransparent()) {
                    /* Search mesh in existing groups */
                    auto itr = m_instancesOpaque.find(mesh);
                    if (itr == m_instancesOpaque.end()) {
                        MeshGroup meshGroup;
                        meshGroup.sceneObjects.push_back(sceneObject);

                        m_instancesOpaque.insert({mesh, meshGroup});
                    } else {
                        itr->second.sceneObjects.push_back(sceneObject);
                    }
                } else {
                    m_transparent.push_back(sceneObject);
                }
            }
        }
        if (sceneObject->has<ComponentLight>()) {
            ComponentLight &lightComponent = sceneObject->get<ComponentLight>();
            if (lightComponent.light() != nullptr)
                m_lights.push_back(sceneObject);
        }
        if (sceneObject->has<ComponentVolume>()) {
            ComponentVolume &volumeComponent = sceneObject->get<ComponentVolume>();
            if (volumeComponent.frontFacing() != nullptr || volumeComponent.backFacing() != nullptr)
                m_volumes.push_back(sceneObject);
        }
    }
}

void InstancesManager::buildInstanceDataFromScratch()
{
    uint32_t currentIndex = 0;

    for (auto &meshGroup : m_instancesOpaque) {
        meshGroup.second.startIndex = currentIndex;

        uint32_t nObjects = static_cast<uint32_t>(meshGroup.second.sceneObjects.size());
        assert(currentIndex + nObjects < m_instancesBufferSize);
        for (uint32_t index = 0; index < nObjects; ++index) {
            SceneObject *sceneObject = meshGroup.second.sceneObjects[index];

            initInstanceData(&m_instancesBuffer[currentIndex + index], sceneObject);
            m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex + index]});
        }

        currentIndex += nObjects;
    }

    for (SceneObject *&sceneObject : m_transparent) {
        assert(currentIndex < m_instancesBufferSize);

        initInstanceData(&m_instancesBuffer[currentIndex], sceneObject);
        m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex]});

        currentIndex++;
    }

    for (SceneObject *&sceneObject : m_lights) {
        assert(currentIndex < m_instancesBufferSize);

        initInstanceData(&m_instancesBuffer[currentIndex], sceneObject);
        m_sceneObjectMap.insert({sceneObject, &m_instancesBuffer[currentIndex]});

        currentIndex++;
    }
}

}  // namespace vengine