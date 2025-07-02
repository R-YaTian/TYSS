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
#include <3ds.h>
#include <string>
#include <vector>
#include <functional>

#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"
#define HEADER_AUTHORIZATION "Authorization: Bearer "

namespace drive
{

struct driveItem {
    std::string name, id, parent;
    bool isDir = false;
    unsigned int size = 0;
};

class IDrive {
public:
    virtual ~IDrive() {}
    virtual void exhangeAuthCode(const std::string& authCode) = 0;
    virtual void refreshToken() = 0;
    virtual bool tokenIsValid() = 0;
    virtual void loadDriveList() = 0;
    virtual bool createDir(const std::string& name, const std::string& parent) = 0;
    virtual void uploadFile(const std::string& filename, const std::string& parent, FILE* upload) = 0;
    virtual void updateFile(const std::string& fileID, FILE* upload) = 0;
    virtual void downloadFile(const std::string& fileID, FILE* download) = 0;
    virtual void deleteFile(const std::string& fileID) = 0;
};

class DriveBase : public IDrive {
public:
    virtual ~DriveBase() {}

    inline Result setupProxy(void)
    {
        Result res;
        bool proxyEnable = false;

        res = acInit();
        if (R_FAILED(res)) return res;

        res = ACU_GetProxyEnable(&proxyEnable);
        if (R_FAILED(res))  return res;

        if (proxyEnable)
        {
            u16 proxyPort;
            res = ACU_GetProxyPort((u16*) &proxyPort);
            if (R_FAILED(res)) return res;

            char *proxyHost = new char[0x100];
            res = ACU_GetProxyHost(proxyHost);
            if (R_SUCCEEDED(res))
                proxyURL = "http://" + std::string(proxyHost) + ":" + std::to_string(proxyPort);

            delete proxyHost;
        }

        acExit();
        return res;
    }

    inline std::string getFolderID(const std::string& _name)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].name == _name && driveList[i].isDir)
                return driveList[i].id;
        }
        return "";
    }

    inline std::string getFolderID(const std::string& _name, const std::string& _parent)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].isDir && driveList[i].name == _name && driveList[i].parent == _parent)
                return driveList[i].id;
        }
        return "";
    }

    inline std::string getFileID(const std::string& _name)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].name == _name && driveList[i].isDir == false)
                return driveList[i].id;
        }
        return "";
    }

    inline std::string getFileID(const std::string& _name, const std::string& _parent)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(!driveList[i].isDir && driveList[i].name == _name && driveList[i].parent == _parent)
                return driveList[i].id;
        }
        return "";
    }

    inline bool dirExists(const std::string& _dirName)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].isDir && driveList[i].name == _dirName)
                return true;
        }
        return false;
    }

    inline bool dirExists(const std::string& _dirName, const std::string& _parent)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].isDir && driveList[i].name == _dirName && driveList[i].parent == _parent)
                return true;
        }
        return false;
    }

    inline bool fileExists(const std::string& _filename)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(!driveList[i].isDir && driveList[i].name == _filename)
                return true;
        }
        return false;
    }

    inline bool fileExists(const std::string& _filename, const std::string& _parent)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(!driveList[i].isDir && driveList[i].name == _filename && driveList[i].parent == _parent)
                return true;
        }
        return false;
    }

    inline void getListWithParent(const std::string& _parent, std::vector<driveItem *>& _out, std::function<bool(const driveItem&)> filter)
    {
        _out.clear();
        if (_parent.empty()) return;
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].parent == _parent)
            {
                if (!filter || filter(driveList[i]))
                    _out.push_back(&driveList[i]);
            }
        }
    }

    inline size_t getDriveListCount() const { return driveList.size(); }

    inline driveItem *getItemAt(unsigned int _ind) { return &driveList[_ind]; }

protected:
    std::vector<driveItem> driveList;
    std::string proxyURL;
};

}
