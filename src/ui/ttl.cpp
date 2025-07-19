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
#include <future>

#include "ui.h"
#include "ttl.h"
#include "gfx.h"
#include "fs.h"
#include "util.h"
#include "cheatmanager.h"

static ui::titleview *ttlView;
static ui::menu *ttlOpts;
static bool fldOpen = false, ttlOptsOpen = false;

static void fldCallback(void *)
{
    switch(ui::padKeysDown())
    {
        case KEY_B:
            if (data::curData.isAGB())
                fs::closePxiSaveArch();
            else
                fs::closeSaveArch();
            fldOpen = false;
            break;
    }
}

static void ttlViewCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    if (ttlView->getSelected() == -1)
        down = down & ~KEY_A & ~KEY_X & ~KEY_Y;
    switch(down)
    {
        case KEY_A:
            {
                data::titleData *t = &data::usrSaveTitles[ttlView->getSelected()];
                data::curData = *t;
                std::string uploadParent;
#ifdef ENABLE_DRIVE
                if(fs::netDrive)
                {
                    fs::currentDirID = fs::usrSaveDirID;
                    if(fs::netDrive->dirExists(t->getTitleUTF8(), fs::usrSaveDirID))
                        uploadParent = fs::netDrive->getFolderID(t->getTitleUTF8(), fs::usrSaveDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_USER_SAVEDATA, false)
                    || fs::openArchive(*t, ARCHIVE_NAND_TWL_FS, false)
                    || (t->isAGB() && fs::openArchive(*t, ARCHIVE_SAVEDATA_AND_CONTENT, false))
                    || t->getExtInfos().isDSCard)
                {
                    std::u16string targetPath = util::createPath(*t, ARCHIVE_USER_SAVEDATA);
                    ui::fldInit(targetPath, uploadParent, fldCallback, NULL);
                    fldOpen = true;
                }
            }
            break;

        case KEY_X:
            {
                data::titleData *t = &data::usrSaveTitles[ttlView->getSelected()];
                data::curData = *t;
                ttlOptsOpen = true;
            }
            break;

        case KEY_Y:
            {
                data::titleData *t = &data::usrSaveTitles[ttlView->getSelected()];
                uint64_t tid = t->getID();
                if(t->getFav())
                    data::favRem(*t);
                else
                    data::favAdd(*t);
                ttlView->refresh(data::usrSaveTitles);
                int newSel = data::findTitleNewIndex(data::usrSaveTitles, tid);
                ttlView->setSelected(newSel);
                ui::extRefresh();
                ui::sysRefresh();
                ui::bossViewRefresh();
            }
            break;
    }

    if (down & KEY_PAGE_LEFT)
        ui::state = SET;
    else if (down & KEY_PAGE_RIGHT)
        ui::state = EXT;
}

static void ttlOptCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    switch (down)
    {
        case KEY_B:
            ttlOptsOpen = false;
            break;
    }
}

static void ttlOptResetSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(fs::openArchive(data::curData, ARCHIVE_USER_SAVEDATA, false)) {
        t->status->setStatus("正在重置存档数据...");
        fs::delDirRec(fs::getSaveArch(), util::toUtf16("/"));
        fs::commitData(fs::getSaveMode());
        fs::closeSaveArch();
    } else if (data::curData.isAGB() && fs::openArchive(data::curData, ARCHIVE_SAVEDATA_AND_CONTENT, false)) {
        t->status->setStatus("正在重置GBAVC存档数据...");
        fs::resetPxiFile(fs::getSaveArch());
    }
    t->finished = true;
}

static void ttlOptResetSaveData(void *a)
{
    data::titleData *t = &data::usrSaveTitles[ttlView->getSelected()];
    if(t->getExtInfos().isDSCard || (t->getHigh() & 0x8000) == 0x8000)
    {
        ui::showMessage("DS卡带游戏以及 DSiWare 不支持此操作!");
        return;
    }
    std::string q = "你确定要为 " + util::toUtf8(t->getTitle()) + " 重置存档数据吗?\n注意: GBAVC存档重置后需运行一次游戏才能\n再次管理存档!";
    ui::confirm(q, ttlOptResetSaveData_t, NULL, NULL);
}

static void ttlOptInstallCheats_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在安装金手指文件...");

    std::string key = data::curData.getIDStr();
    auto asyncTask = std::async(std::launch::async, [key]() {
        return CheatManager::getInstance().install(key);
    });

    bool ret = asyncTask.get();
    if (ret)
        ui::showMessage("金手指文件安装成功!");
    else
        ui::showMessage("金手指文件安装失败!");

    t->finished = true;
}

static void ttlOptDeleteCheats_t(void *a)
{
    threadInfo *t = (threadInfo *)a;

    std::string key = data::curData.getIDStr();
    fs::fdelete("/cheats/" + key + ".txt");
    ui::showMessage("金手指文件已删除!");

    t->finished = true;
}

static void ttlOptManageCheats_t(void *a)
{
    threadInfo *t = (threadInfo *)a;

    data::titleData *title = &data::usrSaveTitles[ttlView->getSelected()];
    std::string key = title->getIDStr();

    if(title->getExtInfos().isDSCard || (title->getHigh() & 0x8000) == 0x8000 || title->isAGB())
    {
        ui::showMessage("GBAVC,DSiWare及DS卡带不支持此操作!");
        t->finished = true;
        return;
    }

    if (!CheatManager::getInstance().cheats())
        data::loadCheatsDB(a);

    if (util::fexists("/cheats/" + key + ".txt")) {
        std::string q = "该应用的金手指文件已安装, 是否删除?";
        ui::confirm(q, ttlOptDeleteCheats_t, NULL, NULL);
    } else if (CheatManager::getInstance().areCheatsAvailable(key)) {
        std::string q = "你确定要为 " + util::toUtf8(title->getTitle()) + " 安装金手指文件?";
        ui::confirm(q, ttlOptInstallCheats_t, NULL, NULL);
    } else {
        ui::showMessage("数据库中未找到该应用可用的金手指.");
    }

    t->finished = true;
}

