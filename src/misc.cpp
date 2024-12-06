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

void misc::PTMSYSM_ClearStepHistory(void *a)
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
