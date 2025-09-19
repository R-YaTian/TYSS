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

#include "fs.h"
#include "util.h"
#include "misc.h"

static u32 homemenuID[] = {0x00000082, 0x0000008f, 0x00000098, 0x00000098, 0x000000a1, 0x000000a9, 0x000000b1};

static Result Loader_SetAppModeToMode3()
{
    Handle loaderHandle;
    Result res = srvGetServiceHandle(&loaderHandle, "Loader");

    if (R_FAILED(res)) return res;

    u32 *cmdbuf = getThreadCommandBuffer();

    ControlApplicationMemoryModeOverrideConfig* mode = (ControlApplicationMemoryModeOverrideConfig*)&cmdbuf[1];

    memset(mode, 0, sizeof(ControlApplicationMemoryModeOverrideConfig));
    mode->enable_o3ds = true;
    mode->o3ds_mode = SYSMODE_DEV2;
    cmdbuf[0] = IPC_MakeHeader(0x101, 1, 0); // ControlApplicationMemoryModeOverride

    res = svcSendSyncRequest(loaderHandle);

    if (R_SUCCEEDED(res))
        res = cmdbuf[1];

    svcCloseHandle(loaderHandle);
    return res;
}

void misc::setPC()
{
    data::titleData tmp;
    tmp.setExtdata(0xF000000B);
    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        fs::fsfile playCoin(fs::getSaveArch(), "/gamecoin.dat", FS_OPEN_READ | FS_OPEN_WRITE);

        int coinAmount = 0;
        playCoin.seek(0x4, fs::seek_set);
        coinAmount = playCoin.getByte() | playCoin.getByte() << 8;

        coinAmount = util::getInt(getTxt("输入 0-300 之间的数值"), coinAmount, 300);
        if (coinAmount != -1)
        {
            playCoin.seek(-2, fs::seek_cur);
            playCoin.putByte(coinAmount);
            playCoin.putByte(coinAmount >> 8);
        }

        playCoin.close();
        fs::closeSaveArch();
    }
}

void misc::clearStepHistory(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除步数历史记录..."));

    Result ret;

    ptmSysmInit();
    ret = PTMSYSM_ClearStepHistory();
    ptmSysmExit();

    if (R_FAILED(ret))
        ui::showMessage(getTxt("清除步数历史记录失败!\n错误: 0x%08X"), (unsigned) ret);
    else
        ui::showMessage(getTxt("步数历史记录清除成功!"));

    t->finished = true;
}

void misc::clearSoftwareLibraryAndPlayHistory(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除游玩时间历史记录及软件图鉴..."));

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage(getTxt("获取系统区域失败,无法完成操作!\n错误: 0x%08X"), (unsigned) res);
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
            ui::showMessage(getTxt("清除软件图鉴记录失败!\n错误: 0x%08X"), (unsigned) res);
            t->finished = true;
            return;
        }

        ptmSysmInit();
        res = PTMSYSM_ClearPlayHistory();
        ptmSysmExit();

        if (R_FAILED(res))
            ui::showMessage(getTxt("清除游玩时间历史记录失败!\n错误: 0x%08X"), (unsigned) res);
        else
            ui::showMessage(getTxt("清除游玩时间历史记录及软件图鉴成功!"));
    }

    t->finished = true;
}

void misc::clearSharedIconCache(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除共享图标缓存数据..."));

    Result res, ret;
    data::titleData tmp;
    tmp.setExtdata(0xF000000B);

    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/idb.dat"));
        ret = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/idbt.dat"));

        fs::closeSaveArch();

        if (R_SUCCEEDED(res) && R_SUCCEEDED(ret))
            ui::showMessage(getTxt("清除共享图标缓存数据成功!"));
        else
            ui::showMessage(getTxt("清除共享图标缓存数据失败!\n代码(idb.dat): 0x%08X\n代码(idbt.dat): 0x%08X"), (unsigned) res, (unsigned) ret);
    }

    t->finished = true;
}

void misc::clearHomeMenuIconCache(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除主菜单图标缓存数据..."));

    Result res, ret;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage(getTxt("获取系统区域失败,无法完成操作!\n错误: 0x%08X"), (unsigned) res);
        t->finished = true;
        return;
    }

    data::titleData tmp;
    tmp.setExtdata(homemenuID[region]);

    if (fs::openArchive(tmp, ARCHIVE_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/Cache.dat"));
        ret = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/CacheD.dat"));

        fs::closeSaveArch();

        if (R_SUCCEEDED(res) && R_SUCCEEDED(ret))
            ui::showMessage(getTxt("清除主菜单图标缓存数据成功!"));
        else
            ui::showMessage(getTxt("清除主菜单图标缓存数据失败!\n代码(Cache.dat): 0x%08X\n代码(CacheD.dat): 0x%08X"), (unsigned) res, (unsigned) ret);
    }

    t->finished = true;
}

void misc::resetDemoPlayCount(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在重置试玩版游戏游玩计数..."));

    Result res = AMNET_DeleteAllDemoLaunchInfos();

    if (R_FAILED(res))
        ui::showMessage(getTxt("重置试玩版游戏游玩计数失败!\n错误: 0x%08X"), (unsigned) res);
    else
        ui::showMessage(getTxt("重置试玩版游戏游玩计数成功!"));

    t->finished = true;
}

