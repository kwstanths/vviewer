#include "SceneUtils.hpp"

#include "vengine/core/AssetManager.hpp"
#include <functional>

void addModel3D(vengine::Scene &scene,
                vengine::SceneObject *parent,
                std::string objectName,
                std::string modelName,
                std::optional<vengine::Transform> overrideRootTransform,
                std::optional<std::string> overrideMaterial)
{
    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();

    auto model3D = instanceModels.get(modelName);
    assert(model3D != nullptr);

    vengine::Tree<vengine::Model3D::Model3DNode> &modelData = model3D->data();

    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();
    vengine::Material *defaultMat = instanceMaterials.get("defaultMaterial");
    vengine::Material *overrideMat = (overrideMaterial.has_value() ? instanceMaterials.get(overrideMaterial.value()) : nullptr);

    std::function<void(vengine::Tree<vengine::Model3D::Model3DNode> &, vengine::SceneObject *, bool)> addModelF =
        [&](vengine::Tree<vengine::Model3D::Model3DNode> &modelData, vengine::SceneObject *parent, bool isRoot) {
            auto &modelNode = modelData.data();
            /* Add all meshes of current node under parent */
            for (uint32_t i = 0; i < modelNode.meshes.size(); i++) {
                auto &mesh = modelNode.meshes[i];
                auto mat = modelNode.materials[i];

                vengine::Transform nodeTransform = modelNode.transform;
                if (isRoot && overrideRootTransform.has_value()) {
                    nodeTransform = overrideRootTransform.value();
                }
                vengine::SceneObject *newObject = scene.addSceneObject(modelName, parent, nodeTransform);

                newObject->add<vengine::ComponentMesh>().setMesh(mesh);
                if (overrideMat != nullptr) {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(overrideMat);
                } else if (mat != nullptr) {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(mat);
                } else {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(defaultMat);
                }
            }

            if (modelData.size() > 0) {
                vengine::Transform nodeTransform = modelNode.transform;
                if (isRoot && overrideRootTransform.has_value()) {
                    nodeTransform = overrideRootTransform.value();
                }
                vengine::SceneObject *newObject = scene.addSceneObject(modelNode.name, parent, nodeTransform);
                for (uint32_t i = 0; i < modelData.size(); i++) {
                    addModelF(modelData.child(i), newObject, false);
                }
            }
        };

    addModelF(modelData, parent, true);
}