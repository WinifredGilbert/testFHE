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

#include <vector>
#include <algorithm>
#include <iostream>

#include "agg.h"
#include "button.h"
#include "dialog.h"
#include "world.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_over.h"
#include "m82.h"
#include "audio_mixer.h"
#include "settings.h"
#include "system.h"

void Interface::Basic::ShowPathOrStartMoveHero(Heroes* hero, s32 dst_index)
{
    if (!hero || hero->Modes(Heroes::GUARDIAN)) return;

    Route::Path& path = hero->GetPath();
    Cursor& cursor = Cursor::Get();

    // show path
    if (path.GetDestinedIndex() != dst_index &&
        path.GetDestinationIndex() != dst_index)
    {
        hero->ResetModes(Heroes::SLEEPER);
        hero->SetMove(false);
        path.Calculate(dst_index);
        gameArea.SetRedraw();
        cursor.SetThemes(GetCursorTileIndex(dst_index));
    }
        // start move
    else if (path.isValid())
    {
        SetFocus(hero);
        RedrawFocus();

        hero->SetMove(true);
        cursor.SetThemes(Cursor::WAIT);
    }
}

void Interface::Basic::MoveHeroFromArrowKeys(Heroes& hero, int direct)
{
    Size wSize(world.w(), world.h());
    if (!Maps::isValidDirection(hero.GetIndex(), direct, wSize)) return;
    s32 dst = Maps::GetDirectionIndex(hero.GetIndex(), direct);
    const Maps::Tiles& tile = world.GetTiles(dst);
    bool allow = false;

    switch (tile.GetObject())
    {
    case MP2::OBJN_CASTLE:
        {
            const Castle* to_castle = world.GetCastle(hero.GetCenter());
            if (to_castle)
            {
                dst = to_castle->GetIndex();
                allow = true;
            }
            break;
        }

    case MP2::OBJ_BOAT:
    case MP2::OBJ_CASTLE:
    case MP2::OBJ_HEROES:
    case MP2::OBJ_MONSTER:
        allow = true;
        break;

    default:
        allow = tile.isPassable(&hero, Direction::CENTER, false) ||
            MP2::isActionObject(tile.GetObject(), hero.isShipMaster());
        break;
    }

    if (allow) ShowPathOrStartMoveHero(&hero, dst);
}

void Interface::Basic::EventNextHero()
{
    const Kingdom& myKingdom = world.GetKingdom(Settings::Get().CurrentColor());
    const KingdomHeroes& myHeroes = myKingdom.GetHeroes();

    if (myHeroes._items.empty()) return;

    if (GetFocusHeroes())
    {
        auto it = find(myHeroes._items.begin(), myHeroes._items.end(),
                       GetFocusHeroes());
        ++it;
        if (it == myHeroes._items.end()) it = myHeroes._items.begin();
        SetFocus(*it);
    }
    else
    {
        ResetFocus(GameFocus::HEROES);
    }
    RedrawFocus();
}

void Interface::Basic::EventContinueMovement()
{
    Heroes* hero = GetFocusHeroes();

    if (hero && hero->GetPath().isValid())
        hero->SetMove(!hero->isEnableMove());
}

void Interface::Basic::EventKingdomInfo() const
{
    Kingdom& myKingdom = world.GetKingdom(Settings::Get().CurrentColor());

    myKingdom.OverviewDialog();

    iconsPanel.SetRedraw();
}

void Interface::Basic::EventCastSpell() const
{
    Heroes* hero = GetFocusHeroes();

    if (!hero) return;
    const Spell spell = hero->OpenSpellBook(SpellBook::ADVN, true);
    // apply cast spell
    if (spell.isValid())
    {
        hero->ActionSpellCast(spell);
        iconsPanel.SetRedraw();
    }
}

int Interface::Basic::EventEndTurn()
{
    const Kingdom& myKingdom = world.GetKingdom(Settings::Get().CurrentColor());

    if (GetFocusHeroes())
        GetFocusHeroes()->SetMove(false);

    if (!myKingdom.HeroesMayStillMove() ||
        Dialog::YES ==
        Dialog::Message("", _("One or more heroes may still move, are you sure you want to end your turn?"), Font::BIG,
                        Dialog::YES | Dialog::NO))
        return Game::ENDTURN;

    return Game::CANCEL;
}

