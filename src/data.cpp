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

#include <3ds.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <zlib.h>
#include <future>

#include "util.h"
#include "data.h"
#include "fs.h"
#include "ui.h"
#include "gfx.h"
#include "type.h"
#include "cheatmanager.h"
#include "makercode.h"

#define ICON_BUFF_SIZE 0x2000

const char *blPath    = "/TYSS/blacklist.txt";
const char *favPath   = "/TYSS/favorites.txt";
const char *titlePath = "/TYSS/cache.bin";
static bool cartValid = false;
static bool titleLoaded = false;

static uint32_t extdataRedirect(const uint32_t& low)
{
    switch(low)
    {
        //Pokemon Y
        case 0x00055E00:
            return 0x0000055D;
            break;

        //Pokemon OR
        case 0x0011C400:
            return 0x000011C5;
            break;

        //Pkmn moon
        case 0x00175E00:
            return 0x00001648;
            break;

        //Ultra moon
        case 0x001B5100:
            return 0x00001B50;
            break;

        //FE Conquest + SE NA
        case 0x00179600:
        case 0x00179800:
            return 0x00001794;
            break;

        //FE Conquest + SE Euro
        case 0x00179700:
        case 0x0017A800:
            return 0x00001795;
            break;

        //FE if JP
        case 0x0012DD00:
        case 0x0012DE00:
            return 0x000012DC;
            break;

        default:
            return (low >> 8) & 0xFFFFF;
            break;
    }
}

std::vector<uint64_t> blacklist;
std::vector<uint64_t> favorites;

std::vector<data::titleData> data::usrSaveTitles;
std::vector<data::titleData> data::extDataTitles;
std::vector<data::titleData> data::sysDataTitles;
std::vector<data::titleData> data::bossDataTitles;

//This is a master list now
static std::vector<data::titleData> titles;
std::vector<uint32_t> filterIds;
data::titleData data::curData;

struct
{
    bool operator()(data::titleData& a, data::titleData& b)
    {
        if (a.getMedia() != b.getMedia()) {
            // MEDIATYPE_GAME_CARD is top first
            if (a.getMedia() == MEDIATYPE_GAME_CARD) return true;
            if (b.getMedia() == MEDIATYPE_GAME_CARD) return false;
            if (a.getFav() != b.getFav())
                return a.getFav() > b.getFav();
            else
                return a.getMedia() > b.getMedia();
        }

        if (a.getFav() != b.getFav()) {
            return a.getFav() > b.getFav();
        }

        bool a_is_agb = a.isAGB();
        bool b_is_agb = b.isAGB();
        if (a_is_agb != b_is_agb)
            return !a_is_agb;

        unsigned aLen = a.getTitleUTF8().length();
        unsigned bLen = b.getTitleUTF8().length();
        unsigned minLen = std::min(aLen, bLen);
        for(unsigned i = 0; i < minLen; i++)
        {
            int aChar = std::tolower(a.getTitleUTF8()[i]), bChar = std::tolower(b.getTitleUTF8()[i]);
            if(aChar != bChar)
                return aChar < bChar;
        }

        return aLen < bLen;
    }
} sortTitles;

static bool isBlacklisted(const uint64_t& id)
{
    u32 idHigh = (u32) (id >> 32); 

    if (idHigh == 0x0004000E || idHigh == 0x0004800F)
        return true;

    for(unsigned i = 0; i < blacklist.size(); i++)
    {
        if(id == blacklist[i])
            return true;
    }

    return false;
}

bool isFavorite(const uint64_t& id)
{
    for(unsigned i = 0; i < favorites.size(); i++)
    {
        if(id == favorites[i])
            return true;
    }
    return false;
}

static C3D_Tex *loadIcon(smdh_s *smdh)
{
    C3D_Tex *ret = new C3D_Tex;
    C3D_TexSetFilter(ret, GPU_LINEAR, GPU_LINEAR);
    uint16_t *icon = smdh->bigIconData;
    if(C3D_TexInit(ret, 64, 64, GPU_RGB565))//GPU can't use below 64x64
    {
        uint16_t *tex  = (uint16_t *)ret->data + (16 * 64);
        for(unsigned y = 0; y < 48; y += 8, icon += 48 *8, tex += 64 * 8)
            memcpy(tex, icon, sizeof(uint16_t) * 48 * 8);
    }
    return ret;
}

void data::init()
{
    CheatManager::getInstance();
}

void data::exit()
{
    for(auto t : titles)
        t.freeIcon();
}

