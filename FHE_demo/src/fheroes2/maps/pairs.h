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

#pragma once

#include <utility>
#include "mp2.h"
#include "color.h"
#include "resource.h"
#include "ByteVectorReader.h"

class IndexDistance : public pair<s32, uint32_t>
{
public:
    IndexDistance() : pair<s32, uint32_t>(-1, 0)
    {
    }

    IndexDistance(s32 i, uint32_t d) : pair<s32, uint32_t>(i, d)
    {
    }

    static bool Shortest(const IndexDistance& id1, const IndexDistance& id2)
    {
        return id1.second < id2.second;
    }

    static bool Longest(const IndexDistance& id1, const IndexDistance& id2)
    {
        return id1.second > id2.second;
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter&, const IndexDistance&);
ByteVectorReader& operator>>(ByteVectorReader&, IndexDistance&);

class IndexObject
{
public:
    pair<s32, int> Value;

    IndexObject() : Value(-1, MP2::OBJ_ZERO)
    {
    };

    IndexObject(s32 index, int object) : Value(index, object)
    {
    }

    bool isIndex(s32 index) const
    {
        return index == Value.first;
    }

    bool isObject(int object) const
    {
        return object == Value.second;
    }
};

ByteVectorReader& operator>>(ByteVectorReader&, IndexObject&);
ByteVectorWriter& operator<<(ByteVectorWriter& sb, const IndexObject& st);

class ObjectColor : public pair<int, int>
{
public:
    ObjectColor() : pair<int, int>(MP2::OBJ_ZERO, Color::NONE)
    {
    };

    ObjectColor(int object, int color) : pair<int, int>(object, color)
    {
    }

    bool isObject(int object) const
    {
        return object == first;
    };

    bool isColor(int colors) const
    {
        return colors & second;
    };
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const ObjectColor& st);
ByteVectorReader& operator>>(ByteVectorReader&, ObjectColor&);

class ResourceCount : public pair<int, uint32_t>
{
public:
    ResourceCount() : pair<int, uint32_t>(Resource::UNKNOWN, 0)
    {
    };

    ResourceCount(int res, uint32_t count) : pair<int, uint32_t>(res, count)
    {
    };

    bool isResource(int res) const
    {
        return res == first;
    };

    bool isValid() const
    {
        return first & Resource::ALL && second;
    };
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const ResourceCount& st);
ByteVectorReader& operator>>(ByteVectorReader&, ResourceCount&);
