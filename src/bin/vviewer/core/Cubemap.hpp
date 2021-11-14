#ifndef __Cubemap_hpp__
#define __Cubemap_hpp__

#include "Image.hpp"

class Cubemap {
public:
    Cubemap(std::string directory);

    std::string m_name;

protected:
    Image * m_image_back = nullptr;
    Image * m_image_bottom = nullptr;
    Image * m_image_front = nullptr;
    Image * m_image_left = nullptr;
    Image * m_image_right = nullptr;
    Image * m_image_top = nullptr;
};

#endif