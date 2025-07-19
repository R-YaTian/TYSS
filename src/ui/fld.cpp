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
#include <string>
#include <vector>

#include "ui.h"
#include "fs.h"
#include "util.h"
#include "type.h"
#include "cfg.h"
#include "gfx.h"

static ui::menu fldMenu;
static fs::dirList fldList;
static std::u16string targetDir;
static std::string uploadParent;

static void fldMenuNew_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在准备...");

    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, targetDir.data()), 0);

    std::u16string newFolder;
    uint32_t held = ui::padKeysHeld();

    if(held & KEY_L)
        newFolder = util::toUtf16(util::getDateString(util::DATE_FMT_YDM));
    else if(held & KEY_R)
        newFolder = util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
    else
        newFolder = util::safeString(util::toUtf16(util::getString("输入新备份名称", true)));

    if(!newFolder.empty()
        && cfg::config["zip"]
        && !data::curData.getExtInfos().isDSCard
        && !data::curData.isAGB())
    {
        std::u16string fullOut = targetDir + newFolder + util::toUtf16(".zip");
        std::u16string svOut = fullOut + util::toUtf16(".sv");
        fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found
        fs::copyArchToZipThreaded(fs::getSaveArch(), util::toUtf16("/"), fullOut);
    }
    else if(!newFolder.empty())
    {
        std::u16string fullOut = targetDir + newFolder;
        if (data::curData.isAGB()) {
            t->status->setStatus("正在备份 GBAVC 存档数据...");
            bool res = fs::pxiFileToSaveFile(fullOut);
            if (!res)
                ui::showMessage("GBAVC 存档数据无效, 备份失败!");
            else {
                ui::showMessage("GBAVC 存档数据备份成功!");
                if (cfg::config["rawvcsave"]) {
                    std::u16string savPath = fullOut + util::toUtf16(".bin");
                    fs::copyFileThreaded(fs::getSaveArch(), util::toUtf16("原始 GBAVC 存档数据"), fs::getSDMCArch(), savPath, false, true);
                }
            }
        } else if (!data::curData.getExtInfos().isDSCard) {
            std::u16string svOut = fullOut + util::toUtf16(".sv");
            fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found

            FS_Path crDir = fsMakePath(PATH_UTF16, fullOut.c_str());
            FSUSER_CreateDirectory(fs::getSDMCArch(), crDir, 0);
            fullOut += util::toUtf16("/");

            fs::copyDirToDirThreaded(fs::getSaveArch(), util::toUtf16("/"), fs::getSDMCArch(), fullOut, false);
        } else {
            t->status->setStatus("正在导出 DS 卡带存档数据...");
            std::u16string savPath = fullOut + util::toUtf16(".sav");
            CardType cardType = data::curData.getExtInfos().spiCardType;
            if (cardType != NO_CHIP)
                fs::backupSPI(savPath, cardType);
            else
                ui::showMessage("不支持该 DS 游戏卡带的存档芯片类型\n或是该卡带不存在存档芯片!");
        }
    }

    t->finished = true;
}

void fldMenuNew(void *a)
{
    ui::newThread(fldMenuNew_t, NULL, NULL);
}

void fldMenuOverwrite_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *in = (fs::dirItem *)t->argPtr;
    t->status->setStatus("正在覆盖存档...");
    if(in->isDir)
    {
        std::u16string overwrite = targetDir + in->name;
        std::u16string svOut = overwrite + util::toUtf16(".sv");
        fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found

        FS_Path delPath = fsMakePath(PATH_UTF16, overwrite.c_str());
        FSUSER_DeleteDirectoryRecursively(fs::getSDMCArch(), delPath);
        FSUSER_CreateDirectory(fs::getSDMCArch(), delPath, 0);
        overwrite += util::toUtf16("/");
        fs::copyDirToDir(fs::getSaveArch(), util::toUtf16("/"), fs::getSDMCArch(), overwrite, false, NULL);
    }
    else
    {
        std::u16string overwrite = targetDir + in->name;
        FS_Path targetPath = fsMakePath(PATH_UTF16, overwrite.c_str());
        FSUSER_DeleteFile(fs::getSDMCArch(), targetPath);

        if (data::curData.isAGB()) {
            if (util::endsWith(overwrite, util::toUtf16(".bin")))
                fs::copyFileThreaded(fs::getSaveArch(), util::toUtf16("原始 GBAVC 存档数据"), fs::getSDMCArch(), overwrite, false, true);
            else {
                t->status->setStatus("正在备份 GBAVC 存档数据...");
                std::u16string savPath = util::removeSuffix(overwrite, util::toUtf16(".sav"));
                bool res = fs::pxiFileToSaveFile(savPath);
                if (!res)
                    ui::showMessage("GBAVC 存档数据无效, 备份失败!");
                else
                    ui::showMessage("GBAVC 存档数据备份成功!");
            }
        } else if (!data::curData.getExtInfos().isDSCard) {
            std::u16string svOut = overwrite + util::toUtf16(".sv");
            fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found

            fs::copyArchToZipThreaded(fs::getSaveArch(), util::toUtf16("/"), overwrite);
        } else {
            t->status->setStatus("正在导出 DS 卡带存档数据...");
            CardType cardType = data::curData.getExtInfos().spiCardType;
            if (cardType != NO_CHIP)
                fs::backupSPI(overwrite, cardType);
            else
                ui::showMessage("不支持该 DS 游戏卡带的存档芯片类型\n或是该卡带不存在存档芯片!");
        }
    }
    t->finished = true;
}

