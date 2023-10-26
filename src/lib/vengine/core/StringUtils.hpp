#ifndef __StringUtils_hpp__
#define __StringUtils_hpp__

#include <string>
#include <vector>

namespace vengine
{

void replaceAll(std::string &str, const std::string &from, const std::string &to);

std::vector<std::string> split(const std::string &in, const std::string &d);

std::string join(const std::vector<std::string> &strings, const std::string &d);

}  // namespace vengine

#endif