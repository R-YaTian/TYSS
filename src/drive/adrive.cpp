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

#include <curl/curl.h>

#include "json.h"
#include "drive/adrive.h"
#include "util.h"
#include "cfg.h"

#define adriveTokenURL "https://openapi.alipan.com/oauth/access_token"
#define adriveGetUserInfoURL "https://openapi.alipan.com/oauth/users/info"
#define adriveGetDriveInfoURL "https://openapi.alipan.com/adrive/v1.0/user/getDriveInfo"
#define adriveURL "https://openapi.alipan.com/adrive/v1.0/openFile"
#define adriveUploadURL "https://openapi.alipan.com/adrive/v1.0/openFile/create"

drive::adrive::adrive(const std::string& _authCode, const std::string& _rToken, const std::string& _driveID)
{
    rToken = _rToken;
    driveID = _driveID;

    setupProxy();

    if(!_authCode.empty())
        exhangeAuthCode(_authCode);
    else if(!rToken.empty())
        refreshToken();

    if (hasToken() && driveID.empty())
        getUserDriveID();
}

void drive::adrive::getUserDriveID()
{
    // Header
    curl_slist *header = NULL;
    header = curl_slist_append(header, std::string(HEADER_AUTHORIZATION + token).c_str());
    header = curl_slist_append(header, HEADER_CONTENT_TYPE_APP_JSON);

    // Curl Request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_URL, adriveGetDriveInfoURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(respParse.contains("default_drive_id"))
            driveID = respParse["default_drive_id"].get<std::string>();
    }

    delete jsonResp;
}

void drive::adrive::exhangeAuthCode(const std::string& _authCode)
{
    // Header
    curl_slist *postHeader = NULL;
    postHeader = curl_slist_append(postHeader, HEADER_CONTENT_TYPE_APP_JSON);

    // Post json
    nlohmann::json post;
    post["client_id"] = clientID;
    post["client_secret"] = secretID;
    post["code"] = _authCode;
    post["grant_type"] = "authorization_code";
    auto json_str = post.dump();

    // Curl Request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeader);
    curl_easy_setopt(curl, CURLOPT_URL, adriveTokenURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeader);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(respParse.contains("access_token") && respParse.contains("refresh_token"))
        {
            token = respParse["access_token"].get<std::string>();
            rToken = respParse["refresh_token"].get<std::string>();
        }
    }

    delete jsonResp;
}

void drive::adrive::refreshToken()
{
    // Header
    curl_slist *header = NULL;
    header = curl_slist_append(header, HEADER_CONTENT_TYPE_APP_JSON);

    // Post Json
    nlohmann::json post;
    post["client_id"] = clientID;
    post["client_secret"] = secretID;
    post["refresh_token"] = rToken;
    post["grant_type"] = "refresh_token";
    auto json_str = post.dump();

    // Curl Request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_URL, adriveTokenURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(respParse.contains("access_token") && respParse.contains("refresh_token"))
        {
            token = respParse["access_token"].get<std::string>();
            if (rToken != respParse["refresh_token"].get<std::string>())
            {
                rToken = respParse["refresh_token"].get<std::string>();
                cfg::saveDrive();
            }
        }
    }

    delete jsonResp;
}

bool drive::adrive::tokenIsValid()
{
    bool ret = false;

    std::string url = adriveGetUserInfoURL;
    url.append("?access_token=" + token);

    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);

    CURLcode error = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(!respParse.contains("code"))
            ret = true;
    }

    delete jsonResp;
    return ret;
}

