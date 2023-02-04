#include "SceneExport.hpp"

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

#include <qdir.h>

#include "Lights.hpp"
#include "utils/Console.hpp"

using namespace rapidjson;

std::string copyFileToDirectoryAndGetFileName(std::string file, std::string directory)
{
    /* Copy file to the directory with the same name */
    QFile sourceFile(QString::fromStdString(file));
    QString destination = QString::fromStdString(directory) + QFileInfo(sourceFile.fileName()).fileName();
    bool ret = sourceFile.copy(destination);
    if (!ret && sourceFile.errorString() != "Destination file exists")
    {
        utils::ConsoleWarning("Can't export file: " + file + " with error: " + sourceFile.errorString().toStdString());
    }
    /* Get the name of the file itself */
    QStringList destinationSplit = destination.split("/");
    QString filename = destinationSplit.back();
    return filename.toStdString();
}

void exportJson(const ExportRenderParams& renderParams, 
    std::shared_ptr<Camera> sceneCamera, 
	std::shared_ptr<DirectionalLight> sceneLight,
    const std::vector<std::shared_ptr<SceneObject>>& sceneGraph, 
    std::shared_ptr<EnvironmentMap> envMap)
{
    /* Create folder with scene */
    std::string sceneFolderName = renderParams.name + "/"; /* Make sure the final back slash exists */
    std::string meshesFolderName = sceneFolderName + "meshes/";
    std::string materialsFolderName = sceneFolderName + "materials/";
    QDir().mkdir(QString::fromStdString(sceneFolderName));
    QDir().mkdir(QString::fromStdString(meshesFolderName));
    QDir().mkdir(QString::fromStdString(materialsFolderName));

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
    {
        Value fileTypes;
        fileTypes.SetArray();
        for(auto fileType : renderParams.fileTypes)
        {
            Value temp;
            temp.SetString(fileType.c_str(), d.GetAllocator());
            fileTypes.PushBack(temp, d.GetAllocator());
        }

        d.AddMember("fileType", fileTypes, d.GetAllocator());
    }
    {
        Value resolution;
        resolution.SetObject();

        Value x;
        x.SetUint(renderParams.width);
        resolution.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetUint(renderParams.height);
        resolution.AddMember("y", y, d.GetAllocator());

        d.AddMember("resolution", resolution, d.GetAllocator());
    }
    {
        Value samples;
        samples.SetUint(renderParams.samples);
        d.AddMember("samples", samples, d.GetAllocator());
    }
    {
        Value depth;
        depth.SetUint(renderParams.depth);
        d.AddMember("depth", depth, d.GetAllocator());
    }
    {
        Value rdepth;
        rdepth.SetUint(renderParams.rdepth);
        d.AddMember("roulleteDepth", rdepth, d.GetAllocator());
    }
    {
        Value flags;
        flags.SetObject();

        Value tessellation;
        tessellation.SetDouble(renderParams.tessellation);
        flags.AddMember("tessellation", tessellation, d.GetAllocator());

        Value hideBackground;
        hideBackground.SetBool(renderParams.hideBackground);
        flags.AddMember("hideBackground", hideBackground, d.GetAllocator());

        addVec3(d, flags, "backgroundColor", renderParams.backgroundColor);

        d.AddMember("flags", flags, d.GetAllocator());
    }
    /* Camera */
    {
        Value camera;
        camera.SetObject();        

        Transform& cameraTransform = sceneCamera->getTransform();
        {

            glm::vec3 cameraPosition = cameraTransform.getPosition();
            addVec3(d, camera, "position", cameraPosition);

            glm::vec3 cameraTarget = cameraPosition + cameraTransform.getForward();
            addVec3(d, camera, "target", cameraTarget);

            glm::vec3 cameraUp = cameraTransform.getUp();
            addVec3(d, camera, "up", cameraUp);
        }
        {
            if (sceneCamera->getType() != CameraType::PERSPECTIVE){
                throw std::runtime_error("SceneExport: Only perspective camera supported");
            }
            float cameraFoV = dynamic_cast<PerspectiveCamera*>(sceneCamera.get())->getFoV();
            
            Value fov;
            fov.SetFloat(cameraFoV);
            camera.AddMember("fov", fov, d.GetAllocator());
        }
        {
            Value focalLength;
            focalLength.SetFloat(renderParams.focalLength);
            camera.AddMember("focalLength", focalLength, d.GetAllocator());
        }
        {
            Value apertureSides;
            apertureSides.SetUint(renderParams.apertureSides);
            camera.AddMember("apertureSides", apertureSides, d.GetAllocator());
        }
        {
            Value aperture;
            aperture.SetFloat(renderParams.aperture);
            camera.AddMember("aperture", aperture, d.GetAllocator());
        }

        d.AddMember("camera", camera, d.GetAllocator());
    }

    /* Parse the scene and keep here the referenced materials */
    std::unordered_set<Material*> materials;
    std::unordered_set<LightMaterial*> lightMaterials;
    {
        Value scene;
        scene.SetArray();

        for (auto& itr : sceneGraph)
        {
            parseSceneObject(d, scene, itr, materials, lightMaterials, meshesFolderName);
        }

        /* Add the directional light as an object at root */
        if (sceneLight->lightMaterial->intensity > 0.00001F)
        {
            Value sceneObjectDirectionalLight;
            sceneObjectDirectionalLight.SetObject();

            Value name;
            name.SetString("Directional light", d.GetAllocator());
            sceneObjectDirectionalLight.AddMember("name", name, d.GetAllocator());

            addTransform(d, sceneObjectDirectionalLight, sceneLight->transform);

            addAnalyticalLight(d, sceneObjectDirectionalLight, sceneLight.get(), "DISTANT");

            lightMaterials.insert(sceneLight->lightMaterial.get());

            scene.PushBack(sceneObjectDirectionalLight, d.GetAllocator());
        }

        d.AddMember("scene", scene, d.GetAllocator());
    }

    /* add mesh materials, mesh lights materials and analytical lights materials */
    {
        Value meshMaterialsObject;
        meshMaterialsObject.SetArray();

        Value lightMaterialsObject;
        lightMaterialsObject.SetArray();

        for (auto& itr : materials)
        {
            if (isMaterialEmissive(itr))
            {
                addLightMaterial(d, lightMaterialsObject, itr);
            } else {
                addMaterial(d, meshMaterialsObject, itr, materialsFolderName);
            }
        }
        for (auto& itr : lightMaterials)
        {
            addLightMaterial(d, lightMaterialsObject, itr);
        }

        d.AddMember("lights", lightMaterialsObject, d.GetAllocator());
        d.AddMember("materials", meshMaterialsObject, d.GetAllocator());
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

        addVec3(d, environment, "rotation", {0 ,0, 0});

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

void parseSceneObject(rapidjson::Document& d, 
    rapidjson::Value& v, 
    const std::shared_ptr<SceneObject>& sceneObject, 
    std::unordered_set<Material*>& materials, 
    std::unordered_set<LightMaterial*>& lightMaterials, 
    std::string meshDirectory)
{
    Value sceneObjectEntry;
    sceneObjectEntry.SetObject();

    /* name */
    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    sceneObjectEntry.AddMember("name", name, d.GetAllocator());

    addTransform(d, sceneObjectEntry, sceneObject->m_localTransform);

    bool hasMeshComponent = sceneObject->has<ComponentMesh>();
    bool hasMaterialComponent = sceneObject->has<ComponentMaterial>();
    bool hasLightComponent = sceneObject->has<ComponentPointLight>();

    if (hasMeshComponent && hasMaterialComponent)
    {
        std::string materialName = sceneObject->get<ComponentMaterial>().material->m_name;
        
        Material* mat = sceneObject->get<ComponentMaterial>().material.get();
        materials.insert(mat);
        
        bool isMatEmissive = isMaterialEmissive(sceneObject->get<ComponentMaterial>().material.get());
        if (!isMatEmissive)
        {
            addMesh(d, sceneObjectEntry, sceneObject, meshDirectory, materialName);
        } else {
            addMeshLight(d, sceneObjectEntry,sceneObject, meshDirectory, materialName);
        }
    }

    if (hasLightComponent)
    {
        Light * light = sceneObject->get<ComponentPointLight>().light.get();
        lightMaterials.insert(light->lightMaterial.get());

        addAnalyticalLight(d, sceneObjectEntry, light, "POINT");
    }

    /* Add children */
    if (sceneObject->m_children.size() != 0){
        Value children;
        children.SetArray();

        for(auto itr : sceneObject->m_children){
            parseSceneObject(d, children, itr, materials, lightMaterials, meshDirectory);
        }

        sceneObjectEntry.AddMember("children", children, d.GetAllocator());
    }

    v.PushBack(sceneObjectEntry, d.GetAllocator());
}

void addTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t)
{
    glm::vec3 transformRotation = glm::degrees(glm::eulerAngles(t.getRotation()));
    bool transformIsDefault = (
        t.getPosition().x == 0.0F && t.getPosition().y == 0.0F && t.getPosition().z == 0.0F &&
        t.getScale().x == 1.0F && t.getScale().y == 1.0F && t.getScale().z == 1.0F &&
        transformRotation.x == 0.0F && transformRotation.y == 0.0F && transformRotation.z == 0.0F
    );
    if (transformIsDefault) return;

    Value transform;
    transform.SetObject();

    addVec3(d, transform, "position", t.getPosition());
    addVec3(d, transform, "scale", t.getScale());
    addVec3(d, transform, "rotation", transformRotation);

    v.AddMember("transform", transform, d.GetAllocator());
}

void addMeshComponent(rapidjson::Document& d, 
	rapidjson::Value& v,
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::string meshDirectory,
	std::string materialName)
{
    /* Get the mesh component */
    const Mesh * mesh = sceneObject->get<ComponentMesh>().mesh.get();

    /* Copy the mesh model file to the mesh folder, and get a file path relative to the scene file */
    std::string relativePathName = "meshes/" + copyFileToDirectoryAndGetFileName(mesh->m_meshModel->getName(), meshDirectory);

    /* Set the path to the copied file */
    Value path;
    path.SetString(relativePathName.c_str(), d.GetAllocator());
    v.AddMember("path", path, d.GetAllocator());

    /* Set the submesh value */
    int submeshIndex = -1;
    /* Find the submesh index of the mesh component */
    for(int i = 0; i < mesh->m_meshModel->getMeshes().size(); i++)
    {
        if (mesh->m_meshModel->getMeshes()[i]->m_name == mesh->m_name)
        {
            submeshIndex = i;
            continue;
        }
    }
    if (submeshIndex == -1)
    {
        throw std::runtime_error("submesh is not present inside the mesh model");
    }
    Value submesh;
    submesh.SetInt(submeshIndex);
    v.AddMember("submesh", submesh, d.GetAllocator());

    Value material;
    material.SetString(materialName.c_str(), d.GetAllocator());
    v.AddMember("material", material, d.GetAllocator());
}

void addMesh(rapidjson::Document& d, 
    rapidjson::Value& v, 
    const std::shared_ptr<SceneObject>& sceneObject, 
    std::string meshDirectory,
    std::string materialName)
{
    Value meshObject;
    meshObject.SetObject();

    addMeshComponent(d, meshObject, sceneObject, meshDirectory, materialName);

    v.AddMember("mesh", meshObject, d.GetAllocator());
}

void addMeshLight(rapidjson::Document& d, 
	rapidjson::Value& v,
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::string meshDirectory,
	std::string materialName)
{
    Value lightObject;
    lightObject.SetObject();

    Value type;
    type.SetString("MESH");
    lightObject.AddMember("type", type, d.GetAllocator());

    addMeshComponent(d, lightObject, sceneObject, meshDirectory, materialName);

    v.AddMember("light", lightObject, d.GetAllocator());
}

void addAnalyticalLight(rapidjson::Document& d, rapidjson::Value& v, Light * light, std::string type)
{
    Value lightEntry;
    lightEntry.SetObject();

    Value typeEntry;
    typeEntry.SetString(type.c_str(), d.GetAllocator());
    lightEntry.AddMember("type", typeEntry, d.GetAllocator());

    Value matEntry;
    matEntry.SetString(light->lightMaterial->name.c_str(), d.GetAllocator());
    lightEntry.AddMember("material", matEntry, d.GetAllocator());

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
            addVec3(d, mat, "albedo", m->getAlbedo());
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

        Value scale;
        scale.SetArray();
        scale.PushBack(m->getUTiling(), d.GetAllocator());
        scale.PushBack(m->getVTiling(), d.GetAllocator());
        mat.AddMember("scale", scale, d.GetAllocator());

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
            addVec3(d, mat, "albedo", m->getAlbedo());
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

        Value scale;
        scale.SetArray();
        scale.PushBack(m->getUTiling(), d.GetAllocator());
        scale.PushBack(m->getVTiling(), d.GetAllocator());
        mat.AddMember("scale", scale, d.GetAllocator());

        break;
    }
    default:
        break;
    }

    v.PushBack(mat, d.GetAllocator());
}

void addLightMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material)
{
    Value light;
    light.SetObject();

    Value name;
    name.SetString(material->m_name.c_str(), d.GetAllocator());
    light.AddMember("name", name, d.GetAllocator());

    glm::vec3 color;
    float intensity;

    MaterialType type = material->getType();
    switch (type)
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        const MaterialPBRStandard* m = dynamic_cast<const MaterialPBRStandard*>(material);
        color = m->getAlbedo();
        intensity = m->getEmissive();
        break;
    }
    case MaterialType::MATERIAL_LAMBERT:
    {
        const MaterialLambert* m = dynamic_cast<const MaterialLambert*>(material);
        color = m->getAlbedo();
        intensity = m->getEmissive();
        break;
    }    
    default:
        throw std::runtime_error("isMaterialEmissive(): This should never happen");
        break;
    }

    addVec3(d, light, "color", color);
    
    Value intensityObject;
    intensityObject.SetFloat(intensity);
    light.AddMember("intensity", intensityObject, d.GetAllocator());

    v.PushBack(light, d.GetAllocator()); 
}

void addLightMaterial(rapidjson::Document& d, rapidjson::Value& v, const LightMaterial* material)
{
    Value light;
    light.SetObject();

    Value name;
    name.SetString(material->name.c_str(), d.GetAllocator());
    light.AddMember("name", name, d.GetAllocator());

    addVec3(d, light, "color", material->color);

    Value intensity;
    intensity.SetFloat(material->intensity);
    light.AddMember("intensity", intensity, d.GetAllocator());

    v.PushBack(light, d.GetAllocator());
}

void addVec3(rapidjson::Document& d, rapidjson::Value& v, std::string name, glm::vec3 value)
{
    Value array;
    array.SetArray();

    Value r;
    r.SetFloat(value.r);
    array.PushBack(r, d.GetAllocator());
    Value g;
    g.SetFloat(value.g);
    array.PushBack(g, d.GetAllocator());
    Value b;
    b.SetFloat(value.b);
    array.PushBack(b, d.GetAllocator());

    Value nameObject;
    nameObject.SetString(name.c_str(), d.GetAllocator());
    v.AddMember(nameObject, array, d.GetAllocator());
}

bool isMaterialEmissive(const Material * material)
{
    if (material == nullptr) return false;

    MaterialType type = material->getType();
    switch (type)
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        const MaterialPBRStandard* m = dynamic_cast<const MaterialPBRStandard*>(material);
        return m->getEmissive() != 0;
        break;
    }
    case MaterialType::MATERIAL_LAMBERT:
    {
        const MaterialLambert* m = dynamic_cast<const MaterialLambert*>(material);
        return m->getEmissive() != 0;
        break;
    }    
    default:
        throw std::runtime_error("isMaterialEmissive(): This should never happen");
        break;
    }
}


