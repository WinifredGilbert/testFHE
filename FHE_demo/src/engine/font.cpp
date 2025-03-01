/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "font.h"
#include <sstream>
#include <iostream>

#include "system.h"

#include <SDL_ttf.h>

FontTTF::FontTTF() = default;

FontTTF::~FontTTF()
{
    if (ptr)
    {
        TTF_CloseFont(ptr);
    }
}

void FontTTF::Init()
{
    if (0 != TTF_Init())
    H2ERROR(SDL_GetError());
}

void FontTTF::Quit()
{
    TTF_Quit();
}

bool FontTTF::isValid() const
{
    return ptr;
}

bool FontTTF::Open(const std::string& filename, int size)
{
    if (ptr) TTF_CloseFont(ptr);
    ptr = TTF_OpenFont(filename.c_str(), size);
    if (!ptr)
    H2ERROR(SDL_GetError());
    return ptr;
}

void FontTTF::SetStyle(int style) const
{
    TTF_SetFontStyle(ptr, style);
}

int FontTTF::Height() const
{
    return TTF_FontHeight(ptr);
}

int FontTTF::Ascent() const
{
    return TTF_FontAscent(ptr);
}

int FontTTF::Descent() const
{
    return TTF_FontDescent(ptr);
}

int FontTTF::LineSkip() const
{
    return TTF_FontLineSkip(ptr);
}

Surface FontTTF::RenderText(const std::string& msg, const RGBA& clr, bool solid) const
{
    const SDL_Color fgColor = clr.packSdlColor();
    auto* paintedSurface = solid
                               ? TTF_RenderUTF8_Solid(ptr, msg.c_str(), fgColor)
                               : TTF_RenderUTF8_Blended(ptr, msg.c_str(), fgColor);
    return Surface(paintedSurface);
}

Surface FontTTF::RenderChar(char ch, const RGBA& clr, bool solid) const
{
    char buf[2] = {'\0', '\0'};
    buf[0] = ch;
    const SDL_Color fgColor = clr.packSdlColor();
    auto* paintedSurface = solid ? TTF_RenderUTF8_Solid(ptr, buf, fgColor) : TTF_RenderUTF8_Blended(ptr, buf, fgColor);
    return Surface(paintedSurface);
}

Surface FontTTF::RenderUnicodeText(const std::vector<u16>& msg, const RGBA& clr, bool solid) const
{
    const SDL_Color fgColor = clr.packSdlColor();
    solid = false;
    return Surface(solid
                       ? TTF_RenderUNICODE_Solid(ptr, &msg[0], fgColor)
                       : TTF_RenderUNICODE_Blended(ptr, &msg[0], fgColor));
}

Surface FontTTF::RenderUnicodeChar(u16 ch, const RGBA& clr, bool solid) const
{
    u16 buf[2] = {L'\0', L'\0'};
    buf[0] = ch;
    solid = false;
    const SDL_Color fgColor = clr.packSdlColor();
    return Surface(solid ? TTF_RenderUNICODE_Solid(ptr, buf, fgColor) : TTF_RenderUNICODE_Blended(ptr, buf, fgColor));
}
