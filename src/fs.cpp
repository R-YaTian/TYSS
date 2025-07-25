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
#include <algorithm>
#include <cstring>
#include <cstdarg>

#include "fs.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"
#include "type.h"
#include "cfg.h"
#include "crypto.h"

#define IO_BUFF_SIZE 0x8000

static FS_Archive sdmcArch, saveArch;
static FS_ArchiveID saveMode = (FS_ArchiveID)0;
static std::u16string dataPath;
static Handle fsPxiHandle;
static const u32 pxiPath[5] = { 1,1,3,0,0 };

typedef struct 
{
    FS_Archive srcArch, dstArch;
    std::u16string src, dst;
    zipFile zip = NULL;
    unzFile unz = NULL;
    uint64_t offset = 0;
    bool commit = false;
    bool isPxi = false;
} cpyArgs;

typedef struct
{
    std::vector<data::titleData>* vect = NULL;
    uint32_t mode = 0;
    fs::BunchType type = fs::BunchType::Bunch_CTR;
} bakArgs;

void fs::createDir(const std::string& path)
{
    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_ASCII, path.c_str()), 0);
}

void fs::init()
{
    FSUSER_OpenArchive(&sdmcArch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    svcControlService(0, &fsPxiHandle, "PxiFS0");

    createDir("/TYSS");
    createDir("/TYSS/Saves");
    createDir("/TYSS/SysSave");
    createDir("/TYSS/ExtData");
    createDir("/TYSS/Boss");
    createDir("/TYSS/Shared");
    createDir("/cheats");
}

void fs::exit()
{
    FSUSER_CloseArchive(sdmcArch);
    svcCloseHandle(fsPxiHandle);
}

#ifdef ENABLE_DRIVE
std::unique_ptr<drive::DriveBase> fs::netDrive;
std::string fs::tyssDirID, fs::usrSaveDirID, fs::extDataDirID, fs::sysSaveDirID, fs::bossExtDirID, fs::sharedExtID;
std::string fs::currentDirID;

void fs::debugWriteDriveList(drive::DriveBase* driveBase)
{
    fs::fsfile list(fs::getSDMCArch(), "/TYSS/drive_list.txt", FS_OPEN_CREATE | FS_OPEN_WRITE);
    for(size_t i = 0; i < driveBase->getDriveListCount(); i++)
    {
        drive::driveItem di = *driveBase->getItemAt(i);
        list.writef("%s\nID:\t%s\n", di.name.c_str(), di.id.c_str());
        if (!di.parent.empty())
            list.writef("Parent:\t%s\n", di.parent.c_str());
        if (i != driveBase->getDriveListCount() - 1)
            list.writef("\n");
    }
}

void fs::driveInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(getTxt("正在启动云端存储服务..."));
    if (!cfg::driveClientID.empty() && !cfg::driveClientSecret.empty())
        netDrive = std::make_unique<drive::gd>(cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken);
    else {
        netDrive = std::make_unique<drive::adrive>(cfg::driveAuthCode, cfg::driveRefreshToken, cfg::driveDiskID);
        cfg::driveDiskID = static_cast<drive::adrive*>(netDrive.get())->getDriveID();
    }
    if(netDrive->hasToken())
    {
        cfg::driveRefreshToken = netDrive->getRefreshToken();
        if(!cfg::driveAuthCode.empty())
            cfg::saveDrive();

        netDrive->loadDriveList();

        if(!netDrive->dirExists(DRIVE_TYSS_DIR))
            netDrive->createDir(DRIVE_TYSS_DIR, "");

        tyssDirID = netDrive->getFolderID(DRIVE_TYSS_DIR);

        if(!netDrive->dirExists(DRIVE_USER_SAVE_DIR, tyssDirID))
            netDrive->createDir(DRIVE_USER_SAVE_DIR, tyssDirID);

        usrSaveDirID = netDrive->getFolderID(DRIVE_USER_SAVE_DIR, tyssDirID);

        if(!netDrive->dirExists(DRIVE_EXTDATA_DIR, tyssDirID))
            netDrive->createDir(DRIVE_EXTDATA_DIR, tyssDirID);

        extDataDirID = netDrive->getFolderID(DRIVE_EXTDATA_DIR, tyssDirID);

        if(!netDrive->dirExists(DRIVE_SYSTEM_DIR, tyssDirID))
            netDrive->createDir(DRIVE_SYSTEM_DIR, tyssDirID);

        sysSaveDirID = netDrive->getFolderID(DRIVE_SYSTEM_DIR, tyssDirID);

        if(!netDrive->dirExists(DRIVE_BOSS_DIR, tyssDirID))
            netDrive->createDir(DRIVE_BOSS_DIR, tyssDirID);

        bossExtDirID = netDrive->getFolderID(DRIVE_BOSS_DIR, tyssDirID);

        if(!netDrive->dirExists(DRIVE_SHARED_DIR, tyssDirID))
            netDrive->createDir(DRIVE_SHARED_DIR, tyssDirID);

        sharedExtID = netDrive->getFolderID(DRIVE_SHARED_DIR, tyssDirID);

        ui::showMessage(getTxt("云端存储: 服务初始化完成!"));
    } else {
        ui::showMessage(getTxt("云端存储: 服务初始化失败!\n请检查云端存储服务配置信息。"));
        netDrive.reset();
    }
    t->finished = true;
}

void fs::driveExit()
{
    if(netDrive)
        netDrive.reset();
}
#endif

FS_Archive fs::getSDMCArch()
{
    return sdmcArch;
}

FS_Archive fs::getSaveArch()
{
    return saveArch;
}

Handle fs::getPxiHandle()
{
    return fsPxiHandle;
}

void fs::closeSaveArch()
{
    FSUSER_CloseArchive(saveArch);
}

void fs::closePxiSaveArch()
{
    FSPXI_CloseArchive(fsPxiHandle, saveArch);
}

FS_ArchiveID fs::getSaveMode()
{
    return saveMode;
}

