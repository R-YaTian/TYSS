#include <3ds.h>

#include "ui.h"
#include "gfx.h"
#include "cfg.h"
#include "fs.h"
#include "util.h"
#include "misc.h"

static ui::menu setMenu, setSubMenu;
static bool setSubMenuOpen = false;

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

static std::string getLightDarkText(const bool& g)
{
    return g ? "浅色" : "暗黑";
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
    std::string q = "你确定要清除步数历史记录吗?";
    ui::confirm(q, misc::clearStepHistory, NULL, NULL);
}

static void setMenuClearSoftwareLibraryAndPlayHistory(void *a)
{
    std::string q = "确定要清除游玩时间历史记录及软件图鉴?";
    ui::confirm(q, misc::clearSoftwareLibraryAndPlayHistory, NULL, NULL);
}

static void setMenuClearSharedIconCache(void *a)
{
    std::string q = "你确定要清除共享图标缓存数据吗?";
    ui::confirm(q, misc::clearSharedIconCache, NULL, NULL);
}

static void setMenuClearHomeMenuIconCache(void *a)
{
    std::string q = "你确定要清除主菜单图标缓存数据吗?";
    ui::confirm(q, misc::clearHomeMenuIconCache, NULL, NULL);
}

static void setMenuResetDemoPlayCount(void *a)
{
    std::string q = "你确定要重置试玩版游戏的游玩计数吗?";
    ui::confirm(q, misc::resetDemoPlayCount, NULL, NULL);
}

static void setMenuClearGameNotes(void *a)
{
    std::string q = "你确定要清除所有的游戏笔记吗?";
    ui::confirm(q, misc::clearGameNotes, NULL, NULL);
}

static void setMenuRemoveSoftwareUpdateNag(void *a)
{
    std::string q = "你确定要清除所有的软件更新通知吗?";
    ui::confirm(q, misc::removeSoftwareUpdateNag, NULL, NULL);
}

static void setMenuUnpackWrappedSoftware(void *a)
{
    std::string q = "你确定要打开所有的软件礼包吗?";
    ui::confirm(q, misc::unpackWrappedSoftware, NULL, NULL);
}

static void setMenuHackStepCount(void *a)
{
    std::string q = "你确定要修改主菜单上显示的今日步数吗?";
    ui::confirm(q, misc::hackStepCount, NULL, NULL);
}

static void setMenuHackPlayCoin(void *a)
{
    misc::setPC();
}

static void setMenuClearFavList(void *a)
{
    remove("/TYSS/favorites.txt");
    ui::newThread(data::clearFav, NULL, NULL);
}

static void setMenuClearBlackList(void *a)
{
    std::string q = "你确定要重置黑名单吗?";
    ui::confirm(q, data::clearBlacklist, NULL, NULL);
}

static void setMenuReloadTitles(void *a)
{
    remove("/TYSS/cache.bin");
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

static void setMenuToggleBOOL_t(void *a)
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

static void setMenuToggleBOOL(void *a)
{
    ui::newThread(setMenuToggleBOOL_t, a, NULL);
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

static void setSubMenuCallback(void *a)
{
    uint32_t down = ui::padKeysDown();
    switch (down)
    {
        case KEY_B:
            setSubMenuOpen = false;
            break;
    }
}

void ui::setInit(void *a)
{
    threadInfo *t = (threadInfo *)a;

    setMenu.addOpt("重载 Titles", 320);
    setMenu.addOptEvent(0, KEY_A, setMenuReloadTitles, NULL);

    setMenu.addOpt("导出到 ZIP", 320);
    setMenu.addOptEvent(1, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["zip"]));

    setMenu.addOpt("ZIP 压缩等级", 320);
    setMenu.addOptEvent(2, KEY_A, setMenuToggleDeflateLevel, &std::get<int>(cfg::config["deflateLevel"]));

    setMenu.addOpt("界面主题色", 320);
    setMenu.addOptEvent(3, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["lightback"]));

    setMenu.addOpt("重置收藏列表", 320);
    setMenu.addOptEvent(4, KEY_A, setMenuClearFavList, NULL);

    setMenu.addOpt("重置黑名单", 320);
    setMenu.addOptEvent(5, KEY_A, setMenuClearBlackList, NULL);

#ifdef ENABLE_GD
    setMenu.addOpt("重载云端存储列表", 320);
    setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuReloadDriveList, NULL);
#endif

    setSubMenu.addOpt("修改 PlayCoin", 320);
    setSubMenu.addOptEvent(0, KEY_A, setMenuHackPlayCoin, NULL);

    setSubMenu.addOpt("清除步数历史记录", 320);
    setSubMenu.addOptEvent(1, KEY_A, setMenuClearStepHistory, NULL);

    setSubMenu.addOpt("清除游玩时间历史记录以及软件图鉴", 320);
    setSubMenu.addOptEvent(2, KEY_A, setMenuClearSoftwareLibraryAndPlayHistory, NULL);

    setSubMenu.addOpt("清除共享图标缓存", 320);
    setSubMenu.addOptEvent(3, KEY_A, setMenuClearSharedIconCache, NULL);

    setSubMenu.addOpt("清除主菜单图标缓存", 320);
    setSubMenu.addOptEvent(4, KEY_A, setMenuClearHomeMenuIconCache, NULL);

    setSubMenu.addOpt("重置试玩版游戏游玩计数", 320);
    setSubMenu.addOptEvent(5, KEY_A, setMenuResetDemoPlayCount, NULL);

    setSubMenu.addOpt("清除游戏笔记", 320);
    setSubMenu.addOptEvent(6, KEY_A, setMenuClearGameNotes, NULL);

    setSubMenu.addOpt("清除软件更新通知", 320);
    setSubMenu.addOptEvent(7, KEY_A, setMenuRemoveSoftwareUpdateNag, NULL);

    setSubMenu.addOpt("打开所有软件礼包", 320);
    setSubMenu.addOptEvent(8, KEY_A, setMenuUnpackWrappedSoftware, NULL);

    setSubMenu.addOpt("修改今日步数", 320);
    setSubMenu.addOptEvent(9, KEY_A, setMenuHackStepCount, NULL);

    setSubMenu.setCallback(setSubMenuCallback, NULL);

    t->finished = true;
}

