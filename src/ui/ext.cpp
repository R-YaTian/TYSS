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

static ui::titleview *extView;
static bool fldOpen = false, extOptsOpen = false;
static ui::menu *extOpts;

static void fldCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case KEY_B:
            fs::closeSaveArch();
            fldOpen = false;
            break;
    }
}

static void extViewCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    if (extView->getSelected() == -1)
        down = down & ~KEY_A & ~KEY_X & ~KEY_Y;
    switch(down)
    {
        case KEY_A:
            {
                data::titleData *t = &data::extDataTitles[extView->getSelected()];
                data::curData = *t;
                std::string uploadParent;
#ifdef ENABLE_GD
                if(fs::gDrive)
                {
                    fs::currentDirID = fs::extDataDirID;
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(fs::gDrive->dirExists(ttlUTF8, fs::extDataDirID))
                        uploadParent = fs::gDrive->getFolderID(ttlUTF8, fs::extDataDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_EXTDATA, false))
                {
                    std::u16string targetPath = util::createPath(*t, ARCHIVE_EXTDATA);
                    ui::fldInit(targetPath, uploadParent, fldCallback, NULL);
                    fldOpen = true;
                }
            }
            break;

        case KEY_X:
            {
                data::titleData *t = &data::extDataTitles[extView->getSelected()];
                data::curData = *t;
                extOptsOpen = true;
            }
            break;

        case KEY_Y:
            {
                data::titleData *t = &data::extDataTitles[extView->getSelected()];
                uint64_t tid = t->getID();
                if(t->getFav())
                    data::favRem(*t);
                else
                    data::favAdd(*t);
                extView->refresh(data::extDataTitles);
                int newSel = data::findTitleNewIndex(data::extDataTitles, tid);
                extView->setSelected(newSel);
                ui::bossViewRefresh();
                ui::sysRefresh();
                ui::ttlRefresh();
            }
            break;

        case KEY_CPAD_LEFT:
            ui::state = USR;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = SYS;
            break;
    }
}

static void extOptCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    switch (down)
    {
        case KEY_B:
            extOptsOpen = false;
            break;
    }
}

static void extOptDeleteExtData(void *a)
{
    data::titleData *t = &data::extDataTitles[extView->getSelected()];
    std::string q = "你确定要删除 " + util::toUtf8(t->getTitle()) + " 的追加数据吗?\n删除后将重写缓存";
    void *arg = &data::curData;
    ui::confirm(q, data::deleteExtData, NULL, arg);
}

static void extOptAddtoBlackList(void *a)
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

static void extOptBackupAll_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupTitles(data::extDataTitles, ARCHIVE_EXTDATA);
    t->finished = true;
}

static void extOptBackupAll(void *a)
{
    std::string q = "你确定要备份此页所有的数据吗?\n这或许需要耗费一定时间, 视 Title 数量而定。\n请耐心等待!";
    ui::confirm(q, extOptBackupAll_t, NULL, NULL);
}

void ui::extInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    extView = new ui::titleview(data::extDataTitles, extViewCallback, NULL);

    extOpts = new ui::menu;
    extOpts->setCallback(extOptCallback, NULL);
    extOpts->addOpt("删除追加数据", 320);
    extOpts->addOptEvent(0, KEY_A, extOptDeleteExtData, NULL);
    extOpts->addOpt("添加到黑名单", 320);
    extOpts->addOptEvent(1, KEY_A, extOptAddtoBlackList, NULL);
    extOpts->addOpt("备份所有的追加数据", 320);
    extOpts->addOptEvent(2, KEY_A, extOptBackupAll, NULL);

    t->finished = true;
}

void ui::extExit()
{
    delete extOpts;
    delete extView;
}

void ui::extOptBack()
{
    extOptsOpen = false;
    extView->setSelected(0);
    ui::extUpdate();
}

void ui::extUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
    else if(extOptsOpen)
        extOpts->update();
    else
        extView->update();
}

void ui::extRefresh(int op)
{
    extView->refresh(data::extDataTitles);
    if (op == SEL_BACK_TO_TOP) extView->setSelected(0);
    else if (op == SEL_AUTO) extView->setSelected(extView->getSelected() == 0 ? 0 : extView->getSelected() - 1);
}

void ui::extDrawTop()
{
    extView->draw();
    ui::drawUIBar(TITLE_TEXT + "- 追加数据", ui::SCREEN_TOP, true);
}

void ui::extDrawBot()
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
    else if(extOptsOpen)
    {
        extOpts->draw(0, 2, 0xFFFFFFFF, 320, false);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::extDataTitles.empty())
            data::extDataTitles[extView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, false);
    }
}
