/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "resource.h"
#include "mp2.h"
#include "race.h"
#include "settings.h"
#include "difficulty.h"
#include "skill_static.h"
#include "skill.h"
#include "game_static.h"

namespace Skill
{
    stats_t _stats[] = {
        {
            "knight", {1, 1, 1, 1}, {2, 2, 1, 1}, 0, 0, {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, 10,
            {35, 45, 10, 10}, {25, 25, 25, 25}, {3, 4, 3, 1, 3, 5, 3, 1, 0, 2, 0, 3, 2, 2}
        },
        {
            "barbarian", {1, 1, 1, 1}, {3, 1, 1, 1}, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0}, 10,
            {55, 35, 5, 5}, {25, 25, 25, 25}, {3, 3, 2, 0, 1, 3, 3, 2, 1, 3, 0, 5, 4, 1}
        },
        {
            "sorceress", {0, 0, 2, 2}, {0, 1, 2, 2}, 1, 15, {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1}, 10,
            {20, 15, 32, 33}, {25, 20, 25, 30}, {3, 2, 2, 2, 2, 0, 2, 5, 3, 3, 0, 2, 1, 4}
        },
        {
            "warlock", {0, 0, 2, 2}, {0, 0, 3, 2}, 1, 19, {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 10,
            {15, 15, 40, 30}, {20, 20, 30, 30}, {0, 1, 2, 4, 3, 2, 2, 1, 3, 2, 1, 2, 3, 5}
        },
        {
            "wizard", {0, 0, 2, 2}, {0, 0, 2, 3}, 1, 17, {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, 10,
            {15, 15, 35, 35}, {20, 20, 30, 30}, {2, 0, 2, 3, 3, 2, 2, 2, 4, 2, 0, 2, 2, 5}
        },
        {
            "necromancer", {0, 0, 2, 2}, {1, 0, 2, 2}, 1, 10, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1}, 10,
            {20, 20, 30, 30}, {25, 25, 25, 25}, {1, 2, 3, 3, 2, 0, 2, 1, 3, 2, 5, 3, 0, 4}
        },
        {
            nullptr, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 10, {0, 0, 0, 0},
            {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        }
    };

    values_t _values[] = {
        {"pathfinding", {25, 50, 100}},
        {"archery", {10, 25, 50}},
        {"logistics", {10, 20, 30}},
        {"scouting", {1, 3, 5}},
        {"diplomacy", {25, 50, 100}},
        {"navigation", {33, 66, 100}},
        {"leadership", {1, 2, 3}},
        {"wisdom", {3, 4, 5}},
        {"mysticism", {2, 3, 4}},
        {"luck", {1, 2, 3}},
        {"ballistics", {0, 0, 0}},
        {"eagleeye", {25, 40, 65}},
        {"necromancy", {10, 20, 30}},
        {"estates", {100, 250, 500}},
        {nullptr, {0, 0, 0}},
    };

    secondary_t _from_witchs_hut = {
        /* archery */ 1, /* ballistics */ 1, /* diplomacy */ 1, /* eagleeye */ 1,
        /* estates */ 1, /* leadership */ 0, /* logistics */ 1, /* luck */ 1,
        /* mysticism */ 1, /* navigation */ 1, /* necromancy*/ 0, /* pathfinding */ 1,
        /* scouting */ 1, /* wisdom */ 1
    };

    ByteVectorWriter& operator<<(ByteVectorWriter& msg, const level_t& obj)
    {
        return msg << obj.basic << obj.advanced << obj.expert;
    }

    ByteVectorReader& operator>>(ByteVectorReader& msg, level_t& obj)
    {
        return msg >> obj.basic >> obj.advanced >> obj.expert;
    }

    ByteVectorWriter& operator<<(ByteVectorWriter& msg, const primary_t& obj)
    {
        return msg << obj.attack << obj.defense << obj.power << obj.knowledge;
    }

    ByteVectorReader& operator>>(ByteVectorReader& msg, primary_t& obj)
    {
        return msg >> obj.attack >> obj.defense >> obj.power >> obj.knowledge;
    }

    ByteVectorWriter& operator<<(ByteVectorWriter& msg, const secondary_t& obj)
    {
        return msg << obj.archery << obj.ballistics << obj.diplomacy << obj.eagleeye << obj.estates << obj.leadership <<
            obj.logistics << obj.luck << obj.mysticism << obj.navigation << obj.necromancy << obj.pathfinding
            << obj.scouting << obj.wisdom;
    }

    ByteVectorReader& operator>>(ByteVectorReader& msg, secondary_t& obj)
    {
        return msg >> obj.archery >> obj.ballistics >> obj.diplomacy >> obj.eagleeye >> obj.estates >> obj.leadership >>
            obj.logistics >> obj.luck >> obj.mysticism >> obj.navigation >> obj.necromancy >> obj.pathfinding
            >> obj.scouting >> obj.wisdom;
    }

    ByteVectorWriter& operator<<(ByteVectorWriter& msg, const stats_t& obj)
    {
        return msg << obj.captain_primary << obj.initial_primary << obj.initial_book << obj.initial_spell <<
            obj.initial_secondary << obj.over_level << obj.mature_primary_under << obj.mature_primary_over
            << obj.mature_secondary;
    }

    ByteVectorReader& operator>>(ByteVectorReader& msg, stats_t& obj)
    {
        return msg >> obj.captain_primary >> obj.initial_primary >> obj.initial_book >> obj.initial_spell >>
            obj.initial_secondary >> obj.over_level >> obj.mature_primary_under >> obj.mature_primary_over
            >> obj.mature_secondary;
    }

    ByteVectorWriter& operator<<(ByteVectorWriter& msg, const values_t& obj)
    {
        return msg << obj.values;
    }

    ByteVectorReader& operator>>(ByteVectorReader& msg, values_t& obj)
    {
        return msg >> obj.values;
    }
}

namespace GameStatic
{
    u8 whirlpool_lost_percent = 50;

    /* town, castle, heroes, artifact_telescope, object_observation_tower, object_magi_eyes */
    u8 overview_distance[] = {4, 5, 4, 1, 10, 9, 8};

    u8 gameover_lost_days = 7;

    // kingdom
    u8 kingdom_max_heroes = 8;
    cost_t kingdom_starting_resource[] = {
        {10000, 30, 10, 30, 10, 10, 10},
        {7500, 20, 5, 20, 5, 5, 5},
        {5000, 10, 2, 10, 2, 2, 2},
        {2500, 5, 0, 5, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0},
        // ai resource
        {10000, 30, 10, 30, 10, 10, 10}
    };

    // castle
    u8 castle_grown_well = 2;
    u8 castle_grown_wel2 = 8;
    u8 castle_grown_week_of = 5;
    u8 castle_grown_month_of = 100;

    u8 mageguild_restore_spell_points_day[] = {20, 40, 60, 80, 100};

    // heroes
    u8 heroes_spell_points_day = 1;

    // spells
    u16 spell_dd_distance = 0;
    u16 spell_dd_sp = 0;
    u16 spell_dd_hp = 0;

    // monsters
    float monster_upgrade_ratio = 1.0;

    // visit objects mod:	OBJ_BUOY, OBJ_OASIS, OBJ_WATERINGHOLE, OBJ_TEMPLE, OBJ_GRAVEYARD, OBJ_DERELICTSHIP,
    //			        OBJ_SHIPWRECK, OBJ_MERMAID, OBJ_FAERIERING, OBJ_FOUNTAIN, OBJ_IDOL, OBJ_PYRAMID
    s8 objects_mod[] = {1, 1, 1, 2, -1, -1, -1, 1, 1, 1, 1, -2};

    // world
    uint32_t uniq = 0;
}


ByteVectorWriter& GameStatic::operator<<(ByteVectorWriter& msg, const Data& obj)
{
    msg <<
        whirlpool_lost_percent <<
        kingdom_max_heroes <<
        castle_grown_well <<
        castle_grown_wel2 <<
        castle_grown_week_of <<
        castle_grown_month_of <<
        heroes_spell_points_day <<
        gameover_lost_days <<
        spell_dd_distance <<
        spell_dd_sp <<
        spell_dd_hp;

    u8 array_size = ARRAY_COUNT(overview_distance);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << overview_distance[ii];

    array_size = ARRAY_COUNT(kingdom_starting_resource);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << kingdom_starting_resource[ii];

    array_size = ARRAY_COUNT(mageguild_restore_spell_points_day);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << mageguild_restore_spell_points_day[ii];

    array_size = ARRAY_COUNT(objects_mod);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << objects_mod[ii];

    msg << monster_upgrade_ratio << uniq;

    // skill statics
    array_size = ARRAY_COUNT(Skill::_stats);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << Skill::_stats[ii];

    array_size = ARRAY_COUNT(Skill::_values);
    msg << array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg << Skill::_values[ii];

    msg << Skill::_from_witchs_hut;

    return msg;
}

ByteVectorReader& GameStatic::operator>>(ByteVectorReader& msg, Data& obj)
{
    msg >>
        whirlpool_lost_percent >>
        kingdom_max_heroes >>
        castle_grown_well >>
        castle_grown_wel2 >>
        castle_grown_week_of >>
        castle_grown_month_of >>
        heroes_spell_points_day >>
        gameover_lost_days >>
        spell_dd_distance >>
        spell_dd_sp >>
        spell_dd_hp;

    u8 array_size = 0;

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> overview_distance[ii];

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> kingdom_starting_resource[ii];

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> mageguild_restore_spell_points_day[ii];

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> objects_mod[ii];

    msg >> monster_upgrade_ratio >> uniq;

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> Skill::_stats[ii];

    msg >> array_size;
    for (uint32_t ii = 0; ii < array_size; ++ii)
        msg >> Skill::_values[ii];

    msg >> Skill::_from_witchs_hut;

    return msg;
}

float GameStatic::GetMonsterUpgradeRatio()
{
    return monster_upgrade_ratio;
}

uint32_t GameStatic::GetLostOnWhirlpoolPercent()
{
    return whirlpool_lost_percent;
}

uint32_t GameStatic::GetOverViewDistance(uint32_t d)
{
    return d >= ARRAY_COUNT(overview_distance) ? 0 : overview_distance[d];
}

uint32_t GameStatic::GetGameOverLostDays()
{
    return gameover_lost_days;
}

cost_t& GameStatic::GetKingdomStartingResource(int df)
{
    switch ((DifficultyEnum)df)
    {
    case DifficultyEnum::EASY:
        return kingdom_starting_resource[0];
    case DifficultyEnum::NORMAL:
        return kingdom_starting_resource[1];
    case DifficultyEnum::HARD:
        return kingdom_starting_resource[2];
    case DifficultyEnum::EXPERT:
        return kingdom_starting_resource[3];
    case DifficultyEnum::IMPOSSIBLE:
        return kingdom_starting_resource[4];
    }

    return kingdom_starting_resource[5];
}

uint32_t GameStatic::GetHeroesRestoreSpellPointsPerDay()
{
    return heroes_spell_points_day;
}

uint32_t GameStatic::GetMageGuildRestoreSpellPointsPercentDay(int level)
{
    return level && level < 6 ? mageguild_restore_spell_points_day[level - 1] : 0;
}

uint32_t GameStatic::GetKingdomMaxHeroes()
{
    return kingdom_max_heroes;
}

uint32_t GameStatic::GetCastleGrownWell()
{
    return castle_grown_well;
}

uint32_t GameStatic::GetCastleGrownWel2()
{
    return castle_grown_wel2;
}

uint32_t GameStatic::GetCastleGrownWeekOf()
{
    return castle_grown_week_of;
}

uint32_t GameStatic::GetCastleGrownMonthOf()
{
    return castle_grown_month_of;
}

s32 GameStatic::ObjectVisitedModifiers(int obj)
{
    switch (obj)
    {
    case MP2::OBJ_BUOY:
        return objects_mod[0];
    case MP2::OBJ_OASIS:
        return objects_mod[1];
    case MP2::OBJ_WATERINGHOLE:
        return objects_mod[2];
    case MP2::OBJ_TEMPLE:
        return objects_mod[3];
    case MP2::OBJ_GRAVEYARD:
        return objects_mod[4];
    case MP2::OBJ_DERELICTSHIP:
        return objects_mod[5];
    case MP2::OBJ_SHIPWRECK:
        return objects_mod[6];
    case MP2::OBJ_MERMAID:
        return objects_mod[7];
    case MP2::OBJ_FAERIERING:
        return objects_mod[8];
    case MP2::OBJ_FOUNTAIN:
        return objects_mod[9];
    case MP2::OBJ_IDOL:
        return objects_mod[10];
    case MP2::OBJ_PYRAMID:
        return objects_mod[11];
    default:
        break;
    }

    return 0;
}

uint32_t GameStatic::Spell_DD_Distance()
{
    return spell_dd_distance;
}

uint32_t GameStatic::Spell_DD_SP()
{
    return spell_dd_sp;
}

uint32_t GameStatic::Spell_DD_HP()
{
    return spell_dd_hp;
}

void GameStatic::SetSpell_DD_Distance(int v)
{
    spell_dd_distance = v;
}

void GameStatic::SetSpell_DD_SP(int v)
{
    spell_dd_sp = v;
}

void GameStatic::SetSpell_DD_HP(int v)
{
    spell_dd_hp = v;
}

const Skill::stats_t* GameStatic::GetSkillStats(int race)
{
    switch (race)
    {
    case Race::KNGT:
        return &Skill::_stats[0];
    case Race::BARB:
        return &Skill::_stats[1];
    case Race::SORC:
        return &Skill::_stats[2];
    case Race::WRLK:
        return &Skill::_stats[3];
    case Race::WZRD:
        return &Skill::_stats[4];
    case Race::NECR:
        return &Skill::_stats[5];
    default:
        break;
    }

    return nullptr;
}

const Skill::values_t* GameStatic::GetSkillValues(Skill::SkillT type)
{
    switch (type)
    {
    case Skill::SkillT::PATHFINDING:
        return &Skill::_values[0];
    case Skill::SkillT::ARCHERY:
        return &Skill::_values[1];
    case Skill::SkillT::LOGISTICS:
        return &Skill::_values[2];
    case Skill::SkillT::SCOUTING:
        return &Skill::_values[3];
    case Skill::SkillT::DIPLOMACY:
        return &Skill::_values[4];
    case Skill::SkillT::NAVIGATION:
        return &Skill::_values[5];
    case Skill::SkillT::LEADERSHIP:
        return &Skill::_values[6];
    case Skill::SkillT::WISDOM:
        return &Skill::_values[7];
    case Skill::SkillT::MYSTICISM:
        return &Skill::_values[8];
    case Skill::SkillT::LUCK:
        return &Skill::_values[9];
    case Skill::SkillT::BALLISTICS:
        return &Skill::_values[10];
    case Skill::SkillT::EAGLEEYE:
        return &Skill::_values[11];
    case Skill::SkillT::NECROMANCY:
        return &Skill::_values[12];
    case Skill::SkillT::ESTATES:
        return &Skill::_values[13];
    default:
        break;
    }

    return nullptr;
}

const Skill::secondary_t* GameStatic::GetSkillForWitchsHut()
{
    return &Skill::_from_witchs_hut;
}

/*
*/


void Skill::UpdateStats(const string& stats)
{
}


GameStatic::Data& GameStatic::Data::Get()
{
    static Data gds;
    return gds;
}

int GameStatic::GetBattleMoatReduceDefense()
{
    return 2;
}
