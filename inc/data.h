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

#ifndef DATA_H
#define DATA_H

#include <3ds.h>
#include <citro2d.h>
#include <vector>
#include <string>

#include "spi.h"
#include "smdh.h"

bool isFavorite(const uint64_t& id);

namespace data
{
    void init();
    void exit();

    typedef struct
    {
        bool hasUser = false;
        bool hasExt  = false;
        bool hasSys  = false;
        bool hasBoss = false;
    } titleSaveTypes;

    typedef struct
    {
        bool isDSCard = false;
        CardType spiCardType = NO_CHIP;
    } titleExtInfos;

    class titleData
    {
        public:
            bool init(const uint64_t& _id, const FS_MediaType& mt);
            bool initTWL(const uint64_t& _id, const FS_MediaType& mt);
            bool initFromCache(const uint64_t& _id, const std::u16string& _title, const std::u16string& _pub, const std::string& code, const data::titleSaveTypes& _st, const uint8_t& mt);
            void testMounts();
            bool hasSaveData();

            uint64_t getID() const { return id; }
            uint32_t getLow() const { return low; }
            uint32_t getHigh() const { return high; }
            uint32_t getUnique() const { return unique; }
            uint32_t getExtData() const { return extdata; }
            uint8_t getMedia() const { return m; }
            bool getFav() const { return isFavorite(id); }
            bool getIconFlag() const { return bhaveIcon; }

            void setIconFlag(bool _haveIcon) { bhaveIcon = _haveIcon; }
            void setFav(bool _set) { fav = _set; }
            void setUnique(const uint32_t& id) { unique = id; }
            void setExtdata(const uint32_t& ex) { extdata = ex; }
            void setTitle(const std::u16string& _t);

            std::string getProdCode() { return prodCode; }
            std::string getIDStr()    { return idStr; }
            std::u16string getTitle() { return title; }
            std::string getTitleUTF8() { return titleUTF8; }
            std::u16string getTitleSafe() { return titleSafe; }
            std::u16string getPublisher() { return publisher; }
            data::titleSaveTypes getSaveTypes() { return types; }
            data::titleExtInfos getExtInfos() { return extInfo; }

            void drawInfo(unsigned x, unsigned y);
            void setIcon(C2D_Image _set) { icon = _set; }
            size_t getIconSize() { return icon.tex->size; }
            uint8_t *getIconData() { return (uint8_t *)icon.tex->data; }
            C2D_Image *getIcon() { return &icon; }

            void assignIcon(C3D_Tex *_icon, bool isDSIcon = false);
            void freeIcon() { if(icon.tex && bhaveIcon) { C3D_TexDelete(icon.tex); } }

        private:
            uint64_t id;
            uint32_t high, low, unique, extdata;
            std::string prodCode, idStr, titleUTF8;
            std::u16string title, titleSafe, publisher;
            FS_MediaType m;
            bool fav = false;
            bool bhaveIcon = false;
            titleSaveTypes types;
            titleExtInfos extInfo;
            C2D_Image icon = {NULL, NULL};
    };

    void cartCheck();

    extern std::vector<titleData> usrSaveTitles;
    extern std::vector<titleData> extDataTitles;
    extern std::vector<titleData> sysDataTitles;
    extern std::vector<titleData> bossDataTitles;
    extern titleData curData;

    smdh_s *loadSMDH(const uint32_t& low, const uint32_t& high, const uint8_t& media);
    void loadCheatsDB(void *a);
    void loadTitles(void *a);
    void deleteExtData(void *a);

    void loadBlacklist();
    void saveBlacklist();
    void clearBlacklist(void *a);
    void blacklistAdd(void *a);

    void loadFav();
    void saveFav();
    void clearFav(void *a);
    void favAdd(titleData& t);
    void favRem(titleData& t);

    //Reads icon to C2D_image
    C2D_Image readIconFromSMDH(smdh_s *smdh);
    C2D_Image readDSIcon(const u8* banner);

    //Writes title data cache to path
    bool readCache(std::vector<titleData>& vect, const std::string& path);
    void createCache(std::vector<titleData>& vect, const std::string& path);

    //Just functions to draw while data load thread runs
    void datDrawTop();
    void datDrawBot();

    //For fav feat
    int findTitleNewIndex(std::vector<titleData>& _t, const uint64_t& tid);
}

#endif // DATA_H