bool fs::openArchive(data::titleData& dat, const uint32_t& mode, bool error, FS_Archive& arch)
{
    Result res = -1;
    saveMode = (FS_ArchiveID) mode;

    switch(mode)
    {
        case ARCHIVE_USER_SAVEDATA:
            {
                uint32_t path[3] = {dat.getMedia(), dat.getLow(), dat.getHigh()};
                FS_Path binData = (FS_Path) {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&arch, ARCHIVE_USER_SAVEDATA, binData);
            }
            break;

        case ARCHIVE_SAVEDATA:
            res = FSUSER_OpenArchive(&arch, ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""));
            break;

        case ARCHIVE_EXTDATA:
            {
                uint32_t path[] = {MEDIATYPE_SD, dat.getExtData(), 0};
                FS_Path binData = (FS_Path) {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&arch, ARCHIVE_EXTDATA, binData);
            }
            break;

        case ARCHIVE_SYSTEM_SAVEDATA:
            {
                uint32_t path[2] = {MEDIATYPE_NAND, (0x00020000 | dat.getUnique())};
                FS_Path binData = {PATH_BINARY, 8, path};
                res = FSUSER_OpenArchive(&arch, ARCHIVE_SYSTEM_SAVEDATA, binData);
            }
            break;

        case ARCHIVE_BOSS_EXTDATA:
            {
                uint32_t path[3] = {MEDIATYPE_SD, dat.getExtData(), 0};
                FS_Path binData = {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&arch, ARCHIVE_BOSS_EXTDATA, binData);
            }
            break;

        case ARCHIVE_SHARED_EXTDATA:
            {
                uint32_t path[3] = {MEDIATYPE_NAND, dat.getExtData(), 0x00048000};
                FS_Path binPath  = {PATH_BINARY, 0xC, path};
                res = FSUSER_OpenArchive(&arch, ARCHIVE_SHARED_EXTDATA, binPath);
            }
            break;

        case ARCHIVE_NAND_TWL_FS:
            {
                res = FSUSER_OpenArchive(&arch, ARCHIVE_NAND_TWL_FS, fsMakePath(PATH_EMPTY, ""));
                if (R_SUCCEEDED(res))
                {
                    char* saveDir = new char[32];
                    sprintf(saveDir, "/title/%08lx/%08lx/data", (dat.getHigh() & 0x00000FFF) | 0x00030000, dat.getLow());
                    dataPath = util::toUtf16(saveDir);
                    Handle tmp;
                    res = FSUSER_OpenDirectory(&tmp, arch, fsMakePath(PATH_UTF16, dataPath.data()));
                    if (R_FAILED(res))
                        FSUSER_CloseArchive(arch);
                    else
                        FSDIR_Close(tmp);
                    delete[] saveDir;
                }
            }
            break;

        case ARCHIVE_SAVEDATA_AND_CONTENT:
            {
                uint32_t path[4] = {dat.getLow(), dat.getHigh(), dat.getMedia(), 0};
                FS_Path binPath  = {PATH_BINARY, 16, path};
                res = FSPXI_OpenArchive(fsPxiHandle, &arch, ARCHIVE_SAVEDATA_AND_CONTENT, binPath);
                if (R_SUCCEEDED(res))
                {
                    FSPXI_File tmp;
                    res = FSPXI_OpenFile(fsPxiHandle, &tmp, arch, {PATH_BINARY, 20, pxiPath}, FS_OPEN_READ, 0);
                    if (R_FAILED(res))
                        FSPXI_CloseArchive(fsPxiHandle, arch);
                    else
                        FSPXI_CloseFile(fsPxiHandle, tmp);
                }
            }
            break;
    }

    if(R_FAILED(res))
    {
        if(error)
            ui::showMessage(getTxt("无法打开该存档位. 该存档类型可能不存在适用此 title 的数据.\n错误: 0x%08X\nTitle: %s"), (unsigned)res, dat.getTitleUTF8().c_str());
        return false;
    }

    return true;
}

bool fs::openArchive(data::titleData& dat, const uint32_t& mode, bool error)
{
    return openArchive(dat, mode, error, saveArch);
}

void fs::commitData(const uint32_t& mode)
{
    if(mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA && mode != ARCHIVE_NAND_TWL_FS)
    {
        Result res = FSUSER_ControlArchive(saveArch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
        if(R_FAILED(res))
            ui::showMessage(getTxt("提交存档数据失败!\n错误: 0x%08X"), (unsigned)res);
    }
}

void fs::deleteSv(const uint32_t& mode, const data::titleData& dat)
{
    if(dat.getMedia() != MEDIATYPE_GAME_CARD
        && mode != ARCHIVE_EXTDATA
        && mode != ARCHIVE_BOSS_EXTDATA
        && mode != ARCHIVE_SHARED_EXTDATA
        && mode != ARCHIVE_NAND_TWL_FS)
    {
        Result res = 0;
        u64 in = ((u64)SECUREVALUE_SLOT_SD << 32) | (dat.getUnique() << 8);
        u8 out;

        res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &in, 8, &out, 1);
        if(R_FAILED(res))
            ui::showMessage(getTxt("删除安全值失败.\n错误: 0x%08X"), (unsigned)res);
    }
}

void fs::exportSv(const uint32_t& mode, const std::u16string& _dst, const data::titleData& dat)
{
    if(dat.getMedia() != MEDIATYPE_GAME_CARD
        && mode != ARCHIVE_EXTDATA
        && mode != ARCHIVE_BOSS_EXTDATA
        && mode != ARCHIVE_SHARED_EXTDATA
        && mode != ARCHIVE_NAND_TWL_FS)
    {
        Result res = 0;
        bool exists = false;
        u64 value = 0;
        res = FSUSER_GetSaveDataSecureValue(&exists, &value, SECUREVALUE_SLOT_SD, dat.getUnique(), (u8) (dat.getLow() & 0xFF));
        if(R_SUCCEEDED(res)) {
            if(!exists) return;
            fs::fsfile dst(getSDMCArch(), _dst, FS_OPEN_WRITE | FS_OPEN_CREATE, sizeof(u64));
            dst.write(&value, sizeof(u64));
            dst.close();
        }

        if(R_FAILED(res))
            ui::showMessage(getTxt("获取安全值失败.\n错误: 0x%08X"), (unsigned)res);
    }
}

void fs::importSv(const uint32_t& mode, const std::u16string& _src, const data::titleData& dat)
{
    if(dat.getMedia() != MEDIATYPE_GAME_CARD
        && mode != ARCHIVE_EXTDATA
        && mode != ARCHIVE_BOSS_EXTDATA
        && mode != ARCHIVE_SHARED_EXTDATA
        && mode != ARCHIVE_NAND_TWL_FS)
    {
        if (!fsfexists(getSDMCArch(), _src)) return; // do nothing if not found
        fs::fsfile src(getSDMCArch(), _src, FS_OPEN_READ);
        u64 value = 0;
        src.read(&value, sizeof(u64));
        src.close();

        Result res = 0;
        res = FSUSER_SetSaveDataSecureValue(value, SECUREVALUE_SLOT_SD, dat.getUnique(), (u8) (dat.getLow() & 0xFF));

        if(R_FAILED(res))
            ui::showMessage(getTxt("导入安全值失败.\n错误: 0x%08X"), (unsigned)res);
    }
}

bool fs::fsfexists(const FS_Archive& _arch, const std::string& _path)
{
    Handle tmp;
    FS_Path testPath = fsMakePath(PATH_ASCII, _path.c_str());
    Result res = FSUSER_OpenFile(&tmp, _arch, testPath, FS_OPEN_READ, 0);
    FSFILE_Close(tmp);
    return R_SUCCEEDED(res);
}

bool fs::fsfexists(const FS_Archive& _arch, const std::u16string& _path)
{
    Handle tmp;
    FS_Path testPath = fsMakePath(PATH_UTF16, _path.c_str());
    Result res = FSUSER_OpenFile(&tmp, _arch, testPath, FS_OPEN_READ, 0);
    FSFILE_Close(tmp);
    return R_SUCCEEDED(res);
}

