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

#include "ui.h"
#include "data.h"
#include "fs.h"
#include "util.h"

static ui::titleview *sysView;
static ui::menu *sysOpts;
static bool fldOpen = false, sysOptsOpen = false;

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

static void sysViewCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    if (sysView->getSelected() == -1)
        down = down & ~KEY_A & ~KEY_X & ~KEY_Y;
    switch(down)
    {
        case KEY_A:
            {
                data::titleData *t = &data::sysDataTitles[sysView->getSelected()];
                data::curData = *t;
                std::string uploadParent;
#ifdef ENABLE_GD
                if(fs::gDrive)
                {
                    fs::currentDirID = fs::sysSaveDirID;
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(fs::gDrive->dirExists(ttlUTF8, fs::sysSaveDirID))
                        uploadParent = fs::gDrive->getFolderID(ttlUTF8, fs::sysSaveDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_SYSTEM_SAVEDATA, false)
                    || fs::openArchive(*t, ARCHIVE_NAND_TWL_FS, false))
                {
                    std::u16string targetDir = util::createPath(*t, ARCHIVE_SYSTEM_SAVEDATA);
                    ui::fldInit(targetDir, uploadParent, fldCallback, NULL);
                    fldOpen = true;
                }
            }
            break;

        case KEY_X:
            {
                data::titleData *t = &data::sysDataTitles[sysView->getSelected()];
                data::curData = *t;
                sysOptsOpen = true;
            }
            break;

        case KEY_Y:
            {
                data::titleData *t = &data::sysDataTitles[sysView->getSelected()];
                uint64_t tid = t->getID();
                if(t->getFav())
                    data::favRem(*t);
                else
                    data::favAdd(*t);
                sysView->refresh(data::sysDataTitles);
                int newSel = data::findTitleNewIndex(data::sysDataTitles, tid);
                sysView->setSelected(newSel);
                ui::bossViewRefresh();
                ui::extRefresh();
                ui::ttlRefresh();
            }
            break;

        case KEY_CPAD_LEFT:
            ui::state = EXT;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = BOS;
            break;
    }
}

static void sysOptCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    switch (down)
    {
        case KEY_B:
            sysOptsOpen = false;
            break;
    }
}

static void sysOptAddtoBlackList(void *a)
{
    std::string q = "你确定要将 " + util::toUtf8(data::curData.getTitle()) + " 添加到黑名单吗?\n这将使其在所有视图中不可见!";
    void *arg = &data::curData;
    ui::confirm(q, data::blacklistAdd, NULL, arg);
}

static void sysOptBackupAll_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::sysDataTitles, ARCHIVE_SYSTEM_SAVEDATA);
    t->finished = true;
}

static void sysOptBackupAll(void *a)
{
    std::string q = "你确定要备份此页所有的系统存档吗?\n这或许需要耗费一定时间, 视Title数量而定。\n请耐心等待!";
    ui::confirm(q, sysOptBackupAll_t, NULL, NULL);
}

static void sysOptBackupAllDSiWare_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::sysDataTitles, ARCHIVE_NAND_TWL_FS);
    t->finished = true;
}

static void sysOptBackupAllDSiWare(void *a)
{
    std::string q = "你确定要备份此页所有的DSiWare存档吗?\n这或许需要耗费一定时间, 视Title数量而定。\n请耐心等待!";
    ui::confirm(q, sysOptBackupAllDSiWare_t, NULL, NULL);
}

void ui::sysInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    sysView = new ui::titleview(data::sysDataTitles, sysViewCallback, NULL);

    sysOpts = new ui::menu;
    sysOpts->setCallback(sysOptCallback, NULL);
    sysOpts->addOpt("添加到黑名单", 320);
    sysOpts->addOptEvent(0, KEY_A, sysOptAddtoBlackList, NULL);
    sysOpts->addOpt("备份所有的系统存档", 320);
    sysOpts->addOptEvent(1, KEY_A, sysOptBackupAll, NULL);
    sysOpts->addOpt("备份所有的 DSiWare 存档", 320);
    sysOpts->addOptEvent(2, KEY_A, sysOptBackupAllDSiWare, NULL);

    t->finished = true;
}

void ui::sysExit()
{
    delete sysView;
    delete sysOpts;
}

void ui::sysOptBack()
{
    sysOptsOpen = false;
    sysView->setSelected(0);
    ui::sysUpdate();
}

void ui::sysUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
    else if(sysOptsOpen)
        sysOpts->update();
    else
        sysView->update();
}

void ui::sysRefresh()
{
    sysView->refresh(data::sysDataTitles);
}

void ui::sysDrawTop()
{
    sysView->draw();
    ui::drawUIBar(TITLE_TEXT + "- 系统存档", ui::SCREEN_TOP, true);
}

void ui::sysDrawBot()
{
    if(fldOpen)
    {
        ui::fldDraw();
#ifdef ENABLE_GD
        ui::drawUIBar(fs::gDrive ? FLD_GUIDE_TEXT_GD : FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#else
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#endif
    }
    else if(sysOptsOpen)
    {
        sysOpts->draw(0, 2, 0xFFFFFFFF, 320, false);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::sysDataTitles.empty())
            data::sysDataTitles[sysView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, false);
    }
}
