#include "SceneExport.hpp"

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include "Lights.hpp"

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
	std::shared_ptr<DirectionalLight> sceneLight,
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

        if (sceneCamera->getType() != CameraType::PERSPECTIVE){
            throw std::runtime_error("SceneExport: Only perspective camera supported");
        }
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

        /* Add the directional light as a root object */
        if (sceneLight->intensity > 0.00001F)
        {
            Value sceneObjectDirectionalLight;
            sceneObjectDirectionalLight.SetObject();

            Value name;
            name.SetString("Directional light", d.GetAllocator());
            sceneObjectDirectionalLight.AddMember("name", name, d.GetAllocator());

            addSceneObjectLight(d, sceneObjectDirectionalLight, sceneLight.get(), "DISTANT");
            addSceneObjectTransform(d, sceneObjectDirectionalLight, sceneLight->transform);

            scene.PushBack(sceneObjectDirectionalLight, d.GetAllocator());
        }

        d.AddMember("scene", scene, d.GetAllocator());
    }

    /* Materials */
    {
        Value mats;
        mats.SetArray();

        for (auto& itr : materials)
        {
            addMaterial(d, mats, itr, sceneMaterialsFolderName);
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
    Value sceneObjectEntry;
    sceneObjectEntry.SetObject();

    /* Add name of scene object */
    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    sceneObjectEntry.AddMember("name", name, d.GetAllocator());

    /* Check if it has a mesh, and add it */
    if (sceneObject->has(ComponentType::MESH)) {
        addSceneObjectMesh(d, sceneObjectEntry, sceneObject, meshDirectory);
    }

    /* Check if it has a material and add it */
    if (sceneObject->has(ComponentType::MATERIAL))
    {
        std::string materialName = sceneObject->get<Material*>(ComponentType::MATERIAL)->m_name;
        Value material;
        material.SetString(materialName.c_str(), d.GetAllocator());
        sceneObjectEntry.AddMember("material", material, d.GetAllocator());

        /* Also add it in the materials map to insert it later in the material list */
        Material* mat = sceneObject->get<Material*>(ComponentType::MATERIAL);
        materials.insert(mat);
    }

    /* Check if it has a point light */
    if (sceneObject->has(ComponentType::POINT_LIGHT))
    {
        addSceneObjectLight(d, sceneObjectEntry, sceneObject->get<PointLight*>(ComponentType::POINT_LIGHT), "POINT");
    }

    /* Add transform */
    addSceneObjectTransform(d, sceneObjectEntry, sceneObject->m_localTransform);

    /* Add children */
    if (sceneObject->m_children.size() != 0){
        Value children;
        children.SetArray();

        for(auto itr : sceneObject->m_children){
            parseSceneObject(d, children, itr, materials, meshDirectory);
        }

        sceneObjectEntry.AddMember("children", children, d.GetAllocator());
    }

    v.PushBack(sceneObjectEntry, d.GetAllocator());
}

void addSceneObjectMesh(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::string meshDirectory)
{
    /* Get the mesh component */
    const Mesh * mesh = sceneObject->get<Mesh*>(ComponentType::MESH);

    /* Copy the mesh model file to the mesh folder, and get a file path relative to the scene file */
    std::string relativePathName = "meshes/" + copyFileToDirectoryAndGetFileName(mesh->m_meshModel->getName(), meshDirectory);

    /* Set the path to the copied file */
    Value path;
    path.SetString(relativePathName.c_str(), d.GetAllocator());
    v.AddMember("path", path, d.GetAllocator());

    /* Set the submesh value */
    int submeshIndex = -1;
    /* Find the submesh index of the mesh component */
    for(int i=0; i < mesh->m_meshModel->getMeshes().size(); i++)
    {
        if (mesh->m_meshModel->getMeshes()[i]->m_name == mesh->m_name)
        {
            submeshIndex = i;
            continue;
        }
    }
    if (submeshIndex == -1)
    {
        throw std::runtime_error("submesh is not present inside parent mesh model");
    }
    Value submesh;
    submesh.SetInt(submeshIndex);
    v.AddMember("submesh", submesh, d.GetAllocator());
}

void addSceneObjectTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t)
{
    Value transform;
    transform.SetObject();

    glm::vec3 transformRotation = glm::eulerAngles(t.getRotation());
    bool transformIsDefault = (
        t.getPosition().x == 0.0F && t.getPosition().y == 0.0F && t.getPosition().z == 0.0F &&
        t.getScale().x == 1.0F && t.getScale().y == 1.0F && t.getScale().z == 1.0F &&
        transformRotation.x == 0.0F && transformRotation.y == 0.0F && transformRotation.z == 0.0F
    );
    if (transformIsDefault) return;

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
        Value rotation;
        rotation.SetObject();

        Value x;
        x.SetFloat(glm::degrees(transformRotation.x));
        rotation.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetFloat(glm::degrees(transformRotation.y));
        rotation.AddMember("y", y, d.GetAllocator());
        Value z;
        z.SetFloat(glm::degrees(transformRotation.z));
        rotation.AddMember("z", z, d.GetAllocator());

        transform.AddMember("rotation", rotation, d.GetAllocator());
    }

    v.AddMember("transform", transform, d.GetAllocator());
}

