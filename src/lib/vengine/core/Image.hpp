#ifndef __Image_hpp__
#define __Image_hpp__

#include <string>
#include <stdexcept>
#include <vector>
#include <span>

#include <stb_image.h>

#include "Asset.hpp"
#include "Color.hpp"

namespace vengine
{

enum class Format {
    UNKNOWN = 0,
    R8_UNORM = 1,
    R32_SFLOAT = 2,
    R8G8B8A8_UNORM = 3,
    R8G8B8A8_SRGB = 4,
    R32G32B32A32_SFLOAT = 5,
    R16G16_SFLOAT = 6,
    BC1_RGB_UNORM_BLOCK = 7,
    BC1_RGBA_UNORM_BLOCK = 8,
    BC3_UNORM_BLOCK = 9,
    BC3_SRGB_BLOCK = 10,
    BC5_UNORM = 11,
};

class Image : public Asset
{
public:
    explicit Image(const AssetInfo &info, ColorSpace colorSpace);
    explicit Image(const AssetInfo &info,
                   void *data,
                   uint32_t width,
                   uint32_t height,
                   Format format,
                   ColorSpace colorSpace,
                   bool hasAlpha,
                   bool freeDataOnDestroy);
    explicit Image(const AssetInfo &info, Color color, ColorSpace colorSpace);
    
    ~Image();

    inline const void *data() const { return m_data; }
    
    struct MipMap {
        void *ptr;
        uint64_t byteSize;
        MipMap(void *p, uint64_t s)
            : ptr(p)
            , byteSize(s){};
    };
    inline const std::vector<MipMap> &mipmaps() const { return m_mipmaps; }

    inline uint32_t width() const { return m_width; }
    inline uint32_t height() const { return m_height; }
    
    inline Format format() const { return m_format; }

    inline ColorSpace colorSpace() const { return m_colorSpace; }

    /* Does the image have meaningful alpha values? */
    inline bool hasAlpha() const { return m_hasAlpha; }

    /* Does this image hold with DXR data? */
    inline bool isDXRData() const { return m_isDXRData; }

private:
    uint32_t m_width, m_height;
    Format m_format;
    ColorSpace m_colorSpace;
    bool m_hasAlpha;

    void *m_data;
    std::vector<MipMap> m_mipmaps;
    
    bool m_freeDataOnDestroy = true;

    /* Does this image represent data that are in the DirextX format? */
    bool m_isDXRData = false;
    
private:
    bool loadImageFromDisk(std::string filename, ColorSpace colorSpace);

    /* Calculates image size in bytes */
    uint32_t calculateImageSize();
};

}  // namespace vengine

#endif