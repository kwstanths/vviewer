#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(std::string filename) {
    m_data = loadDiskImage(filename, m_width, m_height, m_channels);
    if (m_data == nullptr) throw std::runtime_error("File not found: " + filename);
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
