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

    if (*in == 0)
        *in = 1;
}

static std::string getBoolText(const bool& g)
{
    return g ? "开" : "关";
}

static std::string getDeflateLevelText(const int& g)
{
    std::string s = "ZIP 压缩等级: ";

    s += std::to_string(g);

    if (g == 1)
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
    ui::confirm(q, misc::clearStepHistory, NULL, NULL);
}

static void setMenuClearSoftwareLibraryAndPlayHistory(void *a)
{
    std::string q = "你确定要清除游玩时间历史记录及软件图鉴\n吗?这将清除回忆记录册中的软件图鉴记录\n以及所有游玩时间历史记录!\n该操作无法撤销!执行此操作前强烈建议备份\n系统存档页的回忆记录册数据!";
    ui::confirm(q, misc::clearSoftwareLibraryAndPlayHistory, NULL, NULL);
}

static void setMenuClearSharedIconCache(void *a)
{
    std::string q = "你确定要清除共享图标缓存数据吗?\n将使得回忆记录册中的软件图鉴列表被清空\n该操作无法撤销!执行此操作前强烈建议备份\n共享追加数据页的FB记录以及系统存档页的\n回忆记录册数据!\n恢复备份时需要同时恢复上述两项记录!";
    ui::confirm(q, misc::clearSharedIconCache, NULL, NULL);
}

static void setMenuClearHomeMenuIconCache(void *a)
{
    std::string q = "你确定要清除主菜单图标缓存数据吗?\n该操作无法撤销!执行此操作前强烈建议备份\n追加数据页的主菜单(CTR-N-HMM*)记录!";
    ui::confirm(q, misc::clearHomeMenuIconCache, NULL, NULL);
}

static void setMenuResetDemoPlayCount(void *a)
{
    std::string q = "你确定要重置试玩版游戏的游玩计数吗?\n该操作无法撤销!";
    ui::confirm(q, misc::resetDemoPlayCount, NULL, NULL);
}

static void setMenuClearGameNotes(void *a)
{
    std::string q = "你确定要清除所有的游戏笔记吗?\n该操作无法撤销!执行此操作前强烈建议备份\n系统存档页的游戏笔记数据!";
    ui::confirm(q, misc::clearGameNotes, NULL, NULL);
}

static void setMenuRemoveSoftwareUpdateNag(void *a)
{
    std::string q = "你确定要清除所有的软件更新通知吗?\n这将移除所有已安装程序启动前的更新提示\n(直到主菜单再次联网下载软件版本数据库)\n注意!这对某些游戏内置的版本检查无效!\n该操作无法撤销!执行此操作前强烈建议备份\n共享追加数据页的FE记录!";
    ui::confirm(q, misc::removeSoftwareUpdateNag, NULL, NULL);
}

static void setMenuUnpackWrappedSoftware(void *a)
{
    std::string q = "你确定要打开所有的软件礼包吗?\n该操作无法撤销!";
    ui::confirm(q, misc::unpackWrappedSoftware, NULL, NULL);
}

static void setMenuHackStepCount(void *a)
{
    std::string q = "你确定要修改主菜单上显示的今日步数吗?\n由于部分限制因素,该项功能只能影响当前\n小时的步数,此前产生的步数不会被修改!";
    ui::confirm(q, misc::hackStepCount, NULL, NULL);
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
    t->status->setStatus("正在重载云端存储列表...");
    fs::gDrive->loadDriveList();
    t->finished = true;
}

static void setMenuReloadDriveList(void *a)
{
    if (fs::gDrive)
        ui::newThread(setMenuReloadDriveList_t, NULL, NULL);
    else
        ui::showMessage("云端存储: 服务尚未初始化");
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

    setMenu.addOpt("清除游玩时间历史记录以及软件图鉴", 320);
    setMenu.addOptEvent(7, KEY_A, setMenuClearSoftwareLibraryAndPlayHistory, NULL);

    setMenu.addOpt("清除共享图标缓存", 320);
    setMenu.addOptEvent(8, KEY_A, setMenuClearSharedIconCache, NULL);

    setMenu.addOpt("清除主菜单图标缓存", 320);
    setMenu.addOptEvent(9, KEY_A, setMenuClearHomeMenuIconCache, NULL);

    setMenu.addOpt("重置试玩版游戏游玩计数", 320);
    setMenu.addOptEvent(10, KEY_A, setMenuResetDemoPlayCount, NULL);

    setMenu.addOpt("清除游戏笔记", 320);
    setMenu.addOptEvent(11, KEY_A, setMenuClearGameNotes, NULL);

    setMenu.addOpt("清除软件更新通知", 320);
    setMenu.addOptEvent(12, KEY_A, setMenuRemoveSoftwareUpdateNag, NULL);

    setMenu.addOpt("打开所有软件礼包", 320);
    setMenu.addOptEvent(13, KEY_A, setMenuUnpackWrappedSoftware, NULL);

    setMenu.addOpt("修改今日步数", 320);
    setMenu.addOptEvent(14, KEY_A, setMenuHackStepCount, NULL);

#ifdef ENABLE_GD
    setMenu.addOpt("重载云端存储列表", 320);
    setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuReloadDriveList, NULL);
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