void drive::adrive::loadDriveList()
{
    if(!tokenIsValid())
        refreshToken();

    driveList.clear();
    std::string nextPageToken;

    do {
        // Request url with specific fields needed.
        std::string url = adriveURL;
        url.append("/search");

        // JSON To Post
        nlohmann::json post;
        post["drive_id"] = driveID;
        post["query"] = "type = 'folder' or category = 'zip' or file_extension = 'bin' or file_extension = 'sav' or file_extension = 'sv'";
        if (!nextPageToken.empty())
            post["marker"] = nextPageToken;
        auto json_str = post.dump();

        // Headers needed
        curl_slist *postHeaders = NULL;
        postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
        postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

        // Curl request
        CURL *curl = curl_easy_init();
        std::string *jsonResp = new std::string;

        if (!proxyURL.empty())
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
        }
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

        CURLcode error = curl_easy_perform(curl);
        curl_slist_free_all(postHeaders);
        curl_easy_cleanup(curl);

        if (error == CURLE_OK)
        {
            nlohmann::json parse = nlohmann::json::parse(*jsonResp);

            if (parse.contains("items") && parse["items"].is_array())
            {
                const auto& fileArray = parse["items"];
                for (const auto& curFile : fileArray)
                {
                    std::string name = curFile.value("name", "");
                    std::string id = curFile.value("file_id", "");
                    std::string type = curFile.value("type", "");
                    std::string parent = curFile.value("parent_file_id", "");
                    unsigned int size = 0;
                    if (curFile.contains("size") && curFile["size"].is_number())
                        size = curFile["size"].get<unsigned int>();

                    drive::driveItem newItem;
                    newItem.name = name;
                    newItem.id = id;
                    newItem.size = size;
                    newItem.parent = parent;
                    if (type == "folder")
                        newItem.isDir = true;

                    if (newItem.isDir
                        || util::endsWith(newItem.name, std::string(".zip"))
                        || util::endsWith(newItem.name, std::string(".sv"))
                        || util::endsWith(newItem.name, std::string(".sav"))
                        || util::endsWith(newItem.name, std::string(".bin")))
                        driveList.push_back(newItem);
                }
            }

            nextPageToken.clear();
            if (parse.contains("next_marker") && parse["next_marker"].is_string())
                nextPageToken = parse["next_marker"];
        }

        delete jsonResp;
    } while (!nextPageToken.empty());
}

bool drive::adrive::createDir(const std::string& _dirName, const std::string& _parent)
{
    if(!tokenIsValid())
        refreshToken();

    bool ret = true;

    // Headers to use
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // JSON To Post
    nlohmann::json post;
    post["drive_id"] = driveID;
    post["name"] = _dirName;
    post["type"] = "folder";
    post["check_name_mode"] = "refuse";
    if (!_parent.empty())
        post["parent_file_id"] = _parent;
    else
        post["parent_file_id"] = "root";
    auto json_str = post.dump();

    // Curl Request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, adriveUploadURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(!respParse.contains("code"))
        {
            drive::driveItem newDir;
            newDir.name = _dirName;
            newDir.id = respParse["file_id"];
            newDir.isDir = true;
            newDir.size = 0;
            newDir.parent = _parent;
            driveList.push_back(newDir);
        }
    } else
        ret = false;

    delete jsonResp;
    return ret;
}

void drive::adrive::uploadFile(const std::string& _filename, const std::string& _parent, FILE *_upload)
{
    if(!tokenIsValid())
        refreshToken();

    std::string uploadURL;

    // Headers
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // Post JSON
    nlohmann::json post;
    post["drive_id"] = driveID;
    post["name"] = _filename;
    post["type"] = "file";
    post["check_name_mode"] = "refuse";
    post["parent_file_id"] = _parent;
    auto json_str = post.dump();

    // Curl upload request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, adriveUploadURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if (respParse.contains("part_info_list") && respParse["part_info_list"].is_array())
        {
            // Get upload URL
            const auto& infoArray = respParse["part_info_list"];
            for (const auto& partinfo : infoArray) {
                if (partinfo.contains("upload_url") && partinfo["upload_url"].is_string())
                    uploadURL = partinfo["upload_url"].get<std::string>();
            }

            CURL *curlUp = curl_easy_init();
            if (!proxyURL.empty())
            {
                curl_easy_setopt(curlUp, CURLOPT_PROXY, proxyURL.c_str());
                curl_easy_setopt(curlUp, CURLOPT_HTTPPROXYTUNNEL, 1);
            }
            curl_easy_setopt(curlUp, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curlUp, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curlUp, CURLOPT_URL, uploadURL.c_str());
            curl_easy_setopt(curlUp, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
            curl_easy_setopt(curlUp, CURLOPT_READDATA, _upload);
            curl_easy_setopt(curlUp, CURLOPT_UPLOAD_BUFFERSIZE, DRIVE_RT_BUFFER_SIZE);
            curl_easy_setopt(curlUp, CURLOPT_UPLOAD, 1);
            CURLcode upError = curl_easy_perform(curlUp);
            curl_easy_cleanup(curlUp);

            if (upError == CURLE_OK)
            {
                if (respParse.contains("file_id") && respParse.contains("file_name") && respParse.contains("upload_id"))
                {
                    drive::driveItem uploadData;
                    uploadData.id = respParse["file_id"].get<std::string>();
                    uploadData.name = respParse["file_name"].get<std::string>();
                    uploadData.isDir = false;
                    uploadData.parent = _parent;

                    // Complete URL
                    std::string url = adriveURL;
                    url.append("/complete");

                    // Header
                    curl_slist *header = NULL;
                    header = curl_slist_append(header, std::string(HEADER_AUTHORIZATION + token).c_str());
                    header = curl_slist_append(header, HEADER_CONTENT_TYPE_APP_JSON);

                    // Post JSON
                    nlohmann::json postComplete;
                    postComplete["drive_id"] = driveID;
                    postComplete["file_id"] = uploadData.id;
                    postComplete["upload_id"] = respParse["upload_id"].get<std::string>();
                    auto jsonComplete_str = postComplete.dump();

                    CURL *curlComplete = curl_easy_init();
                    if (!proxyURL.empty())
                    {
                        curl_easy_setopt(curlComplete, CURLOPT_PROXY, proxyURL.c_str());
                        curl_easy_setopt(curlComplete, CURLOPT_HTTPPROXYTUNNEL, 1);
                    }
                    curl_easy_setopt(curlComplete, CURLOPT_POST, 1);
                    curl_easy_setopt(curlComplete, CURLOPT_SSL_VERIFYPEER, 0);
                    curl_easy_setopt(curlComplete, CURLOPT_USERAGENT, userAgent);
                    curl_easy_setopt(curlComplete, CURLOPT_HTTPHEADER, header);
                    curl_easy_setopt(curlComplete, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curlComplete, CURLOPT_POSTFIELDS, jsonComplete_str.c_str());

                    CURLcode errorComplete = curl_easy_perform(curlComplete);
                    curl_slist_free_all(header);
                    curl_easy_cleanup(curlComplete);

                    if(errorComplete == CURLE_OK)
                        driveList.push_back(uploadData);
                }
            }
        }
    }

    delete jsonResp;
}

