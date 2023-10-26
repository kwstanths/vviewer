#include "Cubemap.hpp"

namespace vengine
{

Cubemap::Cubemap(std::string name)
    : Asset(name)
{
}

Cubemap::Cubemap(std::string name, std::string directory)
    : Asset(name)
{
    m_image_front = new Image<stbi_uc>(directory + "/front.png", ColorSpace::sRGB);
    m_image_back = new Image<stbi_uc>(directory + "/back.png", ColorSpace::sRGB);
    m_image_top = new Image<stbi_uc>(directory + "/top.png", ColorSpace::sRGB);
    m_image_bottom = new Image<stbi_uc>(directory + "/bottom.png", ColorSpace::sRGB);
    m_image_right = new Image<stbi_uc>(directory + "/right.png", ColorSpace::sRGB);
    m_image_left = new Image<stbi_uc>(directory + "/left.png", ColorSpace::sRGB);
}

Cubemap::~Cubemap()
{
    delete m_image_front;
    delete m_image_back;
    delete m_image_top;
    delete m_image_bottom;
    delete m_image_right;
    delete m_image_left;
}

}  // namespace vengine
