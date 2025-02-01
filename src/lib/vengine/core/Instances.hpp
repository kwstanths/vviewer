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
/* Describes the instance of a scene object */
struct InstanceData {
    /* Transformation matrix */
    glm::mat4 modelMatrix;
    /* R: ID, G: front facing volume, B: back facing volume, A: unused */
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
}; /* sizeof(LightInstance) = 120 */

/* A GPU clone */
/* Describes the instance of a light */
struct LightInstance {
    glm::uvec4 info; /* R = LightData index, G = InstanceData index, B = Object Description index if Mesh type, A = type (LightType) */
    glm::vec4 position;  /* RGB = world position/direction, A = casts shadow or RGBA = row 0 of transform matrix if mesh type */
    glm::vec4 position1; /* RGBA = row 1 of transform matrix if mesh type */
    glm::vec4 position2; /* RGBA = row 2 of transform matrix if mesh type */
};                       /* sizeof(LightInstance) = 64 */

class Scene;

class InstancesManager
{
public:
    struct MeshGroup {
        SceneObjectVector sceneObjects;
        /* starting index for m_instancesBuffer */
        uint32_t startIndex = 0;
    };

    typedef std::unordered_map<SceneObject *, InstanceData *> SceneObjectInstanceMap;

    InstancesManager(Scene *scene);

    /**
     * @brief Initialize object
     *
     * @param instancesBuffer InstanceData buffer pointer
     * @param instancesBufferSize Buffer size
     */
    void initResources(InstanceData *instancesBuffer, uint32_t instancesBufferSize);

    virtual void build();
    bool isBuilt() const { return m_isBuilt; }

    void invalidate();

    const std::unordered_map<Mesh *, MeshGroup> &opaqueMeshes() const { return m_instancesOpaque; }
    const SceneObjectVector &transparentMeshes() const { return m_transparent; }
    const SceneObjectVector &lights() const { return m_lights; }
    const SceneObjectVector &meshLights() const { return m_meshLights; }
    const SceneObjectVector &volumes() const { return m_volumes; }

    const SceneObjectInstanceMap &sceneObjectInstanceMap() const { return m_sceneObjectMap; }
    InstanceData *findInstanceData(SceneObject *so) const;
    uint32_t findInstanceDataIndex(SceneObject *so) const;

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
    /* Holds all scene objects that constitute a volume change */
    SceneObjectVector m_volumes;

    SceneObjectInstanceMap m_sceneObjectMap;

    bool m_isBuilt = false;

    /**
     * @brief Set the InstanceData for a scene object
     *
     * @param instanceData
     * @param so
     */
    virtual void initInstanceData(InstanceData *instanceData, SceneObject *so);

private:
    Scene *m_scene = nullptr;

    /* CPU memory for the InstanceData buffer */
    InstanceData *m_instancesBuffer = nullptr;
    uint32_t m_instancesBufferSize = 0;

    void fillSceneObjectVectors(SceneObjectVector &sceneGraph);
    void buildInstanceDataFromScratch();
};

}  // namespace vengine

#endif