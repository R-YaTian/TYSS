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

#define buff_size 128 * 1024

static FS_Archive sdmcArch, saveArch;
static FS_ArchiveID saveMode = (FS_ArchiveID)0;

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

bool fs::openArchive(data::titleData& dat, const uint32_t& arch, bool error)
{
    Result res = 0;
    saveMode = (FS_ArchiveID)arch;

    switch(arch)
    {
        case ARCHIVE_USER_SAVEDATA:
            {
                uint32_t path[3] = {dat.getMedia(), dat.getLow(), dat.getHigh()};
                FS_Path binData = (FS_Path) {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&saveArch, ARCHIVE_USER_SAVEDATA, binData);
            }
            break;

        case ARCHIVE_SAVEDATA:
            res = FSUSER_OpenArchive(&saveArch, ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, ""));
            break;

        case ARCHIVE_EXTDATA:
            {
                uint32_t path[] = {MEDIATYPE_SD, dat.getExtData(), 0};
                FS_Path binData = (FS_Path) {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&saveArch, ARCHIVE_EXTDATA, binData);
            }
            break;

        case ARCHIVE_SYSTEM_SAVEDATA:
            {
                uint32_t path[2] = {MEDIATYPE_NAND, (0x00020000 | dat.getUnique())};
                FS_Path binData = {PATH_BINARY, 8, path};
                res = FSUSER_OpenArchive(&saveArch, ARCHIVE_SYSTEM_SAVEDATA, binData);
            }
            break;

        case ARCHIVE_BOSS_EXTDATA:
            {
                uint32_t path[3] = {MEDIATYPE_SD, dat.getExtData(), 0};
                FS_Path binData = {PATH_BINARY, 12, path};
                res = FSUSER_OpenArchive(&saveArch, ARCHIVE_BOSS_EXTDATA, binData);
            }
            break;

        case ARCHIVE_SHARED_EXTDATA:
            {
                uint32_t path[3] = {MEDIATYPE_NAND, dat.getExtData(), 0x00048000};
                FS_Path binPath  = {PATH_BINARY, 0xC, path};
                res = FSUSER_OpenArchive(&saveArch, ARCHIVE_SHARED_EXTDATA, binPath);
            }
            break;
    }

    if(R_FAILED(res))
    {
        if(error)
            ui::showMessage("The archive could not be opened. The save data type may not exist for this title.\nError: 0x%08X", (unsigned)res);
        return false;
    }

    return true;
}

void fs::commitData(const uint32_t& mode)
{
    if(mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
    {
        Result res = FSUSER_ControlArchive(saveArch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
        if(res)
            ui::showMessage("Failed to commit save data!\nError: 0x%08X", (unsigned)res);
    }
}

void fs::deleteSv(const uint32_t& mode)
{
    if(mode != ARCHIVE_EXTDATA && mode != ARCHIVE_BOSS_EXTDATA && mode != ARCHIVE_SHARED_EXTDATA)
    {
        Result res = 0;
        u64 in = ((u64)SECUREVALUE_SLOT_SD << 32) | (data::curData.getUnique() << 8);
        u8 out;

        res = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &in, 8, &out, 1);
        if(R_FAILED(res))
            ui::showMessage("Failed to delete secure value.\nError: 0x%08X", (unsigned)res);
    }
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

static inline void nukeFile(const std::string& path)
{
    FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, path.c_str()));
}

static inline void nukeFile(const std::u16string& path)
{
    FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_UTF16, path.data()));
}

fs::fsfile::fsfile(const FS_Archive& _arch, const std::string& _path, const uint32_t& openFlags)
{
    if(openFlags & FS_OPEN_CREATE)
        nukeFile(_path);

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
        nukeFile(_path);

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
        nukeFile(_path);

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
        nukeFile(_path);

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

        std::memset(buf, 0x00, max);
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
    bool operator()(const FS_DirectoryEntry& a, const FS_DirectoryEntry& b)
    {
        if(a.attributes != b.attributes)
            return a.attributes == FS_ATTRIBUTE_DIRECTORY;

        for(unsigned i = 0; i < 0x106; i++)
        {
            int charA = std::tolower(a.name[i]), charB = std::tolower(b.name[i]);
            if(charA != charB)
                return charA < charB;
        }

        return false;
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
        FS_DirectoryEntry get;
        FSDIR_Read(d, &read, 1, &get);
        if(read == 1)
            entry.push_back(get);
    }
    while(read > 0);

    FSDIR_Close(d);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

fs::dirList::~dirList()
{
    entry.clear();
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
            entry.push_back(ent);
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
            entry.push_back(ent);
    }
    while(read > 0);

    FSDIR_Close(d);

    std::sort(entry.begin(), entry.end(), sortDirs);
}

