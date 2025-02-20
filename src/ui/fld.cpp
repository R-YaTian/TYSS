#include <3ds.h>
#include <string>
#include <vector>

#include "ui.h"
#include "fs.h"
#include "util.h"
#include "type.h"
#include "cfg.h"

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

    if(!newFolder.empty() && std::get<bool>(cfg::config["zip"]))
    {
        std::u16string fullOut = targetDir + newFolder + util::toUtf16(".zip");
        std::u16string svOut = fullOut + util::toUtf16(".sv");
        fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found
        fs::copyArchToZipThreaded(fs::getSaveArch(), util::toUtf16("/"), fullOut);
    }
    else if(!newFolder.empty())
    {
        std::u16string fullOut = targetDir + newFolder;
        std::u16string svOut = fullOut + util::toUtf16(".sv");
        fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found

        FS_Path crDir = fsMakePath(PATH_UTF16, fullOut.c_str());
        FSUSER_CreateDirectory(fs::getSDMCArch(), crDir, 0);
        fullOut += util::toUtf16("/");

        fs::copyDirToDirThreaded(fs::getSaveArch(), util::toUtf16("/"), fs::getSDMCArch(), fullOut, false);
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
        std::u16string svOut = overwrite + util::toUtf16(".sv");
        fs::exportSv(fs::getSaveMode(), svOut, data::curData); // export secure value if found

        FS_Path targetPath = fsMakePath(PATH_UTF16, overwrite.c_str());
        FSUSER_DeleteFile(fs::getSDMCArch(), targetPath);

        fs::copyArchToZipThreaded(fs::getSaveArch(), util::toUtf16("/"), overwrite);
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
        fs::delDirRec(fs::getSaveArch(), util::toUtf16("/"));
        fs::commitData(fs::getSaveMode());

        std::u16string rest = targetDir + in->name;
        fs::copyZipToArchThreaded(fs::getSaveArch(), rest);
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

#ifdef ENABLE_GD
std::vector<drive::gdItem *> gdList;

void fldMenuUpload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *in = (fs::dirItem *)t->argPtr;
    std::u16string src = targetDir + in->name;
    t->status->setStatus("正在上传 " + in->nameUTF8 + "...");

    FS_Path srcPath = fsMakePath(PATH_UTF16, src.c_str());
    FS_Path tmpPath = fsMakePath(PATH_ASCII, "/JKSV/tmp.zip");
    FSUSER_RenameFile(fs::getSDMCArch(), srcPath, fs::getSDMCArch(), tmpPath);

    FILE *upload = fopen("/JKSV/tmp.zip", "rb");
    std::string utf8Name = in->nameUTF8;
    if(fs::gDrive->fileExists(utf8Name, uploadParent))
    {
        std::string fileID = fs::gDrive->getFileID(utf8Name, uploadParent);
        fs::gDrive->updateFile(fileID, upload);
    } else
        fs::gDrive->uploadFile(utf8Name, uploadParent, upload);

    fclose(upload);

    FSUSER_RenameFile(fs::getSDMCArch(), tmpPath, fs::getSDMCArch(), srcPath);

    ui::fldRefresh();

    t->finished = true;
}

void fldMenuUpload(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    if(fs::gDrive && !in->isDir)
        ui::newThread(fldMenuUpload_t, a, NULL);
    else if (in->isDir)
        ui::showMessage("云端存储: 仅支持上传压缩包存档");
    else
        ui::showMessage("云端存储: 服务尚未初始化");
}

void fldMenuDriveDownload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *in = (drive::gdItem *)t->argPtr;

    t->status->setStatus("正在下载 " + in->name + "...");

    std::u16string target = targetDir + util::toUtf16(in->name);
    FS_Path targetPath = fsMakePath(PATH_UTF16, target.c_str());
    FS_Path tmpPath = fsMakePath(PATH_ASCII, "/JKSV/tmp.zip");

    if(fs::fsfexists(fs::getSDMCArch(), target))
        FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_UTF16, target.c_str()));

    FILE *tmp = fopen("/JKSV/tmp.zip", "wb");
    fs::gDrive->downloadFile(in->id, tmp);
    fclose(tmp);

    FSUSER_RenameFile(fs::getSDMCArch(), tmpPath, fs::getSDMCArch(), targetPath);

    ui::fldRefresh();

    t->finished = true;
}

