#include <3ds.h>
#include <malloc.h>

#include "gfx.h"
#include "fs.h"
#include "ui.h"
#include "util.h"
#include "data.h"
#include "sys.h"
#include "cfg.h"

extern int state;

#ifdef ENABLE_GD
static uint32_t *socBuffer;
#endif

int main(int argc, const char *argv[])
{
    sys::init();
    data::init();
    gfx::init();
    fs::init();
    cfg::initToDefault();
    cfg::load();
    ui::init();

#ifdef ENABLE_GD
    // Need to init soc so curl and drive work
    socBuffer = (uint32_t *)memalign(SOCU_ALIGN, SOCU_BUFFERSIZE);
    socInit(socBuffer, SOCU_BUFFERSIZE);

    curl_global_init(CURL_GLOBAL_ALL);
#endif

    ui::newThread(data::loadTitles, NULL, NULL);
    ui::newThread(ui::ttlInit, NULL, NULL);
    ui::newThread(ui::extInit, NULL, NULL);
    ui::newThread(ui::sysInit, NULL, NULL);
    ui::newThread(ui::bossViewInit, NULL, NULL);
    ui::newThread(ui::shrdInit, NULL, NULL);
    ui::newThread(ui::setInit, NULL, NULL);

#ifdef ENABLE_GD
    if(!cfg::driveClientID.empty() && !cfg::driveClientSecret.empty())
        ui::newThread(fs::driveInit, NULL, NULL);
#endif

    while(aptMainLoop() && ui::runApp()){ }

#ifdef ENABLE_GD
    curl_global_cleanup();
#endif

    data::saveFav();
    sys::exit();
    gfx::exit();
#ifdef ENABLE_GD
    fs::driveExit();
#endif
    fs::exit();
    data::exit();
    ui::exit();
#ifdef ENABLE_GD
    socExit();
#endif
}
