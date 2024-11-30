#ifndef __Asset_hpp__
#define __Asset_hpp__

#include <string>

namespace vengine
{

enum class AssetSource { IMPORTED = 0, ENGINE = 1 };
enum class AssetLocation { UNDEFINED = -1, DISK_STANDALONE = 0, DISK_EMBEDDED = 1 };

struct AssetInfo {
    std::string name;
    std::string filepath;
    AssetSource source;
    AssetLocation location;

    /**
     * @brief Construct a new Asset Info object
     *
     * @param n Name and filename of the asset
     * @param s
     */
    explicit AssetInfo(const std::string &n, const AssetSource &s = AssetSource::IMPORTED);

    /**
     * @brief Construct a new Asset Info object
     *
     * @param n Name of the asset
     * @param f Filename of the asset
     * @param s
     * @param l
     */
    explicit AssetInfo(const std::string &n,
                       const std::string &f,
                       const AssetSource &s = AssetSource::IMPORTED,
                       const AssetLocation &l = AssetLocation::DISK_STANDALONE);
};

class Asset
{
public:
    Asset(const AssetInfo &info);

    const std::string &name() const;
    const std::string &filepath() const;
    const AssetSource &source() const;
    const AssetLocation &location() const;

    const AssetInfo &info() const;

    const bool isInternal() const;
    const bool isEmbedded() const;

private:
    AssetInfo m_info;
};

}  // namespace vengine

#endif
