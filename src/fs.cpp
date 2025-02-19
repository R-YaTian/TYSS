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

#define buff_size 0x8000

static FS_Archive sdmcArch, saveArch;
static FS_ArchiveID saveMode = (FS_ArchiveID)0;

typedef struct 
{
    FS_Archive srcArch, dstArch;
    std::u16string src, dst;
    zipFile zip = NULL;
    unzFile unz = NULL;
    uint64_t offset = 0;
    bool commit = false;  
} cpyArgs;

typedef struct
{
    std::vector<data::titleData>* vect = NULL;
    uint32_t mode = 0;
} bakArgs;

void fs::createDir(const std::string& path)
{
    FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_ASCII, path.c_str()), 0);
}

void fs::init()
{
    FSUSER_OpenArchive(&sdmcArch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));

    createDir("/JKSV");
    createDir("/JKSV/Saves");
    createDir("/JKSV/SysSave");
    createDir("/JKSV/ExtData");
    createDir("/JKSV/Boss");
    createDir("/JKSV/Shared");
}

void fs::exit()
{
    FSUSER_CloseArchive(sdmcArch);
    FSUSER_CloseArchive(saveArch);
}

#ifdef ENABLE_GD
drive::gd *fs::gDrive = NULL;
std::string fs::jksmDirID, fs::usrSaveDirID, fs::extDataDirID, fs::sysSaveDirID, fs::bossExtDirID, fs::sharedExtID;

void fs::driveInit(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("正在启动 Google Drive...");
    fs::gDrive = new drive::gd(cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken);
    if(gDrive->hasToken())
    {
        ui::showMessage("GD Token 获取成功!");
        cfg::driveRefreshToken = gDrive->getRefreshToken();
        if(!cfg::driveAuthCode.empty())
            cfg::saveGD();

        gDrive->loadDriveList();
        if(!gDrive->dirExists(DRIVE_JKSM_DIR))
            gDrive->createDir(DRIVE_JKSM_DIR, "");
        
        jksmDirID = gDrive->getFolderID("JKSM");

        if(!gDrive->dirExists(DRIVE_USER_SAVE_DIR, jksmDirID))
            gDrive->createDir(DRIVE_USER_SAVE_DIR, jksmDirID);

        usrSaveDirID = gDrive->getFolderID(DRIVE_USER_SAVE_DIR, jksmDirID);

        if(!gDrive->dirExists(DRIVE_EXTDATA_DIR, jksmDirID))
            gDrive->createDir(DRIVE_EXTDATA_DIR, jksmDirID);

        extDataDirID = gDrive->getFolderID(DRIVE_EXTDATA_DIR, jksmDirID);

        if(!gDrive->dirExists(DRIVE_SYSTEM_DIR, jksmDirID))
            gDrive->createDir(DRIVE_SYSTEM_DIR, jksmDirID);

        sysSaveDirID = gDrive->getFolderID(DRIVE_SYSTEM_DIR, jksmDirID);

        if(!gDrive->dirExists(DRIVE_BOSS_DIR, jksmDirID))
            gDrive->createDir(DRIVE_BOSS_DIR, jksmDirID);

        bossExtDirID = gDrive->getFolderID(DRIVE_BOSS_DIR, jksmDirID);

        if(!gDrive->dirExists(DRIVE_SHARED_DIR, jksmDirID))
            gDrive->createDir(DRIVE_SHARED_DIR, jksmDirID);

        sharedExtID = gDrive->getFolderID(DRIVE_SHARED_DIR, jksmDirID);
    }
    else
    {
        delete gDrive;
        gDrive = NULL;
    }
    t->finished = true;
}

