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
#include "cheatmanager.h"

static ui::menu setMenu, setSubMenu;
static bool setSubMenuOpen = false;

static std::unordered_map<std::string, u8> sconfig;
static bool anySettingChanged(const std::unordered_map<std::string, u8>& newSettings) {
    bool changed = false;
    for (const auto& [key, newValue] : newSettings) {
        auto it = sconfig.find(key);
        if (it == sconfig.end() || it->second != newValue) {
            changed = true;
            break;
        }
    }
    return changed;
}

static void toggleBool(void *b)
{
    bool *in = (bool *)b;
    if(*in)
        *in = false;
    else
        *in = true;
}

static void toggleUInt(void *b, int step, int max, const std::vector<int>& blacklist = {})
{
    int *in = (int *)b;
    *in = (*in + step) % (max + 1); 

    for (int x : blacklist) {
        if (*in == x) {
            *in = (*in + step) % (max + 1);
            break;
        }
    }

    if (*in < 0)
        *in = max;
}

#define getText getTxt

static std::string getLightDarkText(const bool& g)
{
    return g ? getText("浅色系") : getText("深色系");
}

static std::string getCheatDBText(const bool& g)
{
    return g ? getText("启动时") : getText("按需");
}

static std::string getBoolText(const bool& g)
{
    return g ? getText("开") : getText("关");
}

static std::string getUILangText(const int& g)
{
    std::string s = getText("界面语言");
    s += ": ";

    if (g == 0)
        s += "简体中文";
    else if (g == 1)
        s += "English";
    else if (g == 2)
        s += "繁體中文";

    return s;
}

static std::string getTitleLangText(const int& g)
{
    std::string s = getText("应用标题语言");
    s += ": ";

    switch (g) {
    case CFG_LANGUAGE_JP:
        s += "日本語";
        break;
    case CFG_LANGUAGE_EN:
        s += "English";
        break;
    case CFG_LANGUAGE_FR:
        s += "Français";
        break;
    case CFG_LANGUAGE_DE:
        s += "Deutsch";
        break;
    case CFG_LANGUAGE_IT:
        s += "Italiano";
        break;
    case CFG_LANGUAGE_ES:
        s += "Español";
        break;
    case CFG_LANGUAGE_ZH:
        s += "简体中文";
        break;
    case CFG_LANGUAGE_KO:
        s += "한국어";
        break;
    case CFG_LANGUAGE_NL:
        s += "Nederlands";
        break;
    case CFG_LANGUAGE_PT:
        s += "Português";
        break;
    case CFG_LANGUAGE_RU:
        s += "Русский";
        break;
    case CFG_LANGUAGE_TW:
        s += "繁體中文";
        break;
    }

    return s;
}

static std::string getCheatLangText(const int& g)
{
    std::string s = getText("内置金手指数据库语言");
    s += ": ";

    if (g == 0)
        s += "简体中文";
    else if (g == 1)
        s += "English";

    return s;
}

static std::string getDeflateLevelText(const int& g)
{
    std::string s = getText("ZIP 压缩等级");
    s += ": ";

    s += std::to_string(g);

    if (g == 1)
        s += getText(" (最快速度)");
    else if (g == 6)
        s += getText(" (标准压缩)");
    else if (g == 9)
        s += getText(" (极限压缩)");

    return s;
}

static void setMenuClearStepHistory(void *a)
{
    std::string q = getText("你确定要清除步数历史记录吗?");
    ui::confirm(q, misc::clearStepHistory, NULL, NULL);
}

static void setMenuClearSoftwareLibraryAndPlayHistory(void *a)
{
    std::string q = getText("确定要清除游玩时间历史记录及软件图鉴?");
    ui::confirm(q, misc::clearSoftwareLibraryAndPlayHistory, NULL, NULL);
}

static void setMenuClearSharedIconCache(void *a)
{
    std::string q = getText("你确定要清除共享图标缓存数据吗?");
    ui::confirm(q, misc::clearSharedIconCache, NULL, NULL);
}

static void setMenuClearHomeMenuIconCache(void *a)
{
    std::string q = getText("你确定要清除主菜单图标缓存数据吗?");
    ui::confirm(q, misc::clearHomeMenuIconCache, NULL, NULL);
}

