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

#include "error.h"
#include <algorithm>
#include "agg.h"
#include "artifact.h"
#include "resource.h"
#include "settings.h"
#include "kingdom.h"
#include "race.h"
#include "game_over.h"
#include "maps_tiles.h"
#include "maps_actions.h"
#include "game.h"
#include "world.h"
#include "BinaryFileReader.h"
#include "rand.h"
#include "icn.h"

namespace GameStatic
{
    extern uint32_t uniq;
}

bool World::LoadMapMAP(const string& filename)
{
    return false;
}

bool World::LoadMapMP2(const string& filename)
{
    Reset();
    Defaults();
    if (!FileUtils::Exists(filename))
    {
        Error::Except(__FUNCTION__, "load maps");
    }
    const auto vectorBytes = FileUtils::readFileBytes(filename);
    ByteVectorReader fs(vectorBytes);

    MapsIndexes vec_object; // index maps for OBJ_CASTLE, OBJ_HEROES, OBJ_SIGN, OBJ_BOTTLE, OBJ_EVENT
    vec_object.reserve(100);

    // check (mp2, mx2) ID
    if (fs.getBE32() != 0x5C000000)
        return false;

    // endof
    const uint32_t endof_mp2 = fs.size();
    fs.seek(endof_mp2 - 4);

    // read uniq
    GameStatic::uniq = fs.getLE32();

    // offset data
    fs.seek(MP2OFFSETDATA - 2 * 4);

    // width
    switch (static_cast<mapsize_t>(fs.getLE32()))
    {
    case mapsize_t::SMALL:
        Size::w = static_cast<u16>(mapsize_t::SMALL);
        break;
    case mapsize_t::MEDIUM:
        Size::w = static_cast<u16>(mapsize_t::MEDIUM);
        break;
    case mapsize_t::LARGE:
        Size::w = static_cast<u16>(mapsize_t::LARGE);
        break;
    case mapsize_t::XLARGE:
        Size::w = static_cast<u16>(mapsize_t::XLARGE);
        break;
    default:
        Size::w = 0;
        break;
    }

    // height
    switch (static_cast<mapsize_t>(fs.getLE32()))
    {
    case mapsize_t::SMALL:
        Size::h = static_cast<u16>(mapsize_t::SMALL);
        break;
    case mapsize_t::MEDIUM:
        Size::h = static_cast<u16>(mapsize_t::MEDIUM);
        break;
    case mapsize_t::LARGE:
        Size::h = static_cast<u16>(mapsize_t::LARGE);
        break;
    case mapsize_t::XLARGE:
        Size::h = static_cast<u16>(mapsize_t::XLARGE);
        break;
    default:
        Size::h = 0;
        break;
    }

    if (Size::w == 0 || Size::h == 0 || Size::w != Size::h)
    {
        return false;
    }

    // seek to ADDONS block
    fs.skip(w() * h() * SIZEOFMP2TILE);

    // read all addons
    vector<MP2::mp2addon_t> vec_mp2addons(fs.getLE32() /* count mp2addon_t */);

    for (auto& mp2addon : vec_mp2addons)
    {
        mp2addon.indexAddon = fs.getLE16();
        mp2addon.objectNameN1 = fs.get() * 2;
        mp2addon.indexNameN1 = fs.get();
        mp2addon.quantityN = fs.get();
        mp2addon.objectNameN2 = fs.get();
        mp2addon.indexNameN2 = fs.get();

        mp2addon.uniqNumberN1 = fs.getLE32();
        mp2addon.uniqNumberN2 = fs.getLE32();
    }

    const uint32_t endof_addons = fs.tell();

    // offset data
    fs.seek(MP2OFFSETDATA);

    vec_tiles.resize(w() * h());

    // read all tiles
    for (auto it = vec_tiles.begin(); it != vec_tiles.end(); ++it)
    {
        const size_t index = distance(vec_tiles.begin(), it);
        Maps::Tiles& tile = *it;

        MP2::mp2tile_t mp2tile{};

        mp2tile.tileIndex = fs.getLE16();
        mp2tile.objectName1 = fs.get();
        mp2tile.indexName1 = fs.get();
        mp2tile.quantity1 = fs.get();
        mp2tile.quantity2 = fs.get();
        mp2tile.objectName2 = fs.get();
        mp2tile.indexName2 = fs.get();
        mp2tile.shape = fs.get();
        mp2tile.generalObject = fs.get();

        switch (mp2tile.generalObject)
        {
        case MP2::OBJ_RNDTOWN:
        case MP2::OBJ_RNDCASTLE:
        case MP2::OBJ_CASTLE:
        case MP2::OBJ_HEROES:
        case MP2::OBJ_SIGN:
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_SPHINX:
        case MP2::OBJ_JAIL:
            vec_object.push_back(index);
            break;
        default:
            break;
        }

        // offset first addon
        size_t offsetAddonsBlock = fs.getLE16();

        mp2tile.uniqNumber1 = fs.getLE32();
        mp2tile.uniqNumber2 = fs.getLE32();

        tile.Init(index, mp2tile);

        // load all addon for current tils
        while (offsetAddonsBlock)
        {
            if (vec_mp2addons.size() <= offsetAddonsBlock)
            {
                break;
            }
            tile.AddonsPushLevel1(vec_mp2addons[offsetAddonsBlock]);
            tile.AddonsPushLevel2(vec_mp2addons[offsetAddonsBlock]);
            offsetAddonsBlock = vec_mp2addons[offsetAddonsBlock].indexAddon;
        }

        tile.AddonsSort();
    }


    // after addons
    fs.seek(endof_addons);

    // cood castles
    // 72 x 3 byte (cx, cy, id)
    for (uint32_t ii = 0; ii < 72; ++ii)
    {
        uint32_t cx = fs.get();
        uint32_t cy = fs.get();
        uint32_t id = fs.get();

        // empty block
        if (0xFF == cx && 0xFF == cy) continue;

        switch (id)
        {
        case 0x00: // tower: knight
        case 0x80: // castle: knight
            vec_castles._items.push_back(new Castle(cx, cy, Race::KNGT));
            break;

        case 0x01: // tower: barbarian
        case 0x81: // castle: barbarian
            vec_castles._items.push_back(new Castle(cx, cy, Race::BARB));
            break;

        case 0x02: // tower: sorceress
        case 0x82: // castle: sorceress
            vec_castles._items.push_back(new Castle(cx, cy, Race::SORC));
            break;

        case 0x03: // tower: warlock
        case 0x83: // castle: warlock
            vec_castles._items.push_back(new Castle(cx, cy, Race::WRLK));
            break;

        case 0x04: // tower: wizard
        case 0x84: // castle: wizard
            vec_castles._items.push_back(new Castle(cx, cy, Race::WZRD));
            break;

        case 0x05: // tower: necromancer
        case 0x85: // castle: necromancer
            vec_castles._items.push_back(new Castle(cx, cy, Race::NECR));
            break;

        case 0x06: // tower: random
        case 0x86: // castle: random
            vec_castles._items.push_back(new Castle(cx, cy, Race::NONE));
            break;

        default:
            break;
        }
        // preload in to capture objects cache
        map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_CASTLE, Color::NONE);
    }

    fs.seek(endof_addons + 72 * 3);

    // cood resource kingdoms
    // 144 x 3 byte (cx, cy, id)
    for (uint32_t ii = 0; ii < 144; ++ii)
    {
        uint32_t cx = fs.get();
        uint32_t cy = fs.get();
        uint32_t id = fs.get();

        // empty block
        if (0xFF == cx && 0xFF == cy) continue;

        switch (id)
        {
            // mines: wood
        case 0x00:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_SAWMILL, Color::NONE);
            break;
            // mines: mercury
        case 0x01:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_ALCHEMYLAB, Color::NONE);
            break;
            // mines: ore
        case 0x02:
            // mines: sulfur
        case 0x03:
            // mines: crystal
        case 0x04:
            // mines: gems
        case 0x05:
            // mines: gold
        case 0x06:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_MINES, Color::NONE);
            break;
            // lighthouse
        case 0x64:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_LIGHTHOUSE, Color::NONE);
            break;
            // dragon city
        case 0x65:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_DRAGONCITY, Color::NONE);
            break;
            // abandoned mines
        case 0x67:
            map_captureobj.Set(Maps::GetIndexFromAbsPoint(cx, cy), MP2::OBJ_ABANDONEDMINE, Color::NONE);
            break;
        default:
            break;
        }
    }

    fs.seek(endof_addons + 72 * 3 + 144 * 3);

    // byte: num obelisks (01 default)
    fs.skip(1);

    // count final mp2 blocks
    uint32_t countblock = 0;
    while (true)
    {
        uint32_t l = fs.get();
        uint32_t h = fs.get();

        //VERBOSE("dump block: 0x" << std::setw(2) << std::setfill('0') << std::hex << l <<
        //	std::setw(2) << std::setfill('0') << std::hex << h);

        if (0 == h && 0 == l) break;
        countblock = 256 * h + l - 1;
    }

    // castle or heroes or (events, rumors, etc)
    for (uint32_t ii = 0; ii < countblock; ++ii)
    {
        s32 findobject = -1;

        // read block
        const size_t sizeblock = fs.getLE16();
        auto pblock = fs.getRaw(sizeblock);

        for (auto it_index = vec_object.begin(); it_index != vec_object.end() && findobject < 0; ++it_index)
        {
            const Maps::Tiles& tile = vec_tiles[*it_index];

            // orders(quantity2, quantity1)
            uint32_t orders = tile.GetQuantity2() ? tile.GetQuantity2() : 0;
            orders <<= 8;
            orders |= tile.GetQuantity1();

            if (orders && !(orders % 0x08) && ii + 1 == orders / 0x08)
                findobject = *it_index;
        }

        if (0 <= findobject)
        {
            const Maps::Tiles& tile = vec_tiles[findobject];
            const Maps::TilesAddon* addon;

            switch (tile.GetObject())
            {
            case MP2::OBJ_CASTLE:
                // add castle
                if (SIZEOFMP2CASTLE != pblock.size())
                {
                }
                else
                {
                    Castle* castle = GetCastle(Maps::GetPoint(findobject));
                    if (castle)
                    {
                        ByteVectorReader bvr(pblock);
                        castle->LoadFromMP2(bvr);
                        Maps::MinimizeAreaForCastle(castle->GetCenter());
                        map_captureobj.SetColor(tile.GetIndex(), castle->GetColor());
                    }
                }
                break;
            case MP2::OBJ_RNDTOWN:
            case MP2::OBJ_RNDCASTLE:
                // add rnd castle
                if (SIZEOFMP2CASTLE != pblock.size())
                {
                }
                else
                {
                    Castle* castle = GetCastle(Maps::GetPoint(findobject));
                    if (castle)
                    {
                        ByteVectorReader bvr(pblock);
                        castle->LoadFromMP2(bvr);
                        Maps::UpdateRNDSpriteForCastle(castle->GetCenter(), castle->GetRace(), castle->isCastle());
                        Maps::MinimizeAreaForCastle(castle->GetCenter());
                        map_captureobj.SetColor(tile.GetIndex(), castle->GetColor());
                    }
                    else
                    {
                    }
                }
                break;
            case MP2::OBJ_JAIL:
                // add jail
                if (SIZEOFMP2HEROES != pblock.size())
                {
                }
                else
                {
                    int race = Race::KNGT;
                    switch (pblock[0x3c])
                    {
                    case 1:
                        race = Race::BARB;
                        break;
                    case 2:
                        race = Race::SORC;
                        break;
                    case 3:
                        race = Race::WRLK;
                        break;
                    case 4:
                        race = Race::WZRD;
                        break;
                    case 5:
                        race = Race::NECR;
                        break;
                    default:
                        break;
                    }

                    Heroes* hero = GetFreemanHeroes(race);

                    if (hero)
                    {
                        ByteVectorReader bvr(pblock);
                        hero->LoadFromMP2(findobject, Color::NONE, hero->GetRace(), bvr);
                        hero->SetModes(Heroes::JAIL);
                    }
                }
                break;
            case MP2::OBJ_HEROES:
                // add heroes
                if (SIZEOFMP2HEROES != pblock.size())
                {
                }
                else if (nullptr != (addon = tile.FindObjectConst(MP2::OBJ_HEROES)))
                {
                    pair<int, int> colorRace = Maps::TilesAddon::ColorRaceFromHeroSprite(*addon);
                    Kingdom& kingdom = GetKingdom(colorRace.first);

                    if (colorRace.second == Race::RAND &&
                        colorRace.first != Color::NONE)
                        colorRace.second = kingdom.GetRace();

                    // check heroes max count
                    if (kingdom.AllowRecruitHero(false, 0))
                    {
                        Heroes* hero = nullptr;

                        if (pblock[17] &&
                            pblock[18] < Heroes::BAX)
                            hero = vec_heroes.Get(pblock[18]);

                        if (!hero || !hero->isFreeman())
                            hero = vec_heroes.GetFreeman(colorRace.second);

                        if (hero)
                        {
                            ByteVectorReader bvr(pblock);
                            hero->LoadFromMP2(findobject, colorRace.first, colorRace.second, bvr);
                        }
                    }
                }
                break;
            case MP2::OBJ_SIGN:
            case MP2::OBJ_BOTTLE:
                // add sign or buttle
                if (SIZEOFMP2SIGN - 1 < pblock.size() && 0x01 == pblock[0])
                {
                    auto* obj = new MapSign();
                    ByteVectorReader bvr(pblock);
                    obj->LoadFromMP2(findobject, bvr);
                    map_objects.add(obj);
                }
                break;
            case MP2::OBJ_EVENT:
                // add event maps
                if (SIZEOFMP2EVENT - 1 < pblock.size() && 0x01 == pblock[0])
                {
                    auto* obj = new MapEvent();
                    ByteVectorReader bvr(pblock);
                    obj->LoadFromMP2(findobject, bvr);
                    map_objects.add(obj);
                }
                break;
            case MP2::OBJ_SPHINX:
                // add riddle sphinx
                if (SIZEOFMP2RIDDLE - 1 < pblock.size() && 0x00 == pblock[0])
                {
                    auto* obj = new MapSphinx();
                    ByteVectorReader bvr(pblock);
                    obj->LoadFromMP2(findobject, bvr);
                    map_objects.add(obj);
                }
                break;
            default:
                break;
            }
        }
            // other events
        else if (0x00 == pblock[0])
        {
            // add event day
            if (SIZEOFMP2EVENT - 1 < pblock.size() && 1 == pblock[42])
            {
                vec_eventsday.emplace_back();
                ByteVectorReader bvr(pblock);
                vec_eventsday.back().LoadFromMP2(bvr);
            }
                // add rumors
            else if (SIZEOFMP2RUMOR - 1 < pblock.size())
            {
                if (pblock[8])
                {
                    std::vector<u8> subBlock(pblock.begin() + 8, pblock.end());
                    ByteVectorReader bvr(subBlock);
                    std::string valueRumor = bvr.toString(subBlock.size());
                    vec_rumors.push_back(Game::GetEncodeString(valueRumor));
                }
            }
        }
            // debug
        else
        {
        }
    }

    PostLoad();

    return true;
}