void drive::adrive::updateFile(const std::string& _fileID, FILE *_upload)
{
    // adrive do not support "UPDATE" operation, delete original file and upload new one later on
    deleteFile(_fileID);
}

void drive::adrive::downloadFile(const std::string& _fileID, FILE *_download)
{
    if(!tokenIsValid())
        refreshToken();

    // URL
    std::string url = adriveURL;
    url.append("/getDownloadUrl");

    // Headers to use
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // JSON To Post
    nlohmann::json post;
    post["drive_id"] = driveID;
    post["file_id"] = _fileID;
    auto json_str = post.dump();

    // Curl
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if(error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(respParse.contains("url"))
        {
            // Download URL
            std::string getURL = respParse["url"].get<std::string>();

            // Headers
            curl_slist *getHeaders = NULL;
            getHeaders = curl_slist_append(getHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

            // Curl
            CURL *curlGet = curl_easy_init();
            if (!proxyURL.empty())
            {
                curl_easy_setopt(curlGet, CURLOPT_PROXY, proxyURL.c_str());
                curl_easy_setopt(curlGet, CURLOPT_HTTPPROXYTUNNEL, 1);
            }
            curl_easy_setopt(curlGet, CURLOPT_HTTPGET, 1);
            curl_easy_setopt(curlGet, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curlGet, CURLOPT_USERAGENT, userAgent);
            curl_easy_setopt(curlGet, CURLOPT_HTTPHEADER, getHeaders);
            curl_easy_setopt(curlGet, CURLOPT_URL, getURL.c_str());
            curl_easy_setopt(curlGet, CURLOPT_BUFFERSIZE, DRIVE_RT_BUFFER_SIZE);
            curl_easy_setopt(curlGet, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataFile);
            curl_easy_setopt(curlGet, CURLOPT_WRITEDATA, _download);
            curl_easy_perform(curlGet);
            curl_slist_free_all(getHeaders);
            curl_easy_cleanup(curlGet);
        }
    }

    delete jsonResp;
}

void drive::adrive::deleteFile(const std::string& _fileID)
{
    if(!tokenIsValid())
        refreshToken();

    // URL
    std::string url = adriveURL;
    url.append("/delete");

    // Headers to use
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // JSON To Post
    nlohmann::json post;
    post["drive_id"] = driveID;
    post["file_id"] = _fileID;
    auto json_str = post.dump();

    // Curl
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if(error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(!respParse.contains("code"))
        {
            for(size_t i = 0; i < driveList.size(); i++)
            {
                if(driveList[i].id == _fileID)
                {
                    driveList.erase(driveList.begin() + i);
                    break;
                }
            }
        }
    }

    delete jsonResp;
}
