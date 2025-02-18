#pragma once

#include <string>
#include <vector>

#include "ui.h"

#define HEADER_ERROR "ERROR"

namespace curlFuncs
{
    size_t writeDataString(const char *buff, size_t sz, size_t cnt, void *u);
    size_t writeHeaders(const char *buff, size_t sz, size_t cnt, void *u);
    size_t readDataFile(char *buff, size_t sz, size_t cnt, void *u);
    size_t writeDataFile(const char *buff, size_t sz, size_t cnt, void *u);

    std::string getHeader(const std::string& _name, std::vector<std::string> *h);
}