static std::u16string getItemFromPath(const std::u16string& path)
{
    size_t ls = path.find_last_of('/');
    if(ls != path.npos)
        return path.substr(ls + 1, path.npos);

    return (char16_t *)"";
}

void fs::copyFileToSD(const FS_Archive& arch, const std::u16string& from, const std::u16string& to)
{
    fs::fsfile in(arch, from, FS_OPEN_READ);
    fs::fsfile out(fs::getSDMCArch(), to, FS_OPEN_WRITE | FS_OPEN_CREATE);

    if(!in.isOpen() || !out.isOpen())
        return;

    uint8_t *buff = new uint8_t[buff_size];
    ui::progressBar prog(in.getSize());
    size_t read = 0;
    std::string progString = util::toUtf8(from);
    while((read = in.read(buff, buff_size)))
    {
        out.write(buff, read);
        prog.update(in.getOffset());

        gfx::frameBegin();
        gfx::frameStartBot();
        prog.draw(progString);
        gfx::frameEnd();
    }
    delete[] buff;
}

void fs::copyDirToSD(const FS_Archive& arch, const std::u16string& from, const std::u16string& to)
{
    dirList list(arch, from);

    for(unsigned i = 0; i < list.getCount(); i++)
    {
        if(list.isDir(i))
        {
            std::u16string newFrom = from + list.getItem(i) + util::toUtf16("/");
            std::u16string newTo = to + list.getItem(i);
            FSUSER_CreateDirectory(getSDMCArch(), fsMakePath(PATH_UTF16, newTo.data()), 0);
            newTo += util::toUtf16("/");

            copyDirToSD(arch, newFrom, newTo);
        }
        else
        {
            std::u16string fullFrom = from + list.getItem(i);
            std::u16string fullTo   = to   + list.getItem(i);

            copyFileToSD(arch, fullFrom, fullTo);
        }
    }
    ui::fldRefresh();
}

void fs::backupArchive(const std::u16string& outpath)
{
    std::u16string pathIn = util::toUtf16("/");
    copyDirToSD(saveArch, pathIn, outpath);
}

void fs::copyFileToArch(const FS_Archive& arch, const std::u16string& from, const std::u16string& to)
{
    fs::fsfile in(fs::getSDMCArch(), from, FS_OPEN_READ);
    fs::fsfile out(arch, to, FS_OPEN_WRITE, in.getSize());

    if(!in.isOpen() || !out.isOpen())
        return;

    uint8_t *buff = new uint8_t[buff_size];
    ui::progressBar prog(in.getSize());
    size_t read = 0;
    std::string progString = util::toUtf8(from);
    while((read = in.read(buff, buff_size)))
    {
        out.write(buff, read);
        prog.update(in.getOffset());

        gfx::frameBegin();
        gfx::frameStartBot();
        prog.draw(progString);
        gfx::frameEnd();
    }
    fs::commitData(fs::getSaveMode());
    delete[] buff;
}

void fs::copyDirToArch(const FS_Archive& arch, const std::u16string& from, const std::u16string& to)
{
    dirList dir(getSDMCArch(), from);

    for(unsigned i = 0; i < dir.getCount(); i++)
    {
        if(dir.isDir(i))
        {
            std::u16string newFrom = from + dir.getItem(i) + util::toUtf16("/");

            std::u16string newTo   = to + dir.getItem(i);
            FSUSER_CreateDirectory(arch, fsMakePath(PATH_UTF16, newTo.data()), 0);
            newTo += util::toUtf16("/");

            copyDirToArch(arch, newFrom, newTo);
        }
        else
        {
            std::u16string sdPath = from + dir.getItem(i);
            std::u16string archPath = to + dir.getItem(i);

            copyFileToArch(arch, sdPath, archPath);
        }
    }
}

void fs::restoreToArchive(const std::u16string& inpath)
{
    std::u16string root = util::toUtf16("/");
    FSUSER_DeleteDirectoryRecursively(saveArch, fsMakePath(PATH_UTF16, root.data()));
    copyDirToArch(saveArch, inpath, root);
    commitData(saveMode);
    deleteSv(saveMode);
}