bool data::titleData::init(const uint64_t& _id, const FS_MediaType& mt, bool isAGB)
{
    m = mt;
    id = _id;

    low = (uint32_t)id;
    high = (uint32_t)(id >> 32);
    unique = (low >> 8) & 0xFFFFF;
    extdata = extdataRedirect(low);

    char tid[32];
    sprintf(tid, "%016llX", id);
    idStr.assign(tid);

    char tmp[16];
    AM_GetTitleProductCode(m, id, tmp);
    prodCode = tmp;

    if(mt != MEDIATYPE_GAME_CARD && isFavorite(id))
        fav = true;

    if (isAGB || IsGbaVirtualConsole(low, high, m))
    {
        if (fs::openArchive(*this, ARCHIVE_SAVEDATA_AND_CONTENT, false))
        {
            types.hasUser = true;
            fs::closePxiSaveArch();
            if (!isAGB)
                prodCode.replace(0, 3, "AGB");
        }
    } else
        testMounts();

    smdh_s *smdh = loadSMDH(low, high, m);
    if(smdh != NULL && hasSaveData())
    {
        title.assign((char16_t *)(smdh->applicationTitles[cfg::config["titlelang"]].shortDescription));
        if(title.empty())
            title.assign((char16_t *)(smdh->applicationTitles[1].shortDescription));

        titleUTF8 = util::toUtf8(title);

        titleSafe.assign(util::safeString(title));

        publisher.assign((char16_t *)(smdh->applicationTitles[cfg::config["titlelang"]].publisher));
        if(publisher.empty())
            publisher.assign((char16_t *)(smdh->applicationTitles[1].publisher));

        icon = readIconFromSMDH(smdh);
        bhaveIcon = true;
        delete smdh;
    }
    else if(hasSaveData())
    {
        title.assign(util::toUtf16(idStr));
        titleUTF8 = idStr;
        titleSafe.assign(util::toUtf16(idStr));
        publisher.assign(util::toUtf16("Unknown"));
        icon = gfx::noIcon();
    }

    return true;
}

bool data::titleData::initTWL(const uint64_t& _id, const FS_MediaType& mt)
{
    m = mt;
    u64 programID = (m == MEDIATYPE_GAME_CARD) ? 0LL : _id;

    u8* headerData = new u8[0x3B4];
    Result res;
    res = FSUSER_GetLegacyRomHeader(m, programID, headerData);
    if (R_FAILED(res)) {
        delete[] headerData;
        return false;
    }

    char _cardTitle[14] = {0};
    char _gameCode[6]   = {0};
    char _makerCode[3]  = {0};

    std::copy(headerData, headerData + 12, _cardTitle);
    std::copy(headerData + 12, headerData + 16, _gameCode);
    std::copy(headerData + 16, headerData + 18, _makerCode);
    _cardTitle[13] = '\0';
    _gameCode[5]   = '\0';
    _makerCode[2]  = '\0';

    prodCode = _gameCode;

    if (m == MEDIATYPE_GAME_CARD)
    {
        id = (static_cast<u32>(_gameCode[0]) << 24) |
             (static_cast<u32>(_gameCode[1]) << 16) |
             (static_cast<u32>(_gameCode[2]) << 8)  |
              static_cast<u32>(_gameCode[3]);
    } else
        id = _id;

    low = (uint32_t)id;
    high = (uint32_t)(id >> 32);
    unique = (low >> 8) & 0xFFFFF;
    extdata = extdataRedirect(low);

    char tid[32];
    sprintf(tid, "%016llX", id);
    idStr.assign(tid);

    delete[] headerData;
    headerData = new u8[0x23C0];
    FSUSER_GetLegacyBannerData(m, programID, headerData);

    icon = readDSIcon(headerData);
    bhaveIcon = true;
    delete[] headerData;

    if (m == MEDIATYPE_GAME_CARD)
    {
        res = SPIGetCardType(&extInfo.spiCardType, (_gameCode[0] == 'I') ? 1 : 0);
        if (R_FAILED(res)) {
            return false;
        }
        extInfo.isDSCard = true;
        types.hasUser = true;
    } else if(fs::openArchive(*this, ARCHIVE_NAND_TWL_FS, false))
    {
        if (high == 0x00048004)
            types.hasUser = true;
        else if (high == 0x00048005)
            types.hasSys = true;
        fs::closeSaveArch();
    }

    title.assign(util::toUtf16(_cardTitle));
    titleSafe.assign(util::safeString(util::toUtf16(_cardTitle)));
    titleUTF8 = _cardTitle;

    Makercode pb;
    publisher.assign(util::toUtf16(pb.findPublisher(_makerCode)));

    return true;
}

