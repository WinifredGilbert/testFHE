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

#include <algorithm>

#include "castle.h"
#include "settings.h"
#include "agg.h"
#include "world.h"
#include "game.h"
#include "game_interface.h"
#include "mus.h"

void Interface::Basic::SetFocus(Heroes* hero)
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    if (!player) return;
    Focus& focus = player->GetFocus();

    if (focus.GetHeroes() && focus.GetHeroes() != hero)
    {
        focus.GetHeroes()->SetMove(false);
        focus.GetHeroes()->ShowPath(false);
    }

    hero->RescanPath();
    hero->ShowPath(true);
    focus.Set(hero);

    iconsPanel.Select(*hero);
    gameArea.SetCenter(hero->GetCenter());
    statusWindow.SetState(STATUS_ARMY);

    if (!Game::ChangeMusicDisabled())
    {
        AGG::PlayMusic(MUS::FromGround(world.GetTiles(hero->GetIndex()).GetGround()));
        Game::EnvironmentSoundMixer();
    }
}

void Interface::Basic::SetFocus(Castle* castle)
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    if (!player) return;
    Focus& focus = player->GetFocus();

    if (focus.GetHeroes())
    {
        focus.GetHeroes()->SetMove(false);
        focus.GetHeroes()->ShowPath(false);
    }

    focus.Set(castle);

    iconsPanel.Select(*castle);
    gameArea.SetCenter(castle->GetCenter());
    statusWindow.SetState(STATUS_FUNDS);

    AGG::PlayMusic(MUS::FromGround(world.GetTiles(castle->GetIndex()).GetGround()));
    Game::EnvironmentSoundMixer();
}

void Interface::Basic::ResetFocus(int priority)
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    if (!player) return;
    Focus& focus = player->GetFocus();
    Kingdom& myKingdom = world.GetKingdom(player->GetColor());

    iconsPanel.ResetIcons();

    switch (priority)
    {
    case GameFocus::FIRSTHERO:
        {
            const KingdomHeroes& heroes = myKingdom.GetHeroes();
            // skip sleeping
            const auto it = find_if(heroes._items.begin(), heroes._items.end(),
                                    [&](const Heroes* it) { return it->Modes(Heroes::SLEEPER); });

            if (it != heroes._items.end())
                SetFocus(*it);
            else
                ResetFocus(GameFocus::CASTLE);
        }
        break;

    case GameFocus::HEROES:
        if (focus.GetHeroes() && focus.GetHeroes()->GetColor() == player->GetColor())
            SetFocus(focus.GetHeroes());
        else if (!myKingdom.GetHeroes()._items.empty())
            SetFocus(myKingdom.GetHeroes()._items.front());
        else if (!myKingdom.GetCastles()._items.empty())
        {
            iconsPanel.SetRedraw(icons_t::ICON_HEROES);
            SetFocus(myKingdom.GetCastles()._items.front());
        }
        else
            focus.Reset();
        break;

    case GameFocus::CASTLE:
        if (focus.GetCastle() && focus.GetCastle()->GetColor() == player->GetColor())
            SetFocus(focus.GetCastle());
        else if (!myKingdom.GetCastles()._items.empty())
            SetFocus(myKingdom.GetCastles()._items.front());
        else if (!myKingdom.GetHeroes()._items.empty())
        {
            iconsPanel.SetRedraw(icons_t::ICON_CASTLES);
            SetFocus(myKingdom.GetHeroes()._items.front());
        }
        else
            focus.Reset();
        break;

    default:
        focus.Reset();
        break;
    }
}

int Interface::GetFocusType()
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    if (player)
    {
        Focus& focus = player->GetFocus();

        if (focus.GetHeroes()) return GameFocus::HEROES;
        if (focus.GetCastle()) return GameFocus::CASTLE;
    }

    return GameFocus::UNSEL;
}

Castle* Interface::GetFocusCastle()
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetCastle() : nullptr;
}

Heroes* Interface::GetFocusHeroes()
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    return player ? player->GetFocus().GetHeroes() : nullptr;

    return nullptr;
}

Point Interface::GetFocusCenter()
{
    auto player = Settings::Get().GetPlayers().GetCurrent();

    if (player)
    {
        Focus& focus = player->GetFocus();

        if (focus.GetHeroes()) return focus.GetHeroes()->GetCenter();
        if (focus.GetCastle()) return focus.GetCastle()->GetCenter();
    }

    return {world.w() / 2, world.h() / 2};
}

void Interface::Basic::RedrawFocus()
{
    int type = GetFocusType();

    if (type != FOCUS_HEROES && iconsPanel.IsSelected(icons_t::ICON_HEROES))
    {
        iconsPanel.ResetIcons(icons_t::ICON_HEROES);
        iconsPanel.SetRedraw();
    }
    else if (type == FOCUS_HEROES && !iconsPanel.IsSelected(icons_t::ICON_HEROES))
    {
        iconsPanel.Select(*GetFocusHeroes());
        iconsPanel.SetRedraw();
    }

    if (type != FOCUS_CASTLE && iconsPanel.IsSelected(icons_t::ICON_CASTLES))
    {
        iconsPanel.ResetIcons(icons_t::ICON_CASTLES);
        iconsPanel.SetRedraw();
    }
    else if (type == FOCUS_CASTLE && !iconsPanel.IsSelected(icons_t::ICON_CASTLES))
    {
        iconsPanel.Select(*GetFocusCastle());
        iconsPanel.SetRedraw();
    }

    SetRedraw(REDRAW_GAMEAREA | REDRAW_RADAR);

    if (type == FOCUS_HEROES)
        iconsPanel.SetRedraw(icons_t::ICON_HEROES);
    else if (type == FOCUS_CASTLE)
        iconsPanel.SetRedraw(icons_t::ICON_CASTLES);

    statusWindow.SetRedraw();
}