static void setMenuResetDemoPlayCount(void *a)
{
    std::string q = getText("你确定要重置试玩版游戏的游玩计数吗?");
    ui::confirm(q, misc::resetDemoPlayCount, NULL, NULL);
}

static void setMenuClearGameNotes(void *a)
{
    std::string q = getText("你确定要清除所有的游戏笔记吗?");
    ui::confirm(q, misc::clearGameNotes, NULL, NULL);
}

static void setMenuRemoveSoftwareUpdateNag(void *a)
{
    std::string q = getText("你确定要清除所有的软件更新通知吗?");
    ui::confirm(q, misc::removeSoftwareUpdateNag, NULL, NULL);
}

static void setMenuUnpackWrappedSoftware(void *a)
{
    std::string q = getText("你确定要打开所有的软件礼包吗?");
    ui::confirm(q, misc::unpackWrappedSoftware, NULL, NULL);
}

static void setMenuHackStepCount(void *a)
{
    std::string q = getText("你确定要修改主菜单上显示的今日步数吗?");
    ui::confirm(q, misc::hackStepCount, NULL, NULL);
}

static void setMenuHackPlayCoin(void *a)
{
    misc::setPC();
}

static void setMenuClearFavList(void *a)
{
    std::string q = getText("你确定要重置收藏列表吗?");
    ui::confirm(q, data::clearFav, NULL, NULL);
}

static void setMenuClearBlackList(void *a)
{
    std::string q = getText("你确定要重置黑名单吗?");
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
    t->status->setStatus(getText("正在重载云端存储列表..."));
    fs::netDrive->loadDriveList();
    t->finished = true;
}

static void setMenuReloadDriveList(void *a)
{
    if (fs::netDrive)
        ui::newThread(setMenuReloadDriveList_t, NULL, NULL);
    else {
        std::string q = getText("云端存储: 服务尚未初始化\n是否立即启动服务并加载云端存储列表?");
        ui::confirm(q, fs::driveInit, NULL, NULL);
    }
}

static void setMenuToggleDriveBOOL_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getText("正在保存设置..."));
    toggleBool(t->argPtr);
    cfg::saveDrive();
    svcSleepThread(1e+9 / 4);
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

static void setMenuSaveCommon(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getText("正在保存设置..."));
    cfg::saveCommon();
    if (sconfig["cheatdblang"] != cfg::config["cheatdblang"] &&
        !util::fexists("/TYSS/cheats.json") && CheatManager::getInstance().cheats())
        CheatManager::getInstance().reset();
    sconfig = cfg::config;
    svcSleepThread(1e+9 / 4);
    t->finished = true;
}

static void setMenuToggleBOOL(void *a)
{
    toggleBool(a);
}

static void setMenuToggleColor(void *a)
{
    toggleBool(a);
    gfx::setColor(cfg::config["lightback"]);
}

static void setMenuIncreaseDeflateLevel(void *a)
{
    toggleUInt(a, 1, 9, {0});
}

static void setMenuDecreaseDeflateLevel(void *a)
{
    toggleUInt(a, -1, 9, {0});
}

static void setMenuIncreaseTitleLang(void *a)
{
    toggleUInt(a, 1, 11);
}

static void setMenuDecreaseTitleLang(void *a)
{
    toggleUInt(a, -1, 11);
}

static void setMenuIncreaseUILang(void *a)
{
    toggleUInt(a, 1, 2);
    cfg::setUILanguage(*(int*)a);
}

static void setMenuDecreaseUILang(void *a)
{
    toggleUInt(a, -1, 2);
    cfg::setUILanguage(*(int*)a);
}