void addSceneObjectLight(rapidjson::Document& d, rapidjson::Value& v, Light * light, std::string type)
{
    Value lightEntry;
    lightEntry.SetObject();

    Value typeEntry;
    typeEntry.SetString(type.c_str(), d.GetAllocator());
    lightEntry.AddMember("type", typeEntry, d.GetAllocator());

    Value intensity;
    intensity.SetFloat(light->intensity);
    lightEntry.AddMember("intensity", intensity, d.GetAllocator());

    Value color;
    color.SetObject();
    addColor(d, color, light->color);
    lightEntry.AddMember("color", color, d.GetAllocator());

    v.AddMember("light", lightEntry, d.GetAllocator());
}

void addMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material, std::string materialsDirectory)
{
    Value mat;
    mat.SetObject();

    Value name;
    name.SetString(material->m_name.c_str(), d.GetAllocator());
    mat.AddMember("name", name, d.GetAllocator());
    /* Create a directory for textures for that material */
    std::string materialDirectory = materialsDirectory + material->m_name + "/";
    auto createMatDirectory = [&materialDirectory]() {
        QDir().mkdir(QString::fromStdString(materialDirectory));
    };
    bool directoryCreated = false;

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
            addColor(d, albedo, glm::vec3(m->getAlbedo()));
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }
        else {
            if (!directoryCreated) createMatDirectory();

            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getAlbedoTexture()->m_name, materialDirectory);

            albedo.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("albedo", albedo, d.GetAllocator());
            }

        /* Set normal */
        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            if (!directoryCreated) createMatDirectory();
            
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getNormalTexture()->m_name, materialDirectory);

            Value normal;
            normal.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("normalMap", normal, d.GetAllocator());
        }

        /* Set emissive */
        Value emissive;
        if (m->getEmissiveTexture() == nullptr || m->getEmissiveTexture()->m_name == "white") {
            emissive.SetFloat(m->getEmissive());
            mat.AddMember("emissive", emissive, d.GetAllocator());
        }
        else {
            if (!directoryCreated) createMatDirectory();
            
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getEmissiveTexture()->m_name, materialDirectory);

            emissive.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("emissive", emissive, d.GetAllocator());
        }

        break;
    }
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        const MaterialPBRStandard* m = dynamic_cast<const MaterialPBRStandard*>(material);

        /* If it's a zip stack material */
        if (m->m_path != "")
        {
            Value type;
            type.SetString("STACK");
            mat.AddMember("type", type, d.GetAllocator());

            std::string relativePath = "materials/" + copyFileToDirectoryAndGetFileName(m->m_path, materialsDirectory);

            Value path;
            path.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("path", path, d.GetAllocator());
            break;
        } 

        Value type;
        type.SetString("DISNEY12");
        mat.AddMember("type", type, d.GetAllocator());

        /* Set albedo */
        Value albedo;
        if (m->getAlbedoTexture() == nullptr || m->getAlbedoTexture()->m_name == "whiteColor") {
            albedo.SetObject();
            addColor(d, albedo, glm::vec3(m->getAlbedo()));
            mat.AddMember("albedo", albedo, d.GetAllocator());
        }
        else {
            if (!directoryCreated) createMatDirectory();
            
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
            if (!directoryCreated) createMatDirectory();

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
            if (!directoryCreated) createMatDirectory();

            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getMetallicTexture()->m_name, materialDirectory);

            metallic.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("metallic", metallic, d.GetAllocator());
        }

        /* Set normal map*/
        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            if (!directoryCreated) createMatDirectory();

            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getNormalTexture()->m_name, materialDirectory);

            Value normal;
            normal.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("normalMap", normal, d.GetAllocator());
        }

        /* Set emissive */
        Value emissive;
        if (m->getEmissiveTexture() == nullptr || m->getEmissiveTexture()->m_name == "white") {
            emissive.SetFloat(m->getEmissive());
            mat.AddMember("emissive", emissive, d.GetAllocator());
        }
        else {
            if (!directoryCreated) createMatDirectory();
            
            std::string relativePath = "materials/" + material->m_name + "/" + copyFileToDirectoryAndGetFileName(m->getEmissiveTexture()->m_name, materialDirectory);

            emissive.SetString(relativePath.c_str(), d.GetAllocator());
            mat.AddMember("emissive", emissive, d.GetAllocator());
        }

        break;
    }
    default:
        break;
    }

    v.PushBack(mat, d.GetAllocator());
}

void addColor(rapidjson::Document& d, rapidjson::Value& v, glm::vec3 color)
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