int Interface::Basic::EventAdventureDialog()
{
    Mixer::Reduce();
    switch (Dialog::AdventureOptions(GameFocus::HEROES == GetFocusType()))
    {
    case Dialog::WORLD:
        break;

    case Dialog::PUZZLE:
        EventPuzzleMaps();
        break;

    case Dialog::INFO:
        EventGameInfo();
        break;

    case Dialog::DIG:
        return EventDigArtifact();

    default:
        break;
    }
    Mixer::Enhance();

    return Game::CANCEL;
}

int Interface::Basic::EventFileDialog() const
{
    switch (Dialog::FileOptions())
    {
    case Game::NEWGAME:
        if (Dialog::YES ==
            Dialog::Message("", _("Are you sure you want to restart? (Your current game will be lost)"), Font::BIG,
                            Dialog::YES | Dialog::NO))
            return Game::NEWGAME;
        break;

    case Game::QUITGAME:
        return Game::QUITGAME;

    case Game::LOADGAME:
        return EventLoadGame() ? Game::LOADGAME : Game::NEWGAME;

    case Game::SAVEGAME:
        return EventSaveGame();

    default:
        break;
    }

    return Game::CANCEL;
}

void Interface::Basic::EventSystemDialog()
{
    const Settings& conf = Settings::Get();

    // Change and save system settings
    const int changes = Dialog::SystemOptions();

    // change scroll
    if (0x10 & changes)
    {
        // hardcore reset pos
        gameArea.SetCenter(0, 0);
        if (GetFocusType() != GameFocus::UNSEL)
            gameArea.SetCenter(GetFocusCenter());
        gameArea.SetRedraw();

        if (conf.ExtGameHideInterface())
            controlPanel.ResetTheme();
    }

    // interface themes
    if (0x08 & changes)
    {
        SetRedraw(REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_BORDER);
    }

    // interface hide/show
    if (0x04 & changes)
    {
        GetRadar().ResetAreaSize();
        SetHideInterface(conf.ExtGameHideInterface());
        SetRedraw(REDRAW_ALL);
        ResetFocus(GameFocus::HEROES);
        Redraw();
    }
}

int Interface::Basic::EventExit()
{
    Heroes* hero = GetFocusHeroes();

    // stop hero
    if (hero && hero->isEnableMove())
        hero->SetMove(false);
    else if (Dialog::YES &
        Dialog::Message("", _("Are you sure you want to quit?"), Font::BIG, Dialog::YES | Dialog::NO))
        return Game::QUITGAME;

    return Game::CANCEL;
}

void Interface::Basic::EventNextTown()
{
    Kingdom& myKingdom = world.GetKingdom(Settings::Get().CurrentColor());
    KingdomCastles& myCastles = myKingdom.GetCastles();

    if (myCastles._items.empty()) return;
    if (GetFocusCastle())
    {
        auto it = find(myCastles._items.begin(), myCastles._items.end(),
                       GetFocusCastle());
        ++it;
        if (it == myCastles._items.end()) it = myCastles._items.begin();
        SetFocus(*it);
    }
    else
        ResetFocus(GameFocus::CASTLE);

    RedrawFocus();
}

int Interface::Basic::EventSaveGame() const
{
    string filename = Dialog::SelectFileSave();
    if (!filename.empty() && Game::Save(filename))
        Message("", _("Game saved successfully."), Font::BIG, Dialog::OK);
    return Game::CANCEL;
}

bool Interface::Basic::EventLoadGame()
{
    return Dialog::YES ==
           Dialog::Message("", _("Are you sure you want to load a new game? (Your current game will be lost)"),
                           Font::BIG, Dialog::YES | Dialog::NO)
               ? Game::LOADGAME
               : Game::CANCEL;
}

void Interface::Basic::EventPuzzleMaps()
{
    world.GetKingdom(Settings::Get().CurrentColor()).PuzzleMaps().ShowMapsDialog();
}

