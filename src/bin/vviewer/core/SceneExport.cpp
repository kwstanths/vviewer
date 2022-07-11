#include "SceneExport.hpp"

#include <fstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/ostreamwrapper.h"

using namespace rapidjson;

void exportJson(std::string name, std::shared_ptr<Camera> sceneCamera, const std::vector<std::shared_ptr<SceneObject>>& sceneGraph, EnvironmentMap* envMap)
{
    std::string fileName = name + ".json";

    // 1. Parse a JSON string into DOM.
    Document d;
    d.SetObject();

    {
        Value version;
        version.SetString("0.1");
        d.AddMember("version", version, d.GetAllocator());
    }
    {
        Value name;
        name.SetString(fileName.c_str(), d.GetAllocator());
        d.AddMember("name", name, d.GetAllocator());
    }

    /* Render */
    {
        Value resolution;
        resolution.SetObject();

        Value x;
        x.SetUint(800);
        resolution.AddMember("x", x, d.GetAllocator());
        Value y;
        y.SetUint(800);
        resolution.AddMember("y", y, d.GetAllocator());

        d.AddMember("resolution", resolution, d.GetAllocator());
    }
    {
        Value samples;
        samples.SetUint(256);
        d.AddMember("samples", samples, d.GetAllocator());
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
            throw std::runtime_error("SceneExport: Only perspective camera not supported");
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
            parseSceneObject(d, scene, itr, materials);
        }

        d.AddMember("scene", scene, d.GetAllocator());
    }

    /* Materials */
    {
        Value mats;
        mats.SetArray();

        for (auto& itr : materials)
        {
            addJsonSceneMaterial(d, mats, itr);
        }

        d.AddMember("materials", mats, d.GetAllocator());
    }

    /* Environment */
    {
        Value environment;
        environment.SetObject();

        Value path;
        path.SetString(envMap->m_name.c_str(), d.GetAllocator());
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

void parseSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::unordered_set<Material*>& materials)
{
    if (sceneObject->getMesh() != nullptr) {
        addJsonSceneObject(d, v, sceneObject, sceneObject->m_parent->m_localTransform);

        Material* mat = sceneObject->getMaterial();
        materials.insert(mat);
    }

    for (auto& child : sceneObject->m_children) {
        parseSceneObject(d, v, std::dynamic_pointer_cast<SceneObject>(child), materials);
    }
}

void addJsonSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, const Transform& t)
{
    Value meshObject;
    meshObject.SetObject();

    Value name;
    name.SetString(sceneObject->m_name.c_str(), d.GetAllocator());
    meshObject.AddMember("name", name, d.GetAllocator());

    std::string meshName = sceneObject->getMesh()->m_meshModel->getName();
    Value path;
    path.SetString(meshName.c_str(), d.GetAllocator());
    meshObject.AddMember("path", path, d.GetAllocator());

    std::string materialName = sceneObject->getMaterial()->m_name;
    Value material;
    material.SetString(materialName.c_str(), d.GetAllocator());
    meshObject.AddMember("material", material, d.GetAllocator());

    addJsonSceneTransform(d, meshObject, t);

    const MaterialLambert* mat = dynamic_cast<const MaterialLambert*>(sceneObject->getMaterial());
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

void addJsonSceneMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material)
{
    Value mat;
    mat.SetObject();

    Value name;
    name.SetString(material->m_name.c_str(), d.GetAllocator());
    mat.AddMember("name", name, d.GetAllocator());

    switch (material->getType())
    {
    case MaterialType::MATERIAL_LAMBERT:
    {
        const MaterialLambert* m = dynamic_cast<const MaterialLambert*>(material);

        Value type;
        type.SetString("DIFFUSE");
        mat.AddMember("type", type, d.GetAllocator());

        Value albedo;
        albedo.SetObject();
        addJsonSceneColor(d, albedo, m->getAlbedo());
        mat.AddMember("albedo", albedo, d.GetAllocator());

        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            Value normal;
            normal.SetString(m->getNormalTexture()->m_name.c_str(), d.GetAllocator());
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

        Value albedo;
        albedo.SetString(m->getAlbedoTexture()->m_name.c_str(), d.GetAllocator());
        mat.AddMember("albedo", albedo, d.GetAllocator());

        Value roughness;
        roughness.SetFloat(m->getRoughness());
        mat.AddMember("roughness", roughness, d.GetAllocator());

        Value metallic;
        metallic.SetFloat(m->getMetallic());
        mat.AddMember("metallic", metallic, d.GetAllocator());

        Value specular;
        specular.SetFloat(0.5F);
        mat.AddMember("specular", specular, d.GetAllocator());

        if (m->getNormalTexture()->m_name != "normalmapdefault") {
            Value normal;
            normal.SetString(m->getNormalTexture()->m_name.c_str(), d.GetAllocator());
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

