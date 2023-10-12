#include "Export.hpp"

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <qdir.h>

#include <core/AssetManager.hpp>
#include <core/Lights.hpp>
#include <core/Model3D.hpp>
#include <math/MathUtils.hpp>

#include "debug_tools/Console.hpp"

using namespace rapidjson;

namespace vengine
{

std::string getFilename(std::string file)
{
    QStringList fileSplit = QString::fromStdString(file).split("/");
    return fileSplit.back().toStdString();
}

std::string copyFileToDirectoryAndGetFileName(std::string file, std::string directory, std::string newFileName = "")
{
    QFile sourceFile(QString::fromStdString(file));

    QString destination;
    if (newFileName.empty()) {
        /* Copy file to the directory with the same name */
        destination = QString::fromStdString(directory) + QFileInfo(sourceFile.fileName()).fileName();
    } else {
        /* Copy file to the directory with the requested name */
        destination = QString::fromStdString(directory + newFileName);
    }

    bool ret = sourceFile.copy(destination);
    if (!ret && sourceFile.errorString() != "Destination file exists") {
        debug_tools::ConsoleWarning("Can't export file: " + file + " with error: " + sourceFile.errorString().toStdString());
    }

    /* Get the name of the file itself */
    QStringList destinationSplit = destination.split("/");
    QString filename = destinationSplit.back();
    return filename.toStdString();
}

rapidjson::Value &addVec2(rapidjson::Document &d, rapidjson::Value &v, std::string name, glm::vec2 value)
{
    Value array;
    array.SetArray();

    Value r;
    r.SetFloat(value.r);
    array.PushBack(r, d.GetAllocator());
    Value g;
    g.SetFloat(value.g);
    array.PushBack(g, d.GetAllocator());

    Value nameObject;
    nameObject.SetString(name.c_str(), d.GetAllocator());
    v.AddMember(nameObject, array, d.GetAllocator());

    return v[name.c_str()];
}

rapidjson::Value &addVec3(rapidjson::Document &d, rapidjson::Value &v, std::string name, glm::vec3 value)
{
    auto &array = addVec2(d, v, name, glm::vec2(value.r, value.g));

    Value b;
    b.SetFloat(value.b);
    array.PushBack(b, d.GetAllocator());

    return v[name.c_str()];
}

void addVec4(rapidjson::Document &d, rapidjson::Value &v, std::string name, glm::vec4 value)
{
    auto &array = addVec3(d, v, name, glm::vec3(value.r, value.g, value.b));

    Value a;
    a.SetFloat(value.a);
    array.PushBack(a, d.GetAllocator());
}

void addTransform(rapidjson::Document &d, rapidjson::Value &v, const Transform &t)
{
    glm::vec3 transformRotation = glm::degrees(glm::eulerAngles(t.getRotation()));
    bool transformIsDefault = (t.getPosition().x == 0.0F && t.getPosition().y == 0.0F && t.getPosition().z == 0.0F &&
                               t.getScale().x == 1.0F && t.getScale().y == 1.0F && t.getScale().z == 1.0F &&
                               transformRotation.x == 0.0F && transformRotation.y == 0.0F && transformRotation.z == 0.0F);
    if (transformIsDefault)
        return;

    Value transform;
    transform.SetObject();

    addVec3(d, transform, "position", t.getPosition());
    addVec3(d, transform, "scale", t.getScale());
    addVec3(d, transform, "rotation", transformRotation);

    v.AddMember("transform", transform, d.GetAllocator());
}

void addMeshComponent(rapidjson::Document &d,
                      rapidjson::Value &v,
                      const std::shared_ptr<SceneObject> &sceneObject,
                      std::string assetDirectoryPrefix)
{
    Value meshObject;
    meshObject.SetObject();

    /* Get the mesh component */
    const Mesh *mesh = sceneObject->get<ComponentMesh>().mesh.get();

    /* Copy the mesh model file to the mesh folder, and get a file path relative to the scene file */
    std::string relativePathName = assetDirectoryPrefix + getFilename(mesh->m_model->name());

    /* Set the path to the copied file */
    Value path;
    path.SetString(relativePathName.c_str(), d.GetAllocator());
    meshObject.AddMember("path", path, d.GetAllocator());

    Value submesh;
    submesh.SetString(mesh->name().c_str(), d.GetAllocator());
    meshObject.AddMember("submesh", submesh, d.GetAllocator());

    v.AddMember("mesh", meshObject, d.GetAllocator());
}

void addMaterialComponent(rapidjson::Document &d, rapidjson::Value &v, const std::shared_ptr<SceneObject> &sceneObject)
{
    Value materialObject;
    materialObject.SetObject();

    /* Get the mesh component */
    const Material *material = sceneObject->get<ComponentMaterial>().material.get();

    /* Set the path to the copied file */
    Value path;
    path.SetString(material->name().c_str(), d.GetAllocator());
    materialObject.AddMember("name", path, d.GetAllocator());

    v.AddMember("material", materialObject, d.GetAllocator());
}

void addLightComponent(rapidjson::Document &d, rapidjson::Value &v, const std::shared_ptr<SceneObject> &sceneObject)
{
    Value lightObject;
    lightObject.SetObject();

    Light *light = sceneObject->get<ComponentLight>().light.get();

    Value type;
    if (light->type == LightType::POINT_LIGHT) {
        type.SetString("POINT", d.GetAllocator());
    } else if (light->type == LightType::DIRECTIONAL_LIGHT) {
        type.SetString("DIRECTIONAL", d.GetAllocator());
    }
    lightObject.AddMember("type", type, d.GetAllocator());

    Value lightMat;
    lightMat.SetString(light->lightMaterial->name().c_str(), d.GetAllocator());
    lightObject.AddMember("material", lightMat, d.GetAllocator());

    v.AddMember("light", lightObject, d.GetAllocator());
}

void addModel(rapidjson::Document &d,
              rapidjson::Value &v,
              const std::shared_ptr<Model3D> &model,
              std::string assetDirectoryPrefix,
              std::string assetDirectory)
{
    std::string relativePath = assetDirectoryPrefix + copyFileToDirectoryAndGetFileName(model->name(), assetDirectory);

    Value name;
    name.SetString(relativePath.c_str(), d.GetAllocator());
    v.PushBack(name, d.GetAllocator());
}

void addTexture(rapidjson::Document &d,
                rapidjson::Value &v,
                const std::shared_ptr<Texture> &tex,
                std::string assetDirectoryPrefix,
                std::string assetDirectory,
                std::string finalName)
{
    Value textureObject;
    textureObject.SetObject();

    if (tex->embedded()) {
        Value type;
        type.SetString("EMBEDDED", d.GetAllocator());
        textureObject.AddMember("type", type, d.GetAllocator());

        Value value;
        value.SetString(tex->name().c_str(), d.GetAllocator());
        textureObject.AddMember("value", value, d.GetAllocator());
    } else {
        Value type;
        type.SetString("DISK_FILE", d.GetAllocator());
        textureObject.AddMember("type", type, d.GetAllocator());

        auto extensionPointPosition = tex->name().find_last_of(".");
        auto fileType = tex->name().substr(extensionPointPosition + 1);

        std::string relativePath =
            assetDirectoryPrefix + copyFileToDirectoryAndGetFileName(tex->name(), assetDirectory, finalName + "." + fileType);

        Value value;
        value.SetString(relativePath.c_str(), d.GetAllocator());
        textureObject.AddMember("name", value, d.GetAllocator());
    }

    v.AddMember("texture", textureObject, d.GetAllocator());
}

void addMaterial(rapidjson::Document &d,
                 rapidjson::Value &v,
                 const std::shared_ptr<Material> &material,
                 std::string assetDirectoryPrefix,
                 std::string assetDirectory)
{
    Value mat;
    mat.SetObject();

    Value name;
    name.SetString(material->name().c_str(), d.GetAllocator());
    mat.AddMember("name", name, d.GetAllocator());

    /* Create a directory for the textures of that material */
    std::string materialDirectory = assetDirectory + material->name() + "/";
    auto createMatDirectory = [&materialDirectory]() { QDir().mkdir(QString::fromStdString(materialDirectory)); };
    bool directoryCreated = false;

    switch (material->getType()) {
        case MaterialType::MATERIAL_LAMBERT: {
            auto m = std::dynamic_pointer_cast<MaterialLambert>(material);

            Value filepath;
            filepath.SetString(material->filepath().c_str(), d.GetAllocator());
            mat.AddMember("filepath", filepath, d.GetAllocator());

            Value type;
            type.SetString("LAMBERT");
            mat.AddMember("type", type, d.GetAllocator());

            assetDirectoryPrefix += material->name() + "/";

            /* Set albedo */
            {
                Value albedoObject;
                albedoObject.SetObject();

                if (m->getAlbedoTexture() != nullptr && m->getAlbedoTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();

                    addTexture(d, albedoObject, m->getAlbedoTexture(), assetDirectoryPrefix, materialDirectory, "albedo");
                }

                addVec4(d, albedoObject, "value", m->albedo());

                mat.AddMember("albedo", albedoObject, d.GetAllocator());
            }

            /* Set emissive */
            {
                Value emissiveObject;
                emissiveObject.SetObject();

                if (m->getEmissiveTexture() != nullptr && m->getEmissiveTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();
                    addTexture(d, emissiveObject, m->getEmissiveTexture(), assetDirectoryPrefix, materialDirectory, "emissive");
                }

                addVec4(d, emissiveObject, "value", m->emissive());

                mat.AddMember("emissive", emissiveObject, d.GetAllocator());
            }

            /* Set normal */
            if (m->getNormalTexture() != nullptr && m->getNormalTexture()->name() != "normalmapdefault") {
                Value normalObject;
                normalObject.SetObject();

                if (!m->embedded() && !directoryCreated)
                    createMatDirectory();

                addTexture(d, normalObject, m->getNormalTexture(), assetDirectoryPrefix, materialDirectory, "normal");

                mat.AddMember("normal", normalObject, d.GetAllocator());
            }

            Value scale;
            scale.SetArray();
            scale.PushBack(m->uTiling(), d.GetAllocator());
            scale.PushBack(m->vTiling(), d.GetAllocator());
            mat.AddMember("scale", scale, d.GetAllocator());

            break;
        }
        case MaterialType::MATERIAL_PBR_STANDARD: {
            auto m = std::dynamic_pointer_cast<MaterialPBRStandard>(material);

            /* If it's a zip material */
            if (m->zipMaterial()) {
                Value type;
                type.SetString("STACK");
                mat.AddMember("type", type, d.GetAllocator());

                std::string relativePath = assetDirectoryPrefix + copyFileToDirectoryAndGetFileName(m->filepath(), assetDirectory);

                Value path;
                path.SetString(relativePath.c_str(), d.GetAllocator());
                mat.AddMember("path", path, d.GetAllocator());
                break;
            }

            if (m->embedded()) {
                Value type;
                type.SetString("EMBEDDED");
                mat.AddMember("type", type, d.GetAllocator());
                break;
            }

            Value type;
            type.SetString("PBR_STANDARD");
            mat.AddMember("type", type, d.GetAllocator());

            assetDirectoryPrefix += material->name() + "/";

            /* Set albedo */
            {
                Value albedoObject;
                albedoObject.SetObject();

                if (m->getAlbedoTexture() != nullptr && m->getAlbedoTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();

                    addTexture(d, albedoObject, m->getAlbedoTexture(), assetDirectoryPrefix, materialDirectory, "albedo");
                }

                addVec4(d, albedoObject, "value", m->albedo());

                mat.AddMember("albedo", albedoObject, d.GetAllocator());
            }

            /* Set roughness */
            {
                Value roughnessObject;
                roughnessObject.SetObject();

                if (m->getRoughnessTexture() != nullptr && m->getRoughnessTexture()->name() != "white") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();
                    addTexture(d, roughnessObject, m->getRoughnessTexture(), assetDirectoryPrefix, materialDirectory, "roughness");
                }

                Value roughnessValue;
                roughnessValue.SetFloat(m->roughness());
                roughnessObject.AddMember("value", roughnessValue, d.GetAllocator());

                mat.AddMember("roughness", roughnessObject, d.GetAllocator());
            }

            /* Set metallic */
            {
                Value metallicObject;
                metallicObject.SetObject();

                if (m->getMetallicTexture() != nullptr && m->getMetallicTexture()->name() != "white") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();
                    addTexture(d, metallicObject, m->getMetallicTexture(), assetDirectoryPrefix, materialDirectory, "metallic");
                }

                Value metallicValue;
                metallicValue.SetFloat(m->metallic());
                metallicObject.AddMember("value", metallicValue, d.GetAllocator());

                mat.AddMember("metallic", metallicObject, d.GetAllocator());
            }

            /* Set ao */
            {
                Value aoObject;
                aoObject.SetObject();

                if (m->getAOTexture() != nullptr && m->getAOTexture()->name() != "white") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();
                    addTexture(d, aoObject, m->getAOTexture(), assetDirectoryPrefix, materialDirectory, "ao");
                }

                Value aoValue;
                aoValue.SetFloat(m->ao());
                aoObject.AddMember("value", aoValue, d.GetAllocator());

                mat.AddMember("ao", aoObject, d.GetAllocator());
            }

