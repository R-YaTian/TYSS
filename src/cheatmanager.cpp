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

/*
 *   This file is part of Checkpoint
 *   Copyright (C) 2017-2021 Bernardo Giordano, FlagBrew
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#include "util.h"
#include "cheatmanager.h"

CheatManager::CheatManager(void)
{
    mCheats = nullptr;
}

bool CheatManager::areCheatsAvailable(const std::string& key)
{
    if (!mCheats || mCheats->empty())
        return false;
    return mCheats->contains(key);
}

bool CheatManager::install(const std::string& key)
{
    std::string cheatFile = "";
    auto cheats           = *CheatManager::getInstance().cheats().get();
    bool first            = true;
    for (auto it = cheats[key].begin(); it != cheats[key].end(); ++it) {
        std::string value = it.key();
        if (!first)
            cheatFile += "\n";
        cheatFile += "[" + value + "]\n";
        for (auto& code : cheats[key][value]) {
            cheatFile += code.get<std::string>() + "\n";
        }

        if (first)
            first = false;
    }

    const std::string outPath = "/cheats/" + key + ".txt";
    FILE* f                   = fopen(outPath.c_str(), "w");
    if (f != NULL) {
        fwrite(cheatFile.c_str(), 1, cheatFile.length(), f);
        fclose(f);
        return true;
    }

    return false;
}

void CheatManager::init()
{
    const std::string path = "/TYSS/cheats.json";
    if (util::fexists(path)) {
        FILE* in               = fopen(path.c_str(), "rt");
        if (in != NULL) {
            mCheats = std::make_shared<nlohmann::ordered_json>(nlohmann::ordered_json::parse(in, nullptr, false));
            fclose(in);
        }
    } else {
        loadBuiltIn();
    } 
}

void CheatManager::reset()
{
    mCheats.reset();
}

void CheatManager::loadBuiltIn()
{
    char path[64];
    std::snprintf(path, sizeof(path), "romfs:/cheats/cheats%02d.json.zip", cfg::config["cheatdblang"]);
    // load compressed archive in memory
    unzFile zipFile = unzOpen64(path);
    if (zipFile != nullptr)
    {
        if (unzGoToFirstFile(zipFile) == UNZ_OK)
        {
            char filename[0x301];
            unz_file_info64 info;
            memset(filename, 0, 0x301);
            unzGetCurrentFileInfo64(zipFile, &info, filename, 0x300, NULL, 0, NULL, 0);
            if (unzOpenCurrentFile(zipFile) == UNZ_OK) {
                std::vector<char> buffer(info.uncompressed_size + 1, 0);
                int bytesRead = unzReadCurrentFile(zipFile, buffer.data(), info.uncompressed_size);
                if (bytesRead == static_cast<int>(info.uncompressed_size))
                    mCheats = std::make_shared<nlohmann::ordered_json>(nlohmann::ordered_json::parse(buffer.data()));
            }
        }
        unzClose(zipFile);
    } 
}
