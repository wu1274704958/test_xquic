#include <mqas/comm/string.h>

std::vector<std::string_view> mqas::comm::split(const std::string_view sv, char delimiter)
{
    std::vector<std::string_view> tokens;
    size_t start = 0, end;
    while ((end = sv.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(sv.substr(start, end - start));
        start = end + 1;
    }
    if(start < sv.size())
		tokens.push_back(sv.substr(start));
    return tokens;
}

bool mqas::comm::erase_substr(std::string& src, std::string_view sub)
{
	auto p = src.find(sub.data());
    if (p != std::string::npos) {
        src.replace(p, sub.size(), "");
        return true;
    }
    return false;
}

