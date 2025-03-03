#include "ImageUtils.hpp"

#include <algorithm>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "debug_tools/Console.hpp"

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
                imageDataInt[i] = static_cast<unsigned char>(255.F * linearToSRGB(std::clamp(in[i], 0.0F, 1.0F)));
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

uint32_t getBitsPerPixel(const Format format)
{
    switch (format) {
        case Format::R32G32B32A32_SFLOAT:
            return 128;
        case Format::R32_SFLOAT:
            return 32;
        case Format::R8G8B8A8_UNORM:
            return 32;
        case Format::R8G8B8A8_SRGB:
            return 32;
        case Format::R8_UNORM:
        case Format::BC3_UNORM_BLOCK:
        case Format::BC3_SRGB_BLOCK:
        case Format::BC5_UNORM:
            return 8;
        case Format::BC1_RGB_UNORM_BLOCK:
        case Format::BC1_RGBA_UNORM_BLOCK:
            return 4;
        case Format::UNKNOWN:
            return 0;
        default:
            return 0;
    }
}

Image *extractChannel(const Image &image, const AssetInfo &info, uint32_t channel)
{
    Format imageFormat = image.format();
    if (imageFormat == Format::R8G8B8A8_UNORM)
    {
        const uint8_t *imageData = static_cast<const uint8_t *>(image.data());

        /* Extra requested channel */
        uint8_t *channelData = new uint8_t[image.width() * image.height()];
        uint32_t index = 0;
        for (uint32_t i = 0; i < image.height(); i++) {
            for (uint32_t j = 0; j < image.width(); j++) {
                channelData[index++] = imageData[image.width() * 4U * i + j * 4U + channel];
            }
        }
        return new Image(info, channelData, image.width(), image.height(), Format::R8_UNORM, ColorSpace::LINEAR, false, true);
    }
    else
    {
        debug_tools::ConsoleWarning("extractChannel(): Unsupported format");
    }
    return nullptr;
}

Format getFormat(DXGI_FORMAT dxgiFormat, bool hasAlpha)
{
    switch (dxgiFormat) {
        case DXGI_FORMAT_BC1_UNORM: {
            if (hasAlpha)
                return Format::BC1_RGBA_UNORM_BLOCK;
            else
                return Format::BC1_RGB_UNORM_BLOCK;
        }
        case DXGI_FORMAT_BC3_UNORM:
            return Format::BC3_UNORM_BLOCK;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return Format::BC3_SRGB_BLOCK;
        case DXGI_FORMAT_BC5_UNORM:
            return Format::BC5_UNORM;
        default:
            return Format::UNKNOWN;
    }
}

}  // namespace vengine