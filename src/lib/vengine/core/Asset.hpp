#ifndef __Asset_hpp__
#define __Asset_hpp__

#include <string>

namespace vengine
{

class Asset
{
public:
    Asset(std::string name);
    Asset(std::string name, std::string filepath);

    const std::string &name() const;
    const std::string &filepath() const;

    const bool embedded() const;

private:
    std::string m_name;
    std::string m_filepath;
};

}  // namespace vengine

#endif
