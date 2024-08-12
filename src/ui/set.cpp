#include <3ds.h>

#include "ui.h"
#include "cfg.h"
#include "fs.h"
#include "util.h"

static ui::menu setMenu;

static void toggleBool(void *b)
{
    bool *in = (bool *)b;
    if(*in)
        *in = false;
    else
        *in = true;
}

static std::string getBoolText(const bool& g)
{
    if(g)
        return "开";
    
    return "关";
}

static void setMenuHackPlayCoin(void *a)
{
    util::setPC();
}

static void setMenuClearFavList(void *a)
{
    remove("/JKSV/favorites.txt");
    ui::newThread(data::clearFav, NULL, NULL);
}

static void setMenuClearBlackList(void *a)
{
    std::string q = "你确定要重置黑名单吗?\n这将会自动执行一次重载 Titles!";
    ui::confirm(q, data::clearBlacklist, NULL, NULL);
}

static void setMenuReloadTitles(void *a)
{
    remove("/JKSV/cache.bin");
    ui::newThread(data::loadTitles, NULL, NULL);
}

#ifdef ENABLE_GD
static void setMenuReloadDriveList_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在重载 Google Drive 列表...");
    fs::gDrive->loadDriveList();
    t->finished = true;
}

static void setMenuReloadDriveList(void *a)
{
    if(fs::gDrive)
        ui::newThread(setMenuReloadDriveList_t, NULL, NULL);
}
#endif

static void setMenuToggleZIP_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    toggleBool(t->argPtr);
    cfg::saveCommon();
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuToggleZIP(void *a)
{
    ui::newThread(setMenuToggleZIP_t, a, NULL);
}

void ui::setInit(void *a)
{
    threadInfo *t = (threadInfo *)a;

    setMenu.addOpt("重载 Titles", 320);
    setMenu.addOptEvent(0, KEY_A, setMenuReloadTitles, NULL);

    setMenu.addOpt("导出到 ZIP", 320);
    setMenu.addOptEvent(1, KEY_A, setMenuToggleZIP, &cfg::config["zip"]);

    setMenu.addOpt("修改 PlayCoin", 320);
    setMenu.addOptEvent(2, KEY_A, setMenuHackPlayCoin, NULL);

    setMenu.addOpt("重置收藏列表", 320);
    setMenu.addOptEvent(3, KEY_A, setMenuClearFavList, NULL);

    setMenu.addOpt("重置黑名单", 320);
    setMenu.addOptEvent(4, KEY_A, setMenuClearBlackList, NULL);

#ifdef ENABLE_GD
    if(fs::gDrive)
    {
        setMenu.addOpt("重载 Google Drive 列表", 320);
        setMenu.addOptEvent(setMenu.getCount(), KEY_A, setMenuReloadDriveList, NULL);
    }
#endif

    t->finished = true;
}

void ui::setExit()
{

}

void ui::setUpdate()
{
    switch(ui::padKeysDown())
    {
        case KEY_CPAD_LEFT:
            ui::state = SHR;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = USR;
            break;
    }

    setMenu.editOpt(1, "导出到 ZIP: " + getBoolText(cfg::config["zip"]));

    setMenu.update();
}

void ui::setDrawTop()
{
    setMenu.draw(0, 22, 0xFFFFFFFF, 400, false);
    ui::drawUIBar(TITLE_TEXT + "- 设置", ui::SCREEN_TOP, true);
}

void ui::setDrawBottom()
{
    ui::drawUIBar("\ue000 选择/切换 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
}
