#include <3ds.h>

#include "ui.h"
#include "cfg.h"
#include "fs.h"
#include "util.h"
#include "misc.h"

static ui::menu setMenu;

static void toggleBool(void *b)
{
    bool *in = (bool *)b;
    if(*in)
        *in = false;
    else
        *in = true;
}

static void toggleDeflateLevel(void *b)
{
    int *in = (int *)b;
    *in = (*in + 1) % 10; 
}

static std::string getBoolText(const bool& g)
{
    return g ? "开" : "关";
}

static std::string getDeflateLevelText(const int& g)
{
    std::string s = "ZIP 压缩等级: ";

    s += std::to_string(g);

    if (g == 0)
        s += " (仅存储)";
    else if (g == 1)
        s += " (最快速度)";
    else if (g == 6)
        s += " (标准压缩)";
    else if (g == 9)
        s += " (极限压缩)";

    return s;
}

static void setMenuClearStepHistory(void *a)
{
    std::string q = "你确定要清除步数历史记录吗?\n这将清除回忆记录册中的总步数记录!\n该操作无法撤销!";
    ui::confirm(q, misc::PTMSYSM_ClearStepHistory, NULL, NULL);
}

static void setMenuHackPlayCoin(void *a)
{
    misc::setPC();
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

static void setMenuToggleDeflateLevel_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    toggleDeflateLevel(t->argPtr);
    cfg::saveCommon();
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuToggleDeflateLevel(void *a)
{
    ui::newThread(setMenuToggleDeflateLevel_t, a, NULL);
}

void ui::setInit(void *a)
{
    threadInfo *t = (threadInfo *)a;

    setMenu.addOpt("重载 Titles", 320);
    setMenu.addOptEvent(0, KEY_A, setMenuReloadTitles, NULL);

    setMenu.addOpt("导出到 ZIP", 320);
    setMenu.addOptEvent(1, KEY_A, setMenuToggleZIP, &std::get<bool>(cfg::config["zip"]));

    setMenu.addOpt("ZIP 压缩等级", 320);
    setMenu.addOptEvent(2, KEY_A, setMenuToggleDeflateLevel, &std::get<int>(cfg::config["deflateLevel"]));

    setMenu.addOpt("重置收藏列表", 320);
    setMenu.addOptEvent(3, KEY_A, setMenuClearFavList, NULL);

    setMenu.addOpt("重置黑名单", 320);
    setMenu.addOptEvent(4, KEY_A, setMenuClearBlackList, NULL);

    setMenu.addOpt("修改 PlayCoin", 320);
    setMenu.addOptEvent(5, KEY_A, setMenuHackPlayCoin, NULL);

    setMenu.addOpt("清除步数历史记录", 320);
    setMenu.addOptEvent(6, KEY_A, setMenuClearStepHistory, NULL);

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

    setMenu.editOpt(1, "导出到 ZIP: " + getBoolText(std::get<bool>(cfg::config["zip"])));
    setMenu.editOpt(2, getDeflateLevelText(std::get<int>(cfg::config["deflateLevel"])));

    setMenu.update();
}

void ui::setDrawTop()
{
    setMenu.draw(0, 22, 0xFFFFFFFF, 400, false);
    ui::drawUIBar(TITLE_TEXT + "- 设置与杂项", ui::SCREEN_TOP, true);
}

void ui::setDrawBottom()
{
    ui::drawUIBar("\ue000 选择/切换 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
}
