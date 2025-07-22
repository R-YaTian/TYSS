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
#include "3dsgettext.h"

#ifdef ENABLE_DRIVE
#include "json.h"
std::string cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken, cfg::driveDiskID;
bool cfg::driveInitOnBoot = true;
#endif

#define TYSS_CFG_VER 2

std::unordered_map<std::string, u8> cfg::config;

void cfg::initToDefault()
{
    u8 syslang = CFG_LANGUAGE_EN;
    CFGU_GetSystemLanguage(&syslang);

    cfg::config["zip"] = (u8) false;
    cfg::config["deflateLevel"] = 1;
    cfg::config["lightback"] = (u8) false;
    cfg::config["rawvcsave"] = (u8) false;
    cfg::config["bootwithcheatdb"] = (u8) false;
    cfg::config["swaplrfunc"] = (u8) false;
    cfg::config["titlelang"] = syslang;
    cfg::config["uilang"] = (syslang == CFG_LANGUAGE_ZH ? 0 : (syslang == CFG_LANGUAGE_TW ? 2 : 1));
    cfg::config["cheatdblang"] = (syslang == CFG_LANGUAGE_ZH ? 0 : 1);
}

void cfg::setUILanguage(u8 langIndex)
{
    switch(langIndex)
    {
        case 1:
            setLanguage("en_US");
            break;
        case 2:
            setLanguage("zh_Hant");
            break;
        default:
            setLanguage("zh_Hans");
            break;
    }
}

void cfg::load()
{
    FILE *cfgIn = fopen("/TYSS/cfg.bin", "rb");
    if(cfgIn)
    {
        u8 getVal = 0;
        fread(&getVal, sizeof(u8), 1, cfgIn);
        if (TYSS_CFG_VER == getVal)
        {
            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["zip"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["deflateLevel"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["lightback"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["rawvcsave"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["bootwithcheatdb"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["swaplrfunc"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["titlelang"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["uilang"] = getVal;

            fread(&getVal, sizeof(u8), 1, cfgIn);
            cfg::config["cheatdblang"] = getVal;
        }

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
        u8 cfgver = TYSS_CFG_VER;
        fwrite(&cfgver, sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["zip"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["deflateLevel"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["lightback"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["rawvcsave"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["bootwithcheatdb"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["swaplrfunc"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["titlelang"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["uilang"], sizeof(u8), 1, cfgOut);
        fwrite(&cfg::config["cheatdblang"], sizeof(u8), 1, cfgOut);
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
