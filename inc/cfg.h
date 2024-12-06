#pragma once

#include <string>
#include <variant>
#include <unordered_map>

using CFGVarType = std::variant<bool, int>;

namespace cfg
{
    void initToDefault();

    void load();
    void saveCommon();

    extern std::unordered_map<std::string, CFGVarType> config;

#ifdef ENABLE_GD
    void saveGD();
    extern std::string driveClientID, driveClientSecret, driveAuthCode, driveRefreshToken;
#endif
}
