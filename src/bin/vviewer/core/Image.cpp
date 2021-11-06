#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(std::string filename) {
    m_data = loadDiskImage(filename, m_width, m_height, m_channels);
    if (m_data == nullptr) throw std::runtime_error("File not found: " + filename);
}

Image::Image(Color color)
{
    m_width = 1;
    m_height = 1;
    m_channels = 4;

    m_data = new stbi_uc[4];
    switch (color)
    {
    case Image::Color::BLACK:
        m_data[0] = 0x00;
        m_data[1] = 0x00;
        m_data[2] = 0x00;
        m_data[3] = 0xFF;
        break;
    case Image::Color::WHITE:
        m_data[0] = 0xFF;
        m_data[1] = 0xFF;
        m_data[2] = 0xFF;
        m_data[3] = 0xFF;
        break;
    case Image::Color::RED:
        m_data[0] = 0xFF;
        m_data[1] = 0x00;
        m_data[2] = 0x00;
        m_data[3] = 0xFF;
        break;
    case Image::Color::GREEN:
        m_data[0] = 0x00;
        m_data[1] = 0xFF;
        m_data[2] = 0x00;
        m_data[3] = 0xFF;
        break;
    case Image::Color::BLUE:
        m_data[0] = 0x00;
        m_data[1] = 0x00;
        m_data[2] = 0xFF;
        m_data[3] = 0xFF;
        break;
    default:
        break;
    }
}

Image::~Image()
{
    stbi_image_free(m_data);
}

stbi_uc * Image::getData()
{
    return m_data;
}

int Image::getWidth()
{
    return m_width;
}

int Image::getHeight()
{
    return m_height;
}

int Image::getChannels()
{
    /* Image is always read as STBI_rgb_alpha */
    return 4;
}

bool Image::hasTransparency()
{
    return m_channels >= 4;
}

stbi_uc * Image::loadDiskImage(std::string filename, int& width, int& height, int& channels)
{
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        return nullptr;
    }

    return pixels;
}
