#ifndef __SceneUtils_hpp__
#define __SceneUtils_hpp__

#include "vengine/core/Engine.hpp"

/**
 * @brief Add a 3D model in the scene
 *
 * @param scene
 * @param parent
 * @param objectName
 * @param modelName
 * @param transform
 * @param overrideMaterial
 */
void addModel3D(vengine::Scene &scene,
                vengine::SceneObject *parent,
                std::string objectName,
                std::string modelName,
                std::optional<vengine::Transform> overrideRootTransform = std::nullopt,
                std::optional<std::string> overrideMaterial = std::nullopt);

#endif /* __SceneUtils_hpp__ */
