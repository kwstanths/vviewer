#include "SceneImport.hpp"

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#include <qdir.h>

#include "Console.hpp"

using namespace rapidjson;

std::string importScene(std::string filename, 
    ImportedSceneCamera& c, 
    std::vector<ImportedSceneMaterial>& materials, 
    ImportedSceneEnvironment&e,
    std::vector<ImportedSceneObject>& sceneObjects)
{
    QStringList filenameSplit = QString::fromStdString(filename).split("/");
    filenameSplit.pop_back();
    std::string sceneFolder = filenameSplit.join("/").toStdString() + "/";

    std::ifstream ifs{ filename };
    if (!ifs.is_open())
    {
        throw std::runtime_error("Can't open file: " + filename);
    }

    IStreamWrapper isw{ ifs };

    Document doc{};
    doc.ParseStream(isw);

    StringBuffer buffer{};
    Writer<StringBuffer> writer{ buffer };
    doc.Accept(writer);

    if (doc.HasParseError())
    {
        throw std::runtime_error("Malformed scene file: " + filename);
    }

    /* Parse camera */
    c.position = glm::vec3(
        doc["camera"]["position"]["x"].GetFloat(), 
        doc["camera"]["position"]["y"].GetFloat(), 
        doc["camera"]["position"]["z"].GetFloat()
    );
    c.target = glm::vec3(
        doc["camera"]["target"]["x"].GetFloat(),
        doc["camera"]["target"]["y"].GetFloat(),
        doc["camera"]["target"]["z"].GetFloat()
    );
    c.up = glm::vec3(
        doc["camera"]["up"]["x"].GetFloat(),
        doc["camera"]["up"]["y"].GetFloat(),
        doc["camera"]["up"]["z"].GetFloat()
    );
    c.fov = doc["camera"]["fov"].GetFloat();

    /* Parse materials */
    materials = std::vector<ImportedSceneMaterial>(doc["materials"].Size());
    for (size_t m = 0; m < doc["materials"].Size(); m++)
    {
        materials[m].name = doc["materials"][m]["name"].GetString();
        materials[m].type = importedMaterialTypeNames[std::string(doc["materials"][m]["type"].GetString())];
        if (materials[m].type == ImportedSceneMaterialType::STACK)
        {
            materials[m].stackDir = doc["materials"][m]["path"].GetString();
        }
        else if (materials[m].type == ImportedSceneMaterialType::DISNEY) {
            parseAlbedo(materials[m], doc["materials"][m]);

            if (doc["materials"][m].HasMember("roughness")) {
                if (doc["materials"][m]["roughness"].IsFloat()) {
                    materials[m].roughnessValue = doc["materials"][m]["roughness"].GetFloat();
                }
                else if (doc["materials"][m]["roughness"].IsString()) {
                    materials[m].roughnessTexture = doc["materials"][m]["roughness"].GetString();
                }
            }

            if (doc["materials"][m].HasMember("metallic")) {
                if (doc["materials"][m]["metallic"].IsFloat()) {
                    materials[m].metallicValue = doc["materials"][m]["metallic"].GetFloat();
                }
                else if (doc["materials"][m]["metallic"].IsString()) {
                    materials[m].metallicTexture = doc["materials"][m]["metallic"].GetString();
                }
            }

            if (doc["materials"][m].HasMember("normalMap")) {
                materials[m].normalTexture = doc["materials"][m]["normalMap"].GetString();
            }

            if (doc["materials"][m].HasMember("emissive")) {
                materials[m].emissiveValue = doc["materials"][m]["emissive"].GetFloat();
            }
        }
        else if (materials[m].type == ImportedSceneMaterialType::DIFFUSE) {
            parseAlbedo(materials[m], doc["materials"][m]);
        }
    }

    /* Parse scene objects */
    for (size_t o = 0; o < doc["scene"].Size(); o++)
    {
        sceneObjects.push_back(parseSceneObject(doc["scene"][o]));
    }

    /* Parse environment */
    if (doc.HasMember("environment"))
    {
        e.path = doc["environment"]["path"].GetString();
    }

    return sceneFolder;
}

ImportedSceneObject parseSceneObject(const rapidjson::Value& o)
{
    ImportedSceneObject object;
    object.name = o["name"].GetString();
    
    object.mesh = parseMesh(o);
    
    if (o.HasMember("material")) {
        object.material = o["material"].GetString();
    }

    object.light = parseLight(o);

    if (o.HasMember("transform")) {
        if (o["transform"].HasMember("position")) {
            object.position = glm::vec3(
                o["transform"]["position"]["x"].GetFloat(),
                o["transform"]["position"]["y"].GetFloat(),
                o["transform"]["position"]["z"].GetFloat()
            );
        }
        if (o["transform"].HasMember("scale")) {
            object.scale = glm::vec3(
                o["transform"]["scale"]["x"].GetFloat(),
                o["transform"]["scale"]["y"].GetFloat(),
                o["transform"]["scale"]["z"].GetFloat()
            );
        }
        if (o["transform"].HasMember("rotation")) {            
            object.rotation = glm::vec3(
                o["transform"]["rotation"]["x"].GetFloat(),
                o["transform"]["rotation"]["y"].GetFloat(),
                o["transform"]["rotation"]["z"].GetFloat()
            );
        }
    }

    if (o.HasMember("children")) {
        for (size_t c = 0; c < o["children"].Size(); c++)
        {
            object.children.push_back(parseSceneObject(o["children"][c]));
        }
    }

    return object;
}

ImportedSceneObjectMesh * parseMesh(const rapidjson::Value& o)
{
    ImportedSceneObjectMesh * mesh = nullptr;
    if (o.HasMember("path")) {
        mesh = new ImportedSceneObjectMesh();
        mesh->path = o["path"].GetString();
    } else return mesh;

    if (o.HasMember("submesh")){
        mesh->submesh = o["submesh"].GetInt();
    }

    return mesh;
}

ImportedSceneObjectLight * parseLight(const rapidjson::Value& o)
{
    ImportedSceneObjectLight * light = nullptr;
    if (o.HasMember("light")) {
        light = new ImportedSceneObjectLight();
    } else return nullptr;

    std::string lightType = o["light"]["type"].GetString();
    if (lightType == "POINT") light->type = ImportedScenePointLightType::POINT;
    else if (lightType == "DISTANT") light->type = ImportedScenePointLightType::DISTANT;
    else utils::ConsoleCritical("Scene importing: Unsupported light type: " + lightType);

    if (o["light"].HasMember("intensity")){
        light->intensity = o["light"]["intensity"].GetFloat();
    }

    if (o["light"].HasMember("color")) {
        light->color = glm::vec3(
            o["light"]["color"]["r"].GetFloat(), 
            o["light"]["color"]["r"].GetFloat(), 
            o["light"]["color"]["b"].GetFloat()
        );
    }

    return light;
}

void parseAlbedo(ImportedSceneMaterial& m, const rapidjson::Value& o)
{
    if (o.HasMember("albedo")) {
        if (o["albedo"].IsObject()) {
            m.albedoValue = glm::vec3(
                o["albedo"]["r"].GetFloat(),
                o["albedo"]["g"].GetFloat(),
                o["albedo"]["b"].GetFloat()
            );
        }
        else if (o["albedo"].IsFloat()) {
            m.albedoValue = glm::vec3(o["albedo"].GetFloat());
        }
        else if (o["albedo"].IsString()) {
            m.albedoTexture = o["albedo"].GetString();
        }
    }
}