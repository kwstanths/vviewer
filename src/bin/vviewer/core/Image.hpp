#ifndef __Image_hpp__
#define __Image_hpp__

#include <string>

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
        if (m_data == nullptr) throw std::runtime_error("File not found: " + filename);
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
        stbi_image_free(data);
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

    T * loadDiskImage(std::string filename, int& width, int& height, int& channels);

private:
    int m_width, m_height, m_channels;
    T * m_data;
};

template<> stbi_uc * Image<stbi_uc>::loadDiskImage(std::string filename, int& width, int& height, int& channels);
template<> float * Image<float>::loadDiskImage(std::string filename, int& width, int& height, int& channels);

#endif