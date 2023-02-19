#include "Import.hpp"

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#include <qdir.h>

#include <glm/gtx/quaternion.hpp>

#include "Console.hpp"

using namespace rapidjson;

std::string importScene(std::string filename, 
	ImportedSceneCamera& c, 
	std::unordered_map<std::string, ImportedSceneMaterial>& materials, 
	std::unordered_map<std::string, ImportedSceneLightMaterial>& lights,
	ImportedSceneEnvironment& e,
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
    c = parseCamera(doc["camera"]);

    /* Parse scene objects */
    for (size_t o = 0; o < doc["scene"].Size(); o++)
    {
        sceneObjects.push_back(parseSceneObject(doc["scene"][o]));
    }

    /* Parse light materials */
    for (size_t lm = 0; lm < doc["lights"].Size(); lm++)
    {
        auto importedLightMaterial = parseLightMaterial(doc["lights"][lm]);
        lights.insert({importedLightMaterial.name, importedLightMaterial });
    }

    /* Parse materials */
    for (size_t m = 0; m < doc["materials"].Size(); m++)
    {
        auto importedMaterial = parseMaterial(doc["materials"][m]);
        materials.insert({importedMaterial.name, importedMaterial});
    }

    /* Parse environment */
    if (doc.HasMember("environment"))
    {
        e = parseEnvironment(doc["environment"]);
    }

    return sceneFolder;
}

ImportedSceneCamera parseCamera(const rapidjson::Value& o)
{
    ImportedSceneCamera c;
    c.position = parseVec3(o, "position", c.position);

    if (o.HasMember("target"))
    {
        c.target = parseVec3(o, "target", {0, 0, -1});
        c.up = parseVec3(o, "up", {0, 1, 0});
    } else if (o.HasMember("rotation"))
    {
        glm::quat rotation = parseRotation(o, "rotation", {1, 0, 0, 0});
        glm::vec4 lookAtDirection = glm::toMat4(rotation) * glm::vec4(0, 0, -1, 0);
        c.target = c.position + glm::vec3(lookAtDirection);

        c.up = glm::vec3(glm::toMat4(rotation) * glm::vec4(0, 1, 0, 0));
    } else {
        throw std::runtime_error("parseCamera(): Can't rotation information");
    }

    if (!o.HasMember("fov"))
    {
        throw std::runtime_error("parseCamera(): Can't find FoV");
    }

    c.fov = o["fov"].GetFloat();

    return c;
}

ImportedSceneObject parseSceneObject(const rapidjson::Value& o)
{
    ImportedSceneObject object;
    object.name = o["name"].GetString();
    
    if (o.HasMember("transform")) {
        glm::vec3 position = parseVec3(o["transform"], "position", object.transform.getPosition());
        glm::vec3 scale = parseVec3(o["transform"], "scale", object.transform.getScale());
        glm::quat rotation = parseRotation(o["transform"], "rotation", object.transform.getRotation());

        object.transform = Transform(position, scale, rotation);
    } else {
        object.transform = Transform({0, 0, 0}, {1, 1, 1}, {1, 0, 0, 0});
    }

    object.mesh = parseMesh(o);
    object.quad = parseQuad(o);
    object.sphere = parseSphere(o);
    object.light = parseLight(o);

    if (o.HasMember("children")) {
        for (size_t c = 0; c < o["children"].Size(); c++)
        {
            object.children.push_back(parseSceneObject(o["children"][c]));
        }
    }

    return object;
}

std::optional<ImportedSceneObjectMesh> parseMesh(const rapidjson::Value& o)
{
    if (!o.HasMember("mesh"))
    {
        return std::nullopt;
    }

    ImportedSceneObjectMesh importedMesh;
    if (o["mesh"].HasMember("path")) {
        importedMesh.path = o["mesh"]["path"].GetString();
    } else {
        throw std::runtime_error("parseMesh(): Imported mesh doesn't have a path set");
    }

    if (o["mesh"].HasMember("submesh")){
        importedMesh.submesh = o["mesh"]["submesh"].GetInt();
    }

    if (o["mesh"].HasMember("material")){
        importedMesh.material = o["mesh"]["material"].GetString();
    } else {
        throw std::runtime_error("parseMesh(): Imported mesh doesn't have a material set");
    }

    return importedMesh;
}

std::optional<ImportedSceneObjectQuad> parseQuad(const rapidjson::Value& o)
{
    if (!o.HasMember("quad"))
    {
        return std::nullopt;
    }

    ImportedSceneObjectQuad importedQuad;

    if (o["quad"].HasMember("material")){
        importedQuad.material = o["quad"]["material"].GetString();
    } else {
        throw std::runtime_error("parseQuad(): Imported quad doesn't have a material set");
    }

    if (o["quad"].HasMember("width"))
    {
        importedQuad.width = o["quad"]["width"].GetFloat();
    }


    if (o["quad"].HasMember("height"))
    {
        importedQuad.height = o["quad"]["height"].GetFloat();
    }

    return importedQuad;
}

