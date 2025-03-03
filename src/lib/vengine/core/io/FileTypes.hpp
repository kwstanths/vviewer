#ifndef __FileTypes_hpp__
#define __FileTypes_hpp__

#include <string>
#include <unordered_map>

namespace vengine
{

enum class FileType {
    UNKNOWN = -1,
    PNG = 0,
    HDR = 1,
    OBJ = 2,
    GLTF = 3,
    FBX = 4,
    DDS = 5,
};

static const std::unordered_map<std::string, FileType> fileTypesFromExtensions = {{"png", FileType::PNG},
                                                                                  {"hdr", FileType::HDR},
                                                                                  {"obj", FileType::OBJ},
                                                                                  {"OBJ", FileType::OBJ},
                                                                                  {"gltf", FileType::GLTF},
                                                                                  {"gltf2", FileType::GLTF},
                                                                                  {"GLTF", FileType::GLTF},
                                                                                  {"glb", FileType::GLTF},
                                                                                  {"GLB", FileType::GLTF},
                                                                                  {"fbx", FileType::FBX},
                                                                                  {"FBX", FileType::FBX},
                                                                                  {"dds", FileType::DDS},
                                                                                  {"DDS", FileType::DDS}};

static const std::unordered_map<FileType, std::string> fileExtensionsFromTypes = {
    {FileType::PNG, "png"},
    {FileType::HDR, "hdr"},
    {FileType::OBJ, "obj"},
    {FileType::GLTF, "gltf"},
    {FileType::FBX, "fbx"},
    {FileType::DDS, "dds"},
};

FileType findFileType(std::string filename);

}  // namespace vengine

#endif