            /* Set emissive */
            {
                Value emissiveObject;
                emissiveObject.SetObject();

                if (m->getEmissiveTexture() != nullptr && m->getEmissiveTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();
                    addTexture(d, emissiveObject, m->getEmissiveTexture(), assetDirectoryPrefix, materialDirectory, "emissive");
                }

                addVec4(d, emissiveObject, "value", m->emissive());

                mat.AddMember("emissive", emissiveObject, d.GetAllocator());
            }

            /* Set normal map*/
            if (m->getNormalTexture() != nullptr && m->getNormalTexture()->name() != "normalmapdefault") {
                Value normalObject;
                normalObject.SetObject();

                if (!m->embedded() && !directoryCreated)
                    createMatDirectory();

                addTexture(d, normalObject, m->getNormalTexture(), assetDirectoryPrefix, materialDirectory, "normal");

                mat.AddMember("normal", normalObject, d.GetAllocator());
            }

            Value scale;
            scale.SetArray();
            scale.PushBack(m->uTiling(), d.GetAllocator());
            scale.PushBack(m->vTiling(), d.GetAllocator());
            mat.AddMember("scale", scale, d.GetAllocator());

            break;
        }
        default:
            break;
    }

    v.PushBack(mat, d.GetAllocator());
}

void addLightMaterial(rapidjson::Document &d, rapidjson::Value &v, const std::shared_ptr<LightMaterial> &material)
{
    Value light;
    light.SetObject();

    Value name;
    name.SetString(material->name().c_str(), d.GetAllocator());
    light.AddMember("name", name, d.GetAllocator());

    addVec3(d, light, "color", material->color);

    Value intensity;
    intensity.SetFloat(material->intensity);
    light.AddMember("intensity", intensity, d.GetAllocator());

    v.PushBack(light, d.GetAllocator());
}

