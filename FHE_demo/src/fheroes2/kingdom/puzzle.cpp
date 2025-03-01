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

#include <random>
#include <vector>
#include <algorithm>
#include "cursor.h"
#include "settings.h"
#include "agg.h"
#include "button.h"
#include "game.h"
#include "game_interface.h"
#include "world.h"
#include "mus.h"
#include "icn.h"

const u8 zone1_index[] = {0, 1, 2, 3, 4, 5, 6, 11, 12, 17, 18, 23, 24, 29, 30, 35, 36, 41, 42, 43, 44, 45, 46, 47};
const u8 zone2_index[] = {7, 8, 9, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 38, 39, 40};
const u8 zone3_index[] = {14, 15, 32, 33};
const u8 zone4_index[] = {20, 21, 26, 27};

bool ClosedTilesExists(const Puzzle&, const u8*, const u8*);

void ZoneOpenFirstTiles(Puzzle&, uint32_t&, const u8*, const u8*);

void ShowStandardDialog(const Puzzle&, const Surface&);

void ShowExtendedDialog(const Puzzle&, const Surface&);

void PuzzlesDraw(const Puzzle&, const Surface&, s32, s32);

Puzzle::Puzzle()
{
    copy(zone1_index, zone1_index + ARRAY_COUNT(zone1_index), zone1_order);
    copy(zone2_index, zone2_index + ARRAY_COUNT(zone2_index), zone2_order);
    copy(zone3_index, zone3_index + ARRAY_COUNT(zone3_index), zone3_order);
    copy(zone4_index, zone4_index + ARRAY_COUNT(zone4_index), zone4_order);

    shuffle(zone1_order, zone1_order + ARRAY_COUNT(zone1_order), std::mt19937(std::random_device()()));
    shuffle(zone2_order, zone2_order + ARRAY_COUNT(zone2_order), std::mt19937(std::random_device()()));
    shuffle(zone3_order, zone3_order + ARRAY_COUNT(zone3_order), std::mt19937(std::random_device()()));
    shuffle(zone4_order, zone4_order + ARRAY_COUNT(zone4_order), std::mt19937(std::random_device()()));
}

Puzzle& Puzzle::operator=(const char* str)
{
    while (str && *str)
    {
        *this <<= 1;
        if (*str == 0x31) set(0);
        ++str;
    }

    return *this;
}

void Puzzle::Update(uint32_t open_obelisk, uint32_t total_obelisk)
{
    const uint32_t open_puzzle = open_obelisk * PUZZLETILES / total_obelisk;
    uint32_t need_puzzle = open_puzzle > count() ? open_puzzle - count() : 0;

    if (need_puzzle && ClosedTilesExists(*this, zone1_order, ARRAY_COUNT_END(zone1_order)))
        ZoneOpenFirstTiles(*this, need_puzzle, zone1_order, ARRAY_COUNT_END(zone1_order));

    if (need_puzzle && ClosedTilesExists(*this, zone2_order, ARRAY_COUNT_END(zone2_order)))
        ZoneOpenFirstTiles(*this, need_puzzle, zone2_order, ARRAY_COUNT_END(zone2_order));

    if (need_puzzle && ClosedTilesExists(*this, zone3_order, ARRAY_COUNT_END(zone3_order)))
        ZoneOpenFirstTiles(*this, need_puzzle, zone3_order, ARRAY_COUNT_END(zone3_order));

    if (need_puzzle && ClosedTilesExists(*this, zone4_order, ARRAY_COUNT_END(zone4_order)))
        ZoneOpenFirstTiles(*this, need_puzzle, zone4_order, ARRAY_COUNT_END(zone4_order));
}

void Puzzle::ShowMapsDialog() const
{
    Cursor& cursor = Cursor::Get();
    Display& display = Display::Get();
    int old_cursor = cursor.Themes();

    if (!Settings::Get().MusicMIDI()) AGG::PlayMusic(MUS::PUZZLE);

    const Surface& sf = world.GetUltimateArtifact().GetPuzzleMapSurface();

    if (sf.isValid())
    {
        cursor.Hide();

        AGG::PlayMusic(MUS::PUZZLE, false);

        if (display.w() == 640 && display.h() == 480 && !Settings::Get().ExtGameHideInterface())
            ShowStandardDialog(*this, sf);
        else
            ShowExtendedDialog(*this, sf);

        cursor.SetThemes(old_cursor);
    }
}

bool ClosedTilesExists(const Puzzle& pzl, const u8* it1, const u8* it2)
{
    while (it1 < it2) if (!pzl.test(*it1++)) return true;
    return false;
}

void ZoneOpenFirstTiles(Puzzle& pzl, uint32_t& opens, const u8* it1, const u8* it2)
{
    while (opens)
    {
        const u8* it = it1;
        while (it < it2 && pzl.test(*it)) ++it;

        if (it != it2)
        {
            pzl.set(*it);
            --opens;
        }
        else
            break;
    }
}

