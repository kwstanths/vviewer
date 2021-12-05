#ifndef __Cubemap_hpp__
#define __Cubemap_hpp__

#include "Image.hpp"

class Cubemap {
public:
    Cubemap();
    Cubemap(std::string directory);

    std::string m_name;

protected:
    Image<stbi_uc> * m_image_back = nullptr;
    Image<stbi_uc> * m_image_bottom = nullptr;
    Image<stbi_uc> * m_image_front = nullptr;
    Image<stbi_uc> * m_image_left = nullptr;
    Image<stbi_uc> * m_image_right = nullptr;
    Image<stbi_uc> * m_image_top = nullptr;
};

#endif