bool isMaterialEmissive(const Material *material)
{
    if (material == nullptr)
        return false;

    MaterialType type = material->getType();
    switch (type) {
        case MaterialType::MATERIAL_PBR_STANDARD: {
            const MaterialPBRStandard *m = dynamic_cast<const MaterialPBRStandard *>(material);
            return !isBlack(m->emissive());
            break;
        }
        case MaterialType::MATERIAL_LAMBERT: {
            const MaterialLambert *m = dynamic_cast<const MaterialLambert *>(material);
            return !isBlack(m->emissive());
            break;
        }
        default:
            throw std::runtime_error("isMaterialEmissive(): This should never happen");
            break;
    }
}

void parseSceneObject(rapidjson::Document &d,
                      rapidjson::Value &v,
                      const std::shared_ptr<SceneObject> &sceneObject,
                      std::string assetDirectoryPrefix)
{
    Value sceneObjectEntry;
    sceneObjectEntry.SetObject();

    /* name */
    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    sceneObjectEntry.AddMember("name", name, d.GetAllocator());

    /* transform */
    addTransform(d, sceneObjectEntry, sceneObject->m_localTransform);

    /* mesh */
    if (sceneObject->has<ComponentMesh>()) {
        addMeshComponent(d, sceneObjectEntry, sceneObject, assetDirectoryPrefix);
    }

    if (sceneObject->has<ComponentMaterial>()) {
        addMaterialComponent(d, sceneObjectEntry, sceneObject);
    }

    /* light */
    if (sceneObject->has<ComponentLight>()) {
        addLightComponent(d, sceneObjectEntry, sceneObject);
    }

    /* Add children */
    if (sceneObject->m_children.size() != 0) {
        Value children;
        children.SetArray();

        for (auto itr : sceneObject->m_children) {
            parseSceneObject(d, children, itr, assetDirectoryPrefix);
        }

        sceneObjectEntry.AddMember("children", children, d.GetAllocator());
    }

    v.PushBack(sceneObjectEntry, d.GetAllocator());
}