bool data::titleData::initFromCache(const uint64_t& _id, const std::u16string& _title, const std::u16string& _pub, const std::string& code, const data::titleSaveTypes& _st, const uint8_t& mt)
{
    id = _id;
    low = (uint32_t)id;
    high = (uint32_t)(id >> 32);
    unique = (low >> 8) & 0xFFFFF;
    extdata = extdataRedirect(low);
    m = (FS_MediaType)mt;
    if(isFavorite(id))
        fav = true;

    title.assign(_title);
    titleUTF8 = util::toUtf8(title);
    titleSafe.assign(util::safeString(title));
    publisher = _pub;
    if(publisher.empty())
        publisher = util::toUtf16("Unknown");
    prodCode.assign(code);
    types = _st;

    char tid[32];
    sprintf(tid, "%016llX", _id);
    idStr.assign(tid);

    return true;
}

void data::titleData::testMounts()
{
    if(getMedia() == MEDIATYPE_GAME_CARD || getMedia() == MEDIATYPE_SD)
    {
        if(fs::openArchive(*this, ARCHIVE_USER_SAVEDATA, false))
        {
            types.hasUser = true;
            fs::closeSaveArch();
        }

        if(fs::openArchive(*this, ARCHIVE_EXTDATA, false))
        {
            types.hasExt = true;
            fs::closeSaveArch();
        }
    }

    if(getMedia() == MEDIATYPE_NAND)
    {
        if(fs::openArchive(*this, ARCHIVE_SYSTEM_SAVEDATA, false))
        {
            types.hasSys = true;
            fs::closeSaveArch();
        }

        if(fs::openArchive(*this, ARCHIVE_EXTDATA, false))
        {
            types.hasExt = true;
            fs::closeSaveArch();
        }

        if(fs::openArchive(*this, ARCHIVE_BOSS_EXTDATA, false))
        {
            types.hasBoss = true;
            fs::closeSaveArch();
        }
    }
}

bool data::titleData::hasSaveData()
{
    return types.hasUser || types.hasExt || types.hasSys || types.hasBoss;
}

bool data::titleData::IsGbaVirtualConsole(const uint32_t& low, const uint32_t& high, const uint8_t& media)
{
    if (!(prodCode.compare(0, 5, "CTR-N") == 0) || (high & 0xFFFF) != 0)
        return false;

    FSPXI_File handle;

    if (fs::openArchive(*this, ARCHIVE_SAVEDATA_AND_CONTENT, false))
    {
        static const uint32_t filePath[] = {0x0, 0x0, 0x2, 0x646F632E, 0x00000065}; //.code
        FS_Path binFilePath = {PATH_BINARY, 0x14, filePath};

        if(R_SUCCEEDED(FSPXI_OpenFile(fs::getPxiHandle(), &handle, fs::getSaveArch(), binFilePath, FS_OPEN_READ, 0)))
        {
            u32 gbaVcHeader[2], read;
            u64 size = 0;
            FSPXI_GetFileSize(fs::getPxiHandle(), handle, &size);
            FSPXI_ReadFile(fs::getPxiHandle(), handle, &read, size - 0x10, gbaVcHeader, sizeof(gbaVcHeader));
            FSPXI_CloseFile(fs::getPxiHandle(), handle);
            fs::closePxiSaveArch();

            return gbaVcHeader[0] == 0x4141432E && gbaVcHeader[1] == 1;
        }

        return false;
    }

    return false;
}

void data::titleData::setTitle(const std::u16string& _t)
{
    title = _t;
    titleSafe = util::safeString(_t);
    titleUTF8 = util::toUtf8(_t);
}

