#ifndef __ImageUtils_hpp__
#define __ImageUtils_hpp__

#include <cmath>
#include <string>
#include <vector>

#include "core/io/FileTypes.hpp"

namespace vengine
{

float linearToSRGB(const float &linear);

float sRGBToLinear(const float &srgb);

float tonemap(const float &value, const float &exposure);

void applyExposure(std::vector<float> &in, float exposure, const uint32_t channels);

void writeToDisk(const std::vector<float> &in,
                 const std::string &filename,
                 const FileType type,
                 const uint32_t width,
                 const uint32_t height,
                 const uint32_t channels);

}  // namespace vengine

#endif