#include "Export.hpp"

#include <fstream>
#include <filesystem>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <core/AssetManager.hpp>
#include <core/Light.hpp>
#include <core/Model3D.hpp>
#include <core/StringUtils.hpp>
#include <math/MathUtils.hpp>

#include "debug_tools/Console.hpp"

using namespace rapidjson;

namespace vengine
{

static const std::string VENGINE_EXPORT_ASSETS_FOLDER = "assets/";
static const std::string VENGINE_EXPORT_MODELS_FOLDER = VENGINE_EXPORT_ASSETS_FOLDER + "models/";
static const std::string VENGINE_EXPORT_TEXTURES_FOLDER = VENGINE_EXPORT_ASSETS_FOLDER + "textures/";

std::string copyFileToDirectoryAndGetFileName(std::string file, std::string directory, std::string newFileName = "")
{
    std::string copiedFilename = newFileName;
    if (newFileName.empty()) {
        copiedFilename = getFilename(file);
    }
    std::string destination = directory + copiedFilename;

    std::error_code err;
    try {
        std::filesystem::copy(file, destination, err);
    } catch (std::exception &e) {
        if (err != std::errc::file_exists) {
            debug_tools::ConsoleWarning("Export copy error: " + std::string(e.what()));
        }
    }

    return copiedFilename;
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
    glm::vec3 transformRotation = glm::degrees(glm::eulerAngles(t.rotation()));
    bool transformIsDefault =
        (t.position().x == 0.0F && t.position().y == 0.0F && t.position().z == 0.0F && t.scale().x == 1.0F && t.scale().y == 1.0F &&
         t.scale().z == 1.0F && transformRotation.x == 0.0F && transformRotation.y == 0.0F && transformRotation.z == 0.0F);
    if (transformIsDefault)
        return;

    Value transform;
    transform.SetObject();

    addVec3(d, transform, "position", t.position());
    addVec3(d, transform, "scale", t.scale());
    addVec3(d, transform, "rotation", transformRotation);

    v.AddMember("transform", transform, d.GetAllocator());
}

void addMeshComponent(rapidjson::Document &d, rapidjson::Value &v, const SceneObject *sceneObject)
{
    Value meshObject;
    meshObject.SetObject();

    /* Get the mesh component */
    const Mesh *mesh = sceneObject->get<ComponentMesh>().mesh;
    const Model3D *model = mesh->m_model;

    Value name;
    name.SetString(model->name().c_str(), d.GetAllocator());
    meshObject.AddMember("modelName", name, d.GetAllocator());

    Value submesh;
    submesh.SetString(mesh->name().c_str(), d.GetAllocator());
    meshObject.AddMember("submesh", submesh, d.GetAllocator());

    v.AddMember("mesh", meshObject, d.GetAllocator());
}

void addMaterialComponent(rapidjson::Document &d, rapidjson::Value &v, const SceneObject *sceneObject)
{
    Value materialObject;
    materialObject.SetObject();

    /* Get the mesh component */
    const Material *material = sceneObject->get<ComponentMaterial>().material;

    /* Set the path to the copied file */
    Value path;
    path.SetString(material->name().c_str(), d.GetAllocator());
    materialObject.AddMember("name", path, d.GetAllocator());

    v.AddMember("material", materialObject, d.GetAllocator());
}

void addLightComponent(rapidjson::Document &d, rapidjson::Value &v, const SceneObject *sceneObject)
{
    Value lightObject;
    lightObject.SetObject();

    Light *light = sceneObject->get<ComponentLight>().light;

    Value name;
    name.SetString(light->name().c_str(), d.GetAllocator());
    lightObject.AddMember("name", name, d.GetAllocator());

    v.AddMember("light", lightObject, d.GetAllocator());
}

void addModel(rapidjson::Document &d, rapidjson::Value &v, const Model3D *model, std::string storeDirectory)
{
    if (model->internal())
        return;

    Value modelObject;
    modelObject.SetObject();

    Value name;
    name.SetString(model->name().c_str(), d.GetAllocator());
    modelObject.AddMember("name", name, d.GetAllocator());

    std::string relativePath = VENGINE_EXPORT_MODELS_FOLDER + copyFileToDirectoryAndGetFileName(model->filepath(), storeDirectory);
    Value filepath;
    filepath.SetString(relativePath.c_str(), d.GetAllocator());
    modelObject.AddMember("filepath", filepath, d.GetAllocator());

    v.PushBack(modelObject, d.GetAllocator());
}

void addTexture(rapidjson::Document &d,
                rapidjson::Value &v,
                const Texture *tex,
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

        Value name;
        name.SetString(tex->name().c_str(), d.GetAllocator());
        textureObject.AddMember("name", name, d.GetAllocator());
    } else {
        Value type;
        type.SetString("STANDALONE", d.GetAllocator());
        textureObject.AddMember("type", type, d.GetAllocator());

        Value name;
        name.SetString(tex->name().c_str(), d.GetAllocator());
        textureObject.AddMember("name", name, d.GetAllocator());

        auto extensionPointPosition = tex->filepath().find_last_of(".");
        auto fileType = tex->filepath().substr(extensionPointPosition + 1);
        std::string relativePath =
            assetDirectoryPrefix + copyFileToDirectoryAndGetFileName(tex->filepath(), assetDirectory, finalName + "." + fileType);

        Value filepath;
        filepath.SetString(relativePath.c_str(), d.GetAllocator());
        textureObject.AddMember("filepath", filepath, d.GetAllocator());
    }

