#ifndef __Texture_hpp__
#define __Texture_hpp__

#include <string>

#include "Asset.hpp"
#include "Color.hpp"
#include "Image.hpp"

namespace vengine
{

class Texture : public Asset
{
public:
    Texture(const AssetInfo &info, size_t width, size_t height, Format format, ColorSpace colorSpace, bool isDXR = false)
        : Asset(info)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_colorSpace(colorSpace)
        , m_isDXR(isDXR)
    {};

    uint32_t width() const { return static_cast<uint32_t>(m_width); }
    uint32_t height() const { return static_cast<uint32_t>(m_height); }
    Format format() const { return m_format; }
    ColorSpace colorSpace() const { return m_colorSpace; }
    bool isDXR() const { return m_isDXR; }

private:
    size_t m_width;
    size_t m_height;
    Format m_format;
    ColorSpace m_colorSpace;
    bool m_isDXR;
};

}  // namespace vengine

#endif