#include "Import.hpp"

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#include <glm/gtx/quaternion.hpp>

#include "Console.hpp"
#include "core/StringUtils.hpp"

using namespace rapidjson;

namespace vengine
{

static std::unordered_map<std::string, ImportedMaterialType> importedMaterialTypeNames = {
    {"LAMBERT", ImportedMaterialType::LAMBERT},
    {"PBR_STANDARD", ImportedMaterialType::PBR_STANDARD},
    {"STACK", ImportedMaterialType::STACK},
    {"EMBEDDED", ImportedMaterialType::EMBEDDED}};
static std::unordered_map<std::string, LightType> importedLightTypeNames = {
    {"POINT", LightType::POINT_LIGHT},
    {"DIRECTIONAL", LightType::DIRECTIONAL_LIGHT},
};
static std::unordered_map<std::string, AssetLocation> importedTextureTypeNames = {
    {"STANDALONE", AssetLocation::STANDALONE},
    {"EMBEDDED", AssetLocation::EMBEDDED},
};

float parseFloat(const rapidjson::Value &o, std::string name, float defaultValue)
{
    const char *nameC = name.c_str();
    if (!o.HasMember(nameC)) {
        return defaultValue;
    }

    return o[nameC].GetFloat();
}

glm::vec2 parseVec2(const rapidjson::Value &o, std::string name, glm::vec2 defaultValue)
{
    const char *nameC = name.c_str();
    if (!o.HasMember(nameC)) {
        return defaultValue;
    }

    glm::vec2 value;
    if (o[nameC].IsObject()) {
        if (o[nameC].HasMember("x"))
            value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r"))
            value.x = o[nameC]["r"].GetFloat();
        else
            throw std::runtime_error("parseVec2(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y"))
            value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g"))
            value.y = o[nameC]["g"].GetFloat();
        else
            throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray()) {
        if (o[nameC].Size() == 1) {
            value = glm::vec2(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 2) {
            value = glm::vec2(o[nameC][0].GetFloat(), o[nameC][1].GetFloat());
        } else {
            throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) {
        value = glm::vec2(o[nameC].GetFloat());
    } else {
        throw std::runtime_error("parseVec2(): Field " + name + " is malformed");
    }

    return value;
}

glm::vec3 parseVec3(const rapidjson::Value &o, std::string name, glm::vec3 defaultValue)
{
    const char *nameC = name.c_str();
    if (!o.HasMember(nameC)) {
        return defaultValue;
    }

    glm::vec3 value;
    if (o[nameC].IsObject()) {
        if (o[nameC].HasMember("x"))
            value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r"))
            value.x = o[nameC]["r"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y"))
            value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g"))
            value.y = o[nameC]["g"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("z"))
            value.z = o[nameC]["z"].GetFloat();
        else if (o[nameC].HasMember("b"))
            value.z = o[nameC]["b"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray()) {
        if (o[nameC].Size() == 1) {
            value = glm::vec3(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 3) {
            value = glm::vec3(o[nameC][0].GetFloat(), o[nameC][1].GetFloat(), o[nameC][2].GetFloat());
        } else {
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) {
        value = glm::vec3(o[nameC].GetFloat());
    } else {
        throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    }

    return value;
}

glm::vec4 parseVec4(const rapidjson::Value &o, std::string name, glm::vec4 defaultValue)
{
    const char *nameC = name.c_str();
    if (!o.HasMember(nameC)) {
        return defaultValue;
    }

    glm::vec4 value;
    if (o[nameC].IsObject()) {
        if (o[nameC].HasMember("x"))
            value.x = o[nameC]["x"].GetFloat();
        else if (o[nameC].HasMember("r"))
            value.x = o[nameC]["r"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("y"))
            value.y = o[nameC]["y"].GetFloat();
        else if (o[nameC].HasMember("g"))
            value.y = o[nameC]["g"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("z"))
            value.z = o[nameC]["z"].GetFloat();
        else if (o[nameC].HasMember("b"))
            value.z = o[nameC]["b"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");

        if (o[nameC].HasMember("w"))
            value.w = o[nameC]["w"].GetFloat();
        else if (o[nameC].HasMember("a"))
            value.w = o[nameC]["a"].GetFloat();
        else
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    } else if (o[nameC].IsArray()) {
        if (o[nameC].Size() == 1) {
            value = glm::vec4(o[nameC][0].GetFloat());
        } else if (o[nameC].Size() == 4) {
            value = glm::vec4(o[nameC][0].GetFloat(), o[nameC][1].GetFloat(), o[nameC][2].GetFloat(), o[nameC][3].GetFloat());
        } else {
            throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
        }
    } else if (o[nameC].IsFloat()) {
        value = glm::vec4(o[nameC].GetFloat());
    } else {
        throw std::runtime_error("parseVec3(): Field " + name + " is malformed");
    }

    return value;
}

glm::quat parseRotation(const rapidjson::Value &o, std::string name, glm::quat defaultValue)
{
    const char *nameC = name.c_str();
    if (!o.HasMember(nameC)) {
        return defaultValue;
    }

    /* try to parse vec3, if not, try to parse vec4 */
    glm::quat value;
    try {
        glm::vec3 vec3 = parseVec3(o, name, glm::vec3());
        value = glm::quat(glm::radians(vec3));
        return value;
    } catch (std::exception &e) {
        glm::vec4 vec4 = parseVec4(o, name, glm::vec4());
        value.x = vec4.x;
        value.y = vec4.y;
        value.z = vec4.z;
        value.w = vec4.w;
        return value;
    }
}

ImportedCamera parseCamera(const rapidjson::Value &o)
{
    ImportedCamera c;
    c.position = parseVec3(o, "position", c.position);

    if (o.HasMember("target")) {
        c.target = parseVec3(o, "target", c.target);
        c.up = parseVec3(o, "up", c.up);
    } else if (o.HasMember("rotation")) {
        glm::quat rotation = parseRotation(o, "rotation", {1, 0, 0, 0});
        glm::vec4 lookAtDirection = glm::toMat4(rotation) * glm::vec4(0, 0, -1, 0);
        c.target = c.position + glm::vec3(lookAtDirection);

        c.up = glm::vec3(glm::toMat4(rotation) * glm::vec4(0, 1, 0, 0));
    } else {
        throw std::runtime_error("parseCamera(): Missing target or rotation information");
    }

    if (!o.HasMember("fov")) {
        throw std::runtime_error("parseCamera(): Can't find fov");
    }
    c.fov = parseFloat(o, "fov", c.fov);

    return c;
}

std::optional<ImportedSceneObjectMesh> parseMeshComponent(const rapidjson::Value &o)
{
    if (!o.HasMember("mesh")) {
        return std::nullopt;
    }

    ImportedSceneObjectMesh importedMesh;
    if (o["mesh"].HasMember("modelName")) {
        importedMesh.modelName = o["mesh"]["modelName"].GetString();
    } else {
        throw std::runtime_error("parseMeshComponent(): Imported mesh doesn't have a modelName set");
    }

    if (o["mesh"].HasMember("submesh")) {
        importedMesh.submesh = o["mesh"]["submesh"].GetString();
    } else {
        throw std::runtime_error("parseMeshComponent(): Imported mesh doesn't have a submesh set");
    }

    return importedMesh;
}

std::optional<ImportedSceneObjectMaterial> parseMaterialComponent(const rapidjson::Value &o)
{
    if (!o.HasMember("material")) {
        return std::nullopt;
    }

    ImportedSceneObjectMaterial importedMaterial;
    if (o["material"].HasMember("name")) {
        importedMaterial.name = o["material"]["name"].GetString();
    } else {
        throw std::runtime_error("parseMaterialComponent(): Imported material doesn't have a name");
    }

    return importedMaterial;
}

std::optional<ImportedSceneObjectLight> parseLightComponent(const rapidjson::Value &o)
{
    if (!o.HasMember("light")) {
        return std::nullopt;
    }

    ImportedSceneObjectLight importedLight;
    if (o["light"].HasMember("name")) {
        importedLight.name = o["light"]["name"].GetString();
    } else {
        throw std::runtime_error("parseLightComponent(): Imported light doesn't have a name");
    }

    return importedLight;
}

Tree<ImportedSceneObject> parseSceneObject(const rapidjson::Value &o)
{
    ImportedSceneObject object;
    object.name = o["name"].GetString();

    if (o.HasMember("transform")) {
        glm::vec3 position = parseVec3(o["transform"], "position", object.transform.position());
        glm::vec3 scale = parseVec3(o["transform"], "scale", object.transform.scale());
        glm::quat rotation = parseRotation(o["transform"], "rotation", object.transform.rotation());

        object.transform = Transform(position, scale, rotation);
    } else {
        object.transform = Transform({0, 0, 0}, {1, 1, 1}, {1, 0, 0, 0});
    }

    object.mesh = parseMeshComponent(o);
    object.material = parseMaterialComponent(o);
    object.light = parseLightComponent(o);

    Tree<ImportedSceneObject> root;
    root.data() = object;

    if (o.HasMember("children")) {
        for (size_t c = 0; c < o["children"].Size(); c++) {
            root.add(parseSceneObject(o["children"][c]));
        }
    }

    return root;
}

ImportedModel parseModel(const rapidjson::Value &o, const std::string &relativePath)
{
    ImportedModel importedModel;

    if (o.HasMember("name")) {
        importedModel.name = o["name"].GetString();
    } else {
        throw std::runtime_error("parseModel(): Imported model doesn't have a name set");
    }

    if (o.HasMember("filepath")) {
        importedModel.filepath = relativePath + o["filepath"].GetString();
    } else {
        throw std::runtime_error("parseModel(): Imported mesh doesn't have a filepath set");
    }

    return importedModel;
}

std::optional<ImportedTexture> parseTexture(const rapidjson::Value &o, const std::string &relativePath, ColorSpace colorSpace)
{
    if (!o.HasMember("texture")) {
        return std::nullopt;
    }

    ImportedTexture value;
    value.location = importedTextureTypeNames[o["texture"]["type"].GetString()];
    value.name = o["texture"]["name"].GetString();

    if (value.location == AssetLocation::STANDALONE && o["texture"].HasMember("filepath")) {
        std::string filepath = o["texture"]["filepath"].GetString();
        value.image = new Image<uint8_t>(AssetInfo(relativePath + filepath), colorSpace);
    }
    return value;
}

void parseMaterial(const rapidjson::Value &o, const std::string &relativePath, ImportedMaterial &material)
{
    std::string name = o["name"].GetString();

    {
        auto type = std::string(o["type"].GetString());
        auto itr = importedMaterialTypeNames.find(type);
        if (itr == importedMaterialTypeNames.end()) {
            throw std::runtime_error("parseMaterial(): " + type + " material not supported");
        }

        material.type = importedMaterialTypeNames[type];
    }

    if (material.type == ImportedMaterialType::EMBEDDED) {
        material.info = AssetInfo(name);
        return;
    }

    if (material.type == ImportedMaterialType::STACK) {
        std::string filepath = relativePath + o["path"].GetString();
        material.info = AssetInfo(name, filepath, AssetSource::IMPORTED, AssetLocation::STANDALONE);
        return;
    }

    material.info = AssetInfo(name);
    if (material.type == ImportedMaterialType::PBR_STANDARD) {
        if (o.HasMember("roughness")) {
            material.roughnessTexture = parseTexture(o["roughness"], relativePath, ColorSpace::LINEAR);
            material.roughness = parseFloat(o["roughness"], "value", material.roughness);
        }
        if (o.HasMember("metallic")) {
            material.metallicTexture = parseTexture(o["metallic"], relativePath, ColorSpace::LINEAR);
            material.metallic = parseFloat(o["metallic"], "value", material.metallic);
        }
    }

    if (o.HasMember("albedo")) {
        material.albedoTexture = parseTexture(o["albedo"], relativePath, ColorSpace::sRGB);
        material.albedo = parseVec4(o["albedo"], "value", material.albedo);
    }
    if (o.HasMember("ao")) {
        material.aoTexture = parseTexture(o["ao"], relativePath, ColorSpace::LINEAR);
        material.ao = parseFloat(o["ao"], "value", material.ao);
    }
    if (o.HasMember("emissive")) {
        material.emissiveTexture = parseTexture(o["emissive"], relativePath, ColorSpace::sRGB);
        glm::vec4 emissiveValue = parseVec4(o["emissive"], "value", glm::vec4(material.emissiveColor, material.emissiveStrength));
        material.emissiveColor = glm::vec3(emissiveValue.r, emissiveValue.g, emissiveValue.b);
        material.emissiveStrength = emissiveValue.a;
    }
    if (o.HasMember("normal")) {
        material.normalTexture = parseTexture(o["normal"], relativePath, ColorSpace::LINEAR);
    }
    if (o.HasMember("alpha")) {
        material.alphaTexture = parseTexture(o["alpha"], relativePath, ColorSpace::LINEAR);
    }
    if (o.HasMember("transparent")) {
        material.transparent = o["transparent"].GetBool();
    }

    if (o.HasMember("scale")) {
        material.scale = parseVec2(o, "scale", material.scale);
    }

    return;
}  // namespace vengine

ImportedLight parseLight(const rapidjson::Value &o)
{
    ImportedLight light;
    light.name = o["name"].GetString();

    if (o.HasMember("type")) {
        auto type = std::string(o["type"].GetString());
        auto itr = importedLightTypeNames.find(type);
        if (itr == importedLightTypeNames.end()) {
            throw std::runtime_error("parseLight(): " + type + " light not supported");
        }

        light.type = itr->second;
    }

    light.color = parseVec3(o, "color", light.color);

    if (o.HasMember("intensity")) {
        light.intensity = o["intensity"].GetFloat();
    }

    return light;
}

ImportedEnvironment parseEnvironment(const rapidjson::Value &o)
{
    ImportedEnvironment env;

    if (o.HasMember("path")) {
        env.path = o["path"].GetString();
    }

    env.environmentType = static_cast<EnvironmentType>(o["environmentType"].GetInt());
    env.backgroundColor = parseVec3(o, "backgroundColor", env.backgroundColor);

    return env;
}

std::string importScene(std::string filename,
                        ImportedCamera &c,
                        Tree<ImportedSceneObject> &sceneObjects,
                        std::vector<ImportedModel> &models,
                        std::vector<ImportedMaterial> &materials,
                        std::vector<ImportedLight> &lights,
                        ImportedEnvironment &e)
{
    auto filenameSplit = split(filename, "/");
    filenameSplit.pop_back();
    std::string sceneFolder = join(filenameSplit, "/");

    std::ifstream ifs{filename};
    if (!ifs.is_open()) {
        throw std::runtime_error("Can't open file: " + filename);
    }

    IStreamWrapper isw{ifs};

    Document doc{};
    doc.ParseStream(isw);

    StringBuffer buffer{};
    Writer<StringBuffer> writer{buffer};
    doc.Accept(writer);

    if (doc.HasParseError()) {
        throw std::runtime_error("Malformed scene file: " + filename);
    }

    /* Parse camera */
    c = parseCamera(doc["camera"]);

    /* Parse scene objects */
    sceneObjects.data().name = "root";
    for (size_t o = 0; o < doc["scene"].Size(); o++) {
        sceneObjects.add(parseSceneObject(doc["scene"][o]));
    }

    /* Parse scene models */
    for (size_t o = 0; o < doc["models"].Size(); o++) {
        models.push_back(parseModel(doc["models"][o], sceneFolder));
    }

    /* Parse materials */
    materials.resize(doc["materials"].Size());
    for (size_t m = 0; m < doc["materials"].Size(); m++) {
        parseMaterial(doc["materials"][m], sceneFolder, materials[m]);
    }

    /* Parse lights */
    for (size_t lm = 0; lm < doc["lights"].Size(); lm++) {
        lights.push_back(parseLight(doc["lights"][lm]));
    }

    /* Parse environment */
    if (doc.HasMember("environment")) {
        e = parseEnvironment(doc["environment"]);
    }

    return sceneFolder;
}

}  // namespace vengine