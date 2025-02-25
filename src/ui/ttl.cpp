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
#ifdef ENABLE_GD
                if(fs::gDrive)
                {
                    fs::currentDirID = fs::usrSaveDirID;
                    if(fs::gDrive->dirExists(t->getTitleUTF8(), fs::usrSaveDirID))
                        uploadParent = fs::gDrive->getFolderID(t->getTitleUTF8(), fs::usrSaveDirID);
                }
#endif
                if(fs::openArchive(*t, ARCHIVE_USER_SAVEDATA, false) || t->getExtInfos().isDSCard)
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

        case KEY_CPAD_LEFT:
            ui::state = SET;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = EXT;
            break;
    }
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
    if(fs::openArchive(data::curData, ARCHIVE_USER_SAVEDATA, false))
    {
        t->status->setStatus("正在重置存档数据...");
        fs::delDirRec(fs::getSaveArch(), util::toUtf16("/"));
        fs::commitData(fs::getSaveMode());
        fs::closeSaveArch();
    }
    t->finished = true;
}

static void ttlOptResetSaveData(void *a)
{
    data::titleData *t = &data::usrSaveTitles[ttlView->getSelected()];
    if(t->getExtInfos().isDSCard)
    {
        ui::showMessage("DS卡带游戏不支持此操作!");
        return;
    }
    std::string q = "你确定要为 " + util::toUtf8(t->getTitle()) + " 重置存档数据吗?";
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

static void ttlOptManageCheats(void *a)
{
    data::titleData *title = &data::usrSaveTitles[ttlView->getSelected()];
    std::string key = title->getIDStr();

    if(title->getExtInfos().isDSCard)
    {
        ui::showMessage("DS卡带游戏不支持此操作!");
        return;
    }

    if (util::fexists("/cheats/" + key + ".txt")) {
        std::string q = "该应用的金手指文件已安装, 是否删除?";
        ui::confirm(q, ttlOptDeleteCheats_t, NULL, NULL);
    } else if (CheatManager::getInstance().cheats() != nullptr && CheatManager::getInstance().areCheatsAvailable(key)) {
        std::string q = "你确定要为 " + util::toUtf8(title->getTitle()) + " 安装金手指文件?";
        ui::confirm(q, ttlOptInstallCheats_t, NULL, NULL);
    } else {
        ui::showMessage("数据库中未找到该应用可用的金手指.");
    }
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
    fs::backupTitles(data::usrSaveTitles, ARCHIVE_USER_SAVEDATA);
    t->finished = true;
}

static void ttlOptBackupAll(void *a)
{
    std::string q = "你确定要备份此页所有的存档吗?\n这或许需要耗费一定时间, 视 Title 数量而定。\n请耐心等待!";
    ui::confirm(q, ttlOptBackupAll_t, NULL, NULL);
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

void ui::ttlRefresh(int selFlag)
{
    ttlView->refresh(data::usrSaveTitles);
    if (selFlag == 1) ttlView->setSelected(0);
    else if (selFlag == 2) ttlView->setSelected(ttlView->getSelected() == 0 ? 0 : ttlView->getSelected() - 1);
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
#ifdef ENABLE_GD
        ui::drawUIBar(fs::gDrive ? FLD_GUIDE_TEXT_GD : FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#else
        ui::drawUIBar(FLD_GUIDE_TEXT, ui::SCREEN_BOT, true);
#endif
    }
    else if(ttlOptsOpen)
    {
        ttlOpts->draw(0, 2, 0xFFFFFFFF, 320, false);
        ui::drawUIBar("\ue000 选择 \ue001 关闭", ui::SCREEN_BOT, false);
    }
    else
    {
        if (!data::usrSaveTitles.empty())
            data::usrSaveTitles[ttlView->getSelected()].drawInfo(0, 0);
        ui::drawUIBar("\ue000 打开 \ue002 选项 \ue003 收藏 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, true);
    }
}