void fs::resetPxiFile(const FS_Archive& _arch)
{
    fs::fsfile pxiFile(_arch, util::toUtf16("/"), FS_OPEN_WRITE);
    size_t size = pxiFile.getSize();
    uint8_t *buffer = new uint8_t[size];
    memset(buffer, 0, size);
    pxiFile.write(buffer, size);
    pxiFile.close();
    delete[] buffer;
    FSPXI_CloseArchive(fsPxiHandle, _arch);
}

void fs::delDirRec(const FS_Archive& _arch, const std::u16string& path)
{
    FS_Path delPath = fsMakePath(PATH_UTF16, path.c_str());
    FSUSER_DeleteDirectoryRecursively(_arch, delPath);
}

void fs::createDir(const FS_Archive& _arch, const std::u16string& _path)
{
    FS_Path createPath = fsMakePath(PATH_UTF16, _path.c_str());
    FSUSER_CreateDirectory(_arch, createPath, 0);
}

void fs::createDirRec(const FS_Archive& _arch, const std::u16string& path)
{
    size_t pos = path.find(L'/', 0) + 1;
    while((pos = path.find(L'/', pos)) != path.npos)
    {
        FSUSER_CreateDirectory(_arch, fsMakePath(PATH_UTF16, path.substr(0, pos).c_str()), 0);
        ++pos;
    }
}

fs::fsfile::fsfile(const FS_Archive& _arch, const std::string& _path, const uint32_t& openFlags)
{
    if(openFlags & FS_OPEN_CREATE)
    {
        FS_Path delPath = fsMakePath(PATH_ASCII, _path.c_str());
        FSUSER_DeleteFile(_arch, delPath);
    }

    if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT)
        error = FSPXI_OpenFile(fsPxiHandle, &fHandle, _arch, {PATH_BINARY, 20, pxiPath}, openFlags, 0);
    else
        error = FSUSER_OpenFile((Handle*) &fHandle, _arch, fsMakePath(PATH_ASCII, _path.c_str()), openFlags, 0);

    if(R_SUCCEEDED(error))
    {
        if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
            isPxi = true;
            FSPXI_GetFileSize(fsPxiHandle, fHandle, &fSize);
        } else
            FSFILE_GetSize((Handle) fHandle, &fSize);
        open = true;
    }
}

fs::fsfile::fsfile(const FS_Archive& _arch, const std::string& _path, const uint32_t& openFlags, const uint64_t& crSize)
{
    if(openFlags & FS_OPEN_CREATE)
    {
        FS_Path delPath = fsMakePath(PATH_ASCII, _path.c_str());
        FSUSER_DeleteFile(_arch, delPath);
    }

    if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
        FSPXI_CreateFile(fsPxiHandle, _arch, {PATH_BINARY, 20, pxiPath}, 0, crSize);
        error = FSPXI_OpenFile(fsPxiHandle, &fHandle, _arch, {PATH_BINARY, 20, pxiPath}, openFlags, 0);
    } else {
        FSUSER_CreateFile(_arch, fsMakePath(PATH_ASCII, _path.c_str()), 0, crSize);
        error = FSUSER_OpenFile((Handle*) &fHandle, _arch, fsMakePath(PATH_ASCII, _path.c_str()), openFlags, 0);
    }

    if(R_SUCCEEDED(error))
    {
        if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
            isPxi = true;
            FSPXI_GetFileSize(fsPxiHandle, fHandle, &fSize);
        } else
            FSFILE_GetSize((Handle) fHandle, &fSize);
        open = true;
    }
}

fs::fsfile::fsfile(const FS_Archive& _arch, const std::u16string& _path, const uint32_t& openFlags)
{
    if(openFlags & FS_OPEN_CREATE)
    {
        FS_Path delPath = fsMakePath(PATH_UTF16, _path.c_str());
        FSUSER_DeleteFile(_arch, delPath);
    }

    if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT)
        error = FSPXI_OpenFile(fsPxiHandle, &fHandle, _arch, {PATH_BINARY, 20, pxiPath}, openFlags, 0);
    else
        error = FSUSER_OpenFile((Handle*) &fHandle, _arch, fsMakePath(PATH_UTF16, _path.c_str()), openFlags, 0);

    if(R_SUCCEEDED(error))
    {
        if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
            isPxi = true;
            FSPXI_GetFileSize(fsPxiHandle, fHandle, &fSize);
        } else
            FSFILE_GetSize((Handle) fHandle, &fSize);
        open = true;
    }
}

fs::fsfile::fsfile(const FS_Archive& _arch, const std::u16string& _path, const uint32_t& openFlags, const uint64_t& crSize)
{
    if(openFlags & FS_OPEN_CREATE)
    {
        FS_Path delPath = fsMakePath(PATH_UTF16, _path.c_str());
        FSUSER_DeleteFile(_arch, delPath);
    }

    if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
        FSPXI_CreateFile(fsPxiHandle, _arch, {PATH_BINARY, 20, pxiPath}, 0, crSize);
        error = FSPXI_OpenFile(fsPxiHandle, &fHandle, _arch, {PATH_BINARY, 20, pxiPath}, openFlags, 0);
    } else {
        FSUSER_CreateFile(_arch, fsMakePath(PATH_UTF16, _path.c_str()), 0, crSize);
        error = FSUSER_OpenFile((Handle*) &fHandle, _arch, fsMakePath(PATH_UTF16, _path.c_str()), openFlags, 0);
    }

    if(R_SUCCEEDED(error))
    {
        if (_arch != getSDMCArch() && saveMode == ARCHIVE_SAVEDATA_AND_CONTENT) {
            isPxi = true;
            FSPXI_GetFileSize(fsPxiHandle, fHandle, &fSize);
        } else
            FSFILE_GetSize((Handle) fHandle, &fSize);
        open = true;
    }
}

fs::fsfile::~fsfile()
{
    if(open)
    {
        if (isPxi)
            FSPXI_CloseFile(fsPxiHandle, fHandle);
        else
            FSFILE_Close((Handle) fHandle);
        open = false;
    }
}

void fs::fsfile::close()
{
    if(open)
    {
        if (isPxi)
            FSPXI_CloseFile(fsPxiHandle, fHandle);
        else
            FSFILE_Close((Handle) fHandle);
        open = false;
    }
}

size_t fs::fsfile::read(void *buf, const uint32_t& max)
{
    uint32_t readOut = 0;
    Result res;

    if (isPxi)
        res = FSPXI_ReadFile(fsPxiHandle, fHandle, &readOut, offset, buf, max);
    else
        res = FSFILE_Read((Handle) fHandle, &readOut, offset, buf, max);

    if(R_FAILED(res))
    {
        if(readOut > max)
            readOut = max;
    }
    offset += readOut;
    return (size_t)readOut;
}

