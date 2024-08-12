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
    switch(ui::padKeysDown())
    {
        case KEY_A:
            {
                data::titleData *t = &data::sysDataTitles[sysView->getSelected()];
                std::string uploadParent;
#ifdef ENABLE_GD
                if(fs::gDrive)
                {
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(!fs::gDrive->dirExists(ttlUTF8, fs::sysSaveDirID))
                        fs::gDrive->createDir(ttlUTF8, fs::sysSaveDirID);

                    uploadParent = fs::gDrive->getFolderID(ttlUTF8, fs::sysSaveDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_SYSTEM_SAVEDATA, false))
                {
                    //util::createTitleDir(*t, ARCHIVE_SYSTEM_SAVEDATA);
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

void ui::sysInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    sysView = new ui::titleview(data::sysDataTitles, sysViewCallback, NULL);

    sysOpts = new ui::menu;
    sysOpts->setCallback(sysOptCallback, NULL);
    sysOpts->addOpt("添加到黑名单", 320);
    sysOpts->addOptEvent(0, KEY_A, sysOptAddtoBlackList, NULL);

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
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
    }
}
