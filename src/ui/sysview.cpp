#include <3ds.h>

#include "ui.h"
#include "data.h"
#include "fs.h"
#include "util.h"

static ui::titleview *sysView;
static bool fldOpen = false;

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
                if(fs::gDrive)
                {
                    std::string ttlUTF8 = t->getTitleUTF8();
                    if(!fs::gDrive->dirExists(ttlUTF8, fs::sysSaveDirID))
                        fs::gDrive->createDir(ttlUTF8, fs::sysSaveDirID);

                    uploadParent = fs::gDrive->getFolderID(ttlUTF8, fs::sysSaveDirID);
                }

                if(fs::openArchive(*t, ARCHIVE_SYSTEM_SAVEDATA, false))
                {
                    util::createTitleDir(*t, ARCHIVE_SYSTEM_SAVEDATA);
                    std::u16string targetDir = util::createPath(*t, ARCHIVE_SYSTEM_SAVEDATA);
                    ui::fldInit(targetDir, uploadParent, fldCallback, NULL);
                    fldOpen = true;
                }
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

void ui::sysInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    sysView = new ui::titleview(data::sysDataTitles, sysViewCallback, NULL);
    t->finished = true;
}

void ui::sysExit()
{
    delete sysView;
}

void ui::sysUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
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
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
    }
    else
    {
        if (!data::sysDataTitles.empty())
            data::sysDataTitles[sysView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue003 收藏 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
    }
}
