#ifndef __Instances_hpp__
#define __Instances_hpp__

#include "glm/glm.hpp"

#include "utils/IDGeneration.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "SceneObject.hpp"
#include "Light.hpp"

namespace vengine
{

/* A GPU clone */
struct InstanceData {
    /* Transformation matrix */
    glm::mat4 modelMatrix;
    /* R: ID, GBA: unused */
    glm::vec4 id;
    /* Material index */
    uint32_t materialIndex;
    /* A pointer to a buffer holding mesh vertex data */
    uint64_t vertexAddress;
    /* A pointer to a buffer holding mesh index data*/
    uint64_t indexAddress;
    /* The number of triangles in the index buffer */
    uint32_t numTriangles;

    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
    uint32_t padding4;
};

class InstancesManager
{
public:
    struct MeshGroup {
        SceneObjectVector sceneObjects;
        /* starting index for m_instancesBuffer */
        uint32_t startIndex = 0;
    };

    typedef std::unordered_map<SceneObject *, InstanceData *> SceneObjectMap;

    InstancesManager();

    /**
     * @brief Initialize object
     *
     * @param instancesBuffer InstanceData buffer pointer
     * @param instancesBufferSize Buffer size
     */
    void initResources(InstanceData *instancesBuffer, uint32_t instancesBufferSize);

    virtual void build(SceneObjectVector &sceneGraph);
    bool isBuilt() const { return m_isBuilt; }

    void reset();

    const SceneObjectMap &sceneObjectMap() const { return m_sceneObjectMap; }

    const std::unordered_map<Mesh *, MeshGroup> &opaqueMeshes() const { return m_instancesOpaque; }
    const SceneObjectVector &transparentMeshes() const { return m_transparent; }
    const SceneObjectVector &lights() const { return m_lights; }
    const SceneObjectVector &meshLights() const { return m_meshLights; }
    const SceneObjectVector &volumes() const { return m_volumes; }

    InstanceData *instanceData(SceneObject *so) const;
    uint32_t instanceDataIndex(SceneObject *so) const;

    void sortTransparent(const glm::vec3 &pos);

protected:
    /* Holds all opaque mesh instances in the scene */
    std::unordered_map<Mesh *, MeshGroup> m_instancesOpaque;
    /* Holds all transparent mesh objects in the scene */
    SceneObjectVector m_transparent;
    /* Holds all scene objects that have light components */
    SceneObjectVector m_lights;
    /* Holds all scene objects that are mesh lights */
    SceneObjectVector m_meshLights;
    /* Holds all scene objects that are volumes */
    SceneObjectVector m_volumes;

    SceneObjectMap m_sceneObjectMap;

    bool m_isBuilt = false;

    /**
     * @brief Set the InstanceData for a scene object
     *
     * @param instanceData
     * @param so
     */
    virtual void initInstanceData(InstanceData *instanceData, SceneObject *so);

private:
    /* CPU memory for the InstanceData buffer */
    InstanceData *m_instancesBuffer = nullptr;
    uint32_t m_instancesBufferSize = 0;

    void fillSceneObjectVectors(SceneObjectVector &sceneGraph);
    void buildInstanceData();
};

}  // namespace vengine

#endif