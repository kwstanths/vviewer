#ifndef __Image_hpp__
#define __Image_hpp__

#include <string>
#include <stdexcept>

#include <stb_image.h>

template<typename T>
class Image {
public:
    enum class Color {
        BLACK = 0,
        WHITE = 1,
        RED = 2,
        GREEN = 3,
        BLUE = 4,
        NORMAL_MAP = 5,
    };

    Image(std::string filename) {
        m_data = loadDiskImage(filename, m_width, m_height, m_channels);
        if (m_data == nullptr) throw std::runtime_error("Unable to load image from disk: " + filename);
    }

    Image(T * data, int width, int height, int channels, bool freeDataOnDestroy) 
    {
        m_data = data;
        m_width = width;
        m_height = height;
        m_channels = channels;
        m_freeDataOnDestroy = freeDataOnDestroy;
    }

    Image(Color color) {
        m_width = 1;
        m_height = 1;
        m_channels = 4;

        m_data = new T[4];
        switch (color)
        {
        case Image::Color::BLACK:
            m_data[0] = 0x00;
            m_data[1] = 0x00;
            m_data[2] = 0x00;
            m_data[3] = 0xFF;
            break;
        case Image::Color::WHITE:
            m_data[0] = 0xFF;
            m_data[1] = 0xFF;
            m_data[2] = 0xFF;
            m_data[3] = 0xFF;
            break;
        case Image::Color::RED:
            m_data[0] = 0xFF;
            m_data[1] = 0x00;
            m_data[2] = 0x00;
            m_data[3] = 0xFF;
            break;
        case Image::Color::GREEN:
            m_data[0] = 0x00;
            m_data[1] = 0xFF;
            m_data[2] = 0x00;
            m_data[3] = 0xFF;
            break;
        case Image::Color::BLUE:
            m_data[0] = 0x00;
            m_data[1] = 0x00;
            m_data[2] = 0xFF;
            m_data[3] = 0xFF;
            break;
        case Image::Color::NORMAL_MAP:
            m_data[0] = 0x80;
            m_data[1] = 0x80;
            m_data[2] = 0xFF;
            m_data[3] = 0xFF;
            break;
        default:
            break;
        }
    }
    ~Image() {
        if (m_freeDataOnDestroy) 
        {
            stbi_image_free(m_data);
        }
    }

    T * getData() {
        return m_data;
    }
    int getWidth() {
        return m_width;
    }
    int getHeight() {
        return m_height;
    }
    int getChannels() {
        return 4;
    }
    bool hasTransparency() {
        return m_channels >= 4;
    }

    static T * loadDiskImage(std::string filename, int& width, int& height, int& channels);

private:
    int m_width, m_height, m_channels;
    T * m_data;
    bool m_freeDataOnDestroy = true;
};

template<> stbi_uc * Image<stbi_uc>::loadDiskImage(std::string filename, int& width, int& height, int& channels);
template<> float * Image<float>::loadDiskImage(std::string filename, int& width, int& height, int& channels);

#endif