// gettext for 3ds, Copyright (C) 2024-2025 R-YaTian
// Ported from ftpd, Copyright (C) 2024 Michael Theall
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

/// \brief Set language for getText
/// \param language_ Language to set
void setLanguage(char const *language_);

/// \brief Translate message
/// \param text_ Text to translate
__attribute__ ((__format_arg__ (1))) char const *getText(char const *text_);

/// \brief Get translated text from string map
/// \param text_ Text to translate
__attribute__ ((__format_arg__ (1))) char const *getTextFromMap(char const *text_);
