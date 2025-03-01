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

#include <algorithm>
#include "agg.h"
#include "text.h"
#include "button.h"
#include "cursor.h"
#include "settings.h"
#include "world.h"
#include "icn.h"
#include "game.h"
#include "dialog.h"
#include <functional>

struct ValueColors : pair<int, int>
{
    ValueColors() : pair<int, int>(0, 0)
    {
    }

    ValueColors(int v, int c) : pair<int, int>(v, c)
    {
    }

    bool IsValue(int v) const
    {
        return v == first;
    }

    //bool IsColor(int c) const { return (c & second); };

    static bool SortValueGreat(const ValueColors& v1, const ValueColors& v2)
    {
        return v1.first > v2.first;
    }
};

void UpdateValuesColors(vector<ValueColors>& v, int value, int color)
{
    const auto it =
        find_if(v.begin(), v.end(),
                [&](const ValueColors& it) { return it.IsValue(value); });

    if (it == v.end())
        v.emplace_back(value, color);
    else
        (*it).second |= color;
}

void GetTownsInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetCountTown();
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetCastlesInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetCountCastle();
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetHeroesInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetHeroes()._items.size();
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetGoldsInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetFunds().Get(Resource::GOLD);
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetWoodOreInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        const Funds& funds = world.GetKingdom(color).GetFunds();
        int value = funds.Get(Resource::WOOD) + funds.Get(Resource::ORE);
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetGemsCrSlfMerInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        const Funds& funds = world.GetKingdom(color).GetFunds();
        int value = funds.Get(Resource::GEMS) + funds.Get(Resource::CRYSTAL) +
            funds.Get(Resource::SULFUR) + funds.Get(Resource::MERCURY);
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetObelisksInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).CountVisitedObjects(MP2::OBJ_OBELISK);
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetArmyInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetArmiesStrength();
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetIncomesInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        int value = world.GetKingdom(color).GetIncome().gold;
        UpdateValuesColors(v, value, color);
    }

    sort(v.begin(), v.end(), ValueColors::SortValueGreat);
}

void GetBestHeroArmyInfo(vector<ValueColors>& v, const Colors& colors)
{
    v.clear();

    for (auto color : colors._items)
    {
        const Heroes* hero = world.GetKingdom(color).GetBestHero();
        v.emplace_back(hero ? hero->GetID() : Heroes::UNKNOWN, color);
    }
}

void DrawFlags(const vector<ValueColors>& v, const Point& pos, uint32_t width, uint32_t count)
{
    for (uint32_t ii = 0; ii < count; ++ii)
    {
        const uint32_t chunk = width / count;
        if (ii >= v.size()) continue;
        const Colors colors(v[ii].second);
        const uint32_t sw = AGG::GetICN(ICN::FLAG32, 1).w();
        s32 px = pos.x + chunk / 2 + ii * chunk - colors._items.size() * sw / 2;

        for (auto color : colors._items)
        {
            const Sprite& flag = AGG::GetICN(ICN::FLAG32, Color::GetIndex(color) * 2 + 1);
            flag.Blit(px, pos.y);
            px = px + sw;
        }
    }
}

void DrawHeroIcons(const vector<ValueColors>& v, const Point& pos, uint32_t width)
{
    if (v.empty()) return;
    Display& display = Display::Get();
    const int chunk = width / v.size();

    for (uint32_t ii = 0; ii < v.size(); ++ii)
    {
        const Heroes* hero = world.GetHeroes(v[ii].first);
        if (!hero) continue;
        Surface icons = hero->GetPortrait(PORT_SMALL);
        s32 px = pos.x + chunk / 2 + ii * chunk;
        const Sprite& window = AGG::GetICN(ICN::LOCATORS, 22);
        window.Blit(px - window.w() / 2, pos.y - 4, display);
        icons.Blit(px - icons.w() / 2, pos.y, display);
    }
}

