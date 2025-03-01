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

#include "engine.h"
#include "gamedefs.h"
#include "race.h"
#include "rand.h"

string Race::String(int race)
{
    string str_race[] = {
        _("Knight"), _("Barbarian"), _("Sorceress"),
        _("Warlock"), _("Wizard"), _("Necromancer"), _("Multi"), "Random", "Neutral"
    };

    switch (race)
    {
    case KNGT:
        return str_race[0];
    case BARB:
        return str_race[1];
    case SORC:
        return str_race[2];
    case WRLK:
        return str_race[3];
    case WZRD:
        return str_race[4];
    case NECR:
        return str_race[5];
    case MULT:
        return str_race[6];
    case RAND:
        return str_race[7];
    case NONE:
        return str_race[8];
    default:
        break;
    }

    return str_race[8];
}

int Race::Rand()
{
    switch (Rand::Get(1, 6))
    {
    case 1:
        return KNGT;
    case 2:
        return BARB;
    case 3:
        return SORC;
    case 4:
        return WRLK;
    case 5:
        return WZRD;
    default:
        break;
    }

    return NECR;
}

int Race::FromInt(int race)
{
    switch (race)
    {
    case KNGT:
    case BARB:
    case SORC:
    case WRLK:
    case WZRD:
    case NECR:
    case MULT:
    case RAND:
        return race;
    default:
        break;
    }

    return NONE;
}
