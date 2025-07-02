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

#pragma once

#include <curl/curl.h>

#include <string>
#include <vector>

#include "drive/IDrive.h"
#include "drive/curlfuncs.h"

#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"
#define HEADER_AUTHORIZATION "Authorization: Bearer "

#define MIMETYPE_FOLDER "application/vnd.google-apps.folder"

namespace drive
{
    class gd : public DriveBase
    {
        public:
            gd(const std::string& _clientID, const std::string& _secretID, const std::string& _authCode, const std::string& _rToken);

            void exhangeAuthCode(const std::string& _authCode) override;
            void refreshToken() override;
            bool tokenIsValid() override;

            void loadDriveList() override;
            bool createDir(const std::string& _dirName, const std::string& _parent) override;
            void uploadFile(const std::string& _filename, const std::string& _parent, FILE *_upload) override;
            void updateFile(const std::string& _fileID, FILE *_upload) override;
            void downloadFile(const std::string& _fileID, FILE *_download) override;
            void deleteFile(const std::string& _fileID) override;

            bool hasToken() { return token.empty() == false; }
            std::string getRefreshToken() const { return rToken; }

        private:
            std::string clientID, secretID, token, rToken;
    };
}