bool fs::fsfile::getLine(char *out, size_t max)
{
    if(offset >= fSize)
        return false;

    memset(out, 0, max);
    unsigned i = 0;
    char byte = 0;
    while(i < max && (byte = getByte()) != '\n')
        *out++ = byte;

    return true;
}

size_t fs::fsfile::write(const void* buf, const uint32_t& size)
{
    uint32_t writeOut = 0;
    if (isPxi)
        FSPXI_WriteFile(fsPxiHandle, fHandle, &writeOut, offset, buf, size, FS_WRITE_FLUSH);
    else
        FSFILE_Write((Handle) fHandle, &writeOut, offset, buf, size, FS_WRITE_FLUSH);
    offset += writeOut;
    return (size_t)writeOut;
}

void fs::fsfile::writef(const char *fmt, ...)
{
    char tmp[512];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);
    write(tmp, strlen(tmp));
}

uint8_t fs::fsfile::getByte()
{
    uint8_t ret = 0;
    if (isPxi)
        FSPXI_ReadFile(fsPxiHandle, fHandle, NULL, offset, &ret, 1);
    else
        FSFILE_Read((Handle) fHandle, NULL, offset, &ret, 1);
    ++offset;
    return ret;
}

void fs::fsfile::putByte(const uint8_t& put)
{
    if (isPxi)
        FSPXI_WriteFile(fsPxiHandle, fHandle, NULL, offset, &put, 1, FS_WRITE_FLUSH);
    else
        FSFILE_Write((Handle) fHandle, NULL, offset, &put, 1, FS_WRITE_FLUSH);
    ++offset;
}

bool fs::fsfile::eof()
{
    return offset < fSize ? false : true;
}

void fs::fsfile::seek(const int& pos, const uint8_t& seekFrom)
{
    switch(seekFrom)
    {
        case seek_set:
            offset = pos;
            break;

        case seek_cur:
            offset += pos;
            break;

        case seek_end:
            offset = fSize + pos;
            break;
    }
}

struct
{
    bool operator()(const fs::dirItem& a, const fs::dirItem& b)
    {
        if(a.isDir != b.isDir)
            return a.isDir;

        unsigned aLen = a.nameUTF8.length();
        unsigned bLen = b.nameUTF8.length();
        unsigned minLen = std::min(aLen, bLen);
        for(unsigned i = 0; i < minLen; i++)
        {
            int charA = std::tolower(a.nameUTF8[i]), charB = std::tolower(b.nameUTF8[i]);
            if(charA != charB)
                return charA < charB;
        }

        return aLen < bLen;
    }
} sortDirs;

fs::dirList::dirList(const FS_Archive& arch, const std::u16string& p)
{
    dirArch = arch;
    path = p;

    scanItem();
}

fs::dirList::~dirList()
{
    entry.clear();
}

void fs::dirList::scanItem(bool filter)
{
    FSUSER_OpenDirectory(&dirHandle, dirArch, fsMakePath(PATH_UTF16, path.data()));

    uint32_t read = 0;
    do
    {
        FS_DirectoryEntry ent;
        FSDIR_Read(dirHandle, &read, 1, &ent);
        if(read == 1)
        {
            fs::dirItem newEntry = {std::u16string((char16_t *)ent.name),
                                    util::toUtf8(std::u16string((char16_t *)ent.name)),
                                    (ent.attributes & FS_ATTRIBUTE_DIRECTORY) != 0};
            if (!filter)
                entry.push_back(newEntry);
            else if (newEntry.isDir
                || util::endsWith(newEntry.nameUTF8, std::string(".zip"))
                || util::endsWith(newEntry.nameUTF8, std::string(".sav"))
                || util::endsWith(newEntry.nameUTF8, std::string(".bin")))
                entry.push_back(newEntry);
        }
    }
    while(read > 0);

    FSDIR_Close(dirHandle);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

void fs::dirList::reassign(const FS_Archive& arch, const std::u16string& p, bool filter)
{
    dirArch = arch;
    path = p;

    entry.clear();

    scanItem(filter);
}

void fs::copyFile(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, bool isPxi, threadInfo *t)
{
    if(t)
        t->status->setStatus(getTxt("正在复制 ") + util::toUtf8(_src) +"...");

    fs::fsfile src(_srcArch, _src, FS_OPEN_READ);

    if (isPxi && saveArch == _dstArch && src.isOpen())
    {
        fs::fsfile tmp(_dstArch, _dst, FS_OPEN_READ);
        if (src.getSize() != tmp.getSize())
        {
            ui::showMessage(getTxt("原始GBAVC存档数据大小不符,无法恢复!"));
            return;
        }
        tmp.close();
    }

    fs::fsfile dst(_dstArch, _dst, FS_OPEN_WRITE, src.getSize());
    if(!src.isOpen() || !dst.isOpen())
        return;

    size_t readIn = 0;
    uint32_t size = 0;
    size = src.getSize() > IO_BUFF_SIZE ? IO_BUFF_SIZE : src.getSize();
    uint8_t *buffer = new uint8_t[size];
    while((readIn = src.read(buffer, size)))
        dst.write(buffer, readIn);

    delete[] buffer;

    if(commit)
    {
        dst.close();
        fs::commitData(fs::getSaveMode());
    }
    if(isPxi) ui::fldRefresh();
}

static void copyFile_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    cpyArgs *cpy = (cpyArgs *)t->argPtr;
    fs::copyFile(cpy->srcArch, cpy->src, cpy->dstArch, cpy->dst, cpy->commit, cpy->isPxi, t);
    delete cpy;
    t->argPtr = NULL;
    t->drawFunc = NULL;
    t->finished = true;
}

void fs::copyFileThreaded(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, bool isPxi)
{
    cpyArgs *send = new cpyArgs;
    send->srcArch = _srcArch;
    send->src = _src;
    send->dstArch = _dstArch;
    send->dst = _dst;
    send->commit = commit;
    send->isPxi = isPxi;
    ui::newThread(copyFile_t, send, NULL);
}

void fs::copyDirToDir(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, threadInfo *t, bool isRecursion)
{
    std::u16string srcPath = _src;
    std::u16string dstPath = _dst;
    if (!isRecursion && saveMode == ARCHIVE_NAND_TWL_FS)
    {
        if (saveArch == _srcArch) srcPath = dataPath + _src;
        else if (saveArch == _dstArch) dstPath = dataPath + _dst;
    }

    fs::dirList srcList(_srcArch, srcPath);
    for(unsigned i = 0; i < srcList.getCount(); i++)
    {
        if(srcList.isDir(i))
        {
            std::u16string newSrc = srcPath + srcList.getItem(i) + util::toUtf16("/");
            std::u16string newDst = dstPath + srcList.getItem(i) + util::toUtf16("/");
            fs::createDir(_dstArch, newDst.substr(0, newDst.length() - 1));
            fs::copyDirToDir(_srcArch, newSrc, _dstArch, newDst, commit, t, true);
        }
        else
        {
            std::u16string fullSrc = srcPath + srcList.getItem(i);
            std::u16string fullDst = dstPath + srcList.getItem(i);
            fs::copyFile(_srcArch, fullSrc, _dstArch, fullDst, commit, false, t);
        }
    }
}