void fldMenuDriveDownload(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    std::u16string checkPath = targetDir + util::toUtf16(in->name);
    if(fs::fsfexists(fs::getSDMCArch(), checkPath))
        ui::confirm("下载此存档将会替换 SD 卡中的数据.\n你确定仍要进行下载吗?", fldMenuDriveDownload_t, NULL, a);
    else
        ui::newThread(fldMenuDriveDownload_t, a, NULL);
}

void fldMenuDriveDelete_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *in = (drive::gdItem *)t->argPtr;
    t->status->setStatus("正在删除 " + in->name + "...");
    fs::gDrive->deleteFile(in->id);
    ui::fldRefresh();
    t->finished = true;
}

void fldMenuDriveDelete(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    ui::confirm("你确定要删除云盘中的 " + in->name + " 文件吗?", fldMenuDriveDelete_t, NULL, a);
}

void fldMenuDriveRestore_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *in = (drive::gdItem *)t->argPtr;
    t->status->setStatus("正在下载 " + in->name + "...");

    FILE *tmp = fopen("/JKSV/tmp.zip", "wb");
    fs::gDrive->downloadFile(in->id, tmp);
    fclose(tmp);

    t->status->setStatus("正在解压存档到存档位...");
    unzFile unz = unzOpen64("/JKSV/tmp.zip");
    fs::copyZipToArch(fs::getSaveArch(), unz, NULL);
    unzClose(unz);

    // Todo: SV logic
    FSUSER_DeleteFile(fs::getSDMCArch(), fsMakePath(PATH_ASCII, "/JKSV/tmp.zip"));

    t->finished = true;
}

void fldMenuDriveRestore(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    ui::confirm("你确定要下载并恢复 " + in->name + "?", fldMenuDriveRestore_t, NULL, a);
}
#endif

void ui::fldInit(const std::u16string& _path, const std::string& _uploadParent, funcPtr _func, void *_args)
{
    fldMenu.reset();
    fldMenu.setCallback(_func, _args);
    fldList.reassign(fs::getSDMCArch(), _path);
    targetDir = _path;
    uploadParent = _uploadParent;

    fldMenu.addOpt("新建", 0);
    fldMenu.addOptEvent(0, KEY_A, fldMenuNew, NULL);

    int fldInd = 1;
#ifdef ENABLE_GD
    if(fs::gDrive)
    {
        fs::gDrive->getListWithParent(uploadParent, gdList);

        for(unsigned i = 0; i < gdList.size(); i++, fldInd++)
        {
            fldMenu.addOpt("[GD] " + gdList[i]->name, 320);

            fldMenu.addOptEvent(fldInd, KEY_A, fldMenuDriveDownload, gdList[i]);
            fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDriveDelete, gdList[i]);
            fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuDriveRestore, gdList[i]);
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
#ifdef ENABLE_GD
        if (fs::gDrive)
            fldMenu.addOptEvent(fldInd, KEY_R, fldMenuUpload, di);
#endif
    }
}

void ui::fldRefresh()
{
    fldMenu.reset();
    fldList.reassign(fs::getSDMCArch(), targetDir);

    fldMenu.addOpt("新建", 0);
    fldMenu.addOptEvent(0, KEY_A, fldMenuNew, NULL);

    int fldInd = 1;
#ifdef ENABLE_GD
    if(fs::gDrive)
    {
        fs::gDrive->getListWithParent(uploadParent, gdList);

        for(unsigned i = 0; i < gdList.size(); i++, fldInd++)
        {
            fldMenu.addOpt("[GD] " + gdList[i]->name, 320);

            fldMenu.addOptEvent(fldInd, KEY_A, fldMenuDriveDownload, gdList[i]);
            fldMenu.addOptEvent(fldInd, KEY_X, fldMenuDriveDelete, gdList[i]);
            fldMenu.addOptEvent(fldInd, KEY_Y, fldMenuDriveRestore, gdList[i]);
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
#ifdef ENABLE_GD
        if (fs::gDrive)
            fldMenu.addOptEvent(fldInd, KEY_R, fldMenuUpload, di);
#endif
    }
}

void ui::fldUpdate()
{
    fldMenu.update();
}

void ui::fldDraw()
{
    fldMenu.draw(0, 2, 0xFFFFFFFF, 320, false);
}
