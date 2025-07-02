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
#include <vector>

#include "ui.h"
#include "data.h"
#include "util.h"
#include "gfx.h"

static ui::titleview *shrdView;
static bool fldOpen = false;
// These are hardcoded/specific
// Never change, don't need caching
static std::vector<data::titleData> shared;

static inline void addSharedEntry(const uint32_t& _id, const std::string& _icnTxt)
{
    char tmp[16];
    sprintf(tmp, "%08X", (unsigned)_id);

    data::titleData newShrd;
    newShrd.setExtdata(_id);
    newShrd.setTitle(util::toUtf16(tmp));
    C2D_Image icn = gfx::noIcon();
    newShrd.setIcon(icn);
    shared.push_back(newShrd);
}

static void fldCallback(void *)
{
    switch(ui::padKeysDown())
    {
        case KEY_B:
            fs::closeSaveArch();
            fldOpen = false;
            break;
    }
}

static void shrdViewBackupAll(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(shared, ARCHIVE_SHARED_EXTDATA);
    t->finished = true;
}

static void shrdViewCallback(void *)
{
    switch(ui::padKeysDown())
    {
        case KEY_A:
        {
            data::titleData *t = &shared[shrdView->getSelected()];
            std::string uploadParent;
#ifdef ENABLE_DRIVE
            if(fs::gDrive)
            {
                fs::currentDirID = fs::sharedExtID;
                std::string ttl = util::toUtf8(shared[shrdView->getSelected()].getTitle());
                if(fs::gDrive->dirExists(ttl, fs::sharedExtID))
                    uploadParent = fs::gDrive->getFolderID(ttl, fs::sharedExtID);
            }
#endif
            if(fs::openArchive(*t, ARCHIVE_SHARED_EXTDATA, false))
            {
                std::u16string targetDir = util::createPath(*t, ARCHIVE_SHARED_EXTDATA);
                ui::fldInit(targetDir, uploadParent, fldCallback, NULL);
                fldOpen = true;
            }
        }
        break;

        case KEY_X:
            {
                std::string q = "你确定要备份所有的共享追加数据吗?\n这或许需要耗费一定时间, 请耐心等待!";
                ui::confirm(q, shrdViewBackupAll, NULL, NULL);
            }
            break;

        case KEY_CPAD_LEFT:
            ui::state = BOS;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = SET;
            break;
    }
}

void ui::shrdInit(void *a)
{
    threadInfo *t = (threadInfo *)a;

    //Skip E0 since it doesn't open
    addSharedEntry(0xF0000001, "F1");
    addSharedEntry(0xF0000002, "F2");
    addSharedEntry(0xF0000009, "F9");
    addSharedEntry(0xF000000B, "FB");
    addSharedEntry(0xF000000C, "FC");
    addSharedEntry(0xF000000D, "FD");
    addSharedEntry(0xF000000E, "FE");
    shrdView = new titleview(shared, shrdViewCallback, NULL);
    t->finished = true;
}

void ui::shrdExit()
{
    for(data::titleData& d : shared)
        d.freeIcon();

    delete shrdView;
}

void ui::shrdUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
    else
        shrdView->update();
}

void ui::shrdDrawTop()
{
    shrdView->draw();
    ui::drawUIBar(TITLE_TEXT + "- 共享追加数据", ui::SCREEN_TOP, true);
}

void ui::shrdDrawBot()
{
    if(fldOpen)
    {
        ui::fldDraw();
#ifdef ENABLE_DRIVE
        ui::drawUIBar(fs::gDrive ? FLD_GUIDE_TEXT_GD : FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#else
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#endif
    }
    else
    {
        // Stolen 3Dbrew descriptions
        std::string sharedDesc;
        switch(shrdView->getSelected())
        {
            case 0:
                sharedDesc = "F1: NAND JPEG/MPO 文件和 phtcache.bin 由相机应用程序存储在这里. 这也包含 UploadData.dat.";
                break;
            case 1:
                sharedDesc = "F2: 声音应用程序中的 NAND M4A 文件存储在这里.";
                break;
            case 2:
                sharedDesc = "F9: 适用于通知的 SpotPass 内容存储.";
                break;
            case 3:
                sharedDesc = "FB: 包含 idb.dat, idbt.dat, gamecoin.dat, ubll.lst, CFL_DB.dat 以及 CFL_OldDB.dat. 这些文件包含明文 Miis 数据和一些与播放/使用记录相关的数据 (也包括缓存的图标数据).";
                break;
            case 4:
                sharedDesc = "FC: 包含 bashotorya.dat 以及 bashotorya2.dat.";
                break;
            case 5:
                sharedDesc = "FD: 主页菜单的 SpotPass 内容存储.";
                break;
            case 6:
                sharedDesc = "FE: 包含 versionList.dat, 用于 7.0.0-13 系统引入的主页菜单软件更新通知.";
                break;
        }
        gfx::drawTextWrap("3DBREW: " + sharedDesc, 8, 8, GFX_DEPTH_DEFAULT, 0.5f, 304, gfx::txtCont);
        ui::drawUIBar("\ue000 打开 \ue002 备份所有 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, false);
    }
}