static void copyDirToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    cpyArgs *cpy = (cpyArgs *)t->argPtr;
    fs::copyDirToDir(cpy->srcArch, cpy->src, cpy->dstArch, cpy->dst, cpy->commit, t);
    delete cpy;

    ui::fldRefresh();

    t->argPtr = NULL;
    t->drawFunc = NULL;
    t->finished = true;
}

void fs::copyDirToDirThreaded(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit)
{
    cpyArgs *send = new cpyArgs;
    send->srcArch = _srcArch;
    send->src = _src;
    send->dstArch = _dstArch;
    send->dst = _dst;
    send->commit = commit;
    ui::newThread(copyDirToDir_t, send, NULL);
}

void fs::copyArchToZip(const FS_Archive& _arch, const std::u16string& _src, zipFile _zip, const std::u16string* _dir, threadInfo *t, bool isRecursion)
{
    std::u16string srcPath = _src;
    if (!isRecursion && saveMode == ARCHIVE_NAND_TWL_FS)
        srcPath = dataPath + _src;

    fs::dirList *archList = new fs::dirList(_arch, srcPath);
    for(unsigned i = 0; i < archList->getCount(); i++)
    {
        if(archList->isDir(i))
        {
            std::u16string newSrc = srcPath + archList->getItem(i) + util::toUtf16("/");
            std::u16string newDir = archList->getItem(i) + util::toUtf16("/");
            if (_dir) newDir = *_dir + newDir; // Join existing path
            std::string dirname = util::toUtf8(newDir); // We should add the folder to zip first
            zipOpenNewFileInZip64(_zip, dirname.c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, cfg::config["deflateLevel"], 0);
            fs::copyArchToZip(_arch, newSrc, _zip, &newDir, t, true);
        }
        else
        {
            time_t raw;
            time(&raw);
            tm *locTime = localtime(&raw);
            zip_fileinfo inf = { locTime->tm_sec, locTime->tm_min, locTime->tm_hour,
                                 locTime->tm_mday, locTime->tm_mon, (1900 + locTime->tm_year), 0, 0, 0 };

            std::string filename = util::toUtf8(archList->getItem(i));
            if (_dir) filename = util::toUtf8(*_dir) + filename; // Join dirname if exists
            if(t) t->status->setStatus(getTxt("正在压缩 ") + filename + "...");
            int openZip = zipOpenNewFileInZip64(_zip, filename.c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, cfg::config["deflateLevel"], 0);
            if(openZip == 0)
            {
                fs::fsfile readFile(_arch, srcPath + archList->getItem(i), FS_OPEN_READ);
                size_t readIn = 0;
                uint32_t size = 0;
                size = readFile.getSize() > IO_BUFF_SIZE ? IO_BUFF_SIZE : readFile.getSize();
                uint8_t *buff = new uint8_t[size];
                while((readIn = readFile.read(buff, size)))
                    zipWriteInFileInZip(_zip, buff, readIn);

                delete[] buff;
                readFile.close();
            }
        }
    }
    delete archList;
}

void copyArchToZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    cpyArgs *cpy = (cpyArgs *)t->argPtr;
    t->status->setStatus(getTxt("正在压缩存档位到 zip 文件..."));

    zipFile zip = zipOpen64("/TYSS/tmp.zip", 0);
    fs::copyArchToZip(cpy->srcArch, util::toUtf16("/"), zip, NULL, t);
    zipClose(zip, NULL);

    FS_Path srcPath = fsMakePath(PATH_ASCII, "/TYSS/tmp.zip");
    FS_Path dstPath = fsMakePath(PATH_UTF16, cpy->dst.c_str());
    FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), dstPath);

    delete cpy;

    ui::fldRefresh();

    t->finished = true;
}

void fs::copyArchToZipThreaded(const FS_Archive& _arch, const std::u16string& _src, const std::u16string& _dst)
{
    cpyArgs *send = new cpyArgs;
    send->srcArch = _arch;
    send->src = _src;
    send->dst = _dst;
    ui::newThread(copyArchToZip_t, send, NULL, ZIP_THRD_STACK_SIZE);
}

void fs::copyZipToArch(const FS_Archive& arch, unzFile _unz, threadInfo *t)
{
    if(unzGoToFirstFile(_unz) == UNZ_OK)
    {
        char filename[0x301];
        unz_file_info64 info;
        do
        {
            memset(filename, 0, 0x301);
            unzGetCurrentFileInfo64(_unz, &info, filename, 0x300, NULL, 0, NULL, 0);
            if(t) t->status->setStatus(getTxt("正在解压 ") + std::string(filename) + "...");
            if(unzOpenCurrentFile(_unz) == UNZ_OK)
            {
                std::u16string nameUTF16 = util::toUtf16(filename);
                std::u16string dstPathUTF16 = util::toUtf16("/") + nameUTF16;
                if (saveMode == ARCHIVE_NAND_TWL_FS) dstPathUTF16 = dataPath + dstPathUTF16;
                size_t pos = nameUTF16.find_last_of(L'/');
                if (pos != std::u16string::npos)
                {
                    fs::createDirRec(arch, dstPathUTF16.substr(0, dstPathUTF16.find_last_of(L'/') + 1));
                    if (pos == dstPathUTF16.length() - 1) continue; // if filename is a directory, skip write
                }
                fs::fsfile writeFile(arch, dstPathUTF16, FS_OPEN_WRITE, info.uncompressed_size);
                int readIn = 0;
                uint32_t size = 0;
                size = writeFile.getSize() > IO_BUFF_SIZE ? IO_BUFF_SIZE : writeFile.getSize();
                uint8_t *buff = new uint8_t[size];
                while((readIn = unzReadCurrentFile(_unz, buff, size)) > 0)
                    writeFile.write(buff, readIn);

                delete[] buff;
            }
        } while (unzGoToNextFile(_unz) != UNZ_END_OF_LIST_OF_FILE);
        fs::commitData(fs::getSaveMode());
    }
}

void copyZipToArch_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    cpyArgs *cpy = (cpyArgs *)t->argPtr;
    t->status->setStatus(getTxt("正在解压数据到存档位..."));

    FS_Path srcPath = fsMakePath(PATH_UTF16, cpy->src.c_str());
    FS_Path dstPath = fsMakePath(PATH_ASCII, "/TYSS/tmp.zip");

    FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), dstPath);

    unzFile unz = unzOpen64("/TYSS/tmp.zip");
    fs::copyZipToArch(cpy->dstArch, unz, t);
    unzClose(unz);

    FSUSER_RenameFile(fs::getSDMCArch(), dstPath, fs::getSDMCArch(), srcPath);

    // Try to import secure value if exists
    std::u16string svIn = cpy->src.c_str() + util::toUtf16(".sv");
    fs::importSv(fs::getSaveMode(), svIn, data::curData);

    delete cpy;
    t->lock();
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