void Interface::Basic::EventGameInfo()
{
    Dialog::GameInfo();
}

void Interface::Basic::EventSwitchHeroSleeping()
{
    Heroes* hero = GetFocusHeroes();

    if (!hero) return;
    if (hero->Modes(Heroes::SLEEPER))
        hero->ResetModes(Heroes::SLEEPER);
    else
    {
        hero->SetModes(Heroes::SLEEPER);
        hero->GetPath().Reset();
    }

    SetRedraw(REDRAW_HEROES);
}

int Interface::Basic::EventDigArtifact()
{
    Heroes* hero = GetFocusHeroes();

    if (!hero)
    {
        Message("", _("No hero is selected. Please select one hero"), Font::BIG, Dialog::OK);
        return Game::CANCEL;
    }

    if (hero->isShipMaster())
    {
        Message("", _("Try looking on land!!!"), Font::BIG, Dialog::OK);
        return Game::CANCEL;
    }

    if (hero->GetMaxMovePoints() > hero->GetMovePoints())
    {
        Message("", _("Digging for artifacts requires a whole day, try again tomorrow."), Font::BIG, Dialog::OK);
        return Game::CANCEL;
    }
    if (!world.GetTiles(hero->GetIndex()).GoodForUltimateArtifact())
    {
        Message("", _("Try searching on clear ground."), Font::BIG, Dialog::OK);
        return Game::CANCEL;
    }

    AGG::PlaySound(M82::DIGSOUND);

    hero->ResetMovePoints();

    if (world.DiggingForUltimateArtifact(hero->GetCenter()))
    {
        AGG::PlaySound(M82::TREASURE);
        const Artifact& ultimate = world.GetUltimateArtifact().GetArtifact();
        hero->PickupArtifact(ultimate);
        string msg(_("After spending many hours digging here, you have uncovered the %{artifact}"));
        StringReplace(msg, "%{artifact}", ultimate.GetName());
        Dialog::ArtifactInfo(_("Congratulations!"), msg, ultimate());

        // set all obelisks visited
        Kingdom& kingdom = world.GetKingdom(hero->GetColor());
        const MapsIndexes obelisks = Maps::GetObjectPositions(MP2::OBJ_OBELISK, true);

        for (int obelisk : obelisks)
            if (!hero->isVisited(world.GetTiles(obelisk), Visit::GLOBAL))
                hero->SetVisited(obelisk, Visit::GLOBAL);

        kingdom.PuzzleMaps().Update(kingdom.CountVisitedObjects(MP2::OBJ_OBELISK),
                                    world.CountObeliskOnMaps());
    }
    else
        Message("", _("Nothing here. Where could it be?"), Font::BIG, Dialog::OK);

    Cursor::Get().Hide();
    iconsPanel.RedrawIcons(icons_t::ICON_HEROES);
    Cursor::Get().Show();
    Display::Get().Flip();

    // check game over for ultimate artifact
    return GameOver::Result::Get().LocalCheckGameOver();
}

void Interface::Basic::EventDefaultAction()
{
    Heroes* hero = GetFocusHeroes();

    if (hero)
    {
        const Maps::Tiles& tile = world.GetTiles(hero->GetIndex());

        // 1. action object
        if (MP2::isActionObject(hero->GetMapsObject(), hero->isShipMaster()) &&
            (!MP2::isMoveObject(hero->GetMapsObject()) || hero->CanMove()))
        {
            hero->Action(hero->GetIndex());
            if (MP2::OBJ_STONELIGHTS == tile.GetObject(false) || MP2::OBJ_WHIRLPOOL == tile.GetObject(false))
                SetRedraw(REDRAW_HEROES);
            SetRedraw(REDRAW_GAMEAREA);
        }
        else
            // 2. continue
            if (hero->GetPath().isValid())
                hero->SetMove(true);
            else
                // 3. hero dialog
                Game::OpenHeroesDialog(*hero);
    }
    else
        // 4. town dialog
        if (GetFocusCastle())
        {
            Game::OpenCastleDialog(*GetFocusCastle());
        }
}

