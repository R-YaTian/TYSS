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

static void IncreaseDeflateLevel(void *b)
{
    int *in = (int *)b;
    *in = (*in + 1) % 10; 

    if (*in == 0)
        *in = 1;
}

static void DecreaseDeflateLevel(void *b)
{
    int *in = (int *)b;
    *in = (*in - 1) % 10; 

    if (*in == 0)
        *in = 9;
}

static std::string getLightDarkText(const bool& g)
{
    return g ? "浅色系" : "深色系";
}

static std::string getCheatDBText(const bool& g)
{
    return g ? "启动时" : "按需";
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

#ifdef ENABLE_DRIVE
static void setMenuReloadDriveList_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在重载云端存储列表...");
    fs::netDrive->loadDriveList();
    t->finished = true;
}

static void setMenuReloadDriveList(void *a)
{
    if (fs::netDrive)
        ui::newThread(setMenuReloadDriveList_t, NULL, NULL);
    else {
        std::string q = "云端存储: 服务尚未初始化\n是否立即启动服务并加载云端存储列表?";
        ui::confirm(q, fs::driveInit, NULL, NULL);
    }
}

static void setMenuToggleDriveBOOL_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    toggleBool(t->argPtr);
    cfg::saveDrive();
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuToggleDriveBOOL(void *a)
{
    ui::newThread(setMenuToggleDriveBOOL_t, a, NULL);
}
#endif

static void setMenuToggleBOOL_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    toggleBool(t->argPtr);
    cfg::saveCommon();
    gfx::setColor(std::get<bool>(cfg::config["lightback"]));
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuToggleBOOL(void *a)
{
    ui::newThread(setMenuToggleBOOL_t, a, NULL);
}

static void setMenuIncreaseDeflateLevel_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    IncreaseDeflateLevel(t->argPtr);
    cfg::saveCommon();
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuIncreaseDeflateLevel(void *a)
{
    ui::newThread(setMenuIncreaseDeflateLevel_t, a, NULL);
}

static void setMenuDecreaseDeflateLevel_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在保存设置...");
    DecreaseDeflateLevel(t->argPtr);
    cfg::saveCommon();
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void setMenuDecreaseDeflateLevel(void *a)
{
    ui::newThread(setMenuDecreaseDeflateLevel_t, a, NULL);
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
    setMenu.addOptEvent(2, KEY_B, setMenuDecreaseDeflateLevel, &std::get<int>(cfg::config["deflateLevel"]));
    setMenu.addOptEvent(2, KEY_A, setMenuIncreaseDeflateLevel, &std::get<int>(cfg::config["deflateLevel"]));

    setMenu.addOpt("界面主题色", 320);
    setMenu.addOptEvent(3, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["lightback"]));

    setMenu.addOpt("GBAVC存档备份成功时保留原始数据", 320);
    setMenu.addOptEvent(4, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["rawvcsave"]));

    setMenu.addOpt("金手指数据库载入时机", 320);
    setMenu.addOptEvent(5, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["bootwithcheatdb"]));

    setMenu.addOpt("切换LR按键功能", 320);
    setMenu.addOptEvent(6, KEY_A, setMenuToggleBOOL, &std::get<bool>(cfg::config["swaplrfunc"]));

#ifdef ENABLE_DRIVE
    if(util::fexists("/TYSS/drive.json"))
    {
        setMenu.addOpt("云端存储服务随软件启动", 320);
        setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuToggleDriveBOOL, &cfg::driveInitOnBoot);

        setMenu.addOpt("重载云端存储列表", 320);
        setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuReloadDriveList, NULL);
    }
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

    setSubMenu.addOpt("重置 TYSS 收藏列表", 320);
    setSubMenu.addOptEvent(10, KEY_A, setMenuClearFavList, NULL);

    setSubMenu.addOpt("重置 TYSS 黑名单", 320);
    setSubMenu.addOptEvent(11, KEY_A, setMenuClearBlackList, NULL);

    setSubMenu.setCallback(setSubMenuCallback, NULL);

    t->finished = true;
}

void ui::setExit()
{
}