void fs::copyZipToArchThreaded(const FS_Archive& _arch, const std::u16string& _src)
{
    cpyArgs *send = new cpyArgs;
    send->dstArch = _arch;
    send->src = _src;
    ui::newThread(copyZipToArch_t, send, NULL, ZIP_THRD_STACK_SIZE);
}

void fs::backupAGBSaves_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    bakArgs *args = (bakArgs *)t->argPtr;
    std::vector<data::titleData>& vect = *args->vect;
    uint32_t mode = args->mode;
    BunchType type = args->type;

    if (type != BunchType::Bunch_AGB)
        return;

    for(unsigned i = 0; i < vect.size(); i++)
    {
        if (!vect[i].isAGB()) continue;
        std::string copyStr = getTxt("正在处理 '") + vect[i].getTitleUTF8() + "'...";
        ui::prog->setText(copyStr);
        ui::prog->update(i);

        if(fs::openArchive(vect[i], ARCHIVE_SAVEDATA_AND_CONTENT, true))
        {
            util::createTitleDir(vect[i], mode);
            std::u16string outpath = util::createPath(vect[i], mode) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));

            bool res = fs::pxiFileToSaveFile(outpath);
            if (!res)
                ui::showMessage(getTxt("%s:\n存档数据无效, 备份失败!"), vect[i].getTitleUTF8().c_str());
            else {
                if (cfg::config["rawvcsave"]) {
                    std::u16string savPath = outpath + util::toUtf16(".bin");
                    fs::copyFile(fs::getSaveArch(), util::toUtf16(getTxt("原始 GBAVC 存档数据")), fs::getSDMCArch(), savPath, false, true, NULL);
                }
            }

            fs::closePxiSaveArch();
        }
    }

    t->argPtr = NULL;
    t->drawFunc = NULL;
    t->finished = true;
}

void fs::backupTitles_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    bakArgs *args = (bakArgs *)t->argPtr;
    std::vector<data::titleData>& vect = *args->vect;
    uint32_t mode = args->mode;
    BunchType type = args->type;

    for(unsigned i = 0; i < vect.size(); i++)
    {
        if (vect[i].getExtInfos().isDSCard) continue;
        if (vect[i].isAGB()) continue;
        if (type != BunchType::Bunch_TWL && (vect[i].getHigh() & 0x8000) == 0x8000) continue;
        if (type == BunchType::Bunch_TWL && (vect[i].getHigh() & 0x8000) != 0x8000) continue;
        std::string copyStr = getTxt("正在处理 '") + vect[i].getTitleUTF8() + "'...";
        ui::prog->setText(copyStr);
        ui::prog->update(i);

        FS_Archive _arch;
        if(fs::openArchive(vect[i], type == BunchType::Bunch_TWL ? ARCHIVE_NAND_TWL_FS : mode, true, _arch))
        {
            util::createTitleDir(vect[i], mode);
            std::u16string outpath = util::createPath(vect[i], mode) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));

            if(cfg::config["zip"])
            {
                std::u16string fullOut = outpath + util::toUtf16(".zip");
                std::u16string svOut = fullOut + util::toUtf16(".sv");
                if (type != BunchType::Bunch_TWL)
                    fs::exportSv(mode, svOut, vect[i]); // export secure value if found

                zipFile zip = zipOpen64("/TYSS/tmp.zip", 0);
                fs::copyArchToZip(_arch, util::toUtf16("/"), zip, NULL, NULL);
                zipClose(zip, NULL);

                FS_Path srcPath = fsMakePath(PATH_ASCII, "/TYSS/tmp.zip");
                FS_Path dstPath = fsMakePath(PATH_UTF16, fullOut.c_str());
                FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), dstPath);
            }
            else
            {
                std::u16string svOut = outpath + util::toUtf16(".sv");
                if (type != BunchType::Bunch_TWL)
                    fs::exportSv(mode, svOut, vect[i]); // export secure value if found

                FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, outpath.c_str()), 0);
                outpath += util::toUtf16("/");
                fs::copyDirToDir(_arch, util::toUtf16("/"), fs::getSDMCArch(), outpath, false, NULL);
            }

            FSUSER_CloseArchive(_arch);
        }
    }

    t->argPtr = NULL;
    t->drawFunc = NULL;
    t->finished = true;
}

void fs::backupTitles(std::vector<data::titleData>& vect, const uint32_t &mode, BunchType type)
{
    ui::prog->setMax(vect.size());
    bakArgs *send = new bakArgs;
    send->vect = &vect;
    send->mode = mode;
    send->type = type;
    ui::newThread(type == BunchType::Bunch_AGB ? backupAGBSaves_t : backupTitles_t, send, ui::progressBarDrawFunc, ZIP_THRD_STACK_SIZE);
}