void fldMenuOverwrite(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    std::string q = "是否覆盖 " + in->nameUTF8 + "?";
    ui::confirm(q, fldMenuOverwrite_t, NULL, a);
}

void fldMenuDelete_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *in = (fs::dirItem *)t->argPtr;
    t->status->setStatus("正在删除存档...");
    std::u16string del = targetDir + in->name;
    if(in->isDir)
        fs::delDirRec(fs::getSDMCArch(), del);
    else
        FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_UTF16, del.c_str()));

    ui::fldRefresh();

    t->finished = true;
}

void fldMenuDelete(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    std::string q = "你确定要删除 " + in->nameUTF8 + "?";
    ui::confirm(q, fldMenuDelete_t, NULL, a);
}

void fldMenuRestore_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *in = (fs::dirItem *)t->argPtr;
    fs::deleteSv(fs::getSaveMode(), data::curData);
    if(in->isDir)
    {
        std::u16string rest = targetDir + in->name + util::toUtf16("/");
        fs::delDirRec(fs::getSaveArch(), util::toUtf16("/"));
        fs::commitData(fs::getSaveMode());
        t->status->setStatus("正在恢复数据到存档位...");
        fs::copyDirToDir(fs::getSDMCArch(), rest, fs::getSaveArch(), util::toUtf16("/"), true, NULL);

        // Try to import secure value if exists
        std::u16string svIn = targetDir + in->name + util::toUtf16(".sv");
        fs::importSv(fs::getSaveMode(), svIn, data::curData);
    }
    else
    {
        if (data::curData.isAGB()) {
            std::u16string savPath = targetDir + in->name;
            if (util::endsWith(savPath, util::toUtf16(".bin")))
                fs::copyFileThreaded(fs::getSDMCArch(), savPath, fs::getSaveArch(), util::toUtf16("/"), false, true);
            else {
                t->status->setStatus("正在恢复 GBAVC 存档数据...");
                bool res = fs::saveFileToPxiFile(savPath);
                if (!res)
                    ui::showMessage("GBAVC 存档数据无效, 恢复失败!");
                else
                    ui::showMessage("GBAVC 存档数据恢复成功!");
            }
        } else if (!data::curData.getExtInfos().isDSCard) {
            fs::delDirRec(fs::getSaveArch(), util::toUtf16("/"));
            fs::commitData(fs::getSaveMode());

            std::u16string rest = targetDir + in->name;
            fs::copyZipToArchThreaded(fs::getSaveArch(), rest);
        } else {
            t->status->setStatus("正在恢复数据到 DS 游戏卡带...");
            CardType cardType = data::curData.getExtInfos().spiCardType;
            if (cardType != NO_CHIP)
                fs::restoreSPI(targetDir + in->name, cardType);
            else
                ui::showMessage("不支持该 DS 游戏卡带的存档芯片类型\n或是该卡带不存在存档芯片!");
        }
    }
    t->finished = true;
}

//Creates confirm that jumps to ^
void fldMenuRestore(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    std::string q = "你确定要恢复 " + in->nameUTF8 + "?";
    ui::confirm(q, fldMenuRestore_t, NULL, a);
}

#ifdef ENABLE_DRIVE
std::vector<drive::driveItem *> driveList;