    v.AddMember("texture", textureObject, d.GetAllocator());
}

void addMaterial(rapidjson::Document &d, rapidjson::Value &v, const Material *material, std::string storeDirectory)
{
    Value mat;
    mat.SetObject();

    Value name;
    name.SetString(material->name().c_str(), d.GetAllocator());
    mat.AddMember("name", name, d.GetAllocator());

    /* Create a directory for the textures of that material */
    std::string materialDirectory = storeDirectory + material->name() + "/";
    auto createMatDirectory = [&materialDirectory]() { std::filesystem::create_directory(materialDirectory); };
    bool directoryCreated = false;

    std::string materialFolderPrefix = VENGINE_EXPORT_TEXTURES_FOLDER;
    switch (material->type()) {
        case MaterialType::MATERIAL_LAMBERT: {
            auto m = dynamic_cast<const MaterialLambert *>(material);

            Value type;
            type.SetString("LAMBERT");
            mat.AddMember("type", type, d.GetAllocator());

            materialFolderPrefix += material->name() + "/";

            /* Set albedo */
            {
                Value albedoObject;
                albedoObject.SetObject();

                if (m->getAlbedoTexture() != nullptr && m->getAlbedoTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();

                    addTexture(d, albedoObject, m->getAlbedoTexture(), materialFolderPrefix, materialDirectory, "albedo");
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
                    addTexture(d, emissiveObject, m->getEmissiveTexture(), materialFolderPrefix, materialDirectory, "emissive");
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

                addTexture(d, normalObject, m->getNormalTexture(), materialFolderPrefix, materialDirectory, "normal");

                mat.AddMember("normal", normalObject, d.GetAllocator());
            }

            /* Set alpha */
            if (m->getAlphaTexture() != nullptr && m->getAlphaTexture()->name() != "white") {
                Value alphaObject;
                alphaObject.SetObject();

                if (!m->embedded() && !directoryCreated)
                    createMatDirectory();

                addTexture(d, alphaObject, m->getAlphaTexture(), materialFolderPrefix, materialDirectory, "alpha");

                mat.AddMember("alpha", alphaObject, d.GetAllocator());
            }

            Value transparent;
            transparent.SetBool(m->transparent());
            mat.AddMember("transparent", transparent, d.GetAllocator());

            Value scale;
            scale.SetArray();
            scale.PushBack(m->uTiling(), d.GetAllocator());
            scale.PushBack(m->vTiling(), d.GetAllocator());
            mat.AddMember("scale", scale, d.GetAllocator());

            break;
        }
        case MaterialType::MATERIAL_PBR_STANDARD: {
            auto m = dynamic_cast<const MaterialPBRStandard *>(material);

            /* If it's a zip material */
            if (m->zipMaterial()) {
                Value type;
                type.SetString("STACK");
                mat.AddMember("type", type, d.GetAllocator());

                std::string relativePath =
                    VENGINE_EXPORT_TEXTURES_FOLDER + copyFileToDirectoryAndGetFileName(m->filepath(), storeDirectory);

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

            materialFolderPrefix += material->name() + "/";

            /* Set albedo */
            {
                Value albedoObject;
                albedoObject.SetObject();

                if (m->getAlbedoTexture() != nullptr && m->getAlbedoTexture()->name() != "whiteColor") {
                    if (!m->embedded() && !directoryCreated)
                        createMatDirectory();

                    addTexture(d, albedoObject, m->getAlbedoTexture(), materialFolderPrefix, materialDirectory, "albedo");
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
                    addTexture(d, roughnessObject, m->getRoughnessTexture(), materialFolderPrefix, materialDirectory, "roughness");
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
                    addTexture(d, metallicObject, m->getMetallicTexture(), materialFolderPrefix, materialDirectory, "metallic");
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
                    addTexture(d, aoObject, m->getAOTexture(), materialFolderPrefix, materialDirectory, "ao");
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
                    addTexture(d, emissiveObject, m->getEmissiveTexture(), materialFolderPrefix, materialDirectory, "emissive");
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

                addTexture(d, normalObject, m->getNormalTexture(), materialFolderPrefix, materialDirectory, "normal");

                mat.AddMember("normal", normalObject, d.GetAllocator());
            }

            /* Set alpha */
            if (m->getAlphaTexture() != nullptr && m->getAlphaTexture()->name() != "white") {
                Value alphaObject;
                alphaObject.SetObject();

                if (!m->embedded() && !directoryCreated)
                    createMatDirectory();

                addTexture(d, alphaObject, m->getAlphaTexture(), materialFolderPrefix, materialDirectory, "alpha");

                mat.AddMember("alpha", alphaObject, d.GetAllocator());
            }

            Value transparent;
            transparent.SetBool(m->transparent());
            mat.AddMember("transparent", transparent, d.GetAllocator());

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

void addLight(rapidjson::Document &d, rapidjson::Value &v, const Light *light)
{
    Value lightObject;
    lightObject.SetObject();

    Value name;
    name.SetString(light->name().c_str(), d.GetAllocator());
    lightObject.AddMember("name", name, d.GetAllocator());

    Value type;
    if (light->type() == LightType::POINT_LIGHT) {
        type.SetString("POINT", d.GetAllocator());
        lightObject.AddMember("type", type, d.GetAllocator());

        auto pl = static_cast<const PointLight *>(light);
        addVec3(d, lightObject, "color", glm::vec3(pl->color()));

        Value intensity;
        intensity.SetFloat(pl->color().a);
        lightObject.AddMember("intensity", intensity, d.GetAllocator());
    } else if (light->type() == LightType::DIRECTIONAL_LIGHT) {
        type.SetString("DIRECTIONAL", d.GetAllocator());
        lightObject.AddMember("type", type, d.GetAllocator());

        auto pl = static_cast<const DirectionalLight *>(light);
        addVec3(d, lightObject, "color", glm::vec3(pl->color()));

        Value intensity;
        intensity.SetFloat(pl->color().a);
        lightObject.AddMember("intensity", intensity, d.GetAllocator());
    }

    v.PushBack(lightObject, d.GetAllocator());
}

bool isMaterialEmissive(const Material *material)
{
    if (material == nullptr)
        return false;

    MaterialType type = material->type();
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
            throw std::runtime_error("Export::isMaterialEmissive(): Unexpected material");
            break;
    }
}

void parseSceneObject(rapidjson::Document &d, rapidjson::Value &v, const SceneObject *sceneObject)
{
    Value sceneObjectEntry;
    sceneObjectEntry.SetObject();

    /* name */
    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    sceneObjectEntry.AddMember("name", name, d.GetAllocator());

    /* transform */
    addTransform(d, sceneObjectEntry, sceneObject->localTransform());

    /* mesh */
    if (sceneObject->has<ComponentMesh>()) {
        addMeshComponent(d, sceneObjectEntry, sceneObject);
    }

    if (sceneObject->has<ComponentMaterial>()) {
        addMaterialComponent(d, sceneObjectEntry, sceneObject);
    }

    /* light */
    if (sceneObject->has<ComponentLight>()) {
        addLightComponent(d, sceneObjectEntry, sceneObject);
    }

    /* Add children */
    if (sceneObject->children().size() != 0) {
        Value children;
        children.SetArray();

        for (auto itr : sceneObject->children()) {
            parseSceneObject(d, children, itr);
        }

        sceneObjectEntry.AddMember("children", children, d.GetAllocator());
    }

    v.PushBack(sceneObjectEntry, d.GetAllocator());
}

void exportJson(const ExportRenderParams &renderParams,
                std::shared_ptr<Camera> sceneCamera,
                const SceneGraph &sceneGraph,
                EnvironmentMap *envMap)
{
    /* Create folder with scene */
    std::string sceneFolderName = renderParams.name + "/"; /* Make sure the final back slash exists */
    std::string assetsFolderName = sceneFolderName + VENGINE_EXPORT_ASSETS_FOLDER;
    std::string assetModelsFolderName = sceneFolderName + VENGINE_EXPORT_MODELS_FOLDER;
    std::string assetTexturesFolderName = sceneFolderName + VENGINE_EXPORT_TEXTURES_FOLDER;
    std::filesystem::create_directory(sceneFolderName);
    std::filesystem::create_directory(assetsFolderName);
    std::filesystem::create_directory(assetModelsFolderName);
    std::filesystem::create_directory(assetTexturesFolderName);

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

        Transform &cameraTransform = sceneCamera->transform();
        {
            glm::vec3 cameraPosition = cameraTransform.position();
            addVec3(d, camera, "position", cameraPosition);

            glm::vec3 cameraTarget = cameraPosition + cameraTransform.forward();
            addVec3(d, camera, "target", cameraTarget);

            glm::vec3 cameraUp = cameraTransform.up();
            addVec3(d, camera, "up", cameraUp);
        }
        {
            if (sceneCamera->type() != CameraType::PERSPECTIVE) {
                throw std::runtime_error("SceneExport: Only perspective camera supported");
            }
            float cameraFoV = dynamic_cast<PerspectiveCamera *>(sceneCamera.get())->fov();

            Value fov;
            fov.SetFloat(cameraFoV);
            camera.AddMember("fov", fov, d.GetAllocator());
        }
    }

    /* Scene */
    Value scene;
    {
        scene.SetArray();

        for (auto &itr : sceneGraph) {
            parseSceneObject(d, scene, itr);
        }
    }

    /* Store models */
    Value models;
    {
        models.SetArray();

        auto &models3D = AssetManager::getInstance().modelsMap();
        for (auto &itr : models3D) {
            addModel(d, models, static_cast<Model3D *>(itr.second), assetModelsFolderName);
        }
    }

    /* Add materials */
    Value materialsObject;
    {
        materialsObject.SetArray();

        auto &materials = AssetManager::getInstance().materialsMap();
        for (auto &itr : materials) {
            addMaterial(d, materialsObject, static_cast<Material *>(itr.second), assetTexturesFolderName);
        }
    }

    /* Add light materials */
    Value lightsObject;
    {
        lightsObject.SetArray();

        auto &lights = AssetManager::getInstance().lightsMap();
        for (auto &itr : lights) {
            addLight(d, lightsObject, static_cast<Light *>(itr.second));
        }
    }

    /* Environment */
    Value environment;
    {
        environment.SetObject();

        if (envMap != nullptr) {
            /* Copy environment map */
            std::string relativePath =
                VENGINE_EXPORT_ASSETS_FOLDER + copyFileToDirectoryAndGetFileName(envMap->name(), assetsFolderName);

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
    d.AddMember("lights", lightsObject, d.GetAllocator());
    d.AddMember("environment", environment, d.GetAllocator());

    std::ofstream of(fileName);
    OStreamWrapper osw(of);
    PrettyWriter<OStreamWrapper> writer(osw);
    d.Accept(writer);
}

}  // namespace vengine