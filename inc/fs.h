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

#ifndef FS_H
#define FS_H

#include <3ds.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <string>
#include "data.h"
#include "ui.h"

#ifdef ENABLE_GD
#include "drive/IDrive.h"
#include "drive/gd.h"
#endif

#define DRIVE_TYSS_DIR "TYSS"
#define DRIVE_USER_SAVE_DIR "Saves"
#define DRIVE_EXTDATA_DIR "ExtData"
#define DRIVE_SYSTEM_DIR "SysSave"
#define DRIVE_BOSS_DIR "Boss"
#define DRIVE_SHARED_DIR "Shared"

namespace fs
{
    void init();
    void exit();

#ifdef ENABLE_GD
    void driveInit(void *a);
    void driveExit();
    extern drive::gd *gDrive;
    extern std::string tyssDirID, usrSaveDirID, extDataDirID, sysSaveDirID, bossExtDirID, sharedExtID, currentDirID;
#endif

    enum fsSeek
    {
        seek_set,
        seek_cur,
        seek_end
    };

    FS_Archive getSDMCArch();
    FS_Archive getSaveArch();
    FS_ArchiveID getSaveMode();

    bool openArchive(data::titleData& dat, const uint32_t& mode, bool error, FS_Archive& arch);
    bool openArchive(data::titleData& dat, const uint32_t& mode, bool error);
    void closeSaveArch();
    void closePxiSaveArch();
    void commitData(const uint32_t& mode);
    void deleteSv(const uint32_t& mode, const data::titleData& dat);
    void exportSv(const uint32_t& mode, const std::u16string& _dst, const data::titleData& dat);
    void importSv(const uint32_t& mode, const std::u16string& _src, const data::titleData& dat);

    bool fsfexists(const FS_Archive& _arch, const std::string& _path);
    bool fsfexists(const FS_Archive& _arch, const std::u16string& _path);
    inline void fcreate(const std::string& path){ FSUSER_CreateFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, path.c_str()), 0, 0); }
    inline void fdelete(const std::string& path){ FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, path.c_str())); }
    void resetPxiFile(const FS_Archive& _arch);

    //Causes a hang for large saves, so threaded
    void delDirRec(const FS_Archive& _arch, const std::u16string& _path);
    //Threaded create dir so I can be sure it's run after ^ is finished
    void createDir(const std::string& path);
    void createDir(const FS_Archive& _arch, const std::u16string& _path);
    void createDirRec(const FS_Archive& _arch, const std::u16string& path);

    class fsfile
    {
        public:
            fsfile(const FS_Archive& _arch, const std::string& _path, const uint32_t& openFlags);
            fsfile(const FS_Archive& _arch, const std::string& _path, const uint32_t& openFlags, const uint64_t& crSize);
            fsfile(const FS_Archive& _arch, const std::u16string& _path, const uint32_t& openFlags);
            fsfile(const FS_Archive& _arch, const std::u16string& _path, const uint32_t& openFlags, const uint64_t& crSize);
            ~fsfile();
            void close();

            size_t read(void *buf, const uint32_t& max);
            bool getLine(char *out, size_t max);
            size_t write(const void *buf, const uint32_t& size);
            void writef(const char *fmt, ...);

            uint8_t getByte();
            void putByte(const uint8_t& put);
            bool eof();

            void seek(const int& pos, const uint8_t& seekfrom);

            Result getError(){ return error; }
            uint64_t getOffset(){ return offset; }
            uint64_t getSize(){ return fSize; }
            u64 getHandle(){ return fHandle; }
            bool isOpen(){ return open; }

        private:
            u64 fHandle;
            Result error;
            uint64_t fSize, offset = 0;
            bool open = false;
            bool isPxi = false;
    };

    typedef struct
    {
        std::u16string name;
        std::string nameUTF8;
        bool isDir;
    } dirItem;

    class dirList
    {
        public:
            dirList() = default;
            dirList(const FS_Archive& arch, const std::u16string& path);
            ~dirList();

            void scanItem(bool filter = false);
            void reassign(const FS_Archive& arch, const std::u16string& p, bool filter = false);
            const uint32_t getCount(){ return entry.size(); }
            bool isDir(unsigned i){ return entry[i].isDir; }
            const std::u16string getItem(unsigned i){ return entry[i].name; }
            dirItem *getDirItemAt(int i) { return &entry[i]; }

        private:
            Handle dirHandle;
            FS_Archive dirArch;
            std::u16string path;
            std::vector<dirItem> entry;
    };

    void copyFile(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, bool isPxi, threadInfo *t);
    void copyFileThreaded(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, bool isPxi = false);
    void copyDirToDir(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit, threadInfo *t, bool isRecursion = false);
    void copyDirToDirThreaded(const FS_Archive& _srcArch, const std::u16string& _src, const FS_Archive& _dstArch, const std::u16string& _dst, bool commit);

    void copyArchToZip(const FS_Archive& _arch, const std::u16string& _src, zipFile _zip, const std::u16string* _dir, threadInfo *t, bool isRecursion = false);
    void copyArchToZipThreaded(const FS_Archive& _arch, const std::u16string& _src, const std::u16string& _dst);
    void copyZipToArch(const FS_Archive& _arch, unzFile _unz, threadInfo *t);
    void copyZipToArchThreaded(const FS_Archive& _arch, const std::u16string& _src);

    void backupTitles(std::vector<data::titleData>& vect, const uint32_t &mode);
    void backupSPI(const std::u16string& savPath, const CardType& cardType);
    void restoreSPI(const std::u16string& savPath, const CardType& cardType);
    bool pxiFileToSaveFile(const std::u16string& _dst);
    bool saveFileToPxiFile(const std::u16string& _src);
}

extern "C" {
    Result svcControlService(uint32_t op, uint32_t* outHandle, const char* name);
}

#endif // FS_H