void fs::backupSPI(const std::u16string& savPath, const CardType& cardType)
{
    u32 saveSize = SPIGetCapacity(cardType);
    u32 sectorSize = (saveSize < 0x10000) ? saveSize : 0x10000;
    u8* saveFile = new u8[saveSize];

    Result res;
    for (u32 i = 0; i < saveSize / sectorSize; ++i) {
        res = SPIReadSaveData(cardType, sectorSize * i, saveFile + sectorSize * i, sectorSize);
        if (R_FAILED(res)) break;
    }
    if (R_FAILED(res)) {
        delete[] saveFile;
        ui::showMessage(getTxt("读取 DS 卡带存档数据时发生错误!"));
        return;
    }

    fs::fsfile savFile(fs::getSDMCArch(), savPath, FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (savFile.isOpen())
    {
        savFile.write(saveFile, saveSize);
        savFile.close();
        ui::fldRefresh();
    }

    delete[] saveFile;
}

void fs::restoreSPI(const std::u16string& savPath, const CardType& cardType)
{
    u32 saveSize = SPIGetCapacity(cardType);
    u32 pageSize = SPIGetPageSize(cardType);
    u8* saveFile = new u8[saveSize];

    u32 readSize = 0;
    fs::fsfile savFile(fs::getSDMCArch(), savPath, FS_OPEN_READ);
    if (savFile.isOpen())
        readSize = savFile.read(saveFile, saveSize);

    if (!savFile.isOpen() || readSize != saveSize)
    {
        delete[] saveFile;
        ui::showMessage(getTxt("读取备份数据时发生错误!"));
        return;
    }
    savFile.close();

    Result res;
    for (u32 i = 0; i < saveSize / pageSize; ++i) {
        res = SPIWriteSaveData(cardType, pageSize * i, saveFile + pageSize * i, pageSize);
        if (R_FAILED(res)) break;
    }

    if (R_FAILED(res)) ui::showMessage(getTxt("写入数据到游戏卡带时发生错误!"));

    delete[] saveFile;
}

__attribute__((naked)) Result svcControlService(uint32_t op, uint32_t* outHandle, const char* name) {
    __asm__ volatile (
        "svc #0xB0 \n"  // call SVC 0xB0
        "bx lr \n"      // return
    );
}

// The following routine use code modify from PKSM:
bool fs::pxiFileToSaveFile(const std::u16string& _dst)
{
    std::shared_ptr<u8[]> data;
    size_t size;
    fs::fsfile in(getSaveArch(), util::toUtf16(""), FS_OPEN_READ);
    std::u16string savePath = _dst + util::toUtf16(".sav");
    std::u16string svPath = _dst + util::toUtf16(".sv");
    u64 a7RegistersValue;

    static constexpr u8 FULL_FS[0x20] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    std::unique_ptr<crypto::AGBSaveHeader> header1 = std::make_unique<crypto::AGBSaveHeader>();
    in.read(header1.get(), sizeof(crypto::AGBSaveHeader));
    if (!memcmp(header1.get(), FULL_FS, sizeof(FULL_FS)))
    {
        // If the first header is garbage FF, we have to search for the second. It can
        // only be at one of these possible sizes + 0x200 (for the size of the first
        // header)
        static constexpr u32 POSSIBLE_SAVE_SIZES[] = {
            GBASAVE_EEPROM_512,   // 4kbit
            GBASAVE_EEPROM_8K,    // 64kbit
            GBASAVE_SRAM_32K,     // 256kbit
            GBASAVE_FLASH_64K,    // 512kbit
            GBASAVE_FLASH_128K,   // 1024kbit/1Mbit
        };
        bool found = false;
        for (const auto& size : POSSIBLE_SAVE_SIZES)
        {
            // Go to the possible offset
            in.seek(size + sizeof(crypto::AGBSaveHeader), fs::seek_set);
            // Read what may be a header
            in.read(header1.get(), sizeof(crypto::AGBSaveHeader));
            // If it's a header, we found it! Break.
            if (!memcmp(header1->magic, ".SAV", 4))
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            // Seek back to the beginning of this header
            in.seek(-0x200, fs::seek_cur);
            std::array<u8, 32> hash = crypto::calcAGBSaveSHA256(in, header1->saveSize);
            std::array<u8, 16> cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, in.getHandle(), hash);
            bool invalid = (bool)memcmp(cmac.data(), header1->cmac, cmac.size());

            if (invalid)
            {
                in.close();
                return false;
            }
            else
            {
                size = header1->saveSize;
                memcpy(&a7RegistersValue, header1->arm7Registers, 8);
                data = std::shared_ptr<u8[]>(new u8[size]);
                // Always 0x200 after the second header
                in.seek(sizeof(crypto::AGBSaveHeader) * 2 + size, fs::seek_set);
                in.read(data.get(), size);
                in.close();
            }
        }
        // Reached end of file? No header present at all? Something weird happened; we
        // can't handle that
        else
        {
            in.close();
            return false;
        }
    }
    // Both headers are initialized. Compare CMACs and such
    else
    {
        std::unique_ptr<crypto::AGBSaveHeader> header2 = std::make_unique<crypto::AGBSaveHeader>();
        in.seek(header1->saveSize, fs::seek_cur);
        in.read(header2.get(), sizeof(crypto::AGBSaveHeader));

        // Check the first CMAC
        in.seek(0, fs::seek_set);
        std::array<u8, 32> hash = crypto::calcAGBSaveSHA256(in, header1->saveSize);
        std::array<u8, 16> cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, in.getHandle(), hash);
        bool firstInvalid       = (bool)memcmp(cmac.data(), header1->cmac, cmac.size());

        // Check the second CMAC
        in.seek(sizeof(crypto::AGBSaveHeader) + header1->saveSize, fs::seek_set);
        hash               = crypto::calcAGBSaveSHA256(in, header2->saveSize);
        cmac               = crypto::calcAGBSaveCMAC(fsPxiHandle, in.getHandle(), hash);
        bool secondInvalid = (bool)memcmp(cmac.data(), header2->cmac, cmac.size());

        if (firstInvalid)
        {
            // Both CMACs are invalid.
            if (secondInvalid)
            {
                in.close();
                return false;
            }
            // The second CMAC is the only valid one. Use it
            else
            {
                size = header2->saveSize;
                memcpy(&a7RegistersValue, header2->arm7Registers, 8);
                data = std::shared_ptr<u8[]>(new u8[size]);
                // Always 0x200 after the second header
                in.seek(sizeof(crypto::AGBSaveHeader) * 2 + size, fs::seek_set);
                in.read(data.get(), size);
                in.close();
            }
        }
        else
        {
            // The first CMAC is the only valid one. Use it
            if (secondInvalid)
            {
                size = header1->saveSize;
                memcpy(&a7RegistersValue, header1->arm7Registers, 8);
                data = std::shared_ptr<u8[]>(new u8[size]);
                // Always 0x200 after the first header
                in.seek(sizeof(crypto::AGBSaveHeader), fs::seek_set);
                in.read(data.get(), size);
                in.close();
            }
            else
            {
                // Will include rollover (header1->savesMade == 0xFFFFFFFF)
                // This is proper logic according to
                // https://github.com/d0k3/GodMode9/issues/494
                if (header2->savesMade == header1->savesMade + 1)
                {
                    size = header2->saveSize;
                    memcpy(&a7RegistersValue, header2->arm7Registers, 8);
                    data = std::shared_ptr<u8[]>(new u8[size]);
                    // Always 0x200 after the second header
                    in.seek(sizeof(crypto::AGBSaveHeader) * 2 + size, fs::seek_set);
                    in.read(data.get(), size);
                    in.close();
                }
                else
                {
                    size = header1->saveSize;
                    memcpy(&a7RegistersValue, header1->arm7Registers, 8);
                    data = std::shared_ptr<u8[]>(new u8[size]);
                    // Always 0x200 after the first header
                    in.seek(sizeof(crypto::AGBSaveHeader), fs::seek_set);
                    in.read(data.get(), size);
                    in.close();
                }
            }
        }
    }

    // byteswap for eeprom type saves (512 Byte / 8 KB)
    if (size == GBASAVE_EEPROM_512 || size == GBASAVE_EEPROM_8K) {
        for (u8* ptr = data.get(); (ptr - data.get()) < (int) size; ptr += 8)
            *(u64*) (void*) ptr = getbe64(ptr);
    }

    // Now we have the SAV data, save it to sdmc
    fs::fsfile saveFile(fs::getSDMCArch(), savePath, FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (saveFile.isOpen())
    {
        saveFile.write(data.get(), size);
        saveFile.close();

        // Export arm7RegistersValue to .sv (Saved registers Value) file.
        fs::fsfile svFile(getSDMCArch(), svPath, FS_OPEN_WRITE | FS_OPEN_CREATE, sizeof(u64));
        svFile.write(&a7RegistersValue, sizeof(u64));
        svFile.close();

        ui::fldRefresh();
        return true;
    }
    return false;
}

