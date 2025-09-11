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
#include "drive/gd.h"
#include "util.h"

/*
Google Drive code for TYSS.
Modified 3DS version
Original author: J-D-K
Continued by: R-YaTian
*/

#define gdTokenURL "https://oauth2.googleapis.com/token"
#define gdTokenCheckURL "https://oauth2.googleapis.com/tokeninfo"
#define gdriveURL "https://www.googleapis.com/drive/v3/files"
#define gdriveUploadURL "https://www.googleapis.com/upload/drive/v3/files"

drive::gd::gd(const std::string &_clientID, const std::string& _secretID, const std::string& _authCode, const std::string& _rToken)
{
    clientID = _clientID;
    secretID = _secretID;
    rToken = _rToken;

    setupProxy();

    if(!_authCode.empty())
        exhangeAuthCode(_authCode);
    else if(!rToken.empty())
        refreshToken();
}

void drive::gd::exhangeAuthCode(const std::string& _authCode)
{
    // Header
    curl_slist *postHeader = NULL;
    postHeader = curl_slist_append(postHeader, HEADER_CONTENT_TYPE_APP_JSON);

    // Post json
    nlohmann::json post;
    post["client_id"] = clientID;
    post["client_secret"] = secretID;
    post["code"] = _authCode;
    post["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    post["grant_type"] = "authorization_code";
    post["scope"] = "https://www.googleapis.com/auth/drive";
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
    curl_easy_setopt(curl, CURLOPT_URL, gdTokenURL);
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

void drive::gd::refreshToken()
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
    curl_easy_setopt(curl, CURLOPT_URL, gdTokenURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if (respParse.contains("access_token"))
            token = respParse["access_token"].get<std::string>();
    }

    delete jsonResp;
}

bool drive::gd::tokenIsValid()
{
    bool ret = false;

    std::string url = gdTokenCheckURL;
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
        if(!respParse.contains("error"))
            ret = true;
    }

    delete jsonResp;
    return ret;
}

void drive::gd::loadDriveList()
{
    if(!tokenIsValid())
        refreshToken();

    driveList.clear();
    std::string nextPageToken;

    do {
        // Request url with specific fields needed.
        std::string url = gdriveURL;
        url.append("?fields=nextPageToken,files(name,id,mimeType,size,parents)&q=trashed=false\%20and\%20\%27me\%27\%20in\%20owners");

        if (!nextPageToken.empty())
            url.append("&pageToken=" + nextPageToken);

        // Headers needed
        curl_slist *postHeaders = NULL;
        postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

        // Curl request
        CURL *curl = curl_easy_init();
        std::string *jsonResp = new std::string;

        if (!proxyURL.empty())
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
        }
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);

        CURLcode error = curl_easy_perform(curl);
        curl_slist_free_all(postHeaders);
        curl_easy_cleanup(curl);

        if (error == CURLE_OK)
        {
            nlohmann::json parse = nlohmann::json::parse(*jsonResp);

            if (parse.contains("files") && parse["files"].is_array())
            {
                const auto& fileArray = parse["files"];
                for (const auto& curFile : fileArray)
                {
                    std::string name = curFile.value("name", std::string(""));
                    std::string id = curFile.value("id", std::string(""));
                    std::string mimeType = curFile.value("mimeType", std::string(""));
                    unsigned int size = 0;
                    if (curFile.contains("size") && curFile["size"].is_number())
                        size = curFile["size"].get<unsigned int>();

                    drive::driveItem newItem;
                    newItem.name = name;
                    newItem.id = id;
                    newItem.size = size;
                    if (mimeType == MIMETYPE_FOLDER)
                        newItem.isDir = true;

                    if (curFile.contains("parents") && curFile["parents"].is_array()) {
                        const auto& parentArray = curFile["parents"];
                        for (const auto& parent : parentArray) {
                            if (parent.is_string())
                                newItem.parent = parent.get<std::string>();
                        }
                    }

                    if (newItem.isDir
                        || util::endsWith(newItem.name, std::string(".zip"))
                        || util::endsWith(newItem.name, std::string(".sv"))
                        || util::endsWith(newItem.name, std::string(".sav"))
                        || util::endsWith(newItem.name, std::string(".bin")))
                        driveList.push_back(newItem);
                }
            }

            nextPageToken.clear();
            if (parse.contains("nextPageToken") && parse["nextPageToken"].is_string())
                nextPageToken = parse["nextPageToken"];
        }

        delete jsonResp;
    } while (!nextPageToken.empty());
}

