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

#ifdef ENABLE_GD
#include <json-c/json.h>
std::string cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken;
#endif

std::unordered_map<std::string, CFGVarType> cfg::config;

void cfg::initToDefault()
{
    cfg::config["zip"] = false;
    cfg::config["deflateLevel"] = 1;
    cfg::config["lightback"] = false;
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

        fclose(cfgIn);
    }

#ifdef ENABLE_GD
    fs::fsfile drvCfg(fs::getSDMCArch(), "/TYSS/drive.json", FS_OPEN_READ);
    if(drvCfg.isOpen())
    {
        char *jsonBuff = new char[drvCfg.getSize()];
        drvCfg.read(jsonBuff, drvCfg.getSize());

        json_object *parse = json_tokener_parse(jsonBuff);
        json_object *clientID, *secret, *auth, *refresh;
        json_object_object_get_ex(parse, "driveClientID", &clientID);
        json_object_object_get_ex(parse, "driveClientSecret", &secret);
        json_object_object_get_ex(parse, "driveAuthCode", &auth);
        json_object_object_get_ex(parse, "driveRefreshToken", &refresh);

        if(clientID)
            driveClientID = json_object_get_string(clientID);

        if(secret)
            driveClientSecret = json_object_get_string(secret);

        if(auth)
            driveAuthCode = json_object_get_string(auth);

        if(refresh)
            driveRefreshToken = json_object_get_string(refresh);

        delete[] jsonBuff;
        json_object_put(clientID);
        json_object_put(secret);
        json_object_put(auth);
        json_object_put(refresh);
        json_object_put(parse);
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
        fclose(cfgOut);
    }
}

#ifdef ENABLE_GD
void cfg::saveGD()
{
    if(!driveRefreshToken.empty())
    {
        json_object *drvCfg = json_object_new_object();
        json_object *drvClientOut = json_object_new_string(driveClientID.c_str());
        json_object *drvClientSecret = json_object_new_string(driveClientSecret.c_str());
        json_object *drvRefresh = json_object_new_string(driveRefreshToken.c_str());
        json_object_object_add(drvCfg, "driveClientID", drvClientOut);
        json_object_object_add(drvCfg, "driveClientSecret", drvClientSecret);
        json_object_object_add(drvCfg, "driveRefreshToken", drvRefresh);

        FILE *drvOut = fopen("/TYSS/drive.json", "w");
        fputs(json_object_get_string(drvCfg), drvOut);
        fclose(drvOut);

        json_object_put(drvCfg);
    }
}
#endif
