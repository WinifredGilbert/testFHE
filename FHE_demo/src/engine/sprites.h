/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#pragma once

#include "surface.h"

class SpritePos : public Surface
{
public:
    SpritePos();

    SpritePos(const Surface&, const Point&);

    void SetSurface(const Surface&);

    void SetPos(const Point&);

    void Reset();

    const Point& GetPos() const;

    Rect GetArea() const;

    uint32_t GetMemoryUsage() const;

protected:
    Point pos;
};

class SpriteBack : protected Surface
{
public:
    SpriteBack();

    explicit SpriteBack(const Rect&);

    bool isValid() const;

    void Save(const Point&);

    void Save(const Rect&);

    void Restore() const;

    void Destroy();

    void SetPos(const Point&);

    const Point& GetPos() const;

    const Size& GetSize() const;

    const Rect& GetArea() const;

    uint32_t GetMemoryUsage() const;

protected:
    Rect pos;
};


class SpriteMove : public Surface
{
public:
    SpriteMove();

    explicit SpriteMove(const Surface&);

    void Move(const Point&);

    void Move(int, int);

    void Hide();

    void Show();

    void Redraw();

    bool isVisible() const;

    const Point& GetPos() const;

    const Rect& GetArea() const;

    uint32_t GetMemoryUsage() const;

protected:
    void Show(const Point&);

    SpriteBack background;
    uint32_t mode;
};
