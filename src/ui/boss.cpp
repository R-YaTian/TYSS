#include <3ds.h>

#include "ui.h"
#include "data.h"
#include "fs.h"
#include "util.h"

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
                std::string uploadParent;
#ifdef ENABLE_GD
                if(fs::gDrive)
                {
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(!fs::gDrive->dirExists(ttlUTF8, fs::bossExtDirID))
                        fs::gDrive->createDir(ttlUTF8, fs::bossExtDirID);

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

void ui::bossViewInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    bossView = new ui::titleview(data::bossDataTitles, bossViewCallback, NULL);

    bossViewOpts = new ui::menu;
    bossViewOpts->setCallback(bossViewOptCallback, NULL);
    bossViewOpts->addOpt("添加到黑名单", 320);
    bossViewOpts->addOptEvent(0, KEY_A, bossViewOptAddtoBlackList, NULL);

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

void ui::bossViewRefresh()
{
    bossView->refresh(data::bossDataTitles);
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
#ifdef ENABLE_GD
        ui::drawUIBar(fs::gDrive ? FLD_GUIDE_TEXT_GD : FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#else
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#endif
    }
    else if(bossViewOptsOpen)
    {
        bossViewOpts->draw(0, 2, 0xFFFFFFFF, 320, false);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::bossDataTitles.empty())
            data::bossDataTitles[bossView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
    }
}