bool drive::gd::createDir(const std::string& _dirName, const std::string& _parent)
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
    post["name"] = _dirName;
    post["mimeType"] = MIMETYPE_FOLDER;
    if (!_parent.empty())
    {
        nlohmann::json parentsArray = nlohmann::json::array();
        parentsArray.push_back(_parent);
        post["parents"] = parentsArray;
    }
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
    curl_easy_setopt(curl, CURLOPT_URL, gdriveURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
        if(!respParse.contains("error"))
        {
            drive::driveItem newDir;
            newDir.name = _dirName;
            newDir.id = respParse["id"];
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

void drive::gd::uploadFile(const std::string& _filename, const std::string& _parent, FILE *_upload)
{
    if(!tokenIsValid())
        refreshToken();

    std::string url = gdriveUploadURL;
    url.append("?uploadType=resumable");

    // Headers
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // Post JSON
    nlohmann::json post;
    post["name"] = _filename;
    if (!_parent.empty())
    {
        nlohmann::json parentsArray = nlohmann::json::array();
        parentsArray.push_back(_parent);
        post["parents"] = parentsArray;
    }
    auto json_str = post.dump();

    // Curl upload request
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;
    std::vector<std::string> *headers = new std::vector<std::string>;

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
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlFuncs::writeHeaders);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    if (error == CURLE_OK)
    {
        std::string location = curlFuncs::getHeader("Location", headers);
        if (location != HEADER_ERROR)
        {
            CURL *curlUp = curl_easy_init();
            if (!proxyURL.empty())
            {
                curl_easy_setopt(curlUp, CURLOPT_PROXY, proxyURL.c_str());
                curl_easy_setopt(curlUp, CURLOPT_HTTPPROXYTUNNEL, 1);
            }
            curl_easy_setopt(curlUp, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curlUp, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curlUp, CURLOPT_URL, location.c_str());
            curl_easy_setopt(curlUp, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
            curl_easy_setopt(curlUp, CURLOPT_WRITEDATA, jsonResp);
            curl_easy_setopt(curlUp, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
            curl_easy_setopt(curlUp, CURLOPT_READDATA, _upload);
            curl_easy_setopt(curlUp, CURLOPT_UPLOAD_BUFFERSIZE, DRIVE_RT_BUFFER_SIZE);
            curl_easy_setopt(curlUp, CURLOPT_UPLOAD, 1);
            CURLcode upError = curl_easy_perform(curlUp);
            curl_easy_cleanup(curlUp);

            if (upError == CURLE_OK)
            {
                nlohmann::json respParse = nlohmann::json::parse(*jsonResp);
                if (respParse.contains("id") && respParse.contains("name") && respParse.contains("mimeType"))
                {
                    drive::driveItem uploadData;
                    uploadData.id = respParse["id"].get<std::string>();
                    uploadData.name = respParse["name"].get<std::string>();
                    uploadData.isDir = false;
                    uploadData.parent = _parent;
                    uploadData.size = (unsigned int) util::getFileSize(_upload);
                    driveList.push_back(uploadData);
                }
            }
        }
    }

    delete jsonResp;
    delete headers;
}

void drive::gd::updateFile(const std::string& _fileID, FILE *_upload)
{
    if(!tokenIsValid())
        refreshToken();

    // URL
    std::string url = gdriveUploadURL;
    url.append("/" + _fileID);
    url.append("?uploadType=resumable");

    // Header
    curl_slist *patchHeader = NULL;
    patchHeader = curl_slist_append(patchHeader, std::string(HEADER_AUTHORIZATION + token).c_str());

    // Curl
    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;
    std::vector<std::string> *headers = new std::vector<std::string>;

    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, patchHeader);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlFuncs::writeHeaders);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, headers);

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(patchHeader);
    curl_easy_cleanup(curl);

    if(error == CURLE_OK)
    {
        std::string location = curlFuncs::getHeader("Location", headers);
        if (location != HEADER_ERROR) {
            CURL *curlPatch = curl_easy_init();
            if (!proxyURL.empty())
            {
                curl_easy_setopt(curlPatch, CURLOPT_PROXY, proxyURL.c_str());
                curl_easy_setopt(curlPatch, CURLOPT_HTTPPROXYTUNNEL, 1);
            }
            curl_easy_setopt(curlPatch, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curlPatch, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curlPatch, CURLOPT_URL, location.c_str());
            curl_easy_setopt(curlPatch, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
            curl_easy_setopt(curlPatch, CURLOPT_READDATA, _upload);
            curl_easy_setopt(curlPatch, CURLOPT_UPLOAD_BUFFERSIZE, DRIVE_RT_BUFFER_SIZE);
            curl_easy_setopt(curlPatch, CURLOPT_UPLOAD, 1);
            curl_easy_perform(curlPatch);
            curl_easy_cleanup(curlPatch);
        }
    }

    delete jsonResp;
    delete headers;
}

void drive::gd::downloadFile(const std::string& _fileID, FILE *_download)
{
    if(!tokenIsValid())
        refreshToken();

    // URL
    std::string url = gdriveURL;
    url.append("/" + _fileID);
    url.append("?alt=media");

    // Headers
    curl_slist *getHeaders = NULL;
    getHeaders = curl_slist_append(getHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

    // Curl
    CURL *curl = curl_easy_init();
    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, getHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, DRIVE_RT_BUFFER_SIZE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, _download);
    curl_easy_perform(curl);
    curl_slist_free_all(getHeaders);
    curl_easy_cleanup(curl);
}

void drive::gd::deleteFile(const std::string& _fileID)
{
    if(!tokenIsValid())
        refreshToken();

    // URL
    std::string url = gdriveURL;
    url.append("/" + _fileID);

    // Header
    curl_slist *delHeaders = NULL;
    delHeaders = curl_slist_append(delHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

    // Curl
    CURL *curl = curl_easy_init();
    if (!proxyURL.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, delHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    CURLcode error = curl_easy_perform(curl);
    curl_slist_free_all(delHeaders);
    curl_easy_cleanup(curl);

    if(error == CURLE_OK)
    {
        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].id == _fileID)
            {
                driveList.erase(driveList.begin() + i);
                break;
            }
        }
    }
}
