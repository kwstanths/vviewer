#ifndef __Textures_hpp__
#define __Textures_hpp__

#include <memory>

#include "Image.hpp"
#include "Texture.hpp"

namespace vengine
{

class Textures
{
public:
    virtual Texture *createTexture(const AssetInfo &info, ColorSpace colorSpace = ColorSpace::sRGB) = 0;
    virtual Texture *createTexture(const Image<stbi_uc> &image) = 0;
    virtual Texture *createTextureHDR(const AssetInfo &info) = 0;
};

}  // namespace vengine
#endif