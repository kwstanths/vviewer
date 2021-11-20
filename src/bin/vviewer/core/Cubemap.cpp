#include "Cubemap.hpp"

Cubemap::Cubemap(std::string directory)
{
    m_name = directory;

    m_image_front = new Image<stbi_uc>(directory + "/front.png");
    m_image_back = new Image<stbi_uc>(directory + "/back.png");
    m_image_top = new Image<stbi_uc>(directory + "/top.png");
    m_image_bottom = new Image<stbi_uc>(directory + "/bottom.png");
    m_image_right = new Image<stbi_uc>(directory + "/right.png");
    m_image_left = new Image<stbi_uc>(directory + "/left.png");
}
