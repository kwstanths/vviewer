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
    /* RGB: ID, A: selected */
    glm::vec4 id;
    /* Material index */
    uint32_t materialIndex;
    /* A pointer to a buffer holding mesh vertex data */
    uint64_t vertexAddress;
    /* A pointer to a buffer holding mesh index data*/
    uint64_t indexAddress;
    /* The number of triangles in the index buffer */
    uint32_t numTriangles;
};

class InstancesManager
{
public:
    struct MeshGroup {
        std::vector<SceneObject *> sceneObjects;
        /* starting index for m_instancesBuffer */
        uint32_t startIndex;
    };

    struct MaterialGroup {
        MaterialType materialType;
        std::unordered_map<Mesh *, MeshGroup> meshGroups;
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

    virtual void build(SceneGraph &sceneGraph);
    bool isBuilt() const { return m_isBuilt; }

    void reset();

    const SceneObjectMap &sceneObjectMap() const { return m_sceneObjectMap; }

    const std::vector<MaterialGroup> &opaqueMeshes() const { return m_instancesOpaque; }
    const std::vector<SceneObject *> &transparentMeshes() const { return m_transparent; }
    const std::vector<SceneObject *> &lights() const { return m_lights; }
    const std::vector<SceneObject *> &meshLights() const { return m_meshLights; }
    const MaterialGroup &materialGroup(MaterialType type) const;

    InstanceData *getInstanceData(SceneObject *so);
    uint32_t getInstanceDataIndex(SceneObject *so);

    void sortTransparent(const glm::vec3 &pos);

protected:
    /* Holds all opaque mesh instances in the scene */
    std::vector<MaterialGroup> m_instancesOpaque;
    /* Holds all transparent mesh objects in the scene */
    std::vector<SceneObject *> m_transparent;
    /* Holds all scene objects that have light components */
    std::vector<SceneObject *> m_lights;
    /* Holds all scene objects that are mesh lights */
    std::vector<SceneObject *> m_meshLights;

    SceneObjectMap m_sceneObjectMap;

    bool m_isBuilt = false;

    /**
     * @brief Set the InstanceData for a scene object
     *
     * @param instanceData
     * @param so
     */
    virtual void setInstanceData(InstanceData *instanceData, SceneObject *so);

private:
    /* CPU memory for the InstanceData buffer */
    InstanceData *m_instancesBuffer = nullptr;
    uint32_t m_instancesBufferSize = 0;

    void fillSceneObjectVectors(SceneGraph &sceneGraph);
    void buildInstanceData();
};

}  // namespace vengine

#endif