void fs::driveExit()
{
    if(fs::gDrive)
        delete fs::gDrive;
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

void fs::closeSaveArch()
{
    FSUSER_CloseArchive(saveArch);
}

FS_ArchiveID fs::getSaveMode()
{
    return saveMode;
}

bool fs::openArchive(data::titleData& dat, const uint32_t& mode, bool error, FS_Archive& arch)
{
    Result res = 0;
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
    }

    if(R_FAILED(res))
    {
        if(error)
            ui::showMessage("无法打开该存档位. 该存档类型可能不存在适用此 title 的数据.\n错误: 0x%08X", (unsigned)res);
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
    if(mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
    {
        Result res = FSUSER_ControlArchive(saveArch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
        if(R_FAILED(res))
            ui::showMessage("提交存档数据失败!\n错误: 0x%08X", (unsigned)res);
    }
}

void fs::deleteSv(const uint32_t& mode)
{
    if(data::curData.getMedia() != MEDIATYPE_GAME_CARD && mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
    {
        Result res = 0;
        u64 in = ((u64)SECUREVALUE_SLOT_SD << 32) | (data::curData.getUnique() << 8);
        u8 out;

        res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &in, 8, &out, 1);
        if(R_FAILED(res))
            ui::showMessage("删除安全值失败.\n错误: 0x%08X", (unsigned)res);
    }
}

void fs::exportSv(const uint32_t& mode, const std::u16string& _dst, const data::titleData& dat)
{
    if(dat.getMedia() != MEDIATYPE_GAME_CARD && mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
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
            ui::showMessage("获取安全值失败.\n错误: 0x%08X", (unsigned)res);
    }
}

void fs::importSv(const uint32_t& mode, const std::u16string& _src, const data::titleData& dat)
{
    if(dat.getMedia() != MEDIATYPE_GAME_CARD && mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
    {
        if (!fsfexists(getSDMCArch(), _src)) return; // do nothing if not found
        fs::fsfile src(getSDMCArch(), _src, FS_OPEN_READ);
        u64 value = 0;
        src.read(&value, sizeof(u64));
        src.close();

        Result res = 0;
        res = FSUSER_SetSaveDataSecureValue(value, SECUREVALUE_SLOT_SD, dat.getUnique(), (u8) (dat.getLow() & 0xFF));

        if(R_FAILED(res))
            ui::showMessage("导入安全值失败.\n错误: 0x%08X", (unsigned)res);
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

    error = FSUSER_OpenFile(&fHandle, _arch, fsMakePath(PATH_ASCII, _path.c_str()), openFlags, 0);
    if(R_SUCCEEDED(error))
    {
        FSFILE_GetSize(fHandle, &fSize);
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

    FSUSER_CreateFile(_arch, fsMakePath(PATH_ASCII, _path.c_str()), 0, crSize);
    error = FSUSER_OpenFile(&fHandle, _arch, fsMakePath(PATH_ASCII, _path.c_str()), openFlags, 0);
    if(R_SUCCEEDED(error))
    {
        FSFILE_GetSize(fHandle, &fSize);
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

    error = FSUSER_OpenFile(&fHandle, _arch, fsMakePath(PATH_UTF16, _path.c_str()), openFlags, 0);
    if(R_SUCCEEDED(error))
    {
        FSFILE_GetSize(fHandle, &fSize);
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

    FSUSER_CreateFile(_arch, fsMakePath(PATH_UTF16, _path.c_str()), 0, crSize);
    error = FSUSER_OpenFile(&fHandle, _arch, fsMakePath(PATH_UTF16, _path.c_str()), openFlags, 0);
    if(R_SUCCEEDED(error))
    {
        FSFILE_GetSize(fHandle, &fSize);
        open = true;
    }
}

fs::fsfile::~fsfile()
{
    if(open)
    {
        FSFILE_Close(fHandle);
        open = false;
    }
}

void fs::fsfile::close()
{
    if(open)
    {
        FSFILE_Close(fHandle);
        open = false;
    }
}

size_t fs::fsfile::read(void *buf, const uint32_t& max)
{
    uint32_t readOut = 0;

    if(R_FAILED(FSFILE_Read(fHandle, &readOut, offset, buf, max)))
    {
        if(readOut > max)
            readOut = max;

        //std::memset(buf, 0x00, max);
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
    FSFILE_Write(fHandle, &writeOut, offset, buf, size, FS_WRITE_FLUSH);
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
    FSFILE_Read(fHandle, NULL, offset, &ret, 1);
    ++offset;
    return ret;
}

void fs::fsfile::putByte(const uint8_t& put)
{
    FSFILE_Write(fHandle, NULL, offset, &put, 1, FS_WRITE_FLUSH);
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
        case seek_beg:
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
    a = arch;

    path = p;

    FSUSER_OpenDirectory(&d, a, fsMakePath(PATH_UTF16, p.data()));

    uint32_t read = 0;
    do
    {
        FS_DirectoryEntry ent;
        FSDIR_Read(d, &read, 1, &ent);
        if(read == 1)
        {
            fs::dirItem newEntry = {std::u16string((char16_t *)ent.name),
                                    util::toUtf8(std::u16string((char16_t *)ent.name)),
                                    ent.attributes == FS_ATTRIBUTE_DIRECTORY};
            entry.push_back(newEntry);
        }
    }
    while(read > 0);

    FSDIR_Close(d);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

fs::dirList::~dirList()
{
    entry.clear();
}

static bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.rfind(suffix) == (str.size() - suffix.size());
}

void fs::dirList::rescan()
{
    entry.clear();

    FSUSER_OpenDirectory(&d, a, fsMakePath(PATH_UTF16, path.data()));

    uint32_t read = 0;
    do
    {
        FS_DirectoryEntry ent;
        FSDIR_Read(d, &read, 1, &ent);
        if(read == 1)
        {
            fs::dirItem newEntry = {std::u16string((char16_t *)ent.name),
                                    util::toUtf8(std::u16string((char16_t *)ent.name)),
                                    ent.attributes == FS_ATTRIBUTE_DIRECTORY};
            if (newEntry.isDir || endsWith(newEntry.nameUTF8, ".zip"))
                entry.push_back(newEntry);
        }
    }
    while(read > 0);

    FSDIR_Close(d);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

void fs::dirList::reassign(const FS_Archive& arch, const std::u16string& p)
{
    entry.clear();

    path = p;
    a = arch;

    FSUSER_OpenDirectory(&d, a, fsMakePath(PATH_UTF16, path.data()));
    uint32_t read = 0;
    do
    {
        FS_DirectoryEntry ent;
        FSDIR_Read(d, &read, 1, &ent);
        if(read == 1)
        {
            fs::dirItem newEntry = {std::u16string((char16_t *)ent.name),
                                    util::toUtf8(std::u16string((char16_t *)ent.name)),
                                    ent.attributes == FS_ATTRIBUTE_DIRECTORY};
            if (newEntry.isDir || endsWith(newEntry.nameUTF8, ".zip"))
                entry.push_back(newEntry);
        }
    }
    while(read > 0);

    FSDIR_Close(d);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

void fs::copyFile(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, threadInfo *t)
{
    if(t)
        t->status->setStatus("正在复制 " + util::toUtf8(_src) +"...");

    fs::fsfile src(_srcArch, _src, FS_OPEN_READ);
    fs::fsfile dst(_dstArch, _dst, FS_OPEN_WRITE, src.getSize());
    if(!src.isOpen() || !dst.isOpen())
        return;

    size_t readIn = 0;
    uint32_t size = 0;
    size = src.getSize() > buff_size ? buff_size : src.getSize();
    uint8_t *buffer = new uint8_t[size];
    while((readIn = src.read(buffer, size)))
        dst.write(buffer, readIn);

    delete[] buffer;

    if(commit)
    {
        dst.close();
        fs::commitData(fs::getSaveMode());
    }
}

static void copyFile_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    cpyArgs *cpy = (cpyArgs *)t->argPtr;
    fs::copyFile(cpy->srcArch, cpy->src, cpy->dstArch, cpy->dst, cpy->commit, t);
    delete cpy;
    t->argPtr = NULL;
    t->drawFunc = NULL;
    t->finished = true;
}

void fs::copyFileThreaded(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit)
{
    cpyArgs *send = new cpyArgs;
    send->srcArch = _srcArch;
    send->src = _src;
    send->dstArch = _dstArch;
    send->dst = _dst;
    send->commit = commit;
    ui::newThread(copyFile_t, send, NULL);
}

void fs::copyDirToDir(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, threadInfo *t)
{
    fs::dirList srcList(_srcArch, _src);
    for(unsigned i = 0; i < srcList.getCount(); i++)
    {
        if(srcList.isDir(i))
        {
            std::u16string newSrc = _src + srcList.getItem(i) + util::toUtf16("/");
            std::u16string newDst = _dst + srcList.getItem(i) + util::toUtf16("/");
            fs::createDir(_dstArch, newDst.substr(0, newDst.length() - 1));
            fs::copyDirToDir(_srcArch, newSrc, _dstArch, newDst, commit, t);
        }
        else
        {
            std::u16string fullSrc = _src + srcList.getItem(i);
            std::u16string fullDst = _dst + srcList.getItem(i);
            fs::copyFile(_srcArch, fullSrc, _dstArch, fullDst, commit, t);
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

void fs::copyArchToZip(const FS_Archive& _arch, const std::u16string& _src, zipFile _zip, const std::u16string* _dir, threadInfo *t)
{
    fs::dirList *archList = new fs::dirList(_arch, _src);
    for(unsigned i = 0; i < archList->getCount(); i++)
    {
        if(archList->isDir(i))
        {
            std::u16string newSrc = _src + archList->getItem(i) + util::toUtf16("/");
            std::u16string newDir = archList->getItem(i) + util::toUtf16("/");
            if (_dir) newDir = *_dir + newDir; // Join existing path
            std::string dirname = util::toUtf8(newDir); // We should add the folder to zip first
            zipOpenNewFileInZip64(_zip, dirname.c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, std::get<int>(cfg::config["deflateLevel"]), 0);
            fs::copyArchToZip(_arch, newSrc, _zip, &newDir, t);
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
            if(t) t->status->setStatus("正在压缩 " + filename + "...");
            int openZip = zipOpenNewFileInZip64(_zip, filename.c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, std::get<int>(cfg::config["deflateLevel"]), 0);
            if(openZip == 0)
            {
                fs::fsfile readFile(_arch, _src + archList->getItem(i), FS_OPEN_READ);
                size_t readIn = 0;
                uint32_t size = 0;
                size = readFile.getSize() > buff_size ? buff_size : readFile.getSize();
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
    t->status->setStatus("正在压缩存档位到 zip 文件...");

    zipFile zip = zipOpen64("/JKSV/tmp.zip", 0);
    fs::copyArchToZip(cpy->srcArch, util::toUtf16("/"), zip, NULL, t);
    zipClose(zip, NULL);

    FS_Path srcPath = fsMakePath(PATH_ASCII, "/JKSV/tmp.zip");
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
            if(t) t->status->setStatus("正在解压 " + std::string(filename) + "...");
            if(unzOpenCurrentFile(_unz) == UNZ_OK)
            {
                std::u16string nameUTF16 = util::toUtf16(filename);
                std::u16string dstPathUTF16 = util::toUtf16("/") + nameUTF16;
                size_t pos = nameUTF16.find_last_of(L'/');
                if (pos != std::u16string::npos)
                {
                    fs::createDirRec(arch, dstPathUTF16.substr(0, dstPathUTF16.find_last_of(L'/') + 1));
                    if (pos == dstPathUTF16.length() - 1) continue; // if filename is a directory, skip write
                }
                fs::fsfile writeFile(arch, dstPathUTF16, FS_OPEN_WRITE, info.uncompressed_size);
                int readIn = 0;
                uint32_t size = 0;
                size = writeFile.getSize() > buff_size ? buff_size : writeFile.getSize();
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
    t->status->setStatus("正在解压数据到存档位...");

    FS_Path srcPath = fsMakePath(PATH_UTF16, cpy->src.c_str());
    FS_Path dstPath = fsMakePath(PATH_ASCII, "/JKSV/tmp.zip");

    FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), dstPath);

    unzFile unz = unzOpen64("/JKSV/tmp.zip");
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

void backupTitles_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    bakArgs *args = (bakArgs *)t->argPtr;
    std::vector<data::titleData>& vect = *args->vect;
    uint32_t mode = args->mode;

    for(unsigned i = 0; i < vect.size(); i++)
    {
        std::string copyStr = "正在处理 '" + util::toUtf8(vect[i].getTitle()) + "'...";
        ui::prog->setText(copyStr);
        ui::prog->update(i);

        FS_Archive _arch;
        if(fs::openArchive(vect[i], mode, true, _arch))
        {
            util::createTitleDir(vect[i], mode);
            std::u16string outpath = util::createPath(vect[i], mode) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));

            if(std::get<bool>(cfg::config["zip"]))
            {
                std::u16string fullOut = outpath + util::toUtf16(".zip");
                std::u16string svOut = fullOut + util::toUtf16(".sv");
                fs::exportSv(mode, svOut, vect[i]); // export secure value if found

                zipFile zip = zipOpen64("/JKSV/tmp.zip", 0);
                fs::copyArchToZip(_arch, util::toUtf16("/"), zip, NULL, NULL);
                zipClose(zip, NULL);

                FS_Path srcPath = fsMakePath(PATH_ASCII, "/JKSV/tmp.zip");
                FS_Path dstPath = fsMakePath(PATH_UTF16, fullOut.c_str());
                FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), dstPath);
            }
            else
            {
                std::u16string svOut = outpath + util::toUtf16(".sv");
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

void fs::backupTitles(std::vector<data::titleData>& vect, const uint32_t &mode)
{
    ui::prog->setMax(vect.size());
    bakArgs *send = new bakArgs;
    send->vect = &vect;
    send->mode = mode;
    ui::newThread(backupTitles_t, send, ui::progressBarDrawFunc, ZIP_THRD_STACK_SIZE);
}
