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
#include <json-c/json.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "curlfuncs.h"

#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"
#define HEADER_AUTHORIZATION "Authorization: Bearer "

#define MIMETYPE_FOLDER "application/vnd.google-apps.folder"

namespace drive
{
    typedef struct 
    {
        std::string name, id, parent;
        bool isDir = false;
        unsigned int size;
    } gdItem;

    class gd
    {
        public:
            gd(const std::string& _clientID, const std::string& _secretID, const std::string& _authCode, const std::string& _rToken);

            void exhangeAuthCode(const std::string& _authCode);
            bool hasToken() { return token.empty() == false; }
            void refreshToken();
            bool tokenIsValid();

            void debugWriteList();
            void loadDriveList();
            void getListWithParent(const std::string& _parent, std::vector<drive::gdItem *>& _out);
            
            bool createDir(const std::string& _dirName, const std::string& _parent);
            bool dirExists(const std::string& _dirName);
            bool dirExists(const std::string& _dirName, const std::string& _parent);
            bool fileExists(const std::string& _filename);
            bool fileExists(const std::string& _filename, const std::string& _parent);
            void uploadFile(const std::string& _filename, const std::string& _parent, FILE *_upload);
            void updateFile(const std::string& _fileID, FILE *_upload);
            void downloadFile(const std::string& _fileID, FILE *_download);
            void deleteFile(const std::string& _fileID);

            std::string getClientID() const { return clientID; }
            std::string getClientSecret() const { return secretID; }
            std::string getRefreshToken() const { return rToken; }
            std::string getFolderID(const std::string& _name);
            std::string getFolderID(const std::string& _name, const std::string& _parent);
            std::string getFileID(const std::string& _name);
            std::string getFileID(const std::string& _name, const std::string& _parent);

            size_t getDriveListCount() const { return driveList.size(); }
            drive::gdItem *getItemAt(unsigned int _ind) { return &driveList[_ind]; }

            Result setupProxy(void);

        private:
            std::vector<gdItem> driveList;
            std::string clientID, secretID, token, rToken, proxyURL;
    };
}
