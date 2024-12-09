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
}

void cfg::load()
{
    FILE *cfgIn = fopen("/JKSV/cfg.bin", "rb");
    if(cfgIn)
    {
        bool getBool = false;
        fread(&getBool, sizeof(bool), 1, cfgIn);
        cfg::config["zip"] = getBool;

        int getInt = 0;
        fread(&getInt, sizeof(int), 1, cfgIn);
        cfg::config["deflateLevel"] = getInt;

        fclose(cfgIn);
    }

#ifdef ENABLE_GD
    fs::fsfile drvCfg(fs::getSDMCArch(), "/JKSV/drive.json", FS_OPEN_READ);
    if(drvCfg.isOpen())
    {
        char *jsonBuff = new char[drvCfg.getSize()];
        drvCfg.read(jsonBuff, drvCfg.getSize());

        json_object *parse = json_tokener_parse(jsonBuff), *clientID, *secret, *auth, *refresh;
        json_object_object_get_ex(parse, "driveClientID", &clientID);
        json_object_object_get_ex(parse, "driveClientSecret", &secret);
        json_object_object_get_ex(parse, "driveAuthCode", &auth);
        json_object_object_get_ex(parse, "driveRefreshToken", &refresh);

        if(clientID)
            cfg::driveClientID = json_object_get_string(clientID);
        
        if(secret)
            cfg::driveClientSecret = json_object_get_string(secret);

        if(auth)
            cfg::driveAuthCode = json_object_get_string(auth);
        
        if(refresh)
            cfg::driveRefreshToken = json_object_get_string(refresh);

        delete[] jsonBuff;
        json_object_put(parse);
    }
#endif
}

void cfg::saveCommon()
{
    FILE *cfgOut = fopen("/JKSV/cfg.bin", "wb");
    if(cfgOut)
    {
        fwrite(&std::get<bool>(cfg::config["zip"]), sizeof(bool), 1, cfgOut);
        fwrite(&std::get<int>(cfg::config["deflateLevel"]), sizeof(int), 1, cfgOut);
        fclose(cfgOut);
    }
}

#ifdef ENABLE_GD
void cfg::saveGD()
{
    if(!cfg::driveRefreshToken.empty())
    {
        json_object *drvCfg = json_object_new_object();
        json_object *drvClientOut = json_object_new_string(cfg::driveClientID.c_str());
        json_object *drvClientSecret = json_object_new_string(cfg::driveClientSecret.c_str());
        json_object *drvRefresh = json_object_new_string(cfg::driveRefreshToken.c_str());
        json_object_object_add(drvCfg, "driveClientID", drvClientOut);
        json_object_object_add(drvCfg, "driveClientSecret", drvClientSecret);
        json_object_object_add(drvCfg, "driveRefreshToken", drvRefresh);

        FILE *drvOut = fopen("/JKSV/drive.json", "w");
        fputs(json_object_get_string(drvCfg), drvOut);
        fclose(drvOut);
        json_object_put(drvCfg);
    }
}
#endif
