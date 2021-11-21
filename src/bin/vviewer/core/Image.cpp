#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <utils/Console.hpp>

template <>
stbi_uc * Image<stbi_uc>::loadDiskImage(std::string filename, int& width, int& height, int& channels) {
    stbi_uc * pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        utils::ConsoleWarning("Failed to load image from disk: " + std::string(stbi_failure_reason()));
    }

    return pixels;
}

template <>
float * Image<float>::loadDiskImage(std::string filename, int& width, int& height, int& channels) {
    stbi_set_flip_vertically_on_load(true);
    float* pixels = stbi_loadf(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        utils::ConsoleWarning("Failed to load image from disk: " + std::string(stbi_failure_reason()));
    }

    return pixels;
}