void ShowStandardDialog(const Puzzle& pzl, const Surface& sf)
{
    Display& display = Display::Get();
    Cursor& cursor = Cursor::Get();

    Interface::Radar& radar = Interface::Basic::Get().GetRadar();
    const Rect& radar_pos = radar.GetArea();
    const bool evil_interface = Settings::Get().ExtGameEvilInterface();

    SpriteBack back(Rect(BORDERWIDTH, BORDERWIDTH, sf.w(), sf.h()));

    AGG::GetICN(evil_interface ? ICN::EVIWPUZL : ICN::VIEWPUZL, 0).Blit(radar_pos);
    sf.Blit(BORDERWIDTH, BORDERWIDTH, display);

    const Point dst_pt(radar_pos.x + 32, radar_pos.y + radar_pos.h - 37);
    Button buttonExit(dst_pt.x, dst_pt.y, (evil_interface ? ICN::LGNDXTRE : ICN::LGNDXTRA), 4, 5);

    buttonExit.Draw();
    PuzzlesDraw(pzl, sf, BORDERWIDTH, BORDERWIDTH);

    cursor.SetThemes(Cursor::POINTER);
    cursor.Show();
    display.Flip();
    LocalEvent& le = LocalEvent::Get();

    while (le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();
        if (le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;
    }

    radar.SetRedraw();

    cursor.Hide();
    back.Restore();
}

void ShowExtendedDialog(const Puzzle& pzl, const Surface& sf)
{
    Display& display = Display::Get();
    Cursor& cursor = Cursor::Get();
    const Settings& conf = Settings::Get();
    const Rect& gameArea = Interface::Basic::Get().GetGameArea().GetArea();

    Dialog::FrameBorder frameborder(gameArea.x + (gameArea.w - sf.w() - BORDERWIDTH * 2) / 2,
                                    gameArea.y + (gameArea.h - sf.h() - BORDERWIDTH * 2) / 2,
                                    sf.w(), sf.h() + 32);

    if (conf.ExtGameEvilInterface())
        display.FillRect(frameborder.GetArea(), RGBA(80, 80, 80));
    else
        display.FillRect(frameborder.GetArea(), RGBA(128, 64, 32));
    sf.Blit(frameborder.GetArea(), display);

    Button buttonExit(frameborder.GetArea().x + sf.w() / 2 - 40,
                      frameborder.GetArea().y + sf.h() + 5,
                      (conf.ExtGameEvilInterface() ? ICN::LGNDXTRE : ICN::LGNDXTRA), 4, 5);

    buttonExit.Draw();
    PuzzlesDraw(pzl, sf, frameborder.GetArea().x, frameborder.GetArea().y);

    cursor.SetThemes(Cursor::POINTER);
    cursor.Show();
    display.Flip();
    LocalEvent& le = LocalEvent::Get();

    while (le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();
        if (le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;
    }
}

void PuzzlesDraw(const Puzzle& pzl, const Surface& sf, s32 dstx, s32 dsty)
{
    Display& display = Display::Get();
    Cursor& cursor = Cursor::Get();

    // show all for debug
    if (IS_DEVEL()) return;

    int alpha = 250;
    LocalEvent& le = LocalEvent::Get();

    while (le.HandleEvents() && 0 < alpha)
    {
        if (AnimateInfrequentDelay(Game::PUZZLE_FADE_DELAY))
        {
            cursor.Hide();
            sf.Blit(dstx, dsty, display);
            for (size_t ii = 0; ii < pzl.size(); ++ii)
            {
                const Sprite& sprite = AGG::GetICN(ICN::PUZZLE, ii);
                Sprite piece = Sprite(sprite.GetSurface(), sprite.x(), sprite.y());

                if (pzl.test(ii))
                    piece.SetAlphaMod(alpha);

                piece.Blit(dstx + piece.x() - BORDERWIDTH, dsty + piece.y() - BORDERWIDTH);
            }
            cursor.Show();
            display.Flip();
            alpha -= 10;
        }
    }
    cursor.Hide();
}

ByteVectorWriter& operator<<(ByteVectorWriter& msg, const Puzzle& pzl)
{
    msg << pzl.to_string<char, char_traits<char>, allocator<char>>();

    // orders
    msg << static_cast<u8>(ARRAY_COUNT(pzl.zone1_order));
    for (unsigned char ii : pzl.zone1_order) msg << ii;

    msg << static_cast<u8>(ARRAY_COUNT(pzl.zone2_order));
    for (unsigned char ii : pzl.zone2_order) msg << ii;

    msg << static_cast<u8>(ARRAY_COUNT(pzl.zone3_order));
    for (unsigned char ii : pzl.zone3_order) msg << ii;

    msg << static_cast<u8>(ARRAY_COUNT(pzl.zone4_order));
    for (unsigned char ii : pzl.zone4_order) msg << ii;

    return msg;
}

ByteVectorReader& operator>>(ByteVectorReader& msg, Puzzle& pzl)
{
    string str;

    msg >> str;
    pzl = str.c_str();

    u8 size;

    msg >> size;
    for (uint32_t ii = 0; ii < size; ++ii) msg >> pzl.zone1_order[ii];

    msg >> size;
    for (uint32_t ii = 0; ii < size; ++ii) msg >> pzl.zone2_order[ii];

    msg >> size;
    for (uint32_t ii = 0; ii < size; ++ii) msg >> pzl.zone3_order[ii];

    msg >> size;
    for (uint32_t ii = 0; ii < size; ++ii) msg >> pzl.zone4_order[ii];

    return msg;
}
