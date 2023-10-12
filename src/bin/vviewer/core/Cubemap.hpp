#ifndef __Cubemap_hpp__
#define __Cubemap_hpp__

#include "Asset.hpp"
#include "Image.hpp"

namespace vengine
{

class Cubemap : public Asset
{
public:
    Cubemap(std::string name);
    Cubemap(std::string name, std::string directory);
    ~Cubemap();

protected:
    Image<stbi_uc> *m_image_back = nullptr;
    Image<stbi_uc> *m_image_bottom = nullptr;
    Image<stbi_uc> *m_image_front = nullptr;
    Image<stbi_uc> *m_image_left = nullptr;
    Image<stbi_uc> *m_image_right = nullptr;
    Image<stbi_uc> *m_image_top = nullptr;
};

}  // namespace vengine

#endif