void fldMenuUpload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *in = (fs::dirItem *)t->argPtr;
    std::u16string src = targetDir + in->name;
    std::string utf8Name = in->nameUTF8;
    t->status->setStatus("正在上传 " + utf8Name + "...");

    FS_Path srcPath = fsMakePath(PATH_UTF16, src.c_str());
    FS_Path tmpPath = fsMakePath(PATH_ASCII, "/TYSS/tmp.zip");
    FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), tmpPath);

    std::string ttlUTF8 = data::curData.getTitleUTF8();
    if(!fs::netDrive->dirExists(ttlUTF8, fs::currentDirID))
        fs::netDrive->createDir(ttlUTF8, fs::currentDirID);
    uploadParent = fs::netDrive->getFolderID(ttlUTF8, fs::currentDirID);

    FILE *upload = fopen("/TYSS/tmp.zip", "rb");
    if(fs::netDrive->fileExists(utf8Name, uploadParent))
    {
        std::string fileID = fs::netDrive->getFileID(utf8Name, uploadParent);
        fs::netDrive->updateFile(fileID, upload);
        if (fs::netDrive->getDriveType() == drive::DriveType::ADrive)
             fs::netDrive->uploadFile(utf8Name, uploadParent, upload);
        ui::showMessage("云端存储: 云盘中已存在该存档,已覆盖");
    } else
        fs::netDrive->uploadFile(utf8Name, uploadParent, upload);

    fclose(upload);
    FSUSER_RenameFile(fs::getSDMCArch(), tmpPath, fs::getSDMCArch(), srcPath);

    // Upload sv file (if found)
    std::u16string srcSV = targetDir + util::removeSuffix(in->name, util::toUtf16(".sav")) + util::toUtf16(".sv");
    if (fs::fsfexists(fs::getSDMCArch(), srcSV))
    {
        std::string utf8NameSV = util::removeSuffix(utf8Name, std::string(".sav")) + ".sv";
        t->status->setStatus("正在上传 " + utf8NameSV + "...");

        FS_Path srcPathSV = fsMakePath(PATH_UTF16, srcSV.c_str());
        FS_Path tmpSV = fsMakePath(PATH_ASCII, "/TYSS/tmp.sv");
        FSUSER_RenameFile(fs::getSDMCArch(), srcPathSV, fs::getSDMCArch(), tmpSV);

        upload = fopen("/TYSS/tmp.sv", "rb");
        if(fs::netDrive->fileExists(utf8NameSV, uploadParent))
        {
            std::string fileID = fs::netDrive->getFileID(utf8NameSV, uploadParent);
            fs::netDrive->updateFile(fileID, upload);
            if (fs::netDrive->getDriveType() == drive::DriveType::ADrive)
                fs::netDrive->uploadFile(utf8NameSV, uploadParent, upload);
        } else
            fs::netDrive->uploadFile(utf8NameSV, uploadParent, upload);
        
        fclose(upload);
        FSUSER_RenameFile(fs::getSDMCArch(), tmpSV, fs::getSDMCArch(), srcPathSV);
    }

    ui::fldRefresh();

    t->finished = true;
}

void fldMenuUpload(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    if(fs::netDrive && !in->isDir)
        ui::newThread(fldMenuUpload_t, a, NULL);
    else if (in->isDir)
        ui::showMessage("云端存储: 仅支持上传压缩包或SAV及BIN存档文件");
    else
        ui::showMessage("云端存储: 服务尚未初始化");
}

void fldMenuDriveDownload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::driveItem *in = (drive::driveItem *)t->argPtr;

    t->status->setStatus("正在下载 " + in->name + "...");

    std::u16string target = targetDir + util::toUtf16(in->name);
    FS_Path targetPath = fsMakePath(PATH_UTF16, target.c_str());
    FS_Path tmpPath = fsMakePath(PATH_ASCII, "/TYSS/tmp.zip");

    if (fs::fsfexists(fs::getSDMCArch(), target))
        FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_UTF16, target.c_str()));

    FILE *tmp = fopen("/TYSS/tmp.zip", "wb");
    fs::netDrive->downloadFile(in->id, tmp);
    fclose(tmp);
    FSUSER_RenameFile(fs::getSDMCArch(), tmpPath, fs::getSDMCArch(), targetPath);

    // Download sv file (if found)
    std::string svName = util::removeSuffix(in->name, std::string(".sav")) + ".sv";
    if (fs::netDrive->fileExists(svName, in->parent))
    {
        t->status->setStatus("正在下载 " + svName + "...");

        std::u16string targetSV = targetDir + util::toUtf16(svName);
        FS_Path targetPathSV = fsMakePath(PATH_UTF16, targetSV.c_str());
        FS_Path tmpPathSV = fsMakePath(PATH_ASCII, "/TYSS/tmp.sv");

        if (fs::fsfexists(fs::getSDMCArch(), targetSV))
            FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_UTF16, targetSV.c_str()));

        tmp = fopen("/TYSS/tmp.sv", "wb");
        std::string fileID = fs::netDrive->getFileID(svName, in->parent);
        fs::netDrive->downloadFile(fileID, tmp);
        fclose(tmp);
        FSUSER_RenameFile(fs::getSDMCArch(), tmpPathSV, fs::getSDMCArch(), targetPathSV);
    }

    ui::fldRefresh();

    t->finished = true;
}

