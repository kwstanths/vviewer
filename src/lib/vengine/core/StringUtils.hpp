#ifndef __StringUtils_hpp__
#define __StringUtils_hpp__

#include <string>
#include <vector>

namespace vengine
{

/**
 * @brief Replaces all instances of 'from' inside 'str' to 'to
 *
 * @param str
 * @param from
 * @param to
 */
void replaceAll(std::string &str, const std::string &from, const std::string &to);

/**
 * @brief Splits 'in' using delimiter 'd'
 *
 * @param in
 * @param d
 * @return std::vector<std::string>
 */
std::vector<std::string> split(const std::string &in, const std::string &d);

/**
 * @brief Joins the strings using delimiter 'd'
 *
 * @param strings
 * @param d
 * @return std::string
 */
std::string join(const std::vector<std::string> &strings, const std::string &d);

}  // namespace vengine

#endif