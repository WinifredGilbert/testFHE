/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "artifact.h"
#include "resource.h"
#include "color.h"

class Heroes;

enum
{
    ACTION_UNKNOWN = 0,
    ACTION_DEFAULT,
    ACTION_ACCESS,
    ACTION_MESSAGE,
    ACTION_RESOURCES,
    ACTION_ARTIFACT
};

class ActionSimple
{
public:
    ActionSimple(int v = 0) : uid(0), type(v)
    {
    }

    virtual ~ActionSimple();

    int GetType() const
    {
        return type;
    }

    uint32_t GetUID() const
    {
        return uid;
    }

    void SetUID(uint32_t v)
    {
        uid = v;
    }

protected:
    friend ByteVectorWriter& operator<<(ByteVectorWriter&, const ActionSimple&);

    friend ByteVectorReader& operator>>(ByteVectorReader&, ActionSimple&);

    uint32_t uid;
    int type;
};

ByteVectorWriter& operator<<(ByteVectorWriter&, const ActionSimple&);

ByteVectorReader& operator>>(ByteVectorReader&, ActionSimple&);

struct ActionMessage : ActionSimple
{
    string message;

    ActionMessage() : ActionSimple(ACTION_MESSAGE)
    {
    }

    static bool Action(ActionMessage*, s32, Heroes&);
};

struct ActionDefault : ActionSimple
{
    bool enabled;
    string message;

    ActionDefault() : ActionSimple(ACTION_DEFAULT), enabled(true)
    {
    }

    static bool Action(ActionDefault*, s32, Heroes&);
};

struct ActionAccess : ActionSimple
{
    int allowPlayers;
    bool allowComputer;
    bool cancelAfterFirstVisit;
    string message;

    ActionAccess() : ActionSimple(ACTION_ACCESS), allowPlayers(Color::ALL), allowComputer(true),
                     cancelAfterFirstVisit(false)
    {
    }

    static bool Action(ActionAccess*, s32, Heroes&);
};

struct ActionArtifact : ActionSimple
{
    Artifact artifact;
    string message;

    ActionArtifact() : ActionSimple(ACTION_ARTIFACT)
    {
    }

    static bool Action(ActionArtifact*, s32, Heroes&);
};

struct ActionResources : ActionSimple
{
    Funds resources;
    string message;

    ActionResources() : ActionSimple(ACTION_RESOURCES)
    {
    }

    static bool Action(ActionResources*, s32, Heroes&);
};
