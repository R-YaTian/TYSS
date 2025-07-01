#pragma once
#include <string>
#include <vector>

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

    inline size_t getDriveListCount() const { return driveList.size(); }

    inline driveItem *getItemAt(unsigned int _ind) { return &driveList[_ind]; }

protected:
    std::vector<driveItem> driveList;
};

}