void Dialog::ThievesGuild(bool oracle)
{
    Display& display = Display::Get();

    // cursor
    Cursor& cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes(Cursor::POINTER);

    FrameBorder frameborder(Size(640, 480));
    const Point& cur_pt = frameborder.GetArea();
    Point dst_pt(cur_pt);

    const uint32_t count = oracle
                               ? 0xFF
                               : world.GetKingdom(Settings::Get().CurrentColor()).GetCountBuilding(
                                   BUILD_THIEVESGUILD);

    vector<ValueColors> v;
    v.reserve(KINGDOMMAX);
    const Colors colors(Game::GetActualKingdomColors());
    const int textx = 185;
    const int startx = 210;
    const int maxw = 430;
    Text text;

    // head 1
    uint32_t ii = 0;
    for (ii = 0; ii < colors._items.size(); ++ii)
    {
        switch (ii + 1)
        {
        case 1:
            text.Set(_("1st"));
            break;
        case 2:
            text.Set(_("2nd"));
            break;
        case 3:
            text.Set(_("3rd"));
            break;
        case 4:
            text.Set(_("4th"));
            break;
        case 5:
            text.Set(_("5th"));
            break;
        case 6:
            text.Set(_("6th"));
            break;
        default:
            break;
        }

        dst_pt.x = cur_pt.x + startx + maxw / (colors._items.size() * 2) + ii * maxw / colors._items.size() - text.w() /
            2;
        dst_pt.y = cur_pt.y + 5;
        text.Blit(dst_pt);
    }

    // bar
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    AGG::GetICN(ICN::WELLXTRA, 2).Blit(dst_pt);

    // text bar
    text.Set(_("Thieves' Guild: Player RanKings"), Font::BIG);
    dst_pt.x = cur_pt.x + 280 - text.w() / 2;
    dst_pt.y = cur_pt.y + 461;
    text.Blit(dst_pt);

    // button exit
    dst_pt.x = cur_pt.x + 578;
    dst_pt.y = cur_pt.y + 461;
    Button buttonExit(dst_pt.x, dst_pt.y, ICN::WELLXTRA, 0, 1);

    text.Set(_("Number of Towns:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 28;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetTownsInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Number of Castles:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 54;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetCastlesInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Number of Heroes:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 80;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetHeroesInfo(v, colors);
    DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Gold in Treasury:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 106;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetGoldsInfo(v, colors);
    if (1 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Wood & Ore:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 132;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetWoodOreInfo(v, colors);
    if (1 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Gems, Cr, Slf & Mer:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 158;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetGemsCrSlfMerInfo(v, colors);
    if (1 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Obelisks Found:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 184;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetObelisksInfo(v, colors);
    if (2 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Total Army Strength:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 210;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetArmyInfo(v, colors);
    if (3 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    text.Set(_("Income:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 238;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetIncomesInfo(v, colors);
    if (4 < count) DrawFlags(v, dst_pt, maxw, colors._items.size());

    // head 2
    ii = 0;
    for (auto
         color = colors._items.begin(); color != colors._items.end(); ++color)
    {
        text.Set(Color::String(*color));
        dst_pt.x = cur_pt.x + startx + maxw / (colors._items.size() * 2) + ii * maxw / colors._items.size() - text.w() /
            2;
        dst_pt.y = cur_pt.y + 270;
        text.Blit(dst_pt);
        ++ii;
    }

    text.Set(_("Best Hero:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 306;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    GetBestHeroArmyInfo(v, colors);
    DrawHeroIcons(v, dst_pt, maxw);

    text.Set(_("Best Hero Stats:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 347;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetBestHeroStatsInfo(v);
    //if(1 < count) DrawHeroIcons(v, dst_pt, maxw);

    text.Set(_("Personality:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 388;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetPersonalityInfo(v);
    //if(2 < count) DrawHeroIcons(v, dst_pt, maxw);

    text.Set(_("Best Monster:"));
    dst_pt.x = cur_pt.x + textx - text.w();
    dst_pt.y = cur_pt.y + 429;
    text.Blit(dst_pt);

    dst_pt.x = cur_pt.x + startx;
    //GetBestMonsterInfo(v);
    //if(3 < count) DrawHeroIcons(v, dst_pt, maxw);

    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent& le = LocalEvent::Get();

    // message loop
    while (le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();

        if (le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;
    }
}
