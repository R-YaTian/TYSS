#pragma once

namespace ui
{
    void extInit(void *a);
    void extExit();
    void extOptBack();
    void extUpdate();
    void extRefresh(int selFlag = 0);
    void extDrawTop();
    void extDrawBot();
}
