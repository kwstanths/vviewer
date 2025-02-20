#include "Asset.hpp"

#include <iostream>

namespace vengine
{

AssetInfo::AssetInfo(const std::string &n, const AssetSource &s)
    : name(n)
    , filepath(n)
    , source(s)
    , location(AssetLocation::DISK_STANDALONE)
{
}

AssetInfo::AssetInfo(const std::string &n, const std::string &f, const AssetSource &s, const AssetLocation &l)
    : name(n)
    , filepath(f)
    , source(s)
    , location(l)
{
}

Asset::Asset(const AssetInfo &info)
    : m_info(info)
{
}

const std::string &Asset::name() const
{
    return m_info.name;
}

const std::string &Asset::filepath() const
{
    return m_info.filepath;
}

const AssetSource &Asset::source() const
{
    return m_info.source;
}

const AssetLocation &Asset::location() const
{
    return m_info.location;
}

const AssetInfo &Asset::info() const
{
    return m_info;
}

const bool Asset::isInternal() const
{
    return m_info.source == AssetSource::ENGINE;
}

const bool Asset::isEmbedded() const
{
    return m_info.location == AssetLocation::DISK_EMBEDDED;
}

}  // namespace vengine