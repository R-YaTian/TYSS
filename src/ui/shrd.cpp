#include <3ds.h>
#include <vector>

#include "ui.h"
#include "data.h"
#include "util.h"
#include "gfx.h"

static ui::titleview *shrdView;
static bool fldOpen = false;
//These are hardcoded/specific
//Never change, don't need caching
static std::vector<data::titleData> shared;

//Stolen 3Dbrew descriptions
static const std::string sharedDesc[] =
{
    "F1: NAND JPEG/MPO 文件和 phtcache.bin 由相机应用程序存储在这里。这也包含 UploadData.dat.",
    "F2: 声音应用程序中的 NAND M4A 文件存储在这里.",
    "F9: 适用于通知的 SpotPass 内容存储.",
    "FB: 包含 idb.dat, idbt.dat, gamecoin.dat, ubll.lst, CFL_DB.dat 以及 CFL_OldDB.dat. 这些文件包含明文 Miis 数据和一些与播放/使用记录相关的数据 (也包括缓存的图标数据).",
    "FC: 包含 bashotorya.dat 以及 bashotorya2.dat.",
    "FD: 主页菜单的 SpotPass 内容数据存储.",
    "FE: 包含 versionlist.dat, 用于 7.0.0-13 系统引入的主页菜单软件更新通知."
};

static inline void addSharedEntry(const uint32_t& _id, const std::string& _icnTxt)
{
    char tmp[16];
    sprintf(tmp, "%08X", (unsigned)_id);

    data::titleData newShrd;
    newShrd.setExtdata(_id);
    newShrd.setTitle(util::toUtf16(tmp));
    C2D_Image icn = gfx::noIcon();
    newShrd.setIcon(icn);
    shared.push_back(newShrd);
}

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

static void shrdViewCallback(void *)
{
    switch(ui::padKeysDown())
    {
        case KEY_A:
        {
            data::titleData *t = &shared[shrdView->getSelected()];
            std::string uploadParent;
#ifdef ENABLE_GD
            if(fs::gDrive)
            {
                std::string ttl = util::toUtf8(shared[shrdView->getSelected()].getTitle());
                if(!fs::gDrive->dirExists(ttl, fs::sharedExtID))
                    fs::gDrive->createDir(ttl, fs::sharedExtID);

                uploadParent = fs::gDrive->getFolderID(ttl, fs::sharedExtID);
            }
#endif
            if(fs::openArchive(*t, ARCHIVE_SHARED_EXTDATA, false))
            {
                std::u16string targetDir = util::createPath(*t, ARCHIVE_SHARED_EXTDATA);
                ui::fldInit(targetDir, uploadParent, fldCallback, NULL);
                fldOpen = true;
            }
        }
        break;

        case KEY_CPAD_LEFT:
            ui::state = BOS;
            break;

        case KEY_CPAD_RIGHT:
            ui::state = SET;
            break;
    }
}

void ui::shrdInit(void *a)
{
    threadInfo *t = (threadInfo *)a;

    //Skip E0 since it doesn't open
    addSharedEntry(0xF0000001, "F1");
    addSharedEntry(0xF0000002, "F2");
    addSharedEntry(0xF0000009, "F9");
    addSharedEntry(0xF000000B, "FB");
    addSharedEntry(0xF000000C, "FC");
    addSharedEntry(0xF000000D, "FD");
    addSharedEntry(0xF000000E, "FE");
    shrdView = new titleview(shared, shrdViewCallback, NULL);
    t->finished = true;
}

void ui::shrdExit()
{
    for(data::titleData& d : shared)
        d.freeIcon();

    delete shrdView;
}

void ui::shrdUpdate()
{
    if(fldOpen)
        ui::fldUpdate();
    else
        shrdView->update();
}

void ui::shrdDrawTop()
{
    shrdView->draw();
    ui::drawUIBar(TITLE_TEXT + "- 共享追加数据", ui::SCREEN_TOP, true);
}

void ui::shrdDrawBot()
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
    else
    {
        gfx::drawTextWrap("3DBREW: " + sharedDesc[shrdView->getSelected()], 8, 8, GFX_DEPTH_DEFAULT, 0.5f, 304, 0xFFFFFFFF);
        ui::drawUIBar("\ue000 打开 \ue01A\ue077\ue019 存档类型", ui::SCREEN_BOT, false);
    }
}