bool fs::saveFileToPxiFile(const std::u16string& _src)
{
    std::shared_ptr<u8[]> data;
    fs::fsfile src(getSDMCArch(), _src, FS_OPEN_READ);
    u64 savSize = src.getSize();

    if (!GBASAVE_VALID(savSize)) return false;

    fs::fsfile out(getSaveArch(), util::toUtf16(""), FS_OPEN_WRITE);
    u64 a7RegistersValue;
    std::u16string svPath = util::removeSuffix(_src, util::toUtf16(".sav")) + util::toUtf16(".sv");
    bool svFileFound = false;

    if (fsfexists(getSDMCArch(), svPath))
    {
        fs::fsfile svFile(getSDMCArch(), svPath, FS_OPEN_READ);
        svFile.read(&a7RegistersValue, sizeof(u64));
        svFile.close();
        svFileFound = true;
    }

    uint8_t *buffer = new uint8_t[savSize];
    src.read(buffer, savSize);
    src.close();

    // byteswap for eeprom type saves (512 Byte / 8 KB)
    if (savSize == GBASAVE_EEPROM_512 || savSize == GBASAVE_EEPROM_8K) {
        for (u8* ptr = buffer; (ptr - buffer) < (int) savSize; ptr += 8)
            *(u64*) (void*) ptr = getbe64(ptr);
    }

    static constexpr u8 FULL_FS[0x20] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    std::unique_ptr<crypto::AGBSaveHeader> header1 = std::make_unique<crypto::AGBSaveHeader>();
    out.read(header1.get(), sizeof(crypto::AGBSaveHeader));
    // If the top save is uninitialized, grab the bottom save's
    // header and copy it to the top's. Then write data
    if (!memcmp(header1.get(), FULL_FS, sizeof(FULL_FS)))
    {
        static constexpr u32 POSSIBLE_SAVE_SIZES[] = {
            GBASAVE_EEPROM_512,   // 4kbit
            GBASAVE_EEPROM_8K,    // 64kbit
            GBASAVE_SRAM_32K,     // 256kbit
            GBASAVE_FLASH_64K,    // 512kbit
            GBASAVE_FLASH_128K,   // 1024kbit/1Mbit
        };
        // Search for bottom header
        bool found = false;
        for (const auto& size : POSSIBLE_SAVE_SIZES)
        {
            // Go to the possible offset
            out.seek(size + sizeof(crypto::AGBSaveHeader), fs::seek_set);
            // Read what may be a header
            out.read(header1.get(), sizeof(crypto::AGBSaveHeader));
            // If it's a header, we found it! Break.
            if (!memcmp(header1->magic, ".SAV", 4))
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            // Doesn't matter whether this CMAC is valid or not. We
            // just need to update it
            out.seek(0, fs::seek_set);
            // Increment save count
            header1->savesMade++;
            if (svFileFound) memcpy(header1->arm7Registers, &a7RegistersValue, 8);
            out.write(header1.get(), sizeof(crypto::AGBSaveHeader));
            out.write(buffer, savSize);

            out.seek(0, fs::seek_set);
            std::array<u8, 32> hash = crypto::calcAGBSaveSHA256(out, header1->saveSize);
            std::array<u8, 16> cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
            out.seek(offsetof(crypto::AGBSaveHeader, cmac), fs::seek_set);
            out.write(cmac.data(), cmac.size());
            out.close();
            return true;
        }
    }
    // Otherwise, compare the top and bottom save counts. If we
    // loaded from the top, save in the bottom; if we loaded from
    // the bottom, save in the top
    else
    {
        std::unique_ptr<crypto::AGBSaveHeader> header2 = std::make_unique<crypto::AGBSaveHeader>();
        out.seek(header1->saveSize, fs::seek_cur);
        out.read(header2.get(), sizeof(crypto::AGBSaveHeader));

        // Check the first CMAC
        out.seek(0, fs::seek_set);
        std::array<u8, 32> hash = crypto::calcAGBSaveSHA256(out, header1->saveSize);
        std::array<u8, 16> cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
        bool firstInvalid = (bool)memcmp(cmac.data(), header1->cmac, cmac.size());

        // Check the second CMAC
        out.seek(sizeof(crypto::AGBSaveHeader) + header1->saveSize, fs::seek_set);
        hash = crypto::calcAGBSaveSHA256(out, header2->saveSize);
        cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
        bool secondInvalid = (bool)memcmp(cmac.data(), header2->cmac, cmac.size());

        if (firstInvalid)
        {
            // Just save to the first with header2->savesMade+1 as
            // save number for simplicity; whether or not the second
            // save was valid to begin with is immaterial
            header2->savesMade++;
            out.seek(0, fs::seek_set);
            if (svFileFound) memcpy(header2->arm7Registers, &a7RegistersValue, 8);
            out.write(header2.get(), sizeof(crypto::AGBSaveHeader));
            out.write(buffer, savSize);
            out.seek(0, fs::seek_set);

            hash = crypto::calcAGBSaveSHA256(out, header2->saveSize);
            cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
            out.seek(offsetof(crypto::AGBSaveHeader, cmac), fs::seek_set);
            out.write(cmac.data(), cmac.size());
            out.close();
            return true;
        }
        else
        {
            // If the second is valid and we loaded from it, save
            // over first save
            if (!secondInvalid &&
                header2->savesMade == header1->savesMade + 1)
            {
                header1->savesMade = header2->savesMade + 1;
                out.seek(0, fs::seek_set);
                if (svFileFound) memcpy(header1->arm7Registers, &a7RegistersValue, 8);
                out.write(header1.get(), sizeof(crypto::AGBSaveHeader));
                out.write(buffer, savSize);
                out.seek(0, fs::seek_set);

                hash = crypto::calcAGBSaveSHA256(out, header1->saveSize);
                cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
                out.seek(offsetof(crypto::AGBSaveHeader, cmac), fs::seek_set);
                out.write(cmac.data(), cmac.size());
                out.close();
                return true;
            }
            // Otherwise, save over the second save
            else
            {
                out.seek(sizeof(crypto::AGBSaveHeader) + header1->saveSize, fs::seek_set);
                header1->savesMade++;
                if (svFileFound) memcpy(header1->arm7Registers, &a7RegistersValue, 8);
                out.write(header1.get(), sizeof(crypto::AGBSaveHeader));
                out.write(buffer, savSize);
                out.seek(sizeof(crypto::AGBSaveHeader) + header1->saveSize, fs::seek_set);

                hash = crypto::calcAGBSaveSHA256(out, header1->saveSize);
                cmac = crypto::calcAGBSaveCMAC(fsPxiHandle, out.getHandle(), hash);
                out.seek(sizeof(crypto::AGBSaveHeader) + header1->saveSize + offsetof(crypto::AGBSaveHeader, cmac), fs::seek_set);
                out.write(cmac.data(), cmac.size());
                out.close();
                return true;
            }
        }
    }
    return false;
}
