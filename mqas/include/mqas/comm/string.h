#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <mqas/macro.h>
namespace mqas::comm
{
    std::vector<std::string_view> MQAS_EXTERN split(const std::string_view sv, char delimiter);
}