void fldMenuDriveDownload(void *a)
{
    drive::driveItem *in = (drive::driveItem *)a;
    std::u16string checkPath = targetDir + util::toUtf16(in->name);
    if (fs::fsfexists(fs::getSDMCArch(), checkPath))
        ui::confirm("下载此存档将会替换 SD 卡中的数据.\n你确定仍要进行下载吗?", fldMenuDriveDownload_t, NULL, a);
    else
        ui::newThread(fldMenuDriveDownload_t, a, NULL);
}

void fldMenuDriveDelete_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::driveItem *in = (drive::driveItem *)t->argPtr;
    std::string tmpParent = in->parent;
    std::string tmpName = in->name;
    t->status->setStatus("正在删除 " + tmpName + "...");
    fs::netDrive->deleteFile(in->id);

    // Delete sv file (if found)
    std::string svName = util::removeSuffix(tmpName, std::string(".sav")) + ".sv";
    if (fs::netDrive->fileExists(svName, tmpParent))
    {
        t->status->setStatus("正在删除 " + svName + "...");
        std::string fileID = fs::netDrive->getFileID(svName, tmpParent);
        fs::netDrive->deleteFile(fileID);
    }

    ui::fldRefresh();
    t->finished = true;
}

void fldMenuDriveDelete(void *a)
{
    drive::driveItem *in = (drive::driveItem *)a;
    ui::confirm("你确定要删除云盘中的 " + in->name + " 文件吗?\n若相对应的.sv文件存在,将同时被删除!", fldMenuDriveDelete_t, NULL, a);
}

void fldMenuDriveRestore_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::driveItem *in = (drive::driveItem *)t->argPtr;
    t->status->setStatus("正在下载 " + in->name + "...");

    FILE *tmp = fopen("/TYSS/tmp.zip", "wb");
    fs::netDrive->downloadFile(in->id, tmp);
    fclose(tmp);

    // Download sv file to tmp (if found)
    bool svFound = false;
    std::string svName = util::removeSuffix(in->name, std::string(".sav")) + ".sv";
    if (fs::netDrive->fileExists(svName, in->parent))
    {
        t->status->setStatus("正在下载 " + svName + "...");
        tmp = fopen("/TYSS/tmp.zip.sv", "wb");
        std::string fileID = fs::netDrive->getFileID(svName, in->parent);
        fs::netDrive->downloadFile(fileID, tmp);
        fclose(tmp);
        svFound = true;
    }

    if (util::endsWith(in->name, std::string(".zip")))
    {
        t->status->setStatus("正在解压存档到存档位...");
        unzFile unz = unzOpen64("/TYSS/tmp.zip");
        fs::copyZipToArch(fs::getSaveArch(), unz, NULL);
        unzClose(unz);
        if (svFound)
            fs::importSv(fs::getSaveMode(), util::toUtf16("/TYSS/tmp.zip.sv"), data::curData);
    } else if (data::curData.isAGB()) {
        if (util::endsWith(in->name, std::string(".bin")))
        {
            t->status->setStatus("正在复制原始 GBAVC 存档数据到存档位...");
            fs::copyFile(fs::getSDMCArch(), util::toUtf16("/TYSS/tmp.zip"), fs::getSaveArch(), util::toUtf16("/"), false, true, NULL);
        } else {
            t->status->setStatus("正在恢复 GBAVC 存档数据...");
            bool res = fs::saveFileToPxiFile(util::toUtf16("/TYSS/tmp.zip"));
            if (!res)
                ui::showMessage("GBAVC 存档数据无效, 恢复失败!");
            else
                ui::showMessage("GBAVC 存档数据恢复成功!");
        }
    } else {
        t->status->setStatus("正在恢复数据到 DS 游戏卡带...");
        CardType cardType = data::curData.getExtInfos().spiCardType;
        if (cardType != NO_CHIP)
            fs::restoreSPI(util::toUtf16("/TYSS/tmp.zip"), cardType);
        else
            ui::showMessage("不支持该 DS 游戏卡带的存档芯片类型\n或是该卡带不存在存档芯片!");
    }

    FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, "/TYSS/tmp.zip"));
    if (svFound)
        FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, "/TYSS/tmp.zip.sv"));

    t->finished = true;
}