void ui::setExit()
{
}

void ui::setUpdate()
{
    if (!setSubMenuOpen)
    {
        switch(ui::padKeysDown())
        {
            case KEY_CPAD_LEFT:
                ui::state = SHR;
                break;

            case KEY_CPAD_RIGHT:
                ui::state = USR;
                break;

            case KEY_Y:
                setSubMenuOpen = true;
                break;
        }

        setMenu.editOpt(1, "导出到 ZIP: " + getBoolText(std::get<bool>(cfg::config["zip"])));
        setMenu.editOpt(2, getDeflateLevelText(std::get<int>(cfg::config["deflateLevel"])));
        setMenu.editOpt(3, "界面主题色: " + getLightDarkText(std::get<bool>(cfg::config["lightback"])));

        setMenu.update();
    } else {
        setSubMenu.update();
    }
}

void ui::setDrawTop()
{
    if (!setSubMenuOpen)
    {
        ui::drawUIBar(TITLE_TEXT + "- 设置与杂项", ui::SCREEN_TOP, true);
        setMenu.draw(0, 22, 0xFFFFFFFF, 400, std::get<bool>(cfg::config["lightback"]));
    } else {
        ui::drawUIBar(TITLE_TEXT + "- 工具箱", ui::SCREEN_TOP, true);
        setSubMenu.draw(0, 22, 0xFFFFFFFF, 400, std::get<bool>(cfg::config["lightback"]));
    }
}

void ui::setDrawBottom()
{
    std::string setOptsDesc;
    if (!setSubMenuOpen)
    {
        switch(setMenu.getSelected())
        {
            case 0:
                setOptsDesc = "清除 Title 缓存并重新扫描应用列表。\n这或许需要耗费一定时间, 视 Title 数量而定。";
                break;
            case 1:
                setOptsDesc = "备份存档时是否使用 ZIP 压缩存储。";
                break;
            case 2:
                setOptsDesc = "选择 ZIP 压缩等级。\n(等级越高, 速度越慢, 压缩率越高)";
                break;
            case 3:
                setOptsDesc = "这将会清空收藏列表, 请谨慎操作。";
                break;
            case 4:
                setOptsDesc = "清空黑名单列表, 将会自动执行一次重载 Titles.";
                break;
            case 5:
                setOptsDesc = "从云端同步存档文件列表。\n这或许需要耗费一定时间, 视网络环境而定。";
                break;
        }
        ui::drawUIBar("\ue000 选择/切换 \ue003 启动工具箱 \ue01A\ue077\ue019 视图类型", ui::SCREEN_BOT, false);
    } else {
        switch(setSubMenu.getSelected())
        {
            case 0:
                setOptsDesc = "修改游戏币数量。(范围: 0-300)";
                break;
            case 1:
                setOptsDesc = "这将清除回忆记录册中的总步数记录!\n该操作无法撤销!";
                break;
            case 2:
                setOptsDesc = "这将清除回忆记录册中的软件图鉴记录以及所有游玩\n时间历史记录!\n操作无法撤销!执行此操作前强烈建议备份系统存档\n页的回忆记录册数据!";
                break;
            case 3:
                setOptsDesc = "将使得回忆记录册中的软件图鉴列表被清空。\n操作无法撤销!执行此操作前强烈建议备份共享追加\n数据页的FB记录以及系统存档页的回忆记录册数据!\n恢复备份时需要同时恢复上述两项记录!";
                break;
            case 4:
                setOptsDesc = "清除主菜单图标缓存(老三需用本软件的Mode3版本)\n操作无法撤销!执行此操作前强烈建议备份追加数据\n页的主菜单(CTR-N-HMM*)记录!";
                break;
            case 5:
                setOptsDesc = "重置试玩版 (Demo) 应用的游玩次数。\n该操作无法撤销!";
                break;
            case 6:
                setOptsDesc = "清除所有游戏笔记数据。\n操作无法撤销!执行此操作前强烈建议备份系统存档\n页的游戏笔记数据!";
                break;
            case 7:
                setOptsDesc = "这将移除所有已安装程序启动前的更新提示。\n(直到主菜单再次联网下载软件版本数据库)\n注意: 这对某些游戏内置的版本检查无效!\n操作无法撤销!执行此操作前强烈建议备份共享追加\n数据页的FE记录!";
                break;
            case 8:
                setOptsDesc = "打开主菜单中所有的软件礼包。该操作无法撤销!";
                break;
            case 9:
                setOptsDesc = "修改主菜单上显示的今日步数数据。\n由于部分限制因素, 该项功能仅影响当前小时步数。\n此前产生的步数不会被修改!";
                break;
        }
        ui::drawUIBar("\ue000 选择 \ue001 退出工具箱", ui::SCREEN_BOT, false);
    }
    gfx::drawTextWrap(setOptsDesc, 8, 8, GFX_DEPTH_DEFAULT, 0.5f, 304, 0xFFFFFFFF);
}
