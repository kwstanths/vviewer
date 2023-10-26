#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vengine
{

template <>
stbi_uc *Image<stbi_uc>::loadDiskImage(std::string filename, int &width, int &height, int &channels)
{
    /* Check if it's a single channel image and load it as is, otherwise force it to be RGBA for better compatibility */
    int ok = stbi_info(filename.c_str(), &width, &height, &channels);
    if (!ok)
        return nullptr;

    if (channels == 1) {
        stbi_uc *pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_default);
        return pixels;
    } else {
        stbi_uc *pixels = stbi_load(filename.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        channels = 4;
        return pixels;
    }
}

template <>
float *Image<float>::loadDiskImage(std::string filename, int &width, int &height, int &channels)
{
    /* Check if it's a single channel image and load it as is, otherwise force it to be RGBA for better compatibility */
    int ok = stbi_info(filename.c_str(), &width, &height, &channels);
    if (!ok)
        return nullptr;

    if (channels == 1) {
        float *pixels = stbi_loadf(filename.c_str(), &width, &height, &channels, STBI_default);
        return pixels;
    } else {
        stbi_set_flip_vertically_on_load(true);
        float *pixels = stbi_loadf(filename.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        channels = 4;
        return pixels;
    }
}

template <>
ColorDepth Image<stbi_uc>::colorDepth() const
{
    return ColorDepth::BITS8;
}

template <>
ColorDepth Image<float>::colorDepth() const
{
    return ColorDepth::BITS32;
}

}  // namespace vengine