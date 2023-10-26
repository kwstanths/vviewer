#include "Asset.hpp"

namespace vengine
{

Asset::Asset(std::string name)
    : m_name(name)
    , m_filepath(name)
{
}

Asset::Asset(std::string name, std::string filepath)
    : m_name(name)
    , m_filepath(filepath)
{
}

const std::string &Asset::name() const
{
    return m_name;
}

const std::string &Asset::filepath() const
{
    return m_filepath;
}

const bool Asset::embedded() const
{
    return m_name != m_filepath;
}

}  // namespace vengine