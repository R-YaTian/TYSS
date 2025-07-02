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
#include "gfx.h"

static ui::titleview *bossView;
static ui::menu *bossViewOpts;
static bool fldOpen = false, bossViewOptsOpen = false;

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

static void bossViewCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    if (bossView->getSelected() == -1)
        down = down & ~KEY_A & ~KEY_X & ~KEY_Y;
    switch(down)
    {
        case KEY_A:
            {
                data::titleData *t = &data::bossDataTitles[bossView->getSelected()];
                data::curData = *t;
                std::string uploadParent;
#ifdef ENABLE_DRIVE
                if(fs::gDrive)
                {
                    fs::currentDirID = fs::bossExtDirID;
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(fs::gDrive->dirExists(ttlUTF8, fs::bossExtDirID))
                        uploadParent = fs::gDrive->getFolderID(ttlUTF8, fs::bossExtDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_BOSS_EXTDATA, false))
                {
                    std::u16string targetDir = util::createPath(*t, ARCHIVE_BOSS_EXTDATA);
                    ui::fldInit(targetDir, uploadParent, fldCallback, NULL);
                    fldOpen = true;
                }
            }
            break;

        case KEY_X:
            {
                data::titleData *t = &data::bossDataTitles[bossView->getSelected()];
                data::curData = *t;
                bossViewOptsOpen = true;
            }
            break;

        case KEY_Y:
            {
                data::titleData *t = &data::bossDataTitles[bossView->getSelected()];
                uint64_t tid = t->getID();
                if(t->getFav())
                    data::favRem(*t);
                else
                    data::favAdd(*t);
                bossView->refresh(data::bossDataTitles);
                int newSel = data::findTitleNewIndex(data::bossDataTitles, tid);
                bossView->setSelected(newSel);
                ui::extRefresh();
                ui::sysRefresh();
                ui::ttlRefresh();
            }
            break;

        case KEY_CPAD_LEFT:
            ui::state = SYS;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = SHR;
            break;
    }
}

static void bossViewOptCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    switch (down)
    {
        case KEY_B:
            bossViewOptsOpen = false;
            break;
    }
}

static void bossViewOptAddtoBlackList(void *a)
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

static void bossViewOptBackupAll_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::bossDataTitles, ARCHIVE_BOSS_EXTDATA);
    t->finished = true;
}

static void bossViewOptBackupAll(void *a)
{
    std::string q = "你确定要备份此页所有的数据吗?\n这或许需要耗费一定时间, 视 Title 数量而定。\n请耐心等待!";
    ui::confirm(q, bossViewOptBackupAll_t, NULL, NULL);
}

void ui::bossViewInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    bossView = new ui::titleview(data::bossDataTitles, bossViewCallback, NULL);

    bossViewOpts = new ui::menu;
    bossViewOpts->setCallback(bossViewOptCallback, NULL);
    bossViewOpts->addOpt("添加到黑名单", 320);
    bossViewOpts->addOptEvent(0, KEY_A, bossViewOptAddtoBlackList, NULL);
    bossViewOpts->addOpt("备份所有的 BOSS 追加数据", 320);
    bossViewOpts->addOptEvent(1, KEY_A, bossViewOptBackupAll, NULL);

    t->finished = true;
}

void ui::bossViewExit()
{
    delete bossView;
    delete bossViewOpts;
}

void ui::bossViewOptBack()
{
    bossViewOptsOpen = false;
    bossView->setSelected(0);
    ui::bossViewUpdate();
}

void ui::bossViewUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
    else if(bossViewOptsOpen)
        bossViewOpts->update();
    else
        bossView->update();
}

void ui::bossViewRefresh(int op)
{
    bossView->refresh(data::bossDataTitles);
    if (op == SEL_BACK_TO_TOP) bossView->setSelected(0);
    else if (op == SEL_AUTO) bossView->setSelected(bossView->getSelected() == 0 ? 0 : bossView->getSelected() - 1);
}

void ui::bossViewDrawTop()
{
    bossView->draw();
    ui::drawUIBar(TITLE_TEXT + "- BOSS 追加数据", ui::SCREEN_TOP, true);
}

void ui::bossViewDrawBot()
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
    else if(bossViewOptsOpen)
    {
        bossViewOpts->draw(0, 2, gfx::txtCont, 320);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::bossDataTitles.empty())
            data::bossDataTitles[bossView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, false);
    }
}