void data::titleData::drawInfo(unsigned x, unsigned y)
{
    std::string media;
    switch(getMedia())
    {
        case MEDIATYPE_GAME_CARD:
            media = "游戏卡带";
            break;

        case MEDIATYPE_SD:
            media = "SD";
            break;

        case MEDIATYPE_NAND:
            media = "NAND";
            break;
    }

    C2D_DrawRectSolid(0, 0, GFX_DEPTH_DEFAULT, 320.0f, 16.0f, gfx::rectLt);
    C2D_DrawRectSolid(0, 16, GFX_DEPTH_DEFAULT, 320.0f, 16.0f, gfx::rectSh);
    C2D_DrawRectSolid(0, 32, GFX_DEPTH_DEFAULT, 320.0f, 16.0f, gfx::rectLt);
    C2D_DrawRectSolid(0, 48, GFX_DEPTH_DEFAULT, 320.0f, 16.0f, gfx::rectSh);
    C2D_DrawRectSolid(0, 64, GFX_DEPTH_DEFAULT, 320.0f, 16.0f, gfx::rectLt);

    gfx::drawU16Text(title, 8, 0, GFX_DEPTH_DEFAULT, gfx::txtCont);
    gfx::drawU16Text(publisher, 8, 16, GFX_DEPTH_DEFAULT, gfx::txtCont);
    gfx::drawText(idStr, 8, 32, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
    gfx::drawText(prodCode, 8, 48, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
    gfx::drawText(media, 8,64, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
}

void data::titleData::assignIcon(C3D_Tex *_icon, bool isDSIcon)
{
    icon = {_icon, isDSIcon ? &gfx::dsIconSubTex : &gfx::iconSubTex};
}

static void loadcart()
{
    Result res = 0;
    u32 count  = 0;
    FS_CardType cardType;
    res = FSUSER_GetCardType(&cardType);
    if (R_SUCCEEDED(res)) {
        if (cardType == CARD_CTR) {
            res = AM_GetTitleCount(MEDIATYPE_GAME_CARD, &count);
            if (R_SUCCEEDED(res) && count > 0) {
                uint64_t cartID = 0;
                res = AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, count, &cartID);
                if (R_SUCCEEDED(res)) {
                    data::titleData cartData;
                    if(cartData.init(cartID, MEDIATYPE_GAME_CARD))
                    {
                        data::titleSaveTypes tmp = cartData.getSaveTypes();
                        if(tmp.hasUser)
                        {
                            data::usrSaveTitles.insert(data::usrSaveTitles.begin(), cartData);
                            ui::ttlRefresh(SEL_BACK_TO_TOP);
                        }

                        if(tmp.hasExt)
                        {
                            data::extDataTitles.insert(data::extDataTitles.begin(), cartData);
                            ui::extRefresh(SEL_BACK_TO_TOP);
                        }

                        cartValid = true;
                    }
                } else
                    cartValid = false;
            } else
                cartValid = false; // Is CARD_CTR, but no valid titles. Or get a wrong type...
        } else {
            data::titleData cartData;
            if(cartData.initTWL(0, MEDIATYPE_GAME_CARD))
            {
                data::usrSaveTitles.insert(data::usrSaveTitles.begin(), cartData);
                ui::ttlRefresh(SEL_BACK_TO_TOP);
                cartValid = true;
            } else
                cartValid = false;
        }
    } else
        cartValid = false;
}

// true = already loaded
// false = prepare to load
static bool isCartLoaded()
{
    if (cartValid)
        return true;

    if (!data::usrSaveTitles.empty())
        return (data::usrSaveTitles[0].getMedia() == MEDIATYPE_GAME_CARD);

    if (!data::extDataTitles.empty())
        return (data::extDataTitles[0].getMedia() == MEDIATYPE_GAME_CARD);

    return false;
}

void data::cartCheck()
{
    bool ins;
    FSUSER_CardSlotIsInserted(&ins);

    if(ins && !isCartLoaded())
    {
        cartValid = true; // assume valid
        auto future = std::async(std::launch::async, []() {
            loadcart();
        });
    }
    else if(!ins && cartValid)
    {
        if(!data::usrSaveTitles.empty() && data::usrSaveTitles[0].getMedia() == MEDIATYPE_GAME_CARD)
        {
            data::usrSaveTitles[0].freeIcon();
            data::usrSaveTitles.erase(data::usrSaveTitles.begin());
            ui::ttlRefresh(SEL_AUTO);
        }

        if(!data::extDataTitles.empty() && data::extDataTitles[0].getMedia() == MEDIATYPE_GAME_CARD)
        {
            data::extDataTitles[0].freeIcon();
            data::extDataTitles.erase(data::extDataTitles.begin());
            ui::extRefresh(SEL_AUTO);
        }

        cartValid = false;
    }
}

smdh_s *data::loadSMDH(const uint32_t& low, const uint32_t& high, const uint8_t& media)
{
    Handle handle;

    uint32_t archPath[] = {low, high, media, 0};
    static const uint32_t filePath[] = {0x0, 0x0, 0x2, 0x6E6F6369, 0x0};

    FS_Path binArchPath = {PATH_BINARY, 0x10, archPath};
    FS_Path binFilePath = {PATH_BINARY, 0x14, filePath};

    if(R_SUCCEEDED(FSUSER_OpenFileDirectly(&handle, ARCHIVE_SAVEDATA_AND_CONTENT, binArchPath, binFilePath, FS_OPEN_READ, 0)))
    {
        uint32_t read = 0;
        smdh_s *ret = new smdh_s;
        FSFILE_Read(handle, &read, 0, ret, sizeof(smdh_s));
        FSFILE_Close(handle);

        return ret;
    }
    return NULL;
}

static inline bool checkHigh(const uint64_t& id)
{
    uint32_t high = (uint32_t)(id >> 32);
    return (high == 0x00040000 || high == 0x00040002);
}

void data::loadCheatsDB(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在加载金手指数据库...");

    auto future = std::async(std::launch::async, []() {
        CheatManager::getInstance().init(); // Initialize the cheats db
    });
    future.get();

    t->finished = true;
}

void data::loadTitles(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在加载 Titles...");

    if (titleLoaded && !titles.empty())
    {
        for(auto t : titles) t.freeIcon();
    }

    titles.clear();
    usrSaveTitles.clear();
    extDataTitles.clear();
    sysDataTitles.clear();
    bossDataTitles.clear();
    favorites.clear();
    loadBlacklist();
    loadFav();

    if(!readCache(titles, titlePath))
    {
        auto future = std::async(std::launch::async, [&]() {
            scanTitles(titles);
        });
        future.get();

        t->status->setStatus("正在写入缓存...");
        createCache(titles, titlePath);
    }

    //Sort sort of like Switch
    //I don't like making copies but eh
    for(unsigned i = 0; i < titles.size(); i++)
    {
        data::titleSaveTypes tmp = titles[i].getSaveTypes();

        if(tmp.hasUser)
            usrSaveTitles.push_back(titles[i]);

        if(tmp.hasExt)
            extDataTitles.push_back(titles[i]);

        if(tmp.hasSys)
            sysDataTitles.push_back(titles[i]);

        if(tmp.hasBoss)
            bossDataTitles.push_back(titles[i]);
    }

    std::sort(usrSaveTitles.begin(), usrSaveTitles.end(), sortTitles);
    std::sort(extDataTitles.begin(), extDataTitles.end(), sortTitles);
    std::sort(sysDataTitles.begin(), sysDataTitles.end(), sortTitles);
    std::sort(bossDataTitles.begin(), bossDataTitles.end(), sortTitles);

    if (!titleLoaded)
        titleLoaded = true;
    else {
        ui::ttlRefresh(SEL_BACK_TO_TOP);
        ui::extRefresh(SEL_BACK_TO_TOP);
        ui::sysRefresh(SEL_BACK_TO_TOP);
        ui::bossViewRefresh(SEL_BACK_TO_TOP);
        if (cartValid) cartValid = false;
    }
    t->finished = true;
}

void data::scanTitles(std::vector<titleData>& titles)
{
    uint32_t count = 0;
    AM_GetTitleCount(MEDIATYPE_SD, &count);

    uint64_t *ids = new uint64_t[count];
    AM_GetTitleList(NULL, MEDIATYPE_SD, count, ids);

    for(unsigned i = 0; i < count; i++)
    {
        if(checkHigh(ids[i]) && !isBlacklisted(ids[i]))
        {
            char tmp[16];
            AM_GetTitleProductCode(MEDIATYPE_SD, ids[i], tmp);
            titleData newTitle;
            if (strncmp(tmp, "AGB-", 4) == 0 || strncmp(tmp, "GBA-", 4) == 0) {
                if(newTitle.init(ids[i], MEDIATYPE_SD, true) && newTitle.hasSaveData())
                    titles.push_back(newTitle);
            } else {
                if(newTitle.init(ids[i], MEDIATYPE_SD) && newTitle.hasSaveData())
                    titles.push_back(newTitle);
            }
        }
    }
    delete[] ids;

    //Load NAND too now
    AM_GetTitleCount(MEDIATYPE_NAND, &count);

    ids = new uint64_t[count];
    AM_GetTitleList(NULL, MEDIATYPE_NAND, count, ids);
    for(unsigned i = 0; i < count; i++)
    {
        if (!isBlacklisted(ids[i]))
        {
            titleData newNandTitle;
            if (((ids[i] >> 44) & 0x08) == 0x08) {
                if (newNandTitle.initTWL(ids[i], MEDIATYPE_NAND)
                    && newNandTitle.hasSaveData()
                    && !newNandTitle.getTitle().empty())
                    titles.push_back(newNandTitle);
            } else if (newNandTitle.init(ids[i], MEDIATYPE_NAND) &&
                        newNandTitle.hasSaveData() && !newNandTitle.getTitle().empty())
                titles.push_back(newNandTitle);
        }
    }
    delete[] ids;
}

void data::deleteExtData(void *a)
{
    threadInfo *t = (threadInfo *)a;
    titleData *in = (titleData *)t->argPtr;
    t->status->setStatus("正在删除追加数据...");

    FS_ExtSaveDataInfo del = { MEDIATYPE_SD, 0, 0, in->getExtData(), 0 };
    Result res = FSUSER_DeleteExtSaveData(del);
    if(R_SUCCEEDED(res))
    {
        // Remove it
        for(unsigned i = 0; i < titles.size(); i++)
        {
            if(titles[i].getID() == in->getID())
            {
                titles[i].freeIcon();
                titles.erase(titles.begin() + i);
                break;
            }
        }

        t->status->setStatus("正在重写缓存并刷新...");

        // Erase cart if it's there
        if(titles[0].getMedia() == MEDIATYPE_GAME_CARD)
            titles.erase(titles.begin());

        // Recreate cache with title missing now
        createCache(titles, titlePath);

        // Refresh
        extDataTitles.clear();
        for(unsigned i = 0; i < titles.size(); i++)
        {
            data::titleSaveTypes tmp = titles[i].getSaveTypes();
            if(tmp.hasExt) extDataTitles.push_back(titles[i]);
        }
        std::sort(extDataTitles.begin(), extDataTitles.end(), sortTitles);

        ui::extRefresh();
        ui::extOptBack();
        ui::showMessage("追加数据删除成功!");
    } else
        ui::showMessage("追加数据删除失败!\n错误: 0x%08X", (unsigned) res);

    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

void data::loadBlacklist()
{
    blacklist.clear();
    if(util::fexists(blPath))
    {
        fs::fsfile bl(fs::getSDMCArch(), blPath, FS_OPEN_READ);

        char line[64];
        while(bl.getLine(line, 64))
        {
            if(line[0] == '#' || line[0] == '\n')
                continue;

            blacklist.push_back(strtoull(line, NULL, 16));
        }
    }
}

void data::saveBlacklist()
{
    fs::fsfile bl(fs::getSDMCArch(), blPath, FS_OPEN_CREATE | FS_OPEN_WRITE);
    for(unsigned i = 0; i < blacklist.size(); i++)
        bl.writef("0x%016llX\n", blacklist[i]);
}

void data::blacklistAdd(void *a)
{
    threadInfo *t = (threadInfo *)a;
    titleData *in = (titleData *)t->argPtr;
    t->status->setStatus("正在保存黑名单...");

    blacklist.push_back(in->getID());
    saveBlacklist();

    //Remove it
    for(unsigned i = 0; i < titles.size(); i++)
    {
        if(titles[i].getID() == in->getID())
        {
            titles[i].freeIcon();
            titles.erase(titles.begin() + i);
            break;
        }
    }

    //Refresh titleview
    usrSaveTitles.clear();
    extDataTitles.clear();
    sysDataTitles.clear();
    bossDataTitles.clear();

    for(unsigned i = 0; i < titles.size(); i++)
    {
        data::titleSaveTypes tmp = titles[i].getSaveTypes();

        if(tmp.hasUser)
            usrSaveTitles.push_back(titles[i]);

        if(tmp.hasExt)
            extDataTitles.push_back(titles[i]);

        if(tmp.hasSys)
            sysDataTitles.push_back(titles[i]);

        if(tmp.hasBoss)
            bossDataTitles.push_back(titles[i]);
    }

    std::sort(usrSaveTitles.begin(), usrSaveTitles.end(), sortTitles);
    std::sort(extDataTitles.begin(), extDataTitles.end(), sortTitles);
    std::sort(sysDataTitles.begin(), sysDataTitles.end(), sortTitles);
    std::sort(bossDataTitles.begin(), bossDataTitles.end(), sortTitles);

    ui::ttlRefresh();
    if (ui::state == USR) ui::ttlOptBack();
    ui::extRefresh();
    if (ui::state == EXT) ui::extOptBack();
    ui::sysRefresh();
    if (ui::state == SYS) ui::sysOptBack();
    ui::bossViewRefresh();
    if (ui::state == BOS) ui::bossViewOptBack();

    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

void data::clearBlacklist(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在重置黑名单...");

    blacklist.clear();
    remove("/TYSS/blacklist.txt");
    remove("/TYSS/cache.bin");
    ui::newThread(data::loadTitles, NULL, NULL);

    t->finished = true;
}

void data::loadFav()
{
    if(util::fexists(favPath))
    {
        char line[64];
        fs::fsfile fav(fs::getSDMCArch(), favPath, FS_OPEN_READ);

        while(fav.getLine(line, 64))
        {
            if(line[0] == '#' || line[0] == '\n')
                continue;

            favorites.push_back(strtoull(line, NULL, 16));
        }
    }
}

void data::saveFav()
{
    if(favorites.size() > 0)
    {
        fs::fsfile fav(fs::getSDMCArch(), favPath, FS_OPEN_CREATE | FS_OPEN_WRITE);
        for(unsigned i = 0; i < favorites.size(); i++)
            fav.writef("0x%016llX\n", favorites[i]);
    }
}

void data::clearFav(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在重置收藏列表...");

    favorites.clear();
    remove("/TYSS/favorites.txt");

    // resort with new fav
    std::sort(data::usrSaveTitles.begin(), data::usrSaveTitles.end(), sortTitles);
    std::sort(data::extDataTitles.begin(), data::extDataTitles.end(), sortTitles);
    std::sort(data::bossDataTitles.begin(), data::bossDataTitles.end(), sortTitles);
    std::sort(data::sysDataTitles.begin(), data::sysDataTitles.end(), sortTitles);

    // refresh
    ui::extRefresh();
    ui::sysRefresh();
    ui::ttlRefresh();
    ui::bossViewRefresh();

    t->finished = true;
}

void data::favAdd(titleData& t)
{
    t.setFav(true);

    favorites.push_back(t.getID());

    //resort with new fav
    std::sort(data::usrSaveTitles.begin(), data::usrSaveTitles.end(), sortTitles);
    std::sort(data::extDataTitles.begin(), data::extDataTitles.end(), sortTitles);
    std::sort(data::bossDataTitles.begin(), data::bossDataTitles.end(), sortTitles);
    std::sort(data::sysDataTitles.begin(), data::sysDataTitles.end(), sortTitles);
}

void data::favRem(titleData& t)
{
    t.setFav(false);

    unsigned i;
    for(i = 0; i < favorites.size(); i++)
    {
        if(favorites[i] == t.getID())
        {
            favorites.erase(favorites.begin() + i);
            break;
        }
    }

    std::sort(data::usrSaveTitles.begin(), data::usrSaveTitles.end(), sortTitles);
    std::sort(data::extDataTitles.begin(), data::extDataTitles.end(), sortTitles);
    std::sort(data::bossDataTitles.begin(), data::bossDataTitles.end(), sortTitles);
    std::sort(data::sysDataTitles.begin(), data::sysDataTitles.end(), sortTitles);
}

C2D_Image data::readIconFromSMDH(smdh_s *smdh)
{
    return (C2D_Image){loadIcon(smdh), &gfx::iconSubTex};
}

C2D_Image data::readDSIcon(const u8* banner)
{
    static constexpr int WIDTH_POW2  = 32;
    static constexpr int HEIGHT_POW2 = 32;

    C3D_Tex *dsIconTex = new C3D_Tex;
    C3D_TexInit(dsIconTex, WIDTH_POW2, HEIGHT_POW2, GPU_RGB565);

    struct bannerData {
        u16 version;
        u16 crc;
        u8 reserved[28];
        u8 data[512];
        u16 palette[16];
    };
    bannerData* iconData = (bannerData*)banner;

    u16* output = (u16*)dsIconTex->data;
    for (size_t x = 0; x < 32; x++) {
        for (size_t y = 0; y < 32; y++) {
            u32 srcOff   = (((y >> 3) * 4 + (x >> 3)) * 8 + (y & 7)) * 4 + ((x & 7) >> 1);
            u32 srcShift = (x & 1) * 4;

            u16 pIndex = (iconData->data[srcOff] >> srcShift) & 0xF;
            u16 color  = 0xFFFF;
            if (pIndex != 0) {
                u16 r = iconData->palette[pIndex] & 0x1F;
                u16 g = (iconData->palette[pIndex] >> 5) & 0x1F;
                u16 b = (iconData->palette[pIndex] >> 10) & 0x1F;
                color = (r << 11) | (g << 6) | (g >> 4) | (b);
            }

            u32 dst     = ((((y >> 3) * (32 >> 3) + (x >> 3)) << 6) +
                       ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
            output[dst] = color;
        }
    }

    return (C2D_Image){dsIconTex, &gfx::dsIconSubTex};
}

void data::createCache(std::vector<titleData>& vect, const std::string& path)
{
    // JIC
    fs::fdelete(path);
    fs::fsfile cache(fs::getSDMCArch(), path, FS_OPEN_CREATE | FS_OPEN_WRITE);

    // Buffer to compress icons
    size_t iconCmpSize = 0;
    uint8_t *iconOut = new uint8_t[ICON_BUFF_SIZE];

    uint16_t countOut = vect.size();
    cache.write(&countOut, sizeof(uint16_t));
    cache.putByte(0x06);

    for(auto t : vect)
    {
        uint16_t titleLength = t.getTitle().size();
        cache.write(&titleLength, sizeof(uint16_t));
        cache.write(t.getTitle().data(), titleLength * sizeof(char16_t));

        uint16_t publisherLength = t.getPublisher().size();
        cache.write(&publisherLength, sizeof(uint16_t));
        cache.write(t.getPublisher().data(), publisherLength * sizeof(char16_t));

        uint8_t prodLength = t.getProdCode().size();
        cache.write(&prodLength, sizeof(uint8_t));
        cache.write(t.getProdCode().c_str(), prodLength);

        uint64_t idOut = t.getID();
        cache.write(&idOut, sizeof(uint64_t));

        data::titleSaveTypes tmp = t.getSaveTypes();
        cache.write(&tmp, sizeof(data::titleSaveTypes));

        uint8_t mediaType = t.getMedia();
        cache.write(&mediaType, sizeof(uint8_t));

        if (t.getIconFlag())
        {
            memset(iconOut, 0, ICON_BUFF_SIZE);
            iconCmpSize = ICON_BUFF_SIZE;
            compress2(iconOut, (uLongf *)&iconCmpSize, t.getIconData(), t.getIconSize(), Z_BEST_COMPRESSION);
            cache.write(&iconCmpSize, sizeof(size_t));
            cache.write(iconOut, iconCmpSize);
        } else {
            iconCmpSize = 0xFFFFFFFF;
            cache.write(&iconCmpSize, sizeof(size_t));
        }
    }

    // EOF
    cache.putByte(0x00);
    cache.putByte(0x45);
    cache.putByte(0x4F);
    cache.putByte(0x46);
    cache.close();
    delete[] iconOut;
}

bool data::readCache(std::vector<titleData>& vect, const std::string& path)
{
    if(!util::fexists(path))
        return false;

    fs::fsfile cache(fs::getSDMCArch(), path, FS_OPEN_READ);
    //Check revision
    uint8_t rev = 0;
    cache.seek(2, fs::seek_set);
    rev = cache.getByte();
    cache.seek(0, fs::seek_set);

    if(rev != 0x06)
        return false;

    uint16_t count = 0;
    cache.read(&count, sizeof(uint16_t));
    cache.getByte();

    uint8_t *readBuff = new uint8_t[ICON_BUFF_SIZE];

    titles.reserve(count);

    for(unsigned i = 0; i < count; i++)
    {
        titleData newData;

        uint16_t titleLength = 0;
        char16_t title[0x40];
        memset(title, 0x00, 0x40 * sizeof(char16_t));
        cache.read(&titleLength, sizeof(uint16_t));
        cache.read(title, titleLength * sizeof(char16_t));

        uint16_t pubLength = 0;
        char16_t pub[0x40];
        memset(pub, 0x00, 0x40 * sizeof(char16_t));
        cache.read(&pubLength, sizeof(uint16_t));
        cache.read(pub, pubLength * sizeof(char16_t));

        uint8_t prodLength = 0;
        char prodCode[16];
        memset(prodCode, 0x00, 16);
        cache.read(&prodLength, sizeof(uint8_t));
        cache.read(prodCode, prodLength);

        uint64_t newID = 0;
        cache.read(&newID, sizeof(uint64_t));

        data::titleSaveTypes tmp;
        cache.read(&tmp, sizeof(data::titleSaveTypes));

        uint8_t mediaType = 0;
        cache.read(&mediaType, sizeof(uint8_t));

        size_t iconSize = 0;
        memset(readBuff, 0x00, ICON_BUFF_SIZE);
        cache.read(&iconSize, sizeof(size_t));

        if (isBlacklisted(newID))
        {
            if (iconSize != 0xFFFFFFFF) cache.seek(iconSize, fs::seek_cur);
            continue;
        }

        if (iconSize == 0xFFFFFFFF)
            newData.setIcon(gfx::noIcon());
        else {
            cache.read(readBuff, iconSize);

            bool res = false;
            C3D_Tex *icon = new C3D_Tex;
            res = (((newID >> 44) & 0x08) == 0x08)
                    ? C3D_TexInit(icon, 32, 32, GPU_RGB565)
                    : C3D_TexInit(icon, 64, 64, GPU_RGB565);
            if (res)
            {
                uLongf sz = ICON_BUFF_SIZE;
                uncompress((uint8_t *)icon->data, &sz, readBuff, iconSize);
                newData.assignIcon(icon, ((newID >> 44) & 0x08) == 0x08);
                newData.setIconFlag(true);
            } else
                newData.setIcon(gfx::noIcon());
        }

        newData.initFromCache(newID, title, pub, prodCode, tmp, mediaType);
        vect.push_back(newData);
    }

    uint32_t eof = 0;
    cache.read(&eof, sizeof(uint32_t));
    cache.close();
    delete[] readBuff;

    if(eof != 0x464F4500)
        return false;

    return true;
}

void data::datDrawTop()
{
    ui::drawUIBar("正在加载...", ui::SCREEN_TOP, true);
}

void data::datDrawBot()
{
    ui::drawUIBar("", ui::SCREEN_BOT, false);
}

int data::findTitleNewIndex(std::vector<titleData>& _t, const uint64_t& tid)
{
    if (!_t.empty())
    {
        for(unsigned i = 0; i < _t.size(); i++)
        {
            if(_t[i].getID() == tid)
                return i;
        }
    }
    return -1;
}
