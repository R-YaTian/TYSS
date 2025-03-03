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

#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <3ds.h>

//Reuse update from Switch version
enum buttonEvents
{
    BUTTON_NOTHING,
    BUTTON_PRESSED,
    BUTTON_RELEASED
};

namespace ui
{
    class button
    {
        public:
            button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h);
            void update();
            bool isOver();
            bool wasOver();
            int getEvent()
            {
                return retEvent;
            }

            void draw();

            unsigned getX()
            {
                return x;
            }
            unsigned getY()
            {
                return y;
            }
            unsigned getTx()
            {
                return tx;
            }
            unsigned getTy()
            {
                return ty;
            }

        protected:
            bool pressed = false, first = false;
            int retEvent = BUTTON_NOTHING;
            unsigned x, y, w, h;
            unsigned tx, ty;
            std::string text;
            touchPosition prev, cur;
    };
}

#endif // BUTTON_H