std::optional<ImportedSceneObjectSphere> parseSphere(const rapidjson::Value& o)
{
    if (!o.HasMember("sphere"))
    {
        return std::nullopt;
    }

    ImportedSceneObjectSphere importedSphere;

    if (o["sphere"].HasMember("material")){
        importedSphere.material = o["sphere"]["material"].GetString();
    } else {
        throw std::runtime_error("parseSphere(): Imported sphere doesn't have a material set");
    }

    if (o["sphere"].HasMember("radius"))
    {
        importedSphere.radius = o["sphere"]["radius"].GetFloat();
    }

    return importedSphere;
}

std::optional<ImportedSceneObjectLight> parseLight(const rapidjson::Value& o)
{
    if (!o.HasMember("light")) {
        return std::nullopt;  
    } 

    ImportedSceneObjectLight importedLight;
    if (o["light"].HasMember("type"))
    {
        auto type = std::string(o["light"]["type"].GetString());
        auto itr = importedLightMaterialTypeNames.find(type);
        if (itr == importedLightMaterialTypeNames.end())
        {
            throw std::runtime_error("parseLight(): " + type + " light not supported");
        }

        importedLight.type = importedLightMaterialTypeNames[type];
    }

    if (o["light"].HasMember("path"))
    {
        importedLight.path = o["light"]["path"].GetString();
    }

    if (o["light"].HasMember("submesh"))
    {
        importedLight.submesh = o["light"]["submesh"].GetInt();
    }

    if (o["light"].HasMember("material"))
    {
        importedLight.lightMaterial = o["light"]["material"].GetString();
    } else {
        throw std::runtime_error("parseLight(): light doesn't have a material");
    }

    if (o["light"].HasMember("width"))
    {
        importedLight.width = o["light"]["width"].GetFloat();
    }

    if (o["light"].HasMember("height"))
    {
        importedLight.height = o["light"]["height"].GetFloat();
    }

    if (o["light"].HasMember("radius"))
    {
        importedLight.radius = o["light"]["radius"].GetFloat();
    }

    return importedLight;
}

ImportedSceneLightMaterial parseLightMaterial(const rapidjson::Value& o)
{
    ImportedSceneLightMaterial lightMaterial;
    lightMaterial.name = o["name"].GetString();
    lightMaterial.color = parseVec3(o, "color", lightMaterial.color);

    if (o.HasMember("intensity"))
    {
        lightMaterial.intensity = o["intensity"].GetFloat();
    }

    return lightMaterial;
}

ImportedSceneMaterial parseMaterial(const rapidjson::Value& o)
{
    ImportedSceneMaterial material;
    material.name = o["name"].GetString();

    {
        auto type = std::string(o["type"].GetString());
        auto itr = importedMaterialTypeNames.find(type);
        if (itr == importedMaterialTypeNames.end())
        {
            throw std::runtime_error("parseMaterial(): " + type + " material not supported");
        }

        material.type = importedMaterialTypeNames[type];
    }

    if (material.type == ImportedSceneMaterialType::STACK)
    {
        /* Check if it's a zip file or a folder path */
        std::string path = o["path"].GetString();
        if (path.substr(path.find_last_of(".") + 1) == "zip"){
            material.stackFile = path;
        }else {
            material.stackDir = path;
        }
    }
    else if (material.type == ImportedSceneMaterialType::DISNEY) {
        if (o.HasMember("albedo"))
        {
            if (o["albedo"].IsString())
            {
                material.albedoTexture = o["albedo"].GetString();
            } else 
            {
                material.albedo = parseVec3(o, "albedo", material.albedo);
            }
        }
        if (o.HasMember("roughness")) {
            if (o["roughness"].IsString()) 
            {
                material.roughnessTexture = o["roughness"].GetString();
            } else if (o["roughness"].IsFloat()) 
            {
                material.roughness = o["roughness"].GetFloat();
            }
        }
        if (o.HasMember("metallic")) {
            if (o["metallic"].IsString()) 
            {
                material.metallicTexture = o["metallic"].GetString();
            } else if (o["metallic"].IsFloat()) 
            {
                material.metallic = o["metallic"].GetFloat();
            } 
        }
        if (o.HasMember("normalMap")) {
            material.normalTexture = o["normalMap"].GetString();
        }
    }
    else if (material.type == ImportedSceneMaterialType::DIFFUSE) 
    {
        if (o.HasMember("albedo"))
        {
            if (o["albedo"].IsString())
            {
                material.albedoTexture = o["albedo"].GetString();
            } else 
            {
                material.albedo = parseVec3(o, "albedo", material.albedo);
            }
        }
        if (o.HasMember("normalMap")) {
            material.normalTexture = o["normalMap"].GetString();
        }
    }

    if (o.HasMember("scale"))
    {
        material.scale = parseVec2(o, "scale", material.scale);
    }

    return material;
}

