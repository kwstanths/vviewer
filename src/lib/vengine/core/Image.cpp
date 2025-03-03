#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "dds.hpp"

#include "io/FileTypes.hpp"
#include "io/FileTypes.hpp"
#include "ImageUtils.hpp"

#include "Console.hpp"

namespace vengine
{

Image::Image(const AssetInfo &info, ColorSpace colorSpace)
    : Asset(info)
    , m_colorSpace(colorSpace)
    , m_hasAlpha(false)
{
    bool ret = loadImageFromDisk(info.filepath, colorSpace);
    if (!ret)
        throw std::runtime_error("Image::Image(): Failed to open image from disk: " + info.filepath);
}

Image::Image(const AssetInfo &info, void *data, uint32_t width, uint32_t height, Format format, ColorSpace colorSpace, bool hasAlpha, bool freeDataOnDestroy)
    : Asset(info)
    , m_format(format)
    , m_colorSpace(colorSpace)
    , m_hasAlpha(hasAlpha)
    , m_data(data)
    , m_width(width)
    , m_height(height)
    , m_freeDataOnDestroy(freeDataOnDestroy)
{
    m_mipmaps.emplace_back(m_data, calculateImageSize());
}

Image::Image(const AssetInfo &info, Color color, ColorSpace colorSpace)
    : Asset(info)
    , m_colorSpace(colorSpace)
    , m_hasAlpha(false)
{
    m_width = 1;
    m_height = 1;

    m_data = new uint8_t[4];
    switch (color) {
        case Color::BLACK:
            static_cast<uint8_t *>(m_data)[0] = 0x00;
            static_cast<uint8_t *>(m_data)[1] = 0x00;
            static_cast<uint8_t *>(m_data)[2] = 0x00;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        case Color::WHITE:
            static_cast<uint8_t *>(m_data)[0] = 0xFF;
            static_cast<uint8_t *>(m_data)[1] = 0xFF;
            static_cast<uint8_t *>(m_data)[2] = 0xFF;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        case Color::RED:
            static_cast<uint8_t *>(m_data)[0] = 0xFF;
            static_cast<uint8_t *>(m_data)[1] = 0x00;
            static_cast<uint8_t *>(m_data)[2] = 0x00;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        case Color::GREEN:
            static_cast<uint8_t *>(m_data)[0] = 0x00;
            static_cast<uint8_t *>(m_data)[1] = 0xFF;
            static_cast<uint8_t *>(m_data)[2] = 0x00;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        case Color::BLUE:
            static_cast<uint8_t *>(m_data)[0] = 0x00;
            static_cast<uint8_t *>(m_data)[1] = 0x00;
            static_cast<uint8_t *>(m_data)[2] = 0xFF;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        case Color::NORMAL_MAP:
            static_cast<uint8_t *>(m_data)[0] = 0x80;
            static_cast<uint8_t *>(m_data)[1] = 0x80;
            static_cast<uint8_t *>(m_data)[2] = 0xFF;
            static_cast<uint8_t *>(m_data)[3] = 0xFF;
            break;
        default:
            break;
    }

    if (colorSpace == ColorSpace::sRGB)
        m_format = Format::R8G8B8A8_SRGB;
    else if (colorSpace == ColorSpace::LINEAR)
        m_format = Format::R8G8B8A8_UNORM;

    m_mipmaps.emplace_back(m_data, calculateImageSize());
}

Image::~Image()
{
    if (m_freeDataOnDestroy) {
        stbi_image_free(m_data);
    }
}

bool Image::loadImageFromDisk(std::string filename, ColorSpace colorSpace)
{
    FileType fileType = findFileType(filename);
    if (fileType == FileType::DDS) 
    {
        debug_tools::ConsoleWarning("Importing .dds file, UVs might be flipped");
        m_isDXRData = true;

        dds::Image image;
        dds::ReadResult ret = readFile(filename, &image);
        if (ret == dds::ReadResult::Success) 
        {
            m_width = image.width;
            m_height = image.height;

            uint8_t *dataTemp = new uint8_t[image.data.size()];
            memcpy(dataTemp, image.data.data(), image.data.size());

            m_format = getFormat(image.format, image.supportsAlpha);
            assert(m_format != Format::UNKNOWN);

            m_hasAlpha = image.supportsAlpha;

            // Set mipmaps 
            for (const auto& mipmap : image.mipmaps)
            {
                uint8_t *mipmapdataptr = dataTemp + (mipmap.data() - image.data.data());
                size_t mipmapdataSize = mipmap.size();
                m_mipmaps.emplace_back(mipmapdataptr, mipmapdataSize);
            }

            m_data = dataTemp;
            return true;
        }
        return false;
    } 
    else if (fileType != FileType::HDR)
    {
        /* Check if it's a single channel image and load it as is, otherwise force it to be RGBA for better compatibility */
        int channels, width, height;
        int ok = stbi_info(filename.c_str(), &width, &height, &channels);
        if (!ok)
            return false;

        if (width < 0 || height < 0)
            return false;

        m_hasAlpha = false;
        if (channels >= 4)
            m_hasAlpha = true;

        if (channels == 1) {
            m_data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_default);
            m_width = static_cast<uint32_t>(width);
            m_height = static_cast<uint32_t>(height);
            m_format = Format::R8_UNORM; 
            m_mipmaps.emplace_back(m_data, calculateImageSize());
            return m_data != nullptr;
        } 
        else 
        {
            m_data = stbi_load(filename.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
            if (colorSpace == ColorSpace::sRGB)
                m_format = Format::R8G8B8A8_SRGB;
            else if (colorSpace == ColorSpace::LINEAR)
                m_format = Format::R8G8B8A8_UNORM;
            m_width = static_cast<uint32_t>(width);
            m_height = static_cast<uint32_t>(height);
            m_mipmaps.emplace_back(m_data, calculateImageSize());
            return m_data != nullptr;
        }
    } 
    else 
    {
        /* Check if it's a single channel image and load it as is, otherwise force it to be RGBA for better compatibility */
        int channels, width, height;
        int ok = stbi_info(filename.c_str(), &width, &height, &channels);
        if (!ok)
            return false;
        
        if (width < 0 || height < 0)
            return false;

        m_hasAlpha = false;
        if (channels >= 4)
            m_hasAlpha = true;

        if (channels == 1)
        {
            m_data = stbi_loadf(filename.c_str(), &width, &height, &channels, STBI_default);
            m_width = static_cast<uint32_t>(width);
            m_height = static_cast<uint32_t>(height);
            m_format = Format::R32_SFLOAT;
            m_mipmaps.emplace_back(m_data, calculateImageSize());
            return m_data != nullptr;
        } 
        else 
        {
            stbi_set_flip_vertically_on_load(true);
            m_data = stbi_loadf(filename.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
            m_width = static_cast<uint32_t>(width);
            m_height = static_cast<uint32_t>(height);
            m_format = Format::R32G32B32A32_SFLOAT;
            m_mipmaps.emplace_back(m_data, calculateImageSize());
            return m_data != nullptr;
        }
    }
}

uint32_t Image::calculateImageSize()
{
    uint32_t bitsPerPixel = getBitsPerPixel(m_format);
    assert(bitsPerPixel != 0);
    float bytesPerPixel = (static_cast<float>(bitsPerPixel) / 8U);
    return static_cast<uint32_t>(m_width * m_height * bytesPerPixel);
}

}  // namespace vengine