void ui::setUpdate()
{
    uint32_t down = ui::padKeysDown();
    if (!setSubMenuOpen)
    {
        switch(down)
        {
            case KEY_Y:
                setSubMenuOpen = true;
                break;
        }

        if (down & KEY_PAGE_LEFT)
            ui::state = SHR;
        else if (down & KEY_PAGE_RIGHT)
            ui::state = USR;

        setMenu.editOpt(1, "导出到 ZIP: " + getBoolText(std::get<bool>(cfg::config["zip"])));
        setMenu.editOpt(2, getDeflateLevelText(std::get<int>(cfg::config["deflateLevel"])));
        setMenu.editOpt(3, "界面主题色: " + getLightDarkText(std::get<bool>(cfg::config["lightback"])));
        setMenu.editOpt(4, "GBAVC存档备份成功时保留原始数据: " + getBoolText(std::get<bool>(cfg::config["rawvcsave"])));
        setMenu.editOpt(5, "金手指数据库载入时机: " + getCheatDBText(std::get<bool>(cfg::config["bootwithcheatdb"])));
        setMenu.editOpt(6, "切换LR按键功能: " + getBoolText(std::get<bool>(cfg::config["swaplrfunc"])));
#ifdef ENABLE_DRIVE
        if(util::fexists("/TYSS/drive.json"))
            setMenu.editOpt(setMenu.getCount() - 2, "云端存储服务随软件启动: " + getBoolText(cfg::driveInitOnBoot));
#endif

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
        setMenu.draw(0, 22, gfx::txtCont, 400);
    } else {
        ui::drawUIBar(TITLE_TEXT + "- 工具箱", ui::SCREEN_TOP, true);
        setSubMenu.draw(0, 22, gfx::txtCont, 400);
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
                setOptsDesc = "备份存档时是否使用 ZIP 压缩存储。\n注:仅支持3DS应用(下载版与卡带)及DSiWare存档\n当备份DS游戏卡带或GBAVC存档时,无论是否开启\n该项设置,存档都将直接转储为.sav或.bin格式!";
                break;
            case 2:
                setOptsDesc = "选择 ZIP 压缩等级。\n(等级越高, 速度越慢, 压缩率越高)";
                break;
            case 3:
                setOptsDesc = "选择 UI 界面主题色。\n(浅色/暗黑)";
                break;
            case 4:
                setOptsDesc = "当GBAVC存档备份成功时保留一份原始(.bin)数据。\n一般情况下不需要开启此项,因为原始数据不可用于\nGBAVC虚拟主机之外的任何地方(GBA模拟器等等)\n注:覆盖备份文件或还原数据时,将根据后缀自动判断";
                break;
            case 5:
                setOptsDesc = "决定金手指数据库的载入时机。\n可设置为按需加载(需要使用时再载入);\n或是应用程序启动时自动载入。\n若选择按需加载则首次检索金手指的耗时将增加。";
                break;
            case 6:
                setOptsDesc = "将LR按键的功能与ZLZR按键的功能对调。\n在老三上启用则LR按键将可用于UI页面切换,\n原始LR按键功能将不可用(由于老三无ZLZR按键)";
                break;
        }
#ifdef ENABLE_DRIVE
        if(util::fexists("/TYSS/drive.json"))
        {
            if ((unsigned) setMenu.getSelected() == setMenu.getCount() - 2)
                setOptsDesc = "云端存储服务是否随软件启动并加载云端存储列表。\n这可能导致应用程序启动耗时增加。";
            else if ((unsigned) setMenu.getSelected() == setMenu.getCount() - 1)
                setOptsDesc = "从云端同步存档文件列表。\n这或许需要耗费一定时间, 视网络环境而定。";
        }
#endif
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
            case 10:
                setOptsDesc = "这将会清空本程序收藏列表, 请谨慎操作。";
                break;
            case 11:
                setOptsDesc = "清空本程序黑名单列表, 随后将自动执行重载 Titles";
                break;
        }
        ui::drawUIBar("\ue000 选择 \ue001 退出工具箱", ui::SCREEN_BOT, false);
    }
    gfx::drawTextWrap(setOptsDesc, 8, 8, GFX_DEPTH_DEFAULT, 0.5f, 304, gfx::txtCont);
}
