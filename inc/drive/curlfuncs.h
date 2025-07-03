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

#include <string>
#include <vector>

#define HEADER_ERROR "ERROR"

namespace curlFuncs
{
    size_t writeDataString(const char *buff, size_t sz, size_t cnt, void *u);
    size_t writeHeaders(const char *buff, size_t sz, size_t cnt, void *u);
    size_t readDataFile(char *buff, size_t sz, size_t cnt, void *u);
    size_t writeDataFile(const char *buff, size_t sz, size_t cnt, void *u);

    std::string getHeader(const std::string& _name, std::vector<std::string> *h);
}
