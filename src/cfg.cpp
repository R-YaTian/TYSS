/*
 *  This file is part of TYSS.
 *  Copyright (C) 2024-2025 R-YaTian
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <string>
#include <unordered_map>

#include "cfg.h"
#include "fs.h"

#ifdef ENABLE_DRIVE
#include "json.h"
std::string cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken, cfg::driveDiskID;
bool cfg::driveInitOnBoot = true;
#endif

std::unordered_map<std::string, CFGVarType> cfg::config;

void cfg::initToDefault()
{
    cfg::config["zip"] = false;
    cfg::config["deflateLevel"] = 1;
    cfg::config["lightback"] = false;
    cfg::config["rawvcsave"] = false;
    cfg::config["bootwithcheatdb"] = false;
    cfg::config["swaplrfunc"] = false;
}

void cfg::load()
{
    FILE *cfgIn = fopen("/TYSS/cfg.bin", "rb");
    if(cfgIn)
    {
        bool getBool = false;
        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["zip"] = getBool;

        int getInt = 0;
        fread(&getInt, sizeof(int), 1, cfgIn);
        cfg::config["deflateLevel"] = getInt;

        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["lightback"] = getBool;

        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["rawvcsave"] = getBool;

        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["bootwithcheatdb"] = getBool;

        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["swaplrfunc"] = getBool;

        fclose(cfgIn);
    }

#ifdef ENABLE_DRIVE
    fs::fsfile drvCfg(fs::getSDMCArch(), "/TYSS/drive.json", FS_OPEN_READ);
    if(drvCfg.isOpen())
    {
        u64 size = drvCfg.getSize();
        char *jsonBuff = new char[size + 1];
        drvCfg.read(jsonBuff, size);
        jsonBuff[size] = '\0';

        nlohmann::json parse = nlohmann::json::parse(jsonBuff);

        if(parse.contains("driveClientID"))
            driveClientID = parse["driveClientID"];

        if(parse.contains("driveClientSecret"))
            driveClientSecret = parse["driveClientSecret"];

        if(parse.contains("driveAuthCode"))
            driveAuthCode = parse["driveAuthCode"];

        if(parse.contains("driveRefreshToken"))
            driveRefreshToken = parse["driveRefreshToken"];

        if(parse.contains("driveDiskID"))
            driveDiskID = parse["driveDiskID"];

        if(parse.contains("driveInitOnBoot"))
            driveInitOnBoot = parse["driveInitOnBoot"].get<bool>();

        delete[] jsonBuff;
    }
#endif
}

void cfg::saveCommon()
{
    FILE *cfgOut = fopen("/TYSS/cfg.bin", "wb");
    if(cfgOut)
    {
        fwrite(&std::get<bool>(cfg::config["zip"]), sizeof(bool), 1, cfgOut);
        fwrite(&std::get<int>(cfg::config["deflateLevel"]), sizeof(int), 1, cfgOut);
        fwrite(&std::get<bool>(cfg::config["lightback"]), sizeof(bool), 1, cfgOut);
        fwrite(&std::get<bool>(cfg::config["rawvcsave"]), sizeof(bool), 1, cfgOut);
        fwrite(&std::get<bool>(cfg::config["bootwithcheatdb"]), sizeof(bool), 1, cfgOut);
        fwrite(&std::get<bool>(cfg::config["swaplrfunc"]), sizeof(bool), 1, cfgOut);
        fclose(cfgOut);
    }
}

#ifdef ENABLE_DRIVE
void cfg::saveDrive()
{
    if(!driveRefreshToken.empty())
    {
        nlohmann::json drvCfg;
        if (!driveClientID.empty())
            drvCfg["driveClientID"] = driveClientID;
        if (!driveClientSecret.empty())
            drvCfg["driveClientSecret"] = driveClientSecret;
        if (!driveDiskID.empty())
            drvCfg["driveDiskID"] = driveDiskID;
        drvCfg["driveRefreshToken"] = driveRefreshToken;
        drvCfg["driveInitOnBoot"] = driveInitOnBoot;
        auto json_str = drvCfg.dump();

        FILE *drvOut = fopen("/TYSS/drive.json", "w");
        fputs(json_str.c_str(), drvOut);
        fclose(drvOut);
    }
}
#endif
