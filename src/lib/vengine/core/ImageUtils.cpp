#include "ImageUtils.hpp"

#include <algorithm>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace vengine
{

float linearToSRGB(const float &linear)
{
    float srgb;
    if (linear <= 0.0031308f) {
        srgb = linear * 12.92f;
    } else {
        srgb = 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }
    return srgb;
}

float sRGBToLinear(const float &srgb)
{
    auto linear = srgb;
    if (linear <= 0.04045f) {
        linear = linear / 12.92f;
    } else {
        linear = std::pow((linear + 0.055f) / 1.055f, 2.4f);
    }
    return linear;
}

float tonemap(const float &value, const float &exposure)
{
    return value * std::pow(2.0F, exposure);
}

void applyExposure(std::vector<float> &in, float exposure, const uint32_t channels)
{
    /* Ignore 4th channel */
    uint32_t step = std::min(channels, 3U);

    for (size_t i = 0; i < in.size(); i += channels) {
        for (size_t c = 0; c < step; c++) {
            in[i + c] = tonemap(in[i + c], exposure);
        }
    }
}

void writeToDisk(const std::vector<float> &in,
                 const std::string &filename,
                 const FileType type,
                 const uint32_t width,
                 const uint32_t height,
                 const uint32_t channels)
{
    switch (type) {
        case FileType::PNG: {
            std::vector<unsigned char> imageDataInt(width * height * channels, 255);
            for (size_t i = 0; i < width * height * channels; i++) {
                imageDataInt[i] = 255.F * linearToSRGB(std::clamp(in[i], 0.0F, 1.0F));
            }
            stbi_write_png((filename + ".png").c_str(), width, height, channels, imageDataInt.data(), width * channels * sizeof(char));
            break;
        }
        case FileType::HDR: {
            stbi_write_hdr((filename + ".hdr").c_str(), width, height, channels, in.data());
            break;
        }
        default:
            throw std::runtime_error("writeToDisk(): Unsupported file type");
            break;
    }
}

}  // namespace vengine