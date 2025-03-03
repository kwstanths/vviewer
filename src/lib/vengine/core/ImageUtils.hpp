#ifndef __ImageUtils_hpp__
#define __ImageUtils_hpp__

#include <cmath>
#include <string>
#include <vector>

#include "Image.hpp"
#include "core/io/FileTypes.hpp"

#include "dds_formats.hpp"

namespace vengine
{

/**
    Convert a linear value to sRGB space
*/
float linearToSRGB(const float &linear);

/**
    Convert an sRGB value to linear
*/
float sRGBToLinear(const float &srgb);

/**
    Apply tonemapping to a value
*/
float tonemap(const float &value, const float &exposure);

/**
    Apply tonemapping to a vector of values
*/
void applyExposure(std::vector<float> &in, float exposure, const uint32_t channels);

/**
    Write an image to the disk
*/
void writeToDisk(const std::vector<float> &in,
                 const std::string &filename,
                 const FileType type,
                 const uint32_t width,
                 const uint32_t height,
                 const uint32_t channels);

/**
    Get bits per pixel for a certain format
*/
uint32_t getBitsPerPixel(const Format format);

/**
    Extract a channel from an image
*/
Image *extractChannel(const Image &image, const AssetInfo &info, uint32_t channel);

/**
    Get the format from a dxgi format
*/
Format getFormat(DXGI_FORMAT dxgiFormat, bool hasAlpha);

}  // namespace vengine

#endif