static void ttlOptManageCheats(void *a)
{
    ui::newThread(ttlOptManageCheats_t, NULL, NULL);
}

static void ttlOptAddtoBlackList(void *a)
{
    if(data::curData.getMedia() == MEDIATYPE_GAME_CARD)
    {
        ui::showMessage("为避免发生问题, 禁止将卡带游戏添加到黑名单!");
        return;
    }
    std::string q = "你确定要将 " + util::toUtf8(data::curData.getTitle()) + " 添加到黑名单吗?\n这将使其在所有视图中不可见!";
    void *arg = &data::curData;
    ui::confirm(q, data::blacklistAdd, NULL, arg);
}

static void ttlOptBackupAll_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::usrSaveTitles, ARCHIVE_USER_SAVEDATA, fs::BunchType::Bunch_CTR);
    t->finished = true;
}

static void ttlOptBackupAll(void *a)
{
    std::string q = "你确定要备份此页所有的用户存档吗?\n这或许需要耗费一定时间, 视Title数量而定。\n请耐心等待!";
    ui::confirm(q, ttlOptBackupAll_t, NULL, NULL);
}

static void ttlOptBackupAllDSiWare_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::usrSaveTitles, ARCHIVE_USER_SAVEDATA, fs::BunchType::Bunch_TWL);
    t->finished = true;
}

static void ttlOptBackupAllDSiWare(void *a)
{
    std::string q = "你确定要备份此页所有的DSiWare存档吗?\n这或许需要耗费一定时间, 视Title数量而定。\n请耐心等待!";
    ui::confirm(q, ttlOptBackupAllDSiWare_t, NULL, NULL);
}

static void ttlOptBackupAllAGBSave_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::usrSaveTitles, ARCHIVE_USER_SAVEDATA, fs::BunchType::Bunch_AGB);
    t->finished = true;
}

static void ttlOptBackupAllAGBSave(void *a)
{
    std::string q = "你确定要备份此页所有的GBAVC存档吗?\n这或许需要耗费一定时间, 视Title数量而定。\n请耐心等待!";
    ui::confirm(q, ttlOptBackupAllAGBSave_t, NULL, NULL);
}

void ui::ttlInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    ttlView = new titleview(data::usrSaveTitles, ttlViewCallback, NULL);
    ui::state = USR;

    ttlOpts = new ui::menu;
    ttlOpts->setCallback(ttlOptCallback, NULL);
    ttlOpts->addOpt("安装或删除该应用的金手指文件", 320);
    ttlOpts->addOptEvent(0, KEY_A, ttlOptManageCheats, NULL);
    ttlOpts->addOpt("重置存档数据", 320);
    ttlOpts->addOptEvent(1, KEY_A, ttlOptResetSaveData, NULL);
    ttlOpts->addOpt("添加到黑名单", 320);
    ttlOpts->addOptEvent(2, KEY_A, ttlOptAddtoBlackList, NULL);
    ttlOpts->addOpt("备份所有的用户存档(DS 游戏卡带除外)", 320);
    ttlOpts->addOptEvent(3, KEY_A, ttlOptBackupAll, NULL);
    ttlOpts->addOpt("备份所有的 DSiWare 存档", 320);
    ttlOpts->addOptEvent(4, KEY_A, ttlOptBackupAllDSiWare, NULL);
    ttlOpts->addOpt("备份所有的 GBAVC 存档", 320);
    ttlOpts->addOptEvent(5, KEY_A, ttlOptBackupAllAGBSave, NULL);

    t->finished = true;
}

void ui::ttlExit()
{
    delete ttlView;
    delete ttlOpts;
}

void ui::ttlOptBack()
{
    ttlOptsOpen = false;
    ttlView->setSelected(0);
    ui::ttlUpdate();
}

void ui::ttlRefresh(int op)
{
    ttlView->refresh(data::usrSaveTitles);
    if (op == SEL_BACK_TO_TOP) ttlView->setSelected(0);
    else if (op == SEL_AUTO) ttlView->setSelected(ttlView->getSelected() == 0 ? 0 : ttlView->getSelected() - 1);
}

void ui::ttlUpdate()
{
    if(fldOpen)
        fldUpdate();
    else if(ttlOptsOpen)
        ttlOpts->update();
    else
        ttlView->update();
}

void ui::ttlDrawTop()
{
    ttlView->draw();
    ui::drawUIBar(TITLE_TEXT + "- 用户存档", ui::SCREEN_TOP, true);
}

void ui::ttlDrawBot()
{
    if(fldOpen)
    {
        ui::fldDraw();
#ifdef ENABLE_DRIVE
        ui::drawUIBar(fs::netDrive ? FLD_GUIDE_TEXT_DRIVE : FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#else
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#endif
    }
    else if(ttlOptsOpen)
    {
        ttlOpts->draw(0, 2, gfx::txtCont, 320);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::usrSaveTitles.empty())
            data::usrSaveTitles[ttlView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, true);
    }
}