void fs::copyArchToZip(const FS_Archive& arch, const std::u16string& from, zipFile zip)
{
    fs::dirList *archList = new fs::dirList(arch, from);
    for(unsigned i = 0; i < archList->getCount(); i++)
    {
        if(archList->isDir(i))
        {
            std::u16string newFrom = from + archList->getItem(i) + util::toUtf16("/");
            fs::copyArchToZip(arch, newFrom, zip);
        }
        else
        {
            time_t raw;
            time(&raw);
            tm *locTime = localtime(&raw);
            zip_fileinfo inf = { (unsigned)locTime->tm_sec, (unsigned)locTime->tm_min, (unsigned)locTime->tm_hour,
                                 (unsigned)locTime->tm_mday, (unsigned)locTime->tm_mon, (unsigned)(1900 + locTime->tm_year), 0, 0, 0 };
            
            std::string filename = util::toUtf8(from + archList->getItem(i));
            int openZip = zipOpenNewFileInZip64(zip, filename.c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(openZip == 0)
            {
                fs::fsfile readFile(arch, from + archList->getItem(i), FS_OPEN_READ);
                ui::progressBar prog(readFile.getSize());
                size_t readIn = 0;
                uint8_t *buff = new uint8_t[buff_size];
                while((readIn = readFile.read(buff, buff_size)))
                {
                    zipWriteInFileInZip(zip, buff, readIn);

                    prog.update(readFile.getOffset());
                    gfx::frameBegin();
                    gfx::frameStartBot();
                    prog.draw(filename);
                    gfx::frameEnd();
                }
        
                delete[] buff;
                readFile.close();
            }
        }
    }
}

void fs::copyZipToArch(const FS_Archive& arch, unzFile unz)
{
    if(unzGoToFirstFile(unz) == UNZ_OK)
    {
        char filename[0x301];
        unz_file_info64 info;
        do
        {
            memset(filename, 0, 0x301);
            unzGetCurrentFileInfo64(unz, &info, filename, 0x300, NULL, 0, NULL, 0);
            if(unzOpenCurrentFile(unz) == UNZ_OK)
            {
                std::u16string dstPathUTF16 = util::toUtf16(filename);
                fs::createDirRec(arch, dstPathUTF16.substr(0, dstPathUTF16.find_last_of(L'/') + 1));
                fs::fsfile writeFile(arch, dstPathUTF16, FS_OPEN_WRITE | FS_OPEN_CREATE);
                int readIn = 0;
                uint8_t *buff = new uint8_t[buff_size];

                ui::progressBar prog(info.uncompressed_size);
                while((readIn = unzReadCurrentFile(unz, buff, buff_size)) > 0)
                {
                    writeFile.write(buff, readIn);
                    prog.update(writeFile.getOffset());
                    gfx::frameBegin();
                    gfx::frameStartBot();
                    prog.draw(filename);
                    gfx::frameEnd();
                }
                
                delete[] buff;
            }
        } while (unzGoToNextFile(unz) != UNZ_END_OF_LIST_OF_FILE);
        fs::commitData(fs::getSaveMode());
    }
}

void fs::backupAll()
{
    ui::progressBar prog(data::usrSaveTitles.size());
    for(unsigned i = 0; i < data::usrSaveTitles.size(); i++)
    {
        std::string copyStr = "Working on '" + util::toUtf8(data::usrSaveTitles[i].getTitle()) + "'...";
        prog.update(i);

        //Sue me
        gfx::frameBegin();
        gfx::frameStartBot();
        prog.draw(copyStr);
        gfx::frameEnd();

        if(fs::openArchive(data::usrSaveTitles[i], ARCHIVE_USER_SAVEDATA, false))
        {
            util::createTitleDir(data::usrSaveTitles[i], ARCHIVE_USER_SAVEDATA);

            std::u16string outpath = util::createPath(data::usrSaveTitles[i], ARCHIVE_USER_SAVEDATA) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
            FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, outpath.data()), 0);
            outpath += util::toUtf16("/");

            backupArchive(outpath);

            closeSaveArch();
        }

        if(fs::openArchive(data::usrSaveTitles[i], ARCHIVE_EXTDATA, false))
        {
            util::createTitleDir(data::usrSaveTitles[i], ARCHIVE_EXTDATA);

            std::u16string outpath = util::createPath(data::usrSaveTitles[i], ARCHIVE_EXTDATA) + util::toUtf16(util::getDateString(util::DATE_FMT_YMD));
            FSUSER_CreateDirectory(fs::getSDMCArch(), fsMakePath(PATH_UTF16, outpath.data()), 0);
            outpath += util::toUtf16("/");

            backupArchive(outpath);

            closeSaveArch();
        }
    }
}

