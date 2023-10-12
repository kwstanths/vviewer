#ifndef __Image_hpp__
#define __Image_hpp__

#include <string>
#include <stdexcept>

#include <stb_image.h>

#include "Asset.hpp"
#include "Color.hpp"

namespace vengine
{

template <typename T>
class Image : public Asset
{
public:
    Image(std::string filename, ColorSpace colorSpace)
        : Asset(filename)
        , m_colorSpace(colorSpace)
    {
        m_data = loadDiskImage(filename, m_width, m_height, m_channels);
        if (m_data == nullptr) {
            throw std::runtime_error("Unable to load image from disk: " + filename);
        }
    }

    Image(std::string name,
          std::string filename,
          T *data,
          int width,
          int height,
          int channels,
          ColorSpace colorSpace,
          bool freeDataOnDestroy)
        : Asset(name, filename)
        , m_colorSpace(colorSpace)
        , m_data(data)
        , m_width(width)
        , m_height(height)
        , m_channels(channels)
        , m_freeDataOnDestroy(freeDataOnDestroy)
    {
    }

    Image(std::string name, Color color, ColorSpace colorSpace)
        : Asset(name)
        , m_colorSpace(colorSpace)
    {
        m_width = 1;
        m_height = 1;
        m_channels = 4;

        m_data = new T[4];
        switch (color) {
            case Color::BLACK:
                m_data[0] = 0x00;
                m_data[1] = 0x00;
                m_data[2] = 0x00;
                m_data[3] = 0xFF;
                break;
            case Color::WHITE:
                m_data[0] = 0xFF;
                m_data[1] = 0xFF;
                m_data[2] = 0xFF;
                m_data[3] = 0xFF;
                break;
            case Color::RED:
                m_data[0] = 0xFF;
                m_data[1] = 0x00;
                m_data[2] = 0x00;
                m_data[3] = 0xFF;
                break;
            case Color::GREEN:
                m_data[0] = 0x00;
                m_data[1] = 0xFF;
                m_data[2] = 0x00;
                m_data[3] = 0xFF;
                break;
            case Color::BLUE:
                m_data[0] = 0x00;
                m_data[1] = 0x00;
                m_data[2] = 0xFF;
                m_data[3] = 0xFF;
                break;
            case Color::NORMAL_MAP:
                m_data[0] = 0x80;
                m_data[1] = 0x80;
                m_data[2] = 0xFF;
                m_data[3] = 0xFF;
                break;
            default:
                break;
        }
    }
    ~Image()
    {
        if (m_freeDataOnDestroy) {
            stbi_image_free(m_data);
        }
    }

    T *data() { return m_data; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int channels() const { return m_channels; }
    ColorSpace colorSpace() const { return m_colorSpace; }
    ColorDepth colorDepth() const;
    bool hasTransparency() const { return m_channels >= 4; }

    static T *loadDiskImage(std::string filename, int &width, int &height, int &channels);

private:
    int m_width, m_height, m_channels;
    ColorSpace m_colorSpace;
    T *m_data;
    bool m_freeDataOnDestroy = true;
};

template <>
stbi_uc *Image<stbi_uc>::loadDiskImage(std::string filename, int &width, int &height, int &channels);
template <>
float *Image<float>::loadDiskImage(std::string filename, int &width, int &height, int &channels);

template <>
ColorDepth Image<stbi_uc>::colorDepth() const;
template <>
ColorDepth Image<float>::colorDepth() const;

}  // namespace vengine

#endif