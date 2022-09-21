#include "SceneExport.hpp"

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <qdir.h>

using namespace rapidjson;

std::string copyFileToDirectoryAndGetFileName(std::string file, std::string directory)
{
    /* Copy file to the directory with the same name */
    QFile sourceFile(QString::fromStdString(file));
    QString destination = QString::fromStdString(directory) + QFileInfo(sourceFile.fileName()).fileName();
    sourceFile.copy(destination);
    /* Get the name of the file itself */
    QStringList destinationSplit = destination.split("/");
    QString filename = destinationSplit.back();
    return filename.toStdString();
}

void exportJson(std::string name, 
    std::shared_ptr<Camera> sceneCamera, 
    const std::vector<std::shared_ptr<SceneObject>>& sceneGraph, 
    EnvironmentMap* envMap,
    uint32_t width, uint32_t height, uint32_t samples)
{
    /* Create folder with scene */
    std::string sceneFolderName = name + "/"; /* Make sure the final back slash exists */
    std::string sceneMeshesFolderName = sceneFolderName + "meshes/";
    std::string sceneMaterialsFolderName = sceneFolderName + "materials/";
    QDir().mkdir(QString::fromStdString(sceneFolderName));
    QDir().mkdir(QString::fromStdString(sceneMeshesFolderName));
    QDir().mkdir(QString::fromStdString(sceneMaterialsFolderName));

    std::string fileName = name + "/scene.json";

    Document d;
    d.SetObject();

    {
        Value version;
        version.SetString("0.1");
        d.AddMember("version", version, d.GetAllocator());
    }
    {
        Value name;
        name.SetString("scene.json", d.GetAllocator());
        d.AddMember("name", name, d.GetAllocator());
    }

    /* Render */
    {
        Value resolution;
        resolution.SetObject();

        Value x;
        x.SetUint(width);
        resolution.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetUint(height);
        resolution.AddMember("y", y, d.GetAllocator());

        d.AddMember("resolution", resolution, d.GetAllocator());
    }
    {
        Value samplesV;
        samplesV.SetUint(samples);
        d.AddMember("samples", samplesV, d.GetAllocator());
    }

    /* Camera */
    {
        Value camera;
        camera.SetObject();        

        Transform& cameraTransform = sceneCamera->getTransform();
        {

            glm::vec3 cameraPosition = cameraTransform.getPosition();
            Value position;
            position.SetObject();
            {
                Value x;
                x.SetFloat(cameraPosition.x);
                position.AddMember("x", x, d.GetAllocator());
                Value y;
                y.SetFloat(cameraPosition.y);
                position.AddMember("y", y, d.GetAllocator());
                Value z;
                z.SetFloat(cameraPosition.z);
                position.AddMember("z", z, d.GetAllocator());
            }
            camera.AddMember("position", position, d.GetAllocator());

            glm::vec3 cameraTarget = cameraPosition + cameraTransform.getForward();
            Value target;
            target.SetObject();
            {
                Value x;
                x.SetFloat(cameraTarget.x);
                target.AddMember("x", x, d.GetAllocator());
                Value y;
                y.SetFloat(cameraTarget.y);
                target.AddMember("y", y, d.GetAllocator());
                Value z;
                z.SetFloat(cameraTarget.z);
                target.AddMember("z", z, d.GetAllocator());
            }
            camera.AddMember("target", target, d.GetAllocator());

            glm::vec3 cameraUp = cameraTransform.getUp();
            Value up;
            up.SetObject();
            {
                Value x;
                x.SetFloat(cameraUp.x);
                up.AddMember("x", x, d.GetAllocator());
                Value y;
                y.SetFloat(cameraUp.y);
                up.AddMember("y", y, d.GetAllocator());
                Value z;
                z.SetFloat(cameraUp.z);
                up.AddMember("z", z, d.GetAllocator());
            }

            camera.AddMember("up", up, d.GetAllocator());
        }

        if (sceneCamera->getType() != CameraType::PERSPECTIVE)
            throw std::runtime_error("SceneExport: Only perspective camera supported");
        float cameraFoV = dynamic_cast<PerspectiveCamera*>(sceneCamera.get())->getFoV();

        Value fov;
        fov.SetFloat(cameraFoV);
        camera.AddMember("fov", fov, d.GetAllocator());

        d.AddMember("camera", camera, d.GetAllocator());
    }

    std::unordered_set<Material*> materials;

    /* Scene */
    {
        Value scene;
        scene.SetArray();

        for (auto& itr : sceneGraph)
        {
            parseSceneObject(d, scene, itr, materials, sceneMeshesFolderName);
        }

        d.AddMember("scene", scene, d.GetAllocator());
    }

    /* Materials */
    {
        Value mats;
        mats.SetArray();

        for (auto& itr : materials)
        {
            addJsonSceneMaterial(d, mats, itr, sceneMaterialsFolderName);
        }

        d.AddMember("materials", mats, d.GetAllocator());
    }

    /* Environment */
    if (envMap != nullptr)
    {
        Value environment;
        environment.SetObject();

        /* Copy environment map */
        std::string relativePath = copyFileToDirectoryAndGetFileName(envMap->m_name, sceneFolderName);

        Value path;
        path.SetString(relativePath.c_str(), d.GetAllocator());
        environment.AddMember("path", path, d.GetAllocator());

        Value rotation;
        rotation.SetObject();
        Value x;
        x.SetFloat(0.0);
        rotation.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetFloat(0.0);
        rotation.AddMember("y", y, d.GetAllocator());
        Value z;
        z.SetFloat(0.0);
        rotation.AddMember("z", z, d.GetAllocator());
        environment.AddMember("rotation", rotation, d.GetAllocator());

        Value intensity;
        intensity.SetFloat(1.0F);
        environment.AddMember("intensity", intensity, d.GetAllocator());

        d.AddMember("environment", environment, d.GetAllocator());
    }

    std::ofstream of(fileName);
    OStreamWrapper osw(of);
    PrettyWriter<OStreamWrapper> writer(osw);
    d.Accept(writer);
}

void parseSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::unordered_set<Material*>& materials, std::string meshDirectory)
{
    if (sceneObject->get<Mesh*>(ComponentType::MESH) != nullptr) {
        addJsonSceneObject(d, v, sceneObject, sceneObject->m_parent->m_localTransform, meshDirectory);

        Material* mat = sceneObject->get<Material*>(ComponentType::MATERIAL);
        materials.insert(mat);
    }

    for (auto& child : sceneObject->m_children) {
        parseSceneObject(d, v, child, materials, meshDirectory);
    }
}

void addJsonSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, const Transform& t, std::string meshDirectory)
{
    Value meshObject;
    meshObject.SetObject();

    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    meshObject.AddMember("name", name, d.GetAllocator());

    std::string meshName = sceneObject->get<Mesh*>(ComponentType::MESH)->m_meshModel->getName();
    /* Copy mesh to mesh folder, and get a file path relative to the scene file */
    std::string relativePathName = "meshes/" + copyFileToDirectoryAndGetFileName(meshName, meshDirectory);

    /* Set the path to the copied file */
    Value path;
    path.SetString(relativePathName.c_str(), d.GetAllocator());
    meshObject.AddMember("path", path, d.GetAllocator());

    std::string materialName = sceneObject->get<Material*>(ComponentType::MATERIAL)->m_name;
    Value material;
    material.SetString(materialName.c_str(), d.GetAllocator());
    meshObject.AddMember("material", material, d.GetAllocator());

    addJsonSceneTransform(d, meshObject, t);

    /* If mesh is emmisive add it as a light */
    const MaterialLambert* mat = dynamic_cast<const MaterialLambert*>(sceneObject->get<Material*>(ComponentType::MATERIAL));
    if (mat != nullptr && mat->getEmissive() >= 0.0001) {
        Value light;
        light.SetObject();
        Value intensity;
        intensity.SetFloat(mat->getEmissive());
        light.AddMember("intensity", intensity, d.GetAllocator());

        Value color;
        color.SetObject();
        addJsonSceneColor(d, color, glm::vec4(1.0));
        light.AddMember("color", color, d.GetAllocator());

        meshObject.AddMember("light", light, d.GetAllocator());
    }

    v.PushBack(meshObject, d.GetAllocator());
}

void addJsonSceneTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t)
{
    Value transform;
    transform.SetObject();

    {
        glm::vec3 pos = t.getPosition();
        Value position;
        position.SetObject();

        Value x;
        x.SetFloat(pos.x);
        position.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetFloat(pos.y);
        position.AddMember("y", y, d.GetAllocator());
        Value z;
        z.SetFloat(pos.z);
        position.AddMember("z", z, d.GetAllocator());

        transform.AddMember("position", position, d.GetAllocator());
    }

    {
        glm::vec3 s = t.getScale();
        Value scale;
        scale.SetObject();

        Value x;
        x.SetFloat(s.x);
        scale.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetFloat(s.y);
        scale.AddMember("y", y, d.GetAllocator());
        Value z;
        z.SetFloat(s.z);
        scale.AddMember("z", z, d.GetAllocator());

        transform.AddMember("scale", scale, d.GetAllocator());
    }

    {
        glm::vec3 r = glm::eulerAngles(t.getRotation());
        Value rotation;
        rotation.SetObject();

        Value x;
        x.SetFloat(glm::degrees(r.x));
        rotation.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetFloat(glm::degrees(r.y));
        rotation.AddMember("y", y, d.GetAllocator());
        Value z;
        z.SetFloat(glm::degrees(r.z));
        rotation.AddMember("z", z, d.GetAllocator());

        transform.AddMember("rotation", rotation, d.GetAllocator());
    }

    v.AddMember("transform", transform, d.GetAllocator());
}

void addJsonSceneMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material, std::string materialsDirectory)
{
    Value mat;
    mat.SetObject();

    Value name;
    name.SetString(material->m_name.c_str(), d.GetAllocator());
    mat.AddMember("name", name, d.GetAllocator());
    /* Create a directory for textures for that material */
    std::string materialDirectory = materialsDirectory + material->m_name + "/";
    QDir().mkdir(QString::fromStdString(materialDirectory));

    switch (material->getType())
    {
    case MaterialType::MATERIAL_LAMBERT:
    {
        const MaterialLambert* m = dynamic_cast<const MaterialLambert*>(material);

        Value type;
        type.SetString("DIFFUSE");
        mat.AddMember("type", type, d.GetAllocator());

        /* Set albedo */
        Value albedo;
        if (m->getAlbedoTexture() == nullptr || m->getAlbedoTexture()->m_name == "whiteColor") {
            albedo.SetObject();
            addJsonSceneColor(d, albedo, m->getAlbedo());
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }
        else {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getAlbedoTexture()->m_name, materialDirectory);

            albedo.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }

        /* Set normal */
        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getNormalTexture()->m_name, materialDirectory);

            Value normal;
            normal.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("normalMap", normal, d.GetAllocator());
        }

        break;
    }
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        const MaterialPBRStandard* m = dynamic_cast<const MaterialPBRStandard*>(material);

        Value type;
        type.SetString("DISNEY12");
        mat.AddMember("type", type, d.GetAllocator());

        /* Set albedo */
        Value albedo;
        if (m->getAlbedoTexture() == nullptr || m->getAlbedoTexture()->m_name == "whiteColor") {
            albedo.SetObject();
            addJsonSceneColor(d, albedo, m->getAlbedo());
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }
        else {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getAlbedoTexture()->m_name, materialDirectory);

            albedo.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }

        /* Set roughness */
        Value roughness;
        if (m->getRoughnessTexture() == nullptr || m->getRoughnessTexture()->m_name == "white") {
            roughness.SetFloat(m->getRoughness());
            mat.AddMember("roughness", roughness, d.GetAllocator());
        }
        else {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getRoughnessTexture()->m_name, materialDirectory);

            roughness.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("roughness", roughness, d.GetAllocator());
        }


        /* Set metallic */
        Value metallic;
        if (m->getMetallicTexture() == nullptr || m->getMetallicTexture()->m_name == "white") {
            metallic.SetFloat(m->getMetallic());
            mat.AddMember("metallic", metallic, d.GetAllocator());
        }
        else {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getMetallicTexture()->m_name, materialDirectory);

            metallic.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("metallic", metallic, d.GetAllocator());
        }

        /* Set normal map*/
        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getNormalTexture()->m_name, materialDirectory);

            Value normal;
            normal.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("normalMap", normal, d.GetAllocator());
        }

        break;
    }
    default:
        break;
    }

    v.PushBack(mat, d.GetAllocator());
}

void addJsonSceneColor(rapidjson::Document& d, rapidjson::Value& v, glm::vec4 color)
{
    Value r;
    r.SetFloat(color.r);
    v.AddMember("r", r, d.GetAllocator());
    Value g;
    g.SetFloat(color.g);
    v.AddMember("g", g, d.GetAllocator());
    Value b;
    b.SetFloat(color.b);
    v.AddMember("b", b, d.GetAllocator());
}

