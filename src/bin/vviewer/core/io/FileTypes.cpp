#include "FileTypes.hpp"

namespace vengine
{

FileType findFileType(std::string filename)
{
    auto extensionPointPosition = filename.find_last_of(".");
    auto extension = filename.substr(extensionPointPosition + 1);

    auto itr = fileTypesFromExtensions.find(extension);
    if (itr == fileTypesFromExtensions.end()) {
        return FileType::UNKNOWN;
    }
    return itr->second;
}

}  // namespace vengine