#pragma once

namespace ui
{
    void ttlInit(void *a);
    void ttlExit();
    void ttlOptBack();
    void ttlRefresh(int selFlag = 0);
    void ttlUpdate();
    void ttlDrawTop();
    void ttlDrawBot();
}
