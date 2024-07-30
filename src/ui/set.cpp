#include <3ds.h>

#include "ui.h"
#include "cfg.h"
#include "fs.h"

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

static void setMenuReloadTitles(void *a)
{
    remove("/JKSV/cache.bin");
    ui::newThread(data::loadTitles, NULL, NULL, 0x200000);
}

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

void ui::setInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    setMenu.addOpt("重载 Titles", 320);
    setMenu.addOptEvent(0, KEY_A, setMenuReloadTitles, NULL);

    setMenu.addOpt("重载 Google Drive 列表", 320);
    setMenu.addOptEvent(1, KEY_A, setMenuReloadDriveList, NULL);

    setMenu.addOpt("导出到 ZIP", 320);
    setMenu.addOptEvent(2, KEY_A, toggleBool, &cfg::config["zip"]);
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

    setMenu.editOpt(2, "导出到 ZIP: " + getBoolText(cfg::config["zip"]));

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