static void setMenuToggleCheatLang(void *a)
{
    toggleUInt(a, 1, 1);
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

#undef getText

void ui::setInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    sconfig = cfg::config;

    setMenu.addOpt(getText("重载 Titles"), 320);
    setMenu.addOptEvent(0, KEY_A, setMenuReloadTitles, NULL);

    setMenu.addOpt(getText("导出到 ZIP"), 320);
    setMenu.addOptEvent(1, KEY_A, setMenuToggleBOOL, &cfg::config["zip"]);

    setMenu.addOpt(getText("ZIP 压缩等级"), 320);
    setMenu.addOptEvent(2, KEY_B, setMenuDecreaseDeflateLevel, &cfg::config["deflateLevel"]);
    setMenu.addOptEvent(2, KEY_A, setMenuIncreaseDeflateLevel, &cfg::config["deflateLevel"]);

    setMenu.addOpt(getText("界面主题色"), 320);
    setMenu.addOptEvent(3, KEY_A, setMenuToggleColor, &cfg::config["lightback"]);

    setMenu.addOpt(getText("界面语言"), 320);
    setMenu.addOptEvent(4, KEY_B, setMenuDecreaseUILang, &cfg::config["uilang"]);
    setMenu.addOptEvent(4, KEY_A, setMenuIncreaseUILang, &cfg::config["uilang"]);

    setMenu.addOpt(getText("应用标题语言"), 320);
    setMenu.addOptEvent(5, KEY_B, setMenuDecreaseTitleLang, &cfg::config["titlelang"]);
    setMenu.addOptEvent(5, KEY_A, setMenuIncreaseTitleLang, &cfg::config["titlelang"]);

    setMenu.addOpt(getText("内置金手指数据库语言"), 320);
    setMenu.addOptEvent(6, KEY_A, setMenuToggleCheatLang, &cfg::config["cheatdblang"]);

    setMenu.addOpt(getText("金手指数据库载入时机"), 320);
    setMenu.addOptEvent(7, KEY_A, setMenuToggleBOOL, &cfg::config["bootwithcheatdb"]);

    setMenu.addOpt(getText("GBAVC存档备份成功时保留原始数据"), 320);
    setMenu.addOptEvent(8, KEY_A, setMenuToggleBOOL, &cfg::config["rawvcsave"]);

    setMenu.addOpt(getText("切换LR按键功能"), 320);
    setMenu.addOptEvent(9, KEY_A, setMenuToggleBOOL, &cfg::config["swaplrfunc"]);

#ifdef ENABLE_DRIVE
    if(util::fexists("/TYSS/drive.json"))
    {
        setMenu.addOpt(getText("云端存储服务随软件启动"), 320);
        setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuToggleDriveBOOL, &cfg::driveInitOnBoot);

        setMenu.addOpt(getText("重载云端存储列表"), 320);
        setMenu.addOptEvent(setMenu.getCount() - 1, KEY_A, setMenuReloadDriveList, NULL);
    }
#endif

    setSubMenu.addOpt(getText("修改 PlayCoin"), 320);
    setSubMenu.addOptEvent(0, KEY_A, setMenuHackPlayCoin, NULL);

    setSubMenu.addOpt(getText("清除步数历史记录"), 320);
    setSubMenu.addOptEvent(1, KEY_A, setMenuClearStepHistory, NULL);

    setSubMenu.addOpt(getText("清除游玩时间历史记录以及软件图鉴"), 320);
    setSubMenu.addOptEvent(2, KEY_A, setMenuClearSoftwareLibraryAndPlayHistory, NULL);

    setSubMenu.addOpt(getText("清除共享图标缓存"), 320);
    setSubMenu.addOptEvent(3, KEY_A, setMenuClearSharedIconCache, NULL);

    setSubMenu.addOpt(getText("清除主菜单图标缓存"), 320);
    setSubMenu.addOptEvent(4, KEY_A, setMenuClearHomeMenuIconCache, NULL);

    setSubMenu.addOpt(getText("重置试玩版游戏游玩计数"), 320);
    setSubMenu.addOptEvent(5, KEY_A, setMenuResetDemoPlayCount, NULL);

    setSubMenu.addOpt(getText("清除游戏笔记"), 320);
    setSubMenu.addOptEvent(6, KEY_A, setMenuClearGameNotes, NULL);

    setSubMenu.addOpt(getText("清除软件更新通知"), 320);
    setSubMenu.addOptEvent(7, KEY_A, setMenuRemoveSoftwareUpdateNag, NULL);

    setSubMenu.addOpt(getText("打开所有软件礼包"), 320);
    setSubMenu.addOptEvent(8, KEY_A, setMenuUnpackWrappedSoftware, NULL);

    setSubMenu.addOpt(getText("修改今日步数"), 320);
    setSubMenu.addOptEvent(9, KEY_A, setMenuHackStepCount, NULL);

    setSubMenu.addOpt(getText("重置 TYSS 收藏列表"), 320);
    setSubMenu.addOptEvent(10, KEY_A, setMenuClearFavList, NULL);

    setSubMenu.addOpt(getText("重置 TYSS 黑名单"), 320);
    setSubMenu.addOptEvent(11, KEY_A, setMenuClearBlackList, NULL);

    setSubMenu.setCallback(setSubMenuCallback, NULL);

    t->finished = true;
}

