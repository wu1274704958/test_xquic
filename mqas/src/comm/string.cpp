#include <mqas/comm/string.h>

std::vector<std::string_view> mqas::comm::split(const std::string_view sv, char delimiter)
{
    std::vector<std::string_view> tokens;
    size_t start = 0, end = 0;
    while ((end = sv.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(sv.substr(start, end - start));
        start = end + 1;
    }
    if(start < sv.size())
		tokens.push_back(sv.substr(start));
    return tokens;
}