void exportJson(const ExportRenderParams &renderParams,
                std::shared_ptr<Camera> sceneCamera,
                const std::vector<std::shared_ptr<SceneObject>> &sceneGraph,
                std::shared_ptr<EnvironmentMap> envMap)
{
    /* Create folder with scene */
    std::string sceneFolderName = renderParams.name + "/"; /* Make sure the final back slash exists */
    std::string assetsDirectoryPrefix = "assets/";
    std::string assetsFolderName = sceneFolderName + assetsDirectoryPrefix;
    QDir().mkdir(QString::fromStdString(sceneFolderName));
    QDir().mkdir(QString::fromStdString(assetsFolderName));

    std::string fileName = renderParams.name + "/scene.json";

    Document d;
    d.SetObject();

    {
        Value version;
        version.SetString("1.0");
        d.AddMember("version", version, d.GetAllocator());
    }
    {
        Value name;
        name.SetString("scene.json", d.GetAllocator());
        d.AddMember("name", name, d.GetAllocator());
    }

    /* Camera */
    Value camera;
    {
        camera.SetObject();

        Transform &cameraTransform = sceneCamera->getTransform();
        {
            glm::vec3 cameraPosition = cameraTransform.getPosition();
            addVec3(d, camera, "position", cameraPosition);

            glm::vec3 cameraTarget = cameraPosition + cameraTransform.getForward();
            addVec3(d, camera, "target", cameraTarget);

            glm::vec3 cameraUp = cameraTransform.getUp();
            addVec3(d, camera, "up", cameraUp);
        }
        {
            if (sceneCamera->getType() != CameraType::PERSPECTIVE) {
                throw std::runtime_error("SceneExport: Only perspective camera supported");
            }
            float cameraFoV = dynamic_cast<PerspectiveCamera *>(sceneCamera.get())->getFoV();

            Value fov;
            fov.SetFloat(cameraFoV);
            camera.AddMember("fov", fov, d.GetAllocator());
        }
    }

    /* Parse the scene and keep here the resources referenced in the scene */
    Value scene;
    {
        scene.SetArray();

        for (auto &itr : sceneGraph) {
            parseSceneObject(d, scene, itr, assetsDirectoryPrefix);
        }
    }

    /* Store models */
    Value models;
    {
        models.SetArray();

        auto &models3D = AssetManager::getInstance().modelsMap();
        for (auto &itr : models3D) {
            addModel(d, models, std::static_pointer_cast<Model3D>(itr.second), assetsDirectoryPrefix, assetsFolderName);
        }
    }

    /* Add materials */
    Value materialsObject;
    {
        materialsObject.SetArray();

        auto &materials = AssetManager::getInstance().materialsMap();
        for (auto &itr : materials) {
            addMaterial(d, materialsObject, std::static_pointer_cast<Material>(itr.second), assetsDirectoryPrefix, assetsFolderName);
        }
    }

    /* Add light materials */
    Value lightMaterialsObject;
    {
        lightMaterialsObject.SetArray();

        auto &lightMaterials = AssetManager::getInstance().lightMaterialsMap();
        for (auto &itr : lightMaterials) {
            addLightMaterial(d, lightMaterialsObject, std::static_pointer_cast<LightMaterial>(itr.second));
        }
    }

    /* Environment */
    Value environment;
    {
        environment.SetObject();

        if (envMap != nullptr) {
            /* Copy environment map */
            std::string relativePath = assetsDirectoryPrefix + copyFileToDirectoryAndGetFileName(envMap->name(), assetsFolderName);

            Value path;
            path.SetString(relativePath.c_str(), d.GetAllocator());
            environment.AddMember("path", path, d.GetAllocator());
        }

        Value hideBackground;
        hideBackground.SetInt(renderParams.environmentType);
        environment.AddMember("environmentType", hideBackground, d.GetAllocator());

        addVec3(d, environment, "backgroundColor", renderParams.backgroundColor);
    }

    d.AddMember("camera", camera, d.GetAllocator());
    d.AddMember("scene", scene, d.GetAllocator());
    d.AddMember("models", models, d.GetAllocator());
    d.AddMember("materials", materialsObject, d.GetAllocator());
    d.AddMember("light materials", lightMaterialsObject, d.GetAllocator());
    d.AddMember("environment", environment, d.GetAllocator());

    std::ofstream of(fileName);
    OStreamWrapper osw(of);
    PrettyWriter<OStreamWrapper> writer(osw);
    d.Accept(writer);
}

}  // namespace vengine