ImportedSceneEnvironment parseEnvironment(const rapidjson::Value& o)
{
    ImportedSceneEnvironment env;

    env.path = o["path"].GetString();

    if (o.HasMember("rotation"))
    {
        glm::quat rotation = parseRotation(o, "rotation", {1, 0, 0, 0});
        env.rotation = glm::eulerAngles(rotation);
    }

    if (o.HasMember("intensity"))
    {
        env.intensity = o["intensity"].GetFloat();
    }

    return env;
}

glm::vec2 parseVec2(const rapidjson::Value& o, std::string name, glm::vec2 defaultValue)
{
    const char * nameC = name.c_str();
    if (!o.HasMember(nameC))
    {
        return defaultValue;
    }

    glm::vec2 value;
    if (o[nameC].IsObject())
    {
        if (o[nameC].HasMember("x")) value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r")) value.x = o[nameC]["r"].GetFloat();
        else throw std::runtime_error("parseVec2(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y")) value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g")) value.y = o[nameC]["g"].GetFloat();
        else throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray())
    {
        if (o[nameC].Size() == 1) 
        {
            value = glm::vec2(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 2) 
        {
            value = glm::vec2(o[nameC][0].GetFloat(), o[nameC][1].GetFloat());
        } else
        {
            throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) 
    {
        value = glm::vec2(o[nameC].GetFloat());
    } else 
    {
        throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
    }

    return value;
}

glm::vec3 parseVec3(const rapidjson::Value& o, std::string name, glm::vec3 defaultValue)
{
    const char * nameC = name.c_str();
    if (!o.HasMember(nameC))
    {
        return defaultValue;
    }

    glm::vec3 value;
    if (o[nameC].IsObject())
    {
        if (o[nameC].HasMember("x")) value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r")) value.x = o[nameC]["r"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y")) value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g")) value.y = o[nameC]["g"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("z")) value.z = o[nameC]["z"].GetFloat();
        else if (o[nameC].HasMember("b")) value.z = o[nameC]["b"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray())
    {
        if (o[nameC].Size() == 1) 
        {
            value = glm::vec3(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 3) 
        {
            value = glm::vec3(o[nameC][0].GetFloat(), o[nameC][1].GetFloat(), o[nameC][2].GetFloat());
        } else
        {
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) 
    {
        value = glm::vec3(o[nameC].GetFloat());
    } else 
    {
        throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    }

    return value;
}

glm::vec4 parseVec4(const rapidjson::Value& o, std::string name, glm::vec4 defaultValue)
{
    const char * nameC = name.c_str();
    if (!o.HasMember(nameC))
    {
        return defaultValue;
    }

    glm::vec4 value;
    if (o[nameC].IsObject())
    {
        if (o[nameC].HasMember("x")) value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r")) value.x = o[nameC]["r"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y")) value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g")) value.y = o[nameC]["g"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("z")) value.z = o[nameC]["z"].GetFloat();
        else if (o[nameC].HasMember("b")) value.z = o[nameC]["b"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("w")) value.w = o[nameC]["w"].GetFloat();
        else if (o[nameC].HasMember("a")) value.w = o[nameC]["a"].GetFloat();
        else throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray())
    {
        if (o[nameC].Size() == 1) 
        {
            value = glm::vec4(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 4) 
        {
            value = glm::vec4(o[nameC][0].GetFloat(), o[nameC][1].GetFloat(), o[nameC][2].GetFloat(), o[nameC][3].GetFloat());
        } else
        {
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) 
    {
        value = glm::vec4(o[nameC].GetFloat());
    } else 
    {
        throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    }

    return value;
}

glm::quat parseRotation(const rapidjson::Value& o, std::string name, glm::quat defaultValue)
{
    const char * nameC = name.c_str();
    if (!o.HasMember(nameC))
    {
        return defaultValue;
    }

    /* try to parse vec3, if not, try to parse vec4 */
    glm::quat value;
    try {
        glm::vec3 vec3 = parseVec3(o, name, glm::vec3());
        value = glm::quat(glm::radians(vec3));
        return value;
    } catch(std::exception& e)
    {
        glm::vec4 vec4 = parseVec4(o, name, glm::vec4());
        value.x = vec4.x;
        value.y = vec4.y;
        value.z = vec4.z;
        value.w = vec4.w;
        return value;
    }
}
