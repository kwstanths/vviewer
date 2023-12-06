#include "Cubemap.hpp"

namespace vengine
{

Cubemap::Cubemap(const AssetInfo &info)
    : Asset(info)
{
}

Cubemap::Cubemap(const AssetInfo &info, std::string directory)
    : Asset(info)
{
    m_image_front = new Image<stbi_uc>(AssetInfo(directory + "/front.png", info.source), ColorSpace::sRGB);
    m_image_back = new Image<stbi_uc>(AssetInfo(directory + "/back.png", info.source), ColorSpace::sRGB);
    m_image_top = new Image<stbi_uc>(AssetInfo(directory + "/top.png", info.source), ColorSpace::sRGB);
    m_image_bottom = new Image<stbi_uc>(AssetInfo(directory + "/bottom.png", info.source), ColorSpace::sRGB);
    m_image_right = new Image<stbi_uc>(AssetInfo(directory + "/right.png", info.source), ColorSpace::sRGB);
    m_image_left = new Image<stbi_uc>(AssetInfo(directory + "/left.png", info.source), ColorSpace::sRGB);
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
