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
    virtual Texture *createTexture(std::string imagePath, ColorSpace colorSpace = ColorSpace::sRGB, bool keepImage = false) = 0;
    virtual Texture *createTexture(Image<stbi_uc> *image) = 0;
    virtual Texture *createTextureHDR(std::string imagePath, bool keepImage = false) = 0;
};

}  // namespace vengine
#endif