void misc::clearGameNotes(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除游戏笔记..."));

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage(getTxt("获取系统区域失败,无法完成操作!\n错误: 0x%08X"), (unsigned) res);
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
            ui::showMessage(getTxt("清除游戏笔记数据失败!\n错误: 0x%08X"), (unsigned) res);
        else
            ui::showMessage(getTxt("清除游戏笔记数据成功!"));
    }

    t->finished = true;
}

void misc::removeSoftwareUpdateNag(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在清除软件更新通知..."));

    Result res;
    data::titleData tmp;
    tmp.setExtdata(0xF000000E);

    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        res = FSUSER_DeleteFile(fs::getSaveArch(), (FS_Path) fsMakePath(PATH_ASCII, "/versionList.dat"));
        fs::closeSaveArch();

        if (R_FAILED(res))
            ui::showMessage(getTxt("清除软件更新通知失败!\n错误: 0x%08X"), (unsigned) res);
        else
            ui::showMessage(getTxt("清除软件更新通知成功!"));
    }

    t->finished = true;
}

void misc::unpackWrappedSoftware(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在打开所有软件礼包..."));

    Result res;
    u8 region = 0;

    res = CFGU_SecureInfoGetRegion(&region);
    if (R_FAILED(res)) {
        ui::showMessage(getTxt("获取系统区域失败,无法完成操作!\n错误: 0x%08X"), (unsigned) res);
        t->finished = true;
        return;
    }

    data::titleData tmp;
    tmp.setExtdata(homemenuID[region]);

    if (fs::openArchive(tmp, ARCHIVE_EXTDATA, true))
    {
        fs::fsfile svHomemenu(fs::getSaveArch(), "/SaveData.dat", FS_OPEN_READ | FS_OPEN_WRITE);

        u8* flags = new u8[0x168];
        u32 readSize = 0;

        svHomemenu.seek(0xB48, fs::seek_set);
        readSize = svHomemenu.read(flags, 0x168);

        if (readSize == 0x168) {
            u32 writeSize = 0;
            memset(flags, 0, 0x168);
            svHomemenu.seek(0xB48, fs::seek_set);
            writeSize = svHomemenu.write(flags, 0x168);
            if (writeSize < 0x168)
                ui::showMessage(getTxt("主菜单数据写入失败,无法完成操作!"));
            else
                ui::showMessage(getTxt("所有的软件礼包打开成功!"));
        } else {
            ui::showMessage(getTxt("主菜单数据读取失败,无法完成操作!"));
        }

        delete flags;
        svHomemenu.close();
        fs::closeSaveArch();
    }

    t->finished = true;
}

void misc::hackStepCount(void *a)
{
    threadInfo *t = (threadInfo *)a;

    int stepValue = 0;

    // Get today step count
    data::titleData tmp;
    tmp.setExtdata(0xF000000B);
    if (fs::openArchive(tmp, ARCHIVE_SHARED_EXTDATA, true))
    {
        fs::fsfile gamecoin(fs::getSaveArch(), "/gamecoin.dat", FS_OPEN_READ);

        gamecoin.seek(0xC, fs::seek_set);
        stepValue = gamecoin.getByte() | gamecoin.getByte() << 8 | gamecoin.getByte() << 16;
        gamecoin.close();

        fs::closeSaveArch();
    }

    Result ret;
    u16 stepCount;

    ptmuInit();
    ret = PTMU_GetStepHistory(2, &stepCount);
    ptmuExit();

    if (R_FAILED(ret))
    {
        ui::showMessage(getTxt("获取当前小时步数失败!\n错误: 0x%08X"), (unsigned) ret);
        t->finished = true;
        return;
    } else {
        ui::showMessage(getTxt("今日步数为: %d\n当前小时步数为: %d"), stepValue, stepCount);
    }

    stepValue = stepValue - stepCount;
    stepCount = util::getInt(getTxt("输入 0-18000 之间的数值"), stepCount, 18000);

    ptmSysmInit();
    ret = PTMSYSM_SetStepHistory(2, &stepCount);
    ptmSysmExit();

    if (R_FAILED(ret)) {
        ui::showMessage(getTxt("修改当前小时步数失败!\n错误: 0x%08X"), (unsigned) ret);
    } else {
        ui::showMessage(getTxt("修改当前小时步数成功!\n今日总步数: %d"), stepValue + stepCount);
    }

    t->finished = true;
}

void misc::rebootToMode3(void *a)
{
    threadInfo *t = (threadInfo *)a;

    Result res = Loader_SetAppModeToMode3();

    if (R_SUCCEEDED(res))
    {
        nsInit();
        res = NS_RebootToTitle(1, 0x000400000B549300LL, SYSMODE_DEV2);
        nsExit();
    } else {
        ui::showMessage(getTxt("扩展内存模式配置失败!\n错误: 0x%08X"), (unsigned) res);
    }

    if (R_FAILED(res))
        ui::showMessage(getTxt("无法重启到扩展内存模式!\n错误: 0x%08X"), (unsigned) res);

    t->finished = true;
}
