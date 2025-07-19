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
#include <malloc.h>

#include "gfx.h"
#include "fs.h"
#include "ui.h"
#include "util.h"
#include "data.h"
#include "sys.h"
#include "cfg.h"

extern int state;

#ifdef ENABLE_DRIVE
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

#ifdef ENABLE_DRIVE
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
    if (cfg::config["bootwithcheatdb"])
        ui::newThread(data::loadCheatsDB, NULL, NULL);

#ifdef ENABLE_DRIVE
    if(cfg::driveInitOnBoot && util::fexists("/TYSS/drive.json"))
        ui::newThread(fs::driveInit, NULL, NULL);
#endif

    while(aptMainLoop() && ui::runApp()){ }

#ifdef ENABLE_DRIVE
    curl_global_cleanup();
#endif

    data::saveFav();
    sys::exit();
    gfx::exit();
#ifdef ENABLE_DRIVE
    fs::driveExit();
#endif
    fs::exit();
    data::exit();
    ui::exit();
#ifdef ENABLE_DRIVE
    socExit();
#endif
}