void fldMenuDriveRestore(void *a)
{
    drive::driveItem *in = (drive::driveItem *)a;
    ui::confirm("你确定要下载并恢复 " + in->name + "?", fldMenuDriveRestore_t, NULL, a);
}
#endif

void ui::fldInit(const std::u16string& _path, const std::string& _uploadParent, funcPtr _func, void *_args)
{
    fldMenu.reset();
    fldMenu.setCallback(_func, _args);
    fldList.reassign(fs::getSDMCArch(), _path, true);
    targetDir = _path;
    uploadParent = _uploadParent;

    fldMenu.addOpt("新建", 0);
    fldMenu.addOptEvent(0, KEY_A, fldMenuNew, NULL);

    int fldInd = 1;
#ifdef ENABLE_DRIVE
    if(fs::netDrive)
    {
        fs::netDrive->getListWithParent(uploadParent, driveList,
            [](const drive::driveItem& item) {
                return !util::endsWith(item.name, std::string(".sv"));
            }
        );

        for(unsigned i = 0; i < driveList.size(); i++, fldInd++)
        {
            fldMenu.addOpt("[云] " + driveList[i]->name, 320);

            fldMenu.addOptEvent(fldInd, KEY_A, fldMenuDriveDownload, driveList[i]);
            fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDriveDelete, driveList[i]);
            fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuDriveRestore, driveList[i]);
        }
    }
#endif
    for(unsigned i = 0; i < fldList.getCount(); i++, fldInd++)
    {
        fldMenu.addOpt(util::toUtf8(fldList.getItem(i)), 320);

        fs::dirItem *di = fldList.getDirItemAt(i);
        fldMenu.addOptEvent(fldInd, KEY_A, fldMenuOverwrite, di);
        fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDelete, di);
        fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuRestore, di);
#ifdef ENABLE_DRIVE
        if (fs::netDrive)
            fldMenu.addOptEvent(fldInd, cfg::config["swaplrfunc"] ? KEY_ZR : KEY_R, fldMenuUpload, di);
#endif
    }
}

void ui::fldRefresh()
{
    fldMenu.reset();
    fldList.reassign(fs::getSDMCArch(), targetDir, true);

    fldMenu.addOpt("新建", 0);
    fldMenu.addOptEvent(0, KEY_A, fldMenuNew, NULL);

    int fldInd = 1;
#ifdef ENABLE_DRIVE
    if(fs::netDrive)
    {
        fs::netDrive->getListWithParent(uploadParent, driveList,
            [](const drive::driveItem& item) {
                return !util::endsWith(item.name, std::string(".sv"));
            }
        );

        for(unsigned i = 0; i < driveList.size(); i++, fldInd++)
        {
            fldMenu.addOpt("[云] " + driveList[i]->name, 320);

            fldMenu.addOptEvent(fldInd, KEY_A, fldMenuDriveDownload, driveList[i]);
            fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDriveDelete, driveList[i]);
            fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuDriveRestore, driveList[i]);
        }
    }
#endif
    for(unsigned i = 0; i < fldList.getCount(); i++, fldInd++)
    {
        fldMenu.addOpt(util::toUtf8(fldList.getItem(i)), 320);

        fs::dirItem *di = fldList.getDirItemAt(i);
        fldMenu.addOptEvent(fldInd, KEY_A, fldMenuOverwrite, di);
        fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDelete, di);
        fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuRestore, di);
#ifdef ENABLE_DRIVE
        if (fs::netDrive)
            fldMenu.addOptEvent(fldInd, cfg::config["swaplrfunc"] ? KEY_ZR : KEY_R, fldMenuUpload, di);
#endif
    }
}

void ui::fldUpdate()
{
    fldMenu.update();
}

void ui::fldDraw()
{
    fldMenu.draw(0, 2, gfx::txtCont, 320);
}
