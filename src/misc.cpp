#include <3ds.h>

#include "util.h"
#include "misc.h"

void misc::setPC()
{
    data::titleData tmp;
    tmp.setExtdata(0xF000000B);
    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        fs::fsfile playCoin(fs::getSaveArch(), "/gamecoin.dat", FS_OPEN_READ | FS_OPEN_WRITE);

        int coinAmount = 0;
        playCoin.seek(0x4, fs::seek_beg);
        coinAmount = playCoin.getByte() | playCoin.getByte() << 8;

        coinAmount = util::getInt("输入 0-300 之间的数值", coinAmount, 300);
        if (coinAmount != -1)
        {
            playCoin.seek(-2, fs::seek_cur);
            playCoin.putByte(coinAmount);
            playCoin.putByte(coinAmount >> 8);
        }

        fs::closeSaveArch();
    }
}

void misc::clearStepHistory(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除步数历史记录...");

    Result ret;
    Handle ptmSysmHandle;
    srvGetServiceHandle(&ptmSysmHandle, "ptm:sysm");

    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x805,0,0); // 0x8050000

    ret = svcSendSyncRequest(ptmSysmHandle);

    svcCloseHandle(ptmSysmHandle);

    if (R_FAILED(ret))
        ui::showMessage("清除步数历史记录失败!\n错误: 0x%08X", (unsigned) ret);
    else
        ui::showMessage("步数历史记录清除成功!");

    t->finished = true;
}

void misc::clearSoftwareLibraryAndPlayHistory(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除游玩时间历史记录及软件图鉴...");

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage("获取系统区域失败,无法完成操作!\n错误: 0x%08X", (unsigned) res);
        t->finished = true;
        return;
    }

    u32 activitylogID[] = {0x0202, 0x0212, 0x0222, 0x0222, 0x0262, 0x0272, 0x0282};
    data::titleData tmp;
    tmp.setUnique(activitylogID[region]);

    if (fs::openArchive(tmp, ARCHIVE_SYSTEM_SAVEDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/pld.dat"));
        fs::commitData(fs::getSaveMode());
        fs::closeSaveArch();
        if (R_FAILED(res)) 
        {
            ui::showMessage("清除软件图鉴记录失败!\n错误: 0x%08X", (unsigned) res);
            t->finished = true;
            return;
        }

        u32 *cmdbuf = getThreadCommandBuffer();
        Handle ptmSysmHandle;
        srvGetServiceHandle(&ptmSysmHandle, "ptm:sysm");
        cmdbuf[0] = IPC_MakeHeader(0x80A,0,0); // 0x80A0000
        res = svcSendSyncRequest(ptmSysmHandle);
        svcCloseHandle(ptmSysmHandle);

        if (R_FAILED(res))
            ui::showMessage("清除游玩时间历史记录失败!\n错误: 0x%08X", (unsigned) res);
        else
            ui::showMessage("清除游玩时间历史记录及软件图鉴成功!");
    }

    t->finished = true;
}

void misc::clearSharedIconCache(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除共享图标缓存数据...");

    Result res;
    data::titleData tmp;
    tmp.setExtdata(0xF000000B);

    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/idb.dat"));
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/idbt.dat"));

        fs::closeSaveArch();

        if (R_FAILED(res))
            ui::showMessage("清除共享图标缓存数据失败!\n错误: 0x%08X", (unsigned) res);
        else
            ui::showMessage("清除共享图标缓存数据成功!");
    }

    t->finished = true;
}

void misc::clearHomeMenuIconCache(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除主菜单图标缓存数据...");

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage("获取系统区域失败,无法完成操作!\n错误: 0x%08X", (unsigned) res);
        t->finished = true;
        return;
    }

    u32 homemenuID[] = {0x00000082, 0x0000008f, 0x00000098, 0x00000098, 0x000000a1, 0x000000a9, 0x000000b1};
    data::titleData tmp;
    tmp.setExtdata(homemenuID[region]);

    if (fs::openArchive(tmp, ARCHIVE_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/Cache.dat"));
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/CacheD.dat"));

        fs::closeSaveArch();

        if (R_FAILED(res))
            ui::showMessage("清除主菜单图标缓存数据失败!\n错误: 0x%08X", (unsigned) res);
        else
            ui::showMessage("清除主菜单图标缓存数据成功!");
    }

    t->finished = true;
}

void misc::resetDemoPlayCount(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在重置试玩版游戏游玩计数...");

    Result res = AM_DeleteAllDemoLaunchInfos();

    if (R_FAILED(res))
        ui::showMessage("重置试玩版游戏游玩计数失败!\n错误: 0x%08X", (unsigned) res);
    else
        ui::showMessage("重置试玩版游戏游玩计数成功!");

    t->finished = true;
}

void misc::clearGameNotes(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除游戏笔记...");

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage("获取系统区域失败,无法完成操作!\n错误: 0x%08X", (unsigned) res);
        t->finished = true;
        return;
    }

    u32 gamenotesID[] = {0x0087, 0x0093, 0x009c, 0x009c, 0x00a5, 0x00ad, 0x00b5};
    data::titleData tmp;
    tmp.setUnique(gamenotesID[region]);

    if (fs::openArchive(tmp, ARCHIVE_SYSTEM_SAVEDATA, true))
    {
        char path[16];

        for (int i = 0; i < 16; i++) {
            snprintf(path, 13, "/memo/memo%02u", i);
            res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, path));
        }

        res = FSUSER_DeleteDirectory(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/memo/"));
        fs::commitData(fs::getSaveMode());
        fs::closeSaveArch();

        if (R_FAILED(res))
            ui::showMessage("清除游戏笔记数据失败!\n错误: 0x%08X", (unsigned) res);
        else
            ui::showMessage("清除游戏笔记数据成功!");
    }

    t->finished = true;
}

void misc::removeSoftwareUpdateNag(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在清除软件更新通知...");

    Result res;
    data::titleData tmp;
    tmp.setExtdata(0xF000000E);

    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/versionList.dat"));
        fs::closeSaveArch();

        if (R_FAILED(res))
            ui::showMessage("清除软件更新通知失败!\n错误: 0x%08X", (unsigned) res);
        else
            ui::showMessage("清除软件更新通知成功!");
    }

    t->finished = true;
}