void World::PostLoad()
{
    // modify other objects
    for (size_t ii = 0; ii < vec_tiles.size(); ++ii)
    {
        Maps::Tiles& tile = vec_tiles[ii];

        Maps::Tiles::FixedPreload(tile);

        //
        switch (tile.GetObject())
        {
        case MP2::OBJ_WITCHSHUT:
        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3:
        case MP2::OBJ_STONELIGHTS:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_BOAT:
        case MP2::OBJ_RNDARTIFACT:
        case MP2::OBJ_RNDARTIFACT1:
        case MP2::OBJ_RNDARTIFACT2:
        case MP2::OBJ_RNDARTIFACT3:
        case MP2::OBJ_RNDRESOURCE:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_TREASURECHEST:
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_MAGICGARDEN:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_SHIPWRECKSURVIROR:
        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_ABANDONEDMINE:
        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_TREEKNOWLEDGE:
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLERTENT:
        case MP2::OBJ_MONSTER:
        case MP2::OBJ_RNDMONSTER:
        case MP2::OBJ_RNDMONSTER1:
        case MP2::OBJ_RNDMONSTER2:
        case MP2::OBJ_RNDMONSTER3:
        case MP2::OBJ_RNDMONSTER4:
        case MP2::OBJ_ANCIENTLAMP:
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        case MP2::OBJ_TROLLBRIDGE:
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_WATERALTAR:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
        case MP2::OBJ_BARROWMOUNDS:
            tile.QuantityReset();
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_HEROES:
            {
                Maps::TilesAddon* addon = tile.FindAddonICN1(ICN::MINIHERO);
                // remove event sprite
                if (addon) tile.Remove(addon->uniq);

                tile.SetHeroes(GetHeroes(Maps::GetPoint(ii)));
            }
            break;

        default:
            break;
        }
    }

    // add heroes to kingdoms
    vec_kingdoms.AddHeroes(vec_heroes);

    // add castles to kingdoms
    vec_kingdoms.AddCastles(vec_castles);

    // update wins, loss conditions
    if (GameOver::WINS_HERO & Settings::Get().ConditionWins())
    {
        Heroes* hero = GetHeroes(Settings::Get().WinsMapsPositionObject());
        heroes_cond_wins = hero ? hero->GetID() : Heroes::UNKNOWN;
    }
    if (GameOver::LOSS_HERO & Settings::Get().ConditionLoss())
    {
        Heroes* hero = GetHeroes(Settings::Get().LossMapsPositionObject());
        if (hero)
        {
            heroes_cond_loss = hero->GetID();
            hero->SetModes(Heroes::NOTDISMISS | Heroes::NOTDEFAULTS);
        }
    }

    // update tile passable
    for_each(vec_tiles.begin(), vec_tiles.end(),
             [](Maps::Tiles& tile)
             {
                 tile.UpdatePassable();
             });

    // play with hero
    vec_kingdoms.ApplyPlayWithStartingHero();

    if (Settings::Get().ExtWorldStartHeroLossCond4Humans())
        vec_kingdoms.AddCondLossHeroes(vec_heroes);

    // play with debug hero
    if (IS_DEVEL())
    {
        // get first castle position
        Kingdom& kingdom = GetKingdom(Color::GetFirst(Players::HumanColors()));

        if (!kingdom.GetCastles()._items.empty())
        {
            const Castle* castle = kingdom.GetCastles()._items.front();
            Heroes* hero = vec_heroes.Get(Heroes::SANDYSANDY);

            if (hero)
            {
                const Point& cp = castle->GetCenter();
                hero->Recruit(castle->GetColor(), Point(cp.x, cp.y + 1));
            }
        }
    }

    // set ultimate
    auto it = find_if(vec_tiles.begin(), vec_tiles.end(),
                      [](Maps::Tiles& tile)
                      {
                          return tile.isObject(static_cast<int>(MP2::OBJ_RNDULTIMATEARTIFACT));
                      });

    Point ultimate_pos;

    // not found
    if (vec_tiles.end() == it)
    {
        // generate position for ultimate
        MapsIndexes pools;
        pools.reserve(vec_tiles.size() / 2);

        for (const auto& tile : vec_tiles)
        {
            const s32 x = tile.GetIndex() % w();
            const s32 y = tile.GetIndex() / w();
            if (tile.GoodForUltimateArtifact() &&
                x > 5 && x < w() - 5 && y > 5 && y < h() - 5)
                pools.push_back(tile.GetIndex());
        }

        if (!pools.empty())
        {
            const s32 pos = *Rand::Get(pools);
            ultimate_artifact.Set(pos, Artifact::Rand(Artifact::ART_ULTIMATE));
            ultimate_pos = Maps::GetPoint(pos);
        }
    }
    else
    {
        const Maps::TilesAddon* addon = it->FindObjectConst(MP2::OBJ_RNDULTIMATEARTIFACT);

        // remove ultimate artifact sprite
        if (addon)
        {
            ultimate_artifact.Set((*it).GetIndex(), Artifact::FromMP2IndexSprite(addon->index));
            (*it).Remove(addon->uniq);
            (*it).SetObject(MP2::OBJ_ZERO);
            ultimate_pos = (*it).GetCenter();
        }
    }

    string rumor = _("The ultimate artifact is really the %{name}");
    StringReplace(rumor, "%{name}", ultimate_artifact.GetName());
    vec_rumors.push_back(rumor);

    rumor = _("The ultimate artifact may be found in the %{name} regions of the world.");

    if (world.h() / 3 > ultimate_pos.y)
    {
        if (world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("north-west"));
        else if (2 * world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("north"));
        else
            StringReplace(rumor, "%{name}", _("north-east"));
    }
    else if (2 * world.h() / 3 > ultimate_pos.y)
    {
        if (world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("west"));
        else if (2 * world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("center"));
        else
            StringReplace(rumor, "%{name}", _("east"));
    }
    else
    {
        if (world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("south-west"));
        else if (2 * world.w() / 3 > ultimate_pos.x)
            StringReplace(rumor, "%{name}", _("south"));
        else
            StringReplace(rumor, "%{name}", _("south-east"));
    }
    vec_rumors.push_back(rumor);

    vec_rumors.emplace_back(_("The truth is out there."));
    vec_rumors.emplace_back(_("The dark side is stronger."));
    vec_rumors.emplace_back(_("The end of the world is near."));
    vec_rumors.emplace_back(_("The bones of Lord Slayer are buried in the foundation of the arena."));
    vec_rumors.emplace_back(_("A Black Dragon will take out a Titan any day of the week."));
    vec_rumors.emplace_back(_("He told her: Yada yada yada...  and then she said: Blah, blah, blah..."));

    vec_rumors.emplace_back(
        _("You can load the newest version of game from a site:\n http://sf.net/projects/fheroes2"));
    vec_rumors.emplace_back(_("This game is now in beta development version. ;)"));
}
