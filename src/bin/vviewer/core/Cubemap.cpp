#include "Cubemap.hpp"

Cubemap::Cubemap(std::string directory)
{
    m_name = directory;

    m_image_front = new Image(directory + "/front.png");
    m_image_back = new Image(directory + "/back.png");
    m_image_top = new Image(directory + "/top.png");
    m_image_bottom = new Image(directory + "/bottom.png");
    m_image_right = new Image(directory + "/right.png");
    m_image_left = new Image(directory + "/left.png");
}
