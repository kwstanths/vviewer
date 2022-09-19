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
        if (materials[m].type == STACK)
        {
            materials[m].stackDir = doc["materials"][m]["path"].GetString();
        }
        else if (materials[m].type == DISNEY) {
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

        }
        else if (materials[m].type == DIFFUSE) {
            parseAlbedo(materials[m], doc["materials"][m]);
        }
    }

    /* Parse scene objects */
    sceneObjects = std::vector<ImportedSceneObject>(doc["scene"].Size());
    for (size_t o = 0; o < doc["scene"].Size(); o++)
    {
        sceneObjects[o].name = doc["scene"][o]["name"].GetString();
        sceneObjects[o].path = doc["scene"][o]["path"].GetString();
        sceneObjects[o].material = doc["scene"][o]["material"].GetString();

        if (doc["scene"][o].HasMember("transform")) {
            sceneObjects[o].position = glm::vec3(
                doc["scene"][o]["transform"]["position"]["x"].GetFloat(),
                doc["scene"][o]["transform"]["position"]["y"].GetFloat(),
                doc["scene"][o]["transform"]["position"]["z"].GetFloat()
            );
            sceneObjects[o].scale = glm::vec3(
                doc["scene"][o]["transform"]["scale"]["x"].GetFloat(),
                doc["scene"][o]["transform"]["scale"]["y"].GetFloat(),
                doc["scene"][o]["transform"]["scale"]["z"].GetFloat()
            );
            sceneObjects[o].rotation = glm::vec3(
                doc["scene"][o]["transform"]["rotation"]["x"].GetFloat(),
                doc["scene"][o]["transform"]["rotation"]["y"].GetFloat(),
                doc["scene"][o]["transform"]["rotation"]["z"].GetFloat()
            );
        }
    }

    /* Parse environment */
    if (doc.HasMember("environment"))
    {
        e.path = doc["environment"]["path"].GetString();
    }

    return sceneFolder;
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
