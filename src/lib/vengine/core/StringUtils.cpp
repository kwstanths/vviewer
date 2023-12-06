#include "StringUtils.hpp"

namespace vengine
{

void replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::vector<std::string> split(const std::string &in, const std::string &d)
{
    std::vector<std::string> out;

    size_t last = 0;
    size_t next = 0;
    while ((next = in.find(d, last)) != std::string::npos) {
        out.push_back(in.substr(last, next - last));
        last = next + 1;
    }
    out.push_back(in.substr(last));
    return out;
}

std::string join(const std::vector<std::string> &strings, const std::string &d)
{
    std::string out;
    for (auto s : strings) {
        out += s + d;
    }
    return out;
}

std::string getFilename(std::string file)
{
    auto fileSplit = split(file, "/");
    return fileSplit.back();
}

}  // namespace vengine