void ui::setExit()
{
    cfg::saveCommon();
}

#define getText getTxt

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
        {
            if (anySettingChanged(cfg::config)) ui::newThread(setMenuSaveCommon, NULL, NULL);
            ui::state = SHR;
        }
        else if (down & KEY_PAGE_RIGHT)
        {
            if (anySettingChanged(cfg::config)) ui::newThread(setMenuSaveCommon, NULL, NULL);
            ui::state = USR;
        }

        setMenu.editOpt(1, getText("导出到 ZIP") + std::string(": ") + getBoolText(cfg::config["zip"]));
        setMenu.editOpt(2, getDeflateLevelText(cfg::config["deflateLevel"]));
        setMenu.editOpt(3, getText("界面主题色") + std::string(": ") + getLightDarkText(cfg::config["lightback"]));
        setMenu.editOpt(4, getUILangText(cfg::config["uilang"]));
        setMenu.editOpt(5, getTitleLangText(cfg::config["titlelang"]));
        setMenu.editOpt(6, getCheatLangText(cfg::config["cheatdblang"]));
        setMenu.editOpt(7, getText("金手指数据库载入时机") + std::string(": ") + getCheatDBText(cfg::config["bootwithcheatdb"]));
        setMenu.editOpt(8, getText("GBAVC存档备份成功时保留原始数据") + std::string(": ") + getBoolText(cfg::config["rawvcsave"]));
        setMenu.editOpt(9, getText("切换LR按键功能") + std::string(": ") + getBoolText(cfg::config["swaplrfunc"]));
#ifdef ENABLE_DRIVE
        if(util::fexists("/TYSS/drive.json"))
            setMenu.editOpt(setMenu.getCount() - 2, getText("云端存储服务随软件启动") + std::string(": ") + getBoolText(cfg::driveInitOnBoot));
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
        ui::drawUIBar(TITLE_TEXT + getText("- 设置与杂项"), ui::SCREEN_TOP, true);
        setMenu.draw(0, 22, gfx::txtCont, 400);
    } else {
        ui::drawUIBar(TITLE_TEXT + getText("- 工具箱"), ui::SCREEN_TOP, true);
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
                setOptsDesc = getText("清除 Title 缓存并重新扫描应用列表。\n这或许需要耗费一定时间, 视 Title 数量而定。");
                break;
            case 1:
                setOptsDesc = getText("备份存档时是否使用 ZIP 压缩存储。\n注:仅支持3DS应用(下载版与卡带)及DSiWare存档\n当备份DS游戏卡带或GBAVC存档时,无论是否开启\n该项设置,存档都将直接转储为.sav或.bin格式!");
                break;
            case 2:
                setOptsDesc = getText("选择 ZIP 压缩等级。\n(等级越高, 速度越慢, 压缩率越高)");
                break;
            case 3:
                setOptsDesc = getText("选择 UI 界面主题色。\n(浅色系/深色系)");
                break;
            case 4:
                setOptsDesc = getText("选择 UI 界面显示语言。");
                break;
            case 5:
                setOptsDesc = getText("以何种语言显示应用程序标题。\n这将影响存放应用程序数据备份的文件夹名称,\n一经设置不建议再修改,并且需要重载Titles才生效!");
                break;
            case 6:
                setOptsDesc = getText("选择内置金手指数据库语言(目前仅支持简中与英语)\n软件优先加载TYSS文件夹中外置金手指数据库文件\ncheats.json,该选项仅决定内置金手指数据库语言,\n当外置数据库存在时无效!\n该项设置更改时,已载入的金手指数据库会被卸载,\n并且将在需要使用时重载!");
                break;
            case 7:
                setOptsDesc = getText("决定金手指数据库的载入时机。\n可设置为按需加载(需要使用时再载入);\n或是应用程序启动时自动载入。\n若选择按需加载则首次检索金手指的耗时将增加。");
                break;
            case 8:
                setOptsDesc = getText("当GBAVC存档备份成功时保留一份原始(.bin)数据。\n一般情况下不需要开启此项,因为原始数据不可用于\nGBAVC虚拟主机之外的任何地方(GBA模拟器等等)\n注:覆盖备份文件或还原数据时,将根据后缀自动判断");
                break;
            case 9:
                setOptsDesc = getText("将LR按键的功能与ZLZR按键的功能对调。\n在老三上启用则LR按键将用于UI页面切换,而LR快速\n浏览应用列表功能将被禁用(由于老三无ZLZR按键)");
                break;
        }
