#pragma once

#include <string>
#include <unordered_map>

namespace cfg
{
    void initToDefault();

    void load();
    void saveCommon();

    extern std::unordered_map<std::string, bool> config;

#ifdef ENABLE_GD
    void saveGD();
    extern std::string driveClientID, driveClientSecret, driveAuthCode, driveRefreshToken;
#endif
}
