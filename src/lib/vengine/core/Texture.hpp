#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

#include "Asset.hpp"
#include "Color.hpp"

namespace vengine
{

class Texture : public Asset
{
public:
    Texture(const AssetInfo &info, ColorSpace colorSpace, ColorDepth colorDepth, size_t width, size_t height, size_t channels)
        : Asset(info)
        , m_colorSpace(colorSpace)
        , m_colorDepth(colorDepth)
        , m_width(width)
        , m_height(height)
        , m_channels(channels){};

    const uint32_t width() const { return m_width; }
    const uint32_t height() const { return m_height; }
    const uint32_t channels() const { return m_channels; }
    const ColorSpace colorSpace() const { return m_colorSpace; }
    const ColorDepth colorDepth() const { return m_colorDepth; }

private:
    ColorSpace m_colorSpace;
    ColorDepth m_colorDepth;
    size_t m_width;
    size_t m_height;
    size_t m_channels;
};

}  // namespace vengine

#endif