#ifdef ENABLE_DRIVE
        if(util::fexists("/TYSS/drive.json"))
        {
            if ((unsigned) setMenu.getSelected() == setMenu.getCount() - 2)
                setOptsDesc = getText("云端存储服务是否随软件启动并加载云端存储列表。\n这可能导致应用程序启动耗时增加。");
            else if ((unsigned) setMenu.getSelected() == setMenu.getCount() - 1)
                setOptsDesc = getText("从云端同步存档文件列表。\n这或许需要耗费一定时间, 视网络环境而定。");
        }
#endif
        ui::drawUIBar(getText("\ue000 选择/切换 \ue003 启动工具箱 \ue01A\ue077\ue019 视图类型"), ui::SCREEN_BOT, false);
    } else {
        switch(setSubMenu.getSelected())
        {
            case 0:
                setOptsDesc = getText("修改游戏币数量。(范围: 0-300)");
                break;
            case 1:
                setOptsDesc = getText("这将清除回忆记录册中的总步数记录!\n该操作无法撤销!");
                break;
            case 2:
                setOptsDesc = getText("这将清除回忆记录册中的软件图鉴记录以及所有游玩\n时间历史记录!\n操作无法撤销!执行此操作前强烈建议备份系统存档\n页的回忆记录册数据!");
                break;
            case 3:
                setOptsDesc = getText("将使得回忆记录册中的软件图鉴列表被清空。\n操作无法撤销!执行此操作前强烈建议备份共享追加\n数据页的FB记录以及系统存档页的回忆记录册数据!\n恢复备份时需要同时恢复上述两项记录!");
                break;
            case 4:
                setOptsDesc = getText("清除主菜单图标缓存(老三需用本软件的Mode3版本)\n操作无法撤销!执行此操作前强烈建议备份追加数据\n页的主菜单(CTR-N-HMM*)记录!");
                break;
            case 5:
                setOptsDesc = getText("重置试玩版 (Demo) 应用的游玩次数。\n该操作无法撤销!");
                break;
            case 6:
                setOptsDesc = getText("清除所有游戏笔记数据。\n操作无法撤销!执行此操作前强烈建议备份系统存档\n页的游戏笔记数据!");
                break;
            case 7:
                setOptsDesc = getText("这将移除所有已安装程序启动前的更新提示。\n(直到主菜单再次联网下载软件版本数据库)\n注意: 这对某些游戏内置的版本检查无效!\n操作无法撤销!执行此操作前强烈建议备份共享追加\n数据页的FE记录!");
                break;
            case 8:
                setOptsDesc = getText("打开主菜单中所有的软件礼包。该操作无法撤销!");
                break;
            case 9:
                setOptsDesc = getText("修改主菜单上显示的今日步数数据。\n由于部分限制因素, 该项功能仅影响当前小时步数。\n此前产生的步数不会被修改!");
                break;
            case 10:
                setOptsDesc = getText("这将会清空本程序收藏列表, 请谨慎操作。");
                break;
            case 11:
                setOptsDesc = getText("清空本程序黑名单列表, 随后将自动执行重载 Titles");
                break;
        }
        ui::drawUIBar(getText("\ue000 选择 \ue001 退出工具箱"), ui::SCREEN_BOT, false);
    }
    gfx::drawTextWrap(setOptsDesc, 8, 8, GFX_DEPTH_DEFAULT, 0.5f, 304, gfx::txtCont);
}

#undef getText