void Interface::Basic::EventOpenFocus()
{
    if (GetFocusHeroes())
        Game::OpenHeroesDialog(*GetFocusHeroes());
    else if (GetFocusCastle())
        Game::OpenCastleDialog(*GetFocusCastle());
}

void Interface::Basic::EventSwitchShowRadar() const
{
    Settings& conf = Settings::Get();

    if (!conf.ExtGameHideInterface()) return;
    if (conf.ShowRadar())
    {
        conf.SetShowRadar(false);
        gameArea.SetRedraw();
        return;
    }

    conf.SetShowRadar(true);
    radar.SetRedraw();
}

void Interface::Basic::EventSwitchShowButtons() const
{
    Settings& conf = Settings::Get();

    if (!conf.ExtGameHideInterface())
        return;
    if (conf.ShowButtons())
    {
        conf.SetShowButtons(false);
        gameArea.SetRedraw();
        return;
    }

    conf.SetShowButtons(true);
    buttonsArea.SetRedraw();
}

void Interface::Basic::EventSwitchShowStatus() const
{
    Settings& conf = Settings::Get();

    if (!conf.ExtGameHideInterface())
        return;
    if (conf.ShowStatus())
    {
        conf.SetShowStatus(false);
        gameArea.SetRedraw();
        return;
    }

    conf.SetShowStatus(true);
    statusWindow.SetRedraw();
}

void Interface::Basic::EventSwitchShowIcons()
{
    Settings& conf = Settings::Get();

    if (!conf.ExtGameHideInterface())
        return;
    if (conf.ShowIcons())
    {
        conf.SetShowIcons(false);
        gameArea.SetRedraw();
        return;
    }

    conf.SetShowIcons(true);
    iconsPanel.SetCurrentVisible();
    iconsPanel.SetRedraw();
}

void Interface::Basic::EventSwitchShowControlPanel() const
{
    Settings& conf = Settings::Get();

    if (!conf.ExtGameHideInterface())
        return;
    conf.SetShowPanel(!conf.ShowControlPanel());
    gameArea.SetRedraw();
}

void Interface::Basic::EventKeyArrowPress(int dir)
{
    Heroes* hero = GetFocusHeroes();

    // move hero
    if (hero)
    {
        MoveHeroFromArrowKeys(*hero, dir);
        return;
    }

    // scroll map
    switch (dir)
    {
    case Direction::TOP_LEFT:
        gameArea.SetScroll(SCROLL_TOP);
        gameArea.SetScroll(SCROLL_LEFT);
        break;
    case Direction::TOP:
        gameArea.SetScroll(SCROLL_TOP);
        break;
    case Direction::TOP_RIGHT:
        gameArea.SetScroll(SCROLL_TOP);
        gameArea.SetScroll(SCROLL_RIGHT);
        break;
    case Direction::RIGHT:
        gameArea.SetScroll(SCROLL_RIGHT);
        break;
    case Direction::BOTTOM_RIGHT:
        gameArea.SetScroll(SCROLL_BOTTOM);
        gameArea.SetScroll(SCROLL_RIGHT);
        break;
    case Direction::BOTTOM:
        gameArea.SetScroll(SCROLL_BOTTOM);
        break;
    case Direction::BOTTOM_LEFT:
        gameArea.SetScroll(SCROLL_BOTTOM);
        gameArea.SetScroll(SCROLL_LEFT);
        break;
    case Direction::LEFT:
        gameArea.SetScroll(SCROLL_LEFT);
        break;
    default:
        break;
    }
}

void Interface::Basic::EventDebug1()
{
    H2VERBOSE("");
    /*
        Heroes* hero = GetFocusHeroes();
    
        if(hero)
        {
        int level = hero->GetLevelFromExperience(hero->GetExperience());
        u32 exp = hero->GetExperienceFromLevel(level + 1);
    
        hero->IncreaseExperience(exp - hero->GetExperience() + 100);
        }
    */
}

void Interface::Basic::EventDebug2()
{
    H2VERBOSE("");
}
