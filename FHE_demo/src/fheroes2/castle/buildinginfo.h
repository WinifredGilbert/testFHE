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

#include "payment.h"
#include "castle.h"

class StatusBar;

class Castle;

namespace castle
{
    class BuildingInfo
    {
    public:
        BuildingInfo(const Castle&, building_t);

        uint32_t operator()() const;

        void SetPos(s32, s32);

        const Rect& GetArea() const;

        std::string GetName() const;

        const string& GetDescription() const;

        void SetStatusMessage(StatusBar&) const;

        bool IsDwelling() const;

        void Redraw() const;

        bool QueueEventProcessing() const;

        bool DialogBuyBuilding(bool buttons) const;

        static void UpdateCosts(const string&);

        static payment_t GetCost(uint32_t, int);

    private:
        void RedrawCaptain() const;

        string GetConditionDescription() const;

        const Castle& castle;
        uint32_t building;
        string description;
        Rect area;
        int bcond;

        static bool CanQuickBuild(const Point& cursor, Rect area);
    };

    struct DwellingItem
    {
        DwellingItem(Castle&, uint32_t dw);

        uint32_t type = 0;
        Monster mons;
    };

    class DwellingsBar : public Interface::ItemsBar<DwellingItem>
    {
    public:
        DwellingsBar(Castle&, const Size&, const RGBA& fill);

        void RedrawBackground(const Rect&, Surface&);

        void RedrawItem(DwellingItem&, const Rect&, Surface&);

        bool ActionBarSingleClick(const Point&, DwellingItem&, const Rect&);

        bool ActionBarPressRight(const Point&, DwellingItem&, const Rect&);

    protected:
        Castle& castle;
        Surface backsf;
        vector<DwellingItem> content;
    };
}