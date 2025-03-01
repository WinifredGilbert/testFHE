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

#include <cmath>
#include <algorithm>
#include "artifact.h"
#include "world.h"
#include "settings.h"
#include "agg.h"
#include "speed.h"
#include "luck.h"
#include "race.h"
#include "ground.h"
#include "morale.h"
#include "cursor.h"
#include "battle.h"
#include "game.h"
#include "game_interface.h"
#include "game_static.h"
#include "ai.h"
#include "m82.h"
#include "rand.h"
#include "icn.h"

#include <sstream>
#include <iostream>

std::string Heroes::GetName(int id)
{
    std::string names[] = {
        // knight
        _("Lord Kilburn"), _("Sir Gallanth"), _("Ector"), _("Gwenneth"), _("Tyro"), _("Ambrose"), _("Ruby"),
        _("Maximus"), _("Dimitry"),
        // barbarian
        _("Thundax"), _("Fineous"), _("Jojosh"), _("Crag Hack"), _("Jezebel"), _("Jaclyn"), _("Ergon"), _("Tsabu"),
        _("Atlas"),
        // sorceress
        _("Astra"), _("Natasha"), _("Troyan"), _("Vatawna"), _("Rebecca"), _("Gem"), _("Ariel"), _("Carlawn"),
        _("Luna"),
        // warlock
        _("Arie"), _("Alamar"), _("Vesper"), _("Crodo"), _("Barok"), _("Kastore"), _("Agar"), _("Falagar"),
        _("Wrathmont"),
        // wizard
        _("Myra"), _("Flint"), _("Dawn"), _("Halon"), _("Myrini"), _("Wilfrey"), _("Sarakin"), _("Kalindra"),
        _("Mandigal"),
        // necromant
        _("Zom"), _("Darlana"), _("Zam"), _("Ranloo"), _("Charity"), _("Rialdo"), _("Roxana"), _("Sandro"),
        _("Celia"),
        // campains
        _("Roland"), _("Lord Corlagon"), _("Sister Eliza"), _("Archibald"), _("Lord Halton"), _("Brother Bax"),
        // loyalty version
        _("Solmyr"), _("Dainwin"), _("Mog"), _("Uncle Ivan"), _("Joseph"), _("Gallavant"), _("Elderian"),
        _("Ceallach"), _("Drakonia"), _("Martine"), _("Jarkonas"),
        // debug
        "SandySandy", "Unknown"
    };

    return names[id];
}

int ObjectVisitedModifiersResult(int type, const u8* objs, uint32_t size, const Heroes& hero, string* strs)
{
    int result = 0;

    for (uint32_t ii = 0; ii < size; ++ii)
    {
        if (hero.isVisited(objs[ii]))
        {
            result += GameStatic::ObjectVisitedModifiers(objs[ii]);

            if (strs)
            {
                strs->append(MP2::StringObject(objs[ii]));
                StringAppendModifiers(*strs, GameStatic::ObjectVisitedModifiers(objs[ii]));
                strs->append("\n");
            }
        }
    }

    return result;
}

Heroes::Heroes() : move_point_scale(-1), army(this), hid(UNKNOWN), portrait(UNKNOWN), race(UNKNOWN),
                   save_maps_object(0), path(*this),
                   direction(Direction::RIGHT), sprite_index(18), patrol_square(0)
{
}

Heroes::Heroes(int heroid, int rc) :
    HeroBase(HEROES, rc), ColorBase(Color::NONE),
    experience(0), move_point_scale(-1), secondary_skills(rc), army(this), hid(heroid),
    portrait(heroid), race(rc),
    save_maps_object(MP2::OBJ_ZERO), path(*this), direction(Direction::RIGHT),
    sprite_index(18), patrol_square(0)
{
    name = _(Heroes::GetName(heroid));

    // set default army
    army.Reset(true);

    // extra hero
    switch (hid)
    {
    case ROLAND:
        attack = 0;
        defense = 1;
        power = 4;
        knowledge = 5;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::ARCHERY, Skill::Level::BASIC));
        break;

    case CORLAGON:
        attack = 5;
        defense = 3;
        power = 1;
        knowledge = 1;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::NECROMANCY, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::BALLISTICS, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::PATHFINDING, Skill::Level::BASIC));
        break;

    case ELIZA:
        attack = 0;
        defense = 1;
        power = 2;
        knowledge = 6;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::NAVIGATION, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::ARCHERY, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LUCK, Skill::Level::BASIC));
        break;

    case ARCHIBALD:
        attack = 1;
        defense = 1;
        power = 4;
        knowledge = 4;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::SCOUTING, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::ADVANCED));
        break;

    case HALTON:
        attack = 3;
        defense = 3;
        power = 3;
        knowledge = 2;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::BALLISTICS, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::DIPLOMACY, Skill::Level::BASIC));
        break;

    case BAX:
        attack = 1;
        defense = 1;
        power = 4;
        knowledge = 3;

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::EXPERT));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::NECROMANCY, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::NAVIGATION, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::PATHFINDING, Skill::Level::BASIC));
        break;

    case SOLMYR:
    case DRAKONIA:
        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::BASIC));
        break;

    case DAINWIN:
    case ELDERIAN:
        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::SCOUTING, Skill::Level::BASIC));
        break;

    case MOG:
        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::WISDOM, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::NECROMANCY, Skill::Level::ADVANCED));
        break;

    case UNCLEIVAN:
        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::PATHFINDING, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::BASIC));
        break;

    case JOSEPH:
        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LEADERSHIP, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::SCOUTING, Skill::Level::BASIC));
        break;

    case GALLAVANT:
        break;

    case CEALLACH:
        break;

    case MARTINE:
        break;

    case JARKONAS:
        break;

    case SANDYSANDY:
        army.m_troops.Clean();
        army.m_troops.JoinTroop(Monster::BLACK_DRAGON, 2);
        army.m_troops.JoinTroop(Monster::RED_DRAGON, 3);

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::PATHFINDING, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::LOGISTICS, Skill::Level::ADVANCED));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::SCOUTING, Skill::Level::BASIC));
        secondary_skills.AddSkill(Skill::Secondary(Skill::SkillT::MYSTICISM, Skill::Level::BASIC));

        PickupArtifact(Artifact::STEALTH_SHIELD);
        PickupArtifact(Artifact::DRAGON_SWORD);
        PickupArtifact(Artifact::NOMAD_BOOTS_MOBILITY);
        PickupArtifact(Artifact::TRAVELER_BOOTS_MOBILITY);
        PickupArtifact(Artifact::TRUE_COMPASS_MOBILITY);

        experience = 777;
        magic_point = 120;

        // all spell in magic book
        for (uint32_t spell = Spell::FIREBALL; spell < Spell::STONE; ++spell) AppendSpellToBook(Spell(spell), true);
        break;

    default:
        break;
    }

    if (!magic_point)
        SetSpellPoints(GetMaxSpellPoints());
    move_point = GetMaxMovePoints();
}

void Heroes::LoadFromMP2(s32 map_index, int cl, int rc, ByteVectorReader& st)
{
    // reset modes
    modes = 0;

    SetIndex(map_index);
    SetColor(cl);

    // unknown
    st.skip(1);

    // custom troops
    if (st.get())
    {
        Troop troops[5];

        // set monster id
        for (auto& troop : troops)
            troop.SetMonster(st.get() + 1);

        // set count
        for (auto& troop : troops)
            troop.SetCount(st.getLE16());

        army.m_troops.Assign(troops, ARRAY_COUNT_END(troops));
    }
    else
        st.skip(15);

    // custom portrate
    bool custom_portrait = st.get();

    if (custom_portrait)
    {
        SetModes(NOTDEFAULTS);

        // index sprite portrait
        portrait = st.get();

        if (UNKNOWN <= portrait)
        {
            portrait = hid;
        }

        // fixed race for custom portrait (after level up)
        if (race != rc)
            race = rc;
    }
    else
        st.skip(1);

    // 3 artifacts
    PickupArtifact(Artifact(st.get()));
    PickupArtifact(Artifact(st.get()));
    PickupArtifact(Artifact(st.get()));

    // unknown byte
    st.skip(1);

    // experience
    experience = st.getLE32();

    bool custom_secskill = st.get();

    // custom skill
    if (custom_secskill)
    {
        SetModes(NOTDEFAULTS);
        SetModes(CUSTOMSKILLS);
        vector<Skill::Secondary> secs(8);

        for (auto& sec : secs)
            sec.SetSkill((Skill::SkillT)(st.get() + 1));

        for (auto& sec : secs)
            sec.SetLevel(st.get());

        secondary_skills = Skill::SecSkills();

        for (vector<Skill::Secondary>::const_iterator
             it = secs.begin(); it != secs.end(); ++it)
            if ((*it).isValid()) secondary_skills.AddSkill(*it);
    }
    else
        st.skip(16);

    // unknown
    st.skip(1);

    // custom name
    if (st.get())
    {
        SetModes(NOTDEFAULTS);
        name = Game::GetEncodeString(st.toString(13));
    }
    else
        st.skip(13);

    // patrol
    if (st.get())
    {
        SetModes(PATROL);
        patrol_center = GetCenter();
    }

    // count square
    patrol_square = st.get();

    PostLoad();
}

void Heroes::PostLoad()
{
    killer_color.SetColor(Color::NONE);

    // save general object
    save_maps_object = MP2::OBJ_ZERO;

    // fix zero army
    if (!army.m_troops.IsValid())
        army.Reset(true);
    else
        SetModes(CUSTOMARMY);

    // level up
    int level = GetLevel();
    while (1 < level--)
    {
        SetModes(NOTDEFAULTS);
        LevelUp(Modes(CUSTOMSKILLS), true);
    }

    if (race & (Race::SORC | Race::WRLK | Race::WZRD | Race::NECR) &&
        !HaveSpellBook())
    {
        Spell spell = GetInitialSpell(race);
        if (spell.isValid())
        {
            SpellBookActivate();
            AppendSpellToBook(spell, true);
        }
    }

    // other param
    SetSpellPoints(GetMaxSpellPoints());
    move_point = GetMaxMovePoints();

    if (isControlAI())
    {
        AI::HeroesPostLoad(*this);
    }
}

int Heroes::GetID() const
{
    return hid;
}

int Heroes::GetRace() const
{
    return race;
}

const string& Heroes::GetName() const
{
    return name;
}

int Heroes::GetColor() const
{
    return ColorBase::GetColor();
}

int Heroes::GetType() const
{
    return HEROES;
}

const Army& Heroes::GetArmy() const
{
    return army;
}

Army& Heroes::GetArmy()
{
    return army;
}

int Heroes::GetMobilityIndexSprite() const
{
    // valid range (0 - 25)
    int index = !CanMove() ? 0 : move_point / 100;
    return 25 >= index ? index : 25;
}

int Heroes::GetManaIndexSprite() const
{
    // valid range (0 - 25)
    int r = GetSpellPoints() / 5;
    return 25 >= r ? r : 25;
}

int Heroes::GetAttack() const
{
    return GetAttack(nullptr);
}

int Heroes::GetAttack(string* strs) const
{
    int result = attack + GetAttackModificator(strs);
    return result < 0 ? 0 : result > 255 ? 255 : result;
}

int Heroes::GetDefense() const
{
    return GetDefense(nullptr);
}

int Heroes::GetDefense(string* strs) const
{
    int result = defense + GetDefenseModificator(strs);
    return result < 0 ? 0 : result > 255 ? 255 : result;
}

int Heroes::GetPower() const
{
    return GetPower(nullptr);
}

int Heroes::GetPower(string* strs) const
{
    int result = power + GetPowerModificator(strs);
    return result < 0 ? 0 : result > 255 ? 255 : result;
}

int Heroes::GetKnowledge() const
{
    return GetKnowledge(nullptr);
}

int Heroes::GetKnowledge(string* strs) const
{
    int result = knowledge + GetKnowledgeModificator(strs);
    return result < 0 ? 0 : result > 255 ? 255 : result;
}

void Heroes::IncreasePrimarySkill(int skill)
{
    switch (skill)
    {
    case ATTACK:
        ++attack;
        break;
    case DEFENSE:
        ++defense;
        break;
    case POWER:
        ++power;
        break;
    case KNOWLEDGE:
        ++knowledge;
        break;
    default:
        break;
    }
}

uint32_t Heroes::GetExperience() const
{
    return experience;
}

void Heroes::IncreaseMovePoints(uint32_t point)
{
    move_point += point;
}

uint32_t Heroes::GetMovePoints() const
{
    return move_point;
}

uint32_t Heroes::GetMaxSpellPoints() const
{
    return 10 * GetKnowledge();
}

uint32_t Heroes::GetMaxMovePoints() const
{
    int point = 0;
    int acount = 0;

    // start point
    if (isShipMaster())
    {
        point = 1500;

        // skill navigation
        point += point * GetSecondaryValues(Skill::SkillT::NAVIGATION) / 100;

        // artifact bonus
        acount = HasArtifact(Artifact::SAILORS_ASTROLABE_MOBILITY);
        if (acount) point += acount * 1000;

        // visited object
        if (isVisited(MP2::OBJ_LIGHTHOUSE)) point += 500;
    }
    else
    {
        Troop* troop = const_cast<Army &>(army).m_troops.GetSlowestTroop();

        if (troop)
            switch (troop->GetSpeed())
            {
            default:
                break;
            case Speed::CRAWLING:
            case Speed::VERYSLOW:
                point = 1000;
                break;
            case Speed::SLOW:
                point = 1100;
                break;
            case Speed::AVERAGE:
                point = 1200;
                break;
            case Speed::FAST:
                point = 1300;
                break;
            case Speed::VERYFAST:
                point = 1400;
                break;
            case Speed::ULTRAFAST:
            case Speed::BLAZING:
            case Speed::INSTANT:
                point = 1500;
                break;
            }

        // skill logistics
        point += point * GetSecondaryValues(Skill::SkillT::LOGISTICS) / 100;

        // artifact bonus
        acount = HasArtifact(Artifact::NOMAD_BOOTS_MOBILITY);
        if (acount) point += acount * 600;

        acount = HasArtifact(Artifact::TRAVELER_BOOTS_MOBILITY);
        if (acount) point += acount * 300;

        // visited object
        if (isVisited(MP2::OBJ_STABLES)) point += 500;
    }

    acount = HasArtifact(Artifact::TRUE_COMPASS_MOBILITY);
    if (acount) point += acount * 500;

    return point;
}

int Heroes::GetMorale() const
{
    return GetMoraleWithModificators(nullptr);
}

int Heroes::GetMoraleWithModificators(string* strs) const
{
    int result = Morale::NORMAL;

    // bonus artifact
    result += GetMoraleModificator(strs);

    if (army.m_troops.AllTroopsIsRace(Race::NECR)) return Morale::NORMAL;

    // bonus leadership
    result += Skill::GetLeadershipModifiers(GetLevelSkill(Skill::SkillT::LEADERSHIP), strs);

    // object visited
    const u8 objs[] = {
        MP2::OBJ_BUOY, MP2::OBJ_OASIS, MP2::OBJ_WATERINGHOLE, MP2::OBJ_TEMPLE, MP2::OBJ_GRAVEYARD,
        MP2::OBJ_DERELICTSHIP, MP2::OBJ_SHIPWRECK
    };
    result += ObjectVisitedModifiersResult(MDF_MORALE, objs, ARRAY_COUNT(objs), *this, strs);

    // result
    if (result < Morale::AWFUL) return Morale::TREASON;
    if (result < Morale::POOR) return Morale::AWFUL;
    if (result < Morale::NORMAL) return Morale::POOR;
    if (result < Morale::GOOD) return Morale::NORMAL;
    if (result < Morale::GREAT) return Morale::GOOD;
    if (result < Morale::BLOOD) return Morale::GREAT;

    return Morale::BLOOD;
}

int Heroes::GetLuck() const
{
    return GetLuckWithModificators(nullptr);
}

int Heroes::GetLuckWithModificators(string* strs) const
{
    int result = Luck::NORMAL;

    // bonus artifact
    result += GetLuckModificator(strs);

    // bonus luck
    result += Skill::GetLuckModifiers(GetLevelSkill(Skill::SkillT::LUCK), strs);

    // object visited
    const u8 objs[] = {MP2::OBJ_MERMAID, MP2::OBJ_FAERIERING, MP2::OBJ_FOUNTAIN, MP2::OBJ_IDOL, MP2::OBJ_PYRAMID};
    result += ObjectVisitedModifiersResult(MDF_LUCK, objs, ARRAY_COUNT(objs), *this, strs);

    if (result < Luck::AWFUL) return Luck::CURSED;
    if (result < Luck::BAD) return Luck::AWFUL;
    if (result < Luck::NORMAL) return Luck::BAD;
    if (result < Luck::GOOD) return Luck::NORMAL;
    if (result < Luck::GREAT) return Luck::GOOD;
    if (result < Luck::IRISH) return Luck::GREAT;

    return Luck::IRISH;
}

/* recrut hero */
bool Heroes::Recruit(int cl, const Point& pt)
{
    if (GetColor() != Color::NONE)
    {
        return false;
    }

    Kingdom& kingdom = world.GetKingdom(cl);

    if (kingdom.AllowRecruitHero(false, 0))
    {
        Maps::Tiles& tiles = world.GetTiles(pt.x, pt.y);
        SetColor(cl);
        killer_color.SetColor(Color::NONE);
        SetCenter(pt);
        if (!Modes(SAVEPOINTS)) move_point = GetMaxMovePoints();
        MovePointsScaleFixed();

        if (!army.m_troops.IsValid()) army.Reset(false);

        tiles.SetHeroes(this);
        kingdom.AddHeroes(this);

        return true;
    }

    return false;
}

bool Heroes::Recruit(const Castle& castle)
{
    if (Recruit(castle.GetColor(), castle.GetCenter()))
    {
        if (castle.GetLevelMageGuild())
        {
            // magic point
            if (!Modes(SAVEPOINTS)) SetSpellPoints(GetMaxSpellPoints());
            // learn spell
            castle.MageGuildEducateHero(*this);
        }
        return true;
    }

    return false;
}

void Heroes::ActionNewDay()
{
    // recovery move points
    move_point = GetMaxMovePoints();
    MovePointsScaleFixed();

    // stables visited?
    if (isVisited(MP2::OBJ_STABLES)) move_point += 400;

    // recovery spell points
    //if(HaveSpellBook())
    {
        uint32_t curr = GetSpellPoints();
        uint32_t maxp = GetMaxSpellPoints();
        const Castle* castle = inCastle();

        // possible visit arteian spring 2 * max
        if (curr < maxp)
        {
            // in castle?
            if (castle && castle->GetLevelMageGuild())
            {
                //restore from mage guild
                if (Settings::Get().ExtCastleGuildRestorePointsTurn())
                    curr += maxp * GameStatic::GetMageGuildRestoreSpellPointsPercentDay(castle->GetLevelMageGuild()) /
                        100;
                else
                    curr = maxp;
            }

            // everyday
            curr += GameStatic::GetHeroesRestoreSpellPointsPerDay();

            // power ring action
            int acount = HasArtifact(Artifact::POWER_RING);
            if (acount)
                curr += acount * Artifact(Artifact::POWER_RING).ExtraValue();

            // secondary skill
            curr += GetSecondaryValues(Skill::SkillT::MYSTICISM);

            SetSpellPoints(curr > maxp ? maxp : curr);
        }
    }

    // remove day visit object
    std::remove_if(visit_object.begin(), visit_object.end(), Visit::isDayLife);


    // new day, new capacities
    ResetModes(SAVEPOINTS);
}

void Heroes::ActionNewWeek()
{
    // remove week visit object
    std::remove_if(visit_object.begin(), visit_object.end(), Visit::isWeekLife);

    // fix artesian spring effect
    if (GetSpellPoints() > GetMaxSpellPoints()) SetSpellPoints(GetMaxSpellPoints());
}

void Heroes::ActionNewMonth()
{
    // remove month visit object
    std::remove_if(visit_object.begin(), visit_object.end(), Visit::isMonthLife);
}

void Heroes::ActionAfterBattle()
{
    // remove month visit object
    std::remove_if(visit_object.begin(), visit_object.end(), Visit::isBattleLife);
    //
    SetModes(ACTION);
}

void Heroes::RescanPathPassable()
{
    if (path.isValid()) path.RescanPassable();
}

void Heroes::RescanPath()
{
    if (path.isValid())
    {
        const Maps::Tiles& tile = world.GetTiles(path.GetDestinationIndex());

        if (!isShipMaster() && tile.isWater() &&
            !MP2::isNeedStayFront(tile.GetObject()))
            path.PopBack();
    }

    if (path.isValid())
    {
        if (isControlAI())
        {
            if (path.hasObstacle()) path.Reset();
        }
        else
        {
            path.RescanObstacle();
        }
    }
}

/* if hero in castle */
const Castle* Heroes::inCastle() const
{
    const Castle* castle = Color::NONE != GetColor() ? world.GetCastle(GetCenter()) : nullptr;
    return castle && castle->GetHeroes() == this ? castle : nullptr;
}

Castle* Heroes::inCastle()
{
    Castle* castle = Color::NONE != GetColor() ? world.GetCastle(GetCenter()) : nullptr;
    return castle && castle->GetHeroes() == this ? castle : nullptr;
}

/* is visited cell */
bool Heroes::isVisited(const Maps::Tiles& tile, Visit::type_t type) const
{
    const s32& index = tile.GetIndex();
    int object = tile.GetObject(false);

    if (Visit::GLOBAL == type) return GetKingdom().isVisited(index, object);

    IndexObject valueToFind(index, object);
    auto findIt = std::find_if(visit_object.begin(), visit_object.end(),
                               [&](const IndexObject& item)
                               {
                                   return item.Value == valueToFind.Value;
                               });

    return visit_object.end() != findIt;
}

/* return true if object visited */
bool Heroes::isVisited(int object, Visit::type_t type) const
{
    if (Visit::GLOBAL == type) return GetKingdom().isVisited(object);

    return visit_object.end() != find_if(visit_object.begin(), visit_object.end(),
                                         [&](const IndexObject& it) { return it.isObject(object); });
}

/* set visited cell */
void Heroes::SetVisited(s32 index, Visit::type_t type)
{
    const Maps::Tiles& tile = world.GetTiles(index);
    int object = tile.GetObject(false);

    if (Visit::GLOBAL == type)
        GetKingdom().SetVisited(index, object);
    else if (!isVisited(tile) && MP2::OBJ_ZERO != object)
        visit_object.insert(visit_object.begin(), IndexObject(index, object));
}

void Heroes::SetVisitedWideTile(s32 index, int object, Visit::type_t type)
{
    const Maps::Tiles& tile = world.GetTiles(index);
    const Maps::TilesAddon* addon = tile.FindObjectConst(object);
    int wide = 0;

    switch (object)
    {
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_OASIS:
    case MP2::OBJ_STANDINGSTONES:
    case MP2::OBJ_ARTESIANSPRING:
        wide = 2;
        break;
    case MP2::OBJ_WATERINGHOLE:
        wide = 4;
        break;
    default:
        break;
    }

    if (addon && wide)
    {
        for (s32 ii = tile.GetIndex() - (wide - 1); ii <= tile.GetIndex() + (wide - 1); ++ii)
            if (Maps::isValidAbsIndex(ii) &&
                world.GetTiles(ii).FindAddonLevel1(addon->uniq))
                SetVisited(ii, type);
    }
}

int Heroes::GetSpriteIndex() const
{
    return sprite_index;
}

bool Heroes::isAction() const
{
    return Modes(ACTION);
}

void Heroes::ResetAction()
{
    ResetModes(ACTION);
}

uint32_t Heroes::GetCountArtifacts() const
{
    return bag_artifacts.CountArtifacts();
}

bool Heroes::HasUltimateArtifact() const
{
    return bag_artifacts.ContainUltimateArtifact();
}

bool Heroes::IsFullBagArtifacts() const
{
    return bag_artifacts.isFull();
}

bool Heroes::PickupArtifact(const Artifact& art)
{
    if (!art.IsValid()) return false;

    //const Settings & conf = Settings::Get();

    if (!bag_artifacts.PushArtifact(art))
    {
        if (isControlHuman())
        {
            art() == Artifact::MAGIC_BOOK
                ? Message("",
                          _(
                              "You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero."
                          ),
                          Font::BIG, Dialog::OK)
                : Message(art.GetName(), _("You have no room to carry another artifact!"), Font::BIG, Dialog::OK);
        }
        return false;
    }

    // check: anduran garb
    if (bag_artifacts.MakeBattleGarb())
    {
        if (isControlHuman())
            Dialog::ArtifactInfo("", _("The three Anduran artifacts magically combine into one."),
                                 Artifact::BATTLE_GARB);
    }

    return true;
}

/* return level hero */
int Heroes::GetLevel() const
{
    return GetLevelFromExperience(experience);
}

const Route::Path& Heroes::GetPath() const
{
    return path;
}

Route::Path& Heroes::GetPath()
{
    return path;
}

void Heroes::ShowPath(bool f)
{
    f ? path.Show() : path.Hide();
}

void Heroes::IncreaseExperience(uint32_t exp)
{
    int level_old = GetLevelFromExperience(experience);
    int level_new = GetLevelFromExperience(experience + exp);

    for (int ii = 0; ii < level_new - level_old; ++ii) LevelUp(false);

    experience += exp;
}

/* calc level from exp */
int Heroes::GetLevelFromExperience(uint32_t exp)
{
    for (int lvl = 1; lvl < 255; ++lvl) if (exp < GetExperienceFromLevel(lvl)) return lvl;

    return 0;
}

/* calc exp from level */
uint32_t Heroes::GetExperienceFromLevel(int lvl)
{
    switch (lvl)
    {
    case 0:
        return 0;
    case 1:
        return 1000;
    case 2:
        return 2000;
    case 3:
        return 3200;
    case 4:
        return 4500;
    case 5:
        return 6000;
    case 6:
        return 7700;
    case 7:
        return 9000;
    case 8:
        return 11000;
    case 9:
        return 13200;
    case 10:
        return 15500;
    case 11:
        return 18500;
    case 12:
        return 22100;
    case 13:
        return 26400;
    case 14:
        return 31600;
    case 15:
        return 37800;
    case 16:
        return 45300;
    case 17:
        return 54200;
    case 18:
        return 65000;
    case 19:
        return 78000;
    case 20:
        return 93600;
    case 21:
        return 112300;
    case 22:
        return 134700;
    case 23:
        return 161600;
    case 24:
        return 193900;
    case 25:
        return 232700;
    case 26:
        return 279300;
    case 27:
        return 335200;
    case 28:
        return 402300;
    case 29:
        return 482800;
    case 30:
        return 579400;
    case 31:
        return 695300;
    case 32:
        return 834400;
    case 33:
        return 1001300;
    case 34:
        return 1201600;
    case 35:
        return 1442000;
    case 36:
        return 1730500;
    case 37:
        return 2076700;
    case 38:
        return 2492100;
    case 39:
        return 2990600;

    default:
        break;
    }

    const uint32_t l1 = GetExperienceFromLevel(lvl - 1);
    return l1 + static_cast<uint32_t>(round((l1 - GetExperienceFromLevel(lvl - 2)) * 1.2 / 100) * 100);
}

/* buy book */
bool Heroes::BuySpellBook(const Castle* castle, int shrine)
{
    if (HaveSpellBook() || Color::NONE == GetColor()) return false;

    const payment_t payment = PaymentConditions::BuySpellBook(shrine);
    Kingdom& kingdom = GetKingdom();

    string header = _("To cast spells, you must first buy a spell book for %{gold} gold.");
    StringReplace(header, "%{gold}", payment.gold);

    if (!kingdom.AllowPayment(payment))
    {
        if (isControlHuman())
        {
            header.append(" ");
            header.append(_("Unfortunately, you seem to be a little short of cash at the moment."));
            Message("", header, Font::BIG, Dialog::OK);
        }
        return false;
    }

    if (isControlHuman())
    {
        const Sprite& border = AGG::GetICN(ICN::RESOURCE, 7);
        Surface sprite = border.GetSurface();

        AGG::GetICN(ICN::ARTIFACT, Artifact(Artifact::MAGIC_BOOK).IndexSprite64()).Blit(5, 5, sprite);

        header.append(" ");
        header.append(_("Do you wish to buy one?"));

        if (Dialog::NO == Dialog::SpriteInfo(GetName(), header, sprite, Dialog::YES | Dialog::NO)) return false;
    }

    if (SpellBookActivate())
    {
        kingdom.OddFundsResource(payment);

        // add all spell to book
        if (castle) castle->MageGuildEducateHero(*this);

        return true;
    }

    return false;
}

/* return true is move enable */
bool Heroes::isEnableMove() const
{
    return Modes(ENABLEMOVE) && path.isValid() && path.GetFrontPenalty() <= move_point;
}

bool Heroes::CanMove() const
{
    return move_point >=
        Maps::Ground::GetPenalty(GetIndex(), Direction::CENTER, GetLevelSkill(Skill::SkillT::PATHFINDING));
}

/* set enable move */
void Heroes::SetMove(bool f)
{
    if (f)
    {
        SetModes(ENABLEMOVE);
    }
    else
    {
        ResetModes(ENABLEMOVE);

        // reset sprite position
        switch (direction)
        {
        case Direction::TOP:
            sprite_index = 0;
            break;
        case Direction::BOTTOM:
            sprite_index = 36;
            break;
        case Direction::TOP_RIGHT:
        case Direction::TOP_LEFT:
            sprite_index = 9;
            break;
        case Direction::BOTTOM_RIGHT:
        case Direction::BOTTOM_LEFT:
            sprite_index = 27;
            break;
        case Direction::RIGHT:
        case Direction::LEFT:
            sprite_index = 18;
            break;
        default:
            break;
        }
    }
}

bool Heroes::isShipMaster() const
{
    return Modes(SHIPMASTER);
}

void Heroes::SetShipMaster(bool f)
{
    f ? SetModes(SHIPMASTER) : ResetModes(SHIPMASTER);
}

Skill::SecSkills& Heroes::GetSecondarySkills()
{
    return secondary_skills;
}

bool Heroes::HasSecondarySkill(Skill::SkillT skill) const
{
    return Skill::Level::NONE != secondary_skills.GetLevel(skill);
}

uint32_t Heroes::GetSecondaryValues(Skill::SkillT skill) const
{
    return secondary_skills.GetValues(skill);
}

bool Heroes::HasMaxSecondarySkill() const
{
    return HEROESMAXSKILL <= secondary_skills.Count();
}

int Heroes::GetLevelSkill(Skill::SkillT skill) const
{
    return secondary_skills.GetLevel(skill);
}

void Heroes::LearnSkill(const Skill::Secondary& skill)
{
    if (skill.isValid())
        secondary_skills.AddSkill(skill);
}

void Heroes::Scoute() const
{
    Maps::ClearFog(GetIndex(), GetScoute(), GetColor());
}

int Heroes::GetScoute() const
{
    int acount = HasArtifact(Artifact::TELESCOPE);

    return (acount ? acount * GetViewDistance(Game::VIEW_TELESCOPE) : 0) +
        GetViewDistance(Game::VIEW_HEROES) + GetSecondaryValues(Skill::SkillT::SCOUTING);
}

uint32_t Heroes::GetVisionsDistance() const
{
    int dist = Spell(Spell::VISIONS).ExtraValue();
    int acount = HasArtifact(Artifact::CRYSTAL_BALL);

    if (acount)
        dist = acount * (Settings::Get().UseAltResource() ? dist * 2 + 2 : 8);

    return dist;
}

int Heroes::GetDirection() const
{
    return direction;
}

/* return route range in days */
int Heroes::GetRangeRouteDays(s32 dst) const
{
    const uint32_t max = GetMaxMovePoints();
    const uint32_t limit = max * 5 / 100; // limit ~5 day

    // approximate distance, this restriction calculation
    if (4 * max / 100 < Maps::GetApproximateDistance(GetIndex(), dst))
    {
        return 0;
    }

    Route::Path test(*this);
    // approximate limit, this restriction path finding algorithm
    if (test.Calculate(dst, limit))
    {
        uint32_t total = test.GetTotalPenalty();
        if (move_point >= total) return 1;

        total -= move_point;
        if (max >= total) return 2;

        total -= move_point;
        if (max >= total) return 3;

        return 4;
    }

    return 0;
}

/* up level */
void Heroes::LevelUp(bool skipsecondary, bool autoselect)
{
    int primary = LevelUpPrimarySkill();
    if (!skipsecondary)
        LevelUpSecondarySkill(primary, autoselect || isControlAI());
    if (isControlAI()) AI::HeroesLevelUp(*this);
}

int Heroes::LevelUpPrimarySkill()
{
    int skill = Primary::LevelUp(race, GetLevel());

    return skill;
}

void Heroes::LevelUpSecondarySkill(int primary, bool autoselect)
{
    Skill::Secondary sec1;
    Skill::Secondary sec2;

    secondary_skills.FindSkillsForLevelUp(race, sec1, sec2);
    Skill::Secondary* selected = nullptr;

    if (autoselect)
    {
        if (Skill::SkillT::UNKNOWN == sec1.Skill() || Skill::SkillT::UNKNOWN == sec2.Skill())
        {
            if (Skill::SkillT::UNKNOWN != sec1.Skill())
                selected = &sec1;
            else if (Skill::SkillT::UNKNOWN != sec2.Skill())
                selected = &sec2;
        }
        else if (Skill::SkillT::UNKNOWN != sec1.Skill() && Skill::SkillT::UNKNOWN != sec2.Skill())
            selected = Rand::Get(0, 1) ? &sec1 : &sec2;
    }
    else
    {
        AGG::PlaySound(M82::NWHEROLV);
        Skill::SkillT result = Dialog::LevelUpSelectSkill(name, Primary::String(primary), sec1, sec2, *this);

        if (Skill::SkillT::UNKNOWN != result)
            selected = result == sec2.Skill() ? &sec2 : &sec1;
    }

    // level up sec. skill
    if (selected)
    {
        Skill::Secondary* secs = secondary_skills.FindSkill(selected->Skill());

        if (secs)
            secs->NextLevel();
        else
            secondary_skills.AddSkill(Skill::Secondary(selected->Skill(), Skill::Level::BASIC));

        // post action
        switch (selected->Skill())
        {
        case Skill::SkillT::SCOUTING:
            Scoute();
            break;

        default:
            break;
        }
    }
}

/* apply penalty */
bool Heroes::ApplyPenaltyMovement()
{
    uint32_t penalty = path.isValid()
                           ? path.GetFrontPenalty()
                           : Maps::Ground::GetPenalty(GetIndex(), Direction::CENTER,
                                                      GetLevelSkill(Skill::SkillT::PATHFINDING));

    if (move_point >= penalty) move_point -= penalty;
    else return false;

    return true;
}

void Heroes::ResetMovePoints()
{
    ApplyPenaltyMovement();
    ApplyPenaltyMovement();
    ApplyPenaltyMovement();
}

bool Heroes::MayStillMove() const
{
    if (Modes(SLEEPER | GUARDIAN) || isFreeman()) return false;
    return path.isValid() ? move_point >= path.GetFrontPenalty() : CanMove();
}

bool Heroes::isValid() const
{
    return hid != UNKNOWN;
}

bool Heroes::isFreeman() const
{
    return isValid() && Color::NONE == GetColor() && !Modes(JAIL);
}

void Heroes::SetFreeman(int reason)
{
    if (isFreeman()) return;
    bool savepoints = false;
    Kingdom& kingdom = GetKingdom();

    if ((Battle::RESULT_RETREAT | Battle::RESULT_SURRENDER) & reason)
    {
        if (Settings::Get().ExtHeroRememberPointsForRetreating()) savepoints = true;
        kingdom.SetLastLostHero(*this);
    }

    if (!army.m_troops.IsValid() || Battle::RESULT_RETREAT & reason) army.Reset(false);
    else if (Battle::RESULT_LOSS & reason && !(Battle::RESULT_SURRENDER & reason)) army.Reset(true);

    if (GetColor() != Color::NONE) kingdom.RemoveHeroes(this);

    SetColor(Color::NONE);
    world.GetTiles(GetIndex()).SetHeroes(nullptr);
    modes = 0;
    SetIndex(-1);
    move_point_scale = -1;
    path.Reset();
    SetMove(false);
    SetModes(ACTION);
    if (savepoints) SetModes(SAVEPOINTS);
}

void Heroes::SetKillerColor(int col)
{
    killer_color.SetColor(col);
}

int Heroes::GetKillerColor() const
{
    return killer_color.GetColor();
}

int Heroes::GetControl() const
{
    return GetKingdom().GetControl();
}

int Heroes::GetMapsObject() const
{
    return save_maps_object;
}

void Heroes::SetMapsObject(int obj)
{
    save_maps_object = obj != MP2::OBJ_HEROES ? obj : MP2::OBJ_ZERO;
}

bool Heroes::AllowBattle(bool attacker) const
{
    if (!attacker)
        switch (world.GetTiles(GetIndex()).GetObject(false))
        {
        case MP2::OBJ_TEMPLE:
            return false;
        default:
            break;
        }

    return true;
}

void Heroes::ActionPreBattle()
{
}

void RedrawGameAreaAndHeroAttackMonster(Heroes& hero, s32 dst)
{
    // redraw gamearea for monster action sprite
    if (hero.isControlHuman())
    {
        Interface::Basic& I = Interface::Basic::Get();
        Cursor::Get().Hide();
        I.GetGameArea().SetCenter(hero.GetCenter());
        I.RedrawFocus();
        I.Redraw();
        Cursor::Get().Show();
        // force flip, for monster attack show sprite
        Display::Get().Flip();
    }
    hero.Action(dst);
}

void Heroes::ActionNewPosition()
{
    const Settings& conf = Settings::Get();
    // check around monster
    MapsIndexes targets = Maps::GetTilesUnderProtection(GetIndex());

    if (!targets.empty())
    {
        bool skip_battle = false;
        SetMove(false);
        GetPath().Hide();

        // first target
        auto it = find(targets.begin(), targets.end(), GetPath().GetDestinedIndex());
        if (it != targets.end())
        {
            RedrawGameAreaAndHeroAttackMonster(*this, *it);
            targets.erase(it);
            if (conf.ExtWorldOnlyFirstMonsterAttack()) skip_battle = true;
        }

        // other around targets
        for (MapsIndexes::const_iterator
             it = targets.begin(); it != targets.end() && !isFreeman() && !skip_battle; ++it)
        {
            RedrawGameAreaAndHeroAttackMonster(*this, *it);
            if (conf.ExtWorldOnlyFirstMonsterAttack()) skip_battle = true;
        }
    }

    if (!isFreeman() &&
        GetMapsObject() == MP2::OBJ_EVENT)
    {
        const MapEvent* event = world.GetMapEvent(GetCenter());

        if (event && event->isAllow(GetColor()))
        {
            Action(GetIndex());
            SetMove(false);
        }
    }

    if (isControlAI())
        AI::HeroesActionNewPosition(*this);

    ResetModes(VISIONS);
}

void Heroes::SetCenterPatrol(const Point& pt)
{
    patrol_center = pt;
}

const Point& Heroes::GetCenterPatrol() const
{
    return patrol_center;
}

int Heroes::GetSquarePatrol() const
{
    return patrol_square;
}

int Heroes::CanScouteTile(s32 dst) const
{
    int scouting = GetSecondaryValues(Skill::SkillT::SCOUTING);
    bool army_info = false;

    switch (world.GetTiles(dst).GetObject())
    {
    case MP2::OBJ_MONSTER:
    case MP2::OBJ_CASTLE:
    case MP2::OBJ_HEROES:
        army_info = true;
        break;

    default:
        break;
    }

    if (army_info)
    {
        // depends from distance
        if (Maps::GetApproximateDistance(GetIndex(), dst) <= GetVisionsDistance())
        {
            // check crystal ball
            return HasArtifact(Artifact::CRYSTAL_BALL) ? Skill::Level::EXPERT : scouting;
        }
        // check spell identify hero
        if (GetKingdom().Modes(Kingdom::IDENTIFYHERO) &&
            MP2::OBJ_HEROES == world.GetTiles(dst).GetObject())
            return Skill::Level::EXPERT;
    }
    else
    {
        if (Settings::Get().ExtWorldScouteExtended())
        {
            //const Maps::Tiles & tile = world.GetTiles(dst);

            uint32_t dist = GetSecondaryValues(Skill::SkillT::SCOUTING) ? GetScoute() : 0;
            if (Modes(VISIONS) && dist < GetVisionsDistance()) dist = GetVisionsDistance();

            if (dist > Maps::GetApproximateDistance(GetIndex(), dst))
                return scouting;
        }
    }

    return 0;
}

void Heroes::MovePointsScaleFixed()
{
    move_point_scale = move_point * 1000 / GetMaxMovePoints();
}

void Heroes::RecalculateMovePoints()
{
    if (0 <= move_point_scale) move_point = GetMaxMovePoints() * move_point_scale / 1000;
}

void Heroes::Move2Dest(const s32& dst_index, bool skip_action /* false */)
{
    if (dst_index != GetIndex())
    {
        world.GetTiles(GetIndex()).SetHeroes(nullptr);
        SetIndex(dst_index);
        Scoute();
        ApplyPenaltyMovement();
        world.GetTiles(dst_index).SetHeroes(this);

        if (!skip_action)
            ActionNewPosition();
    }
}

Surface Heroes::GetPortrait(int id, int type)
{
    if (UNKNOWN == id)
    {
        return Surface();
    }
    switch (type)
    {
    case PORT_BIG:
        return AGG::GetICN(ICN::PORTxxxx(id), 0);
    case PORT_MEDIUM:
        return SANDYSANDY > id
                   ? AGG::GetICN(ICN::PORTMEDI, id + 1)
                   : AGG::GetICN(ICN::PORTMEDI,
                                 BAX + 1);
    case PORT_SMALL:
        return SANDYSANDY > id ? AGG::GetICN(ICN::MINIPORT, id) : AGG::GetICN(ICN::MINIPORT, BAX);
    default:
        break;
    }

    return Surface();
}

Surface Heroes::GetPortrait(int type) const
{
    return GetPortrait(portrait, type);
}

void Heroes::PortraitRedraw(s32 px, s32 py, int type, Surface& dstsf) const
{
    Surface port = GetPortrait(portrait, type);
    Point mp;

    if (port.isValid())
    {
        if (PORT_BIG == type)
        {
            port.Blit(px, py, dstsf);
            mp.y = 2;
            mp.x = port.w() - 12;
        }
        else if (PORT_MEDIUM == type)
        {
            port.Blit(px, py, dstsf);
            mp.x = port.w() - 10;
        }
        else if (PORT_SMALL == type)
        {
            const Sprite& mobility = AGG::GetICN(ICN::MOBILITY, GetMobilityIndexSprite());
            const Sprite& mana = AGG::GetICN(ICN::MANA, GetManaIndexSprite());

            const int iconsw = Interface::IconsBar::GetItemWidth();
            const int iconsh = Interface::IconsBar::GetItemHeight();
            const int barw = 7;

            dstsf.FillRect(Rect(px, py, iconsw, iconsh), ColorBlack);
            const RGBA blue = RGBA(15, 30, 120);

            // mobility
            dstsf.FillRect(Rect(px, py, barw, iconsh), blue);
            mobility.Blit(px, py + mobility.y(), dstsf);

            // portrait
            port.Blit(px + barw + 1, py, dstsf);

            // mana
            dstsf.FillRect(Rect(px + barw + port.w() + 2, py, barw, iconsh), blue);
            mana.Blit(px + barw + port.w() + 2, py + mana.y(), dstsf);

            mp.x = 35;
        }
    }

    // heroes marker
    if (Modes(SHIPMASTER))
    {
        const Sprite& sprite = AGG::GetICN(ICN::BOAT12, 0);
        const Rect pos(px + mp.x, py + mp.y - 1, sprite.w(), sprite.h());
        dstsf.FillRect(pos, ColorBlack);
        sprite.Blit(pos.x, pos.y, dstsf);
        mp.y = sprite.h();
    }
    else if (Modes(GUARDIAN))
    {
        const Sprite& sprite = AGG::GetICN(ICN::MISC6, 11);
        const Rect pos(px + mp.x + 3, py + mp.y, sprite.w(), sprite.h());
        dstsf.FillRect(pos, ColorBlack);
        sprite.Blit(pos.x, pos.y, dstsf);
        mp.y = sprite.h();
    }

    if (Modes(SLEEPER))
    {
        const Sprite& sprite = AGG::GetICN(ICN::MISC4, 14);
        const Rect pos(px + mp.x + 3, py + mp.y - 1, sprite.w() - 4, sprite.h() - 4);
        dstsf.FillRect(pos, ColorBlack);
        sprite.Blit(pos.x - 2, pos.y - 2);
    }
}

string Heroes::String() const
{
    ostringstream os;

    os <<
        "name            : " << name << endl <<
        "race            : " << Race::String(race) << endl <<
        "color           : " << Color::String(GetColor()) << endl <<
        "experience      : " << experience << endl <<
        "level           : " << GetLevel() << endl <<
        "magic point     : " << GetSpellPoints() << endl <<
        "position x      : " << GetCenter().x << endl <<
        "position y      : " << GetCenter().y << endl <<
        "move point      : " << move_point << endl <<
        "max magic point : " << GetMaxSpellPoints() << endl <<
        "max move point  : " << GetMaxMovePoints() << endl <<
        "direction       : " << Direction::String(direction) << endl <<
        "index sprite    : " << sprite_index << endl <<
        "in castle       : " << (inCastle() ? "true" : "false") << endl <<
        "save object     : " << MP2::StringObject(world.GetTiles(GetIndex()).GetObject(false)) << endl <<
        "flags           : " << (Modes(SHIPMASTER) ? "SHIPMASTER," : "") <<
        (Modes(PATROL) ? "PATROL" : "") << endl;

    if (Modes(PATROL))
    {
        os << "patrol square   : " << patrol_square << endl;
    }

    if (!visit_object.empty())
    {
        os << "visit objects   : ";
        for (const auto& it : visit_object)
            os << MP2::StringObject(it.Value.second) << "(" << it.Value.first << "), ";
        os << endl;
    }

    if (isControlAI())
    {
        os <<
            "skills          : " << secondary_skills.String() << endl <<
            "artifacts       : " << bag_artifacts.String() << endl <<
            "spell book      : " << (HaveSpellBook() ? spell_book.String() : "disabled") << endl <<
            "army dump       : " << army.String() << endl;

        os << AI::HeroesString(*this);
    }

    return os.str();
}


bool InCastleAndGuardian(const Castle* castle, Heroes* hero)
{
    const Point& cpt = castle->GetCenter();
    const Point& hpt = hero->GetCenter();
    return cpt.x == hpt.x && cpt.y == hpt.y + 1 && hero->Modes(Heroes::GUARDIAN);
}


bool InCastleNotGuardian(const Castle* castle, Heroes* hero)
{
    return castle->GetCenter() == hero->GetCenter() && !hero->Modes(Heroes::GUARDIAN);
}

bool InJailMode(s32 index, Heroes* hero)
{
    return hero->Modes(Heroes::JAIL) && index == hero->GetIndex();
}


AllHeroes::AllHeroes()
{
    _items.reserve(HEROESMAXCOUNT + 2);
}

AllHeroes::~AllHeroes()
{
    clear();
}

void AllHeroes::Init()
{
    if (!_items.empty())
        clear();

    const bool loyalty = Settings::Get().PriceLoyaltyVersion();

    // knight: LORDKILBURN, SIRGALLANTH, ECTOR, GVENNETH, TYRO, AMBROSE, RUBY, MAXIMUS, DIMITRY
    for (uint32_t hid = Heroes::LORDKILBURN; hid <= Heroes::DIMITRY; ++hid)
        _items.push_back(new Heroes(hid, Race::KNGT));

    // barbarian: THUNDAX, FINEOUS, JOJOSH, CRAGHACK, JEZEBEL, JACLYN, ERGON, TSABU, ATLAS
    for (uint32_t hid = Heroes::THUNDAX; hid <= Heroes::ATLAS; ++hid)
        _items.push_back(new Heroes(hid, Race::BARB));

    // sorceress: ASTRA, NATASHA, TROYAN, VATAWNA, REBECCA, GEM, ARIEL, CARLAWN, LUNA
    for (uint32_t hid = Heroes::ASTRA; hid <= Heroes::LUNA; ++hid)
        _items.push_back(new Heroes(hid, Race::SORC));

    // warlock: ARIE, ALAMAR, VESPER, CRODO, BAROK, KASTORE, AGAR, FALAGAR, WRATHMONT
    for (uint32_t hid = Heroes::ARIE; hid <= Heroes::WRATHMONT; ++hid)
        _items.push_back(new Heroes(hid, Race::WRLK));

    // wizard: MYRA, FLINT, DAWN, HALON, MYRINI, WILFREY, SARAKIN, KALINDRA, MANDIGAL
    for (uint32_t hid = Heroes::MYRA; hid <= Heroes::MANDIGAL; ++hid)
        _items.push_back(new Heroes(hid, Race::WZRD));

    // necromancer: ZOM, DARLANA, ZAM, RANLOO, CHARITY, RIALDO, ROXANA, SANDRO, CELIA
    for (uint32_t hid = Heroes::ZOM; hid <= Heroes::CELIA; ++hid)
        _items.push_back(new Heroes(hid, Race::NECR));

    // from campain
    _items.push_back(new Heroes(Heroes::ROLAND, Race::WZRD));
    _items.push_back(new Heroes(Heroes::CORLAGON, Race::KNGT));
    _items.push_back(new Heroes(Heroes::ELIZA, Race::SORC));
    _items.push_back(new Heroes(Heroes::ARCHIBALD, Race::WRLK));
    _items.push_back(new Heroes(Heroes::HALTON, Race::KNGT));
    _items.push_back(new Heroes(Heroes::BAX, Race::NECR));

    // loyalty version
    _items.push_back(new Heroes(loyalty ? Heroes::SOLMYR : Heroes::UNKNOWN, Race::WZRD));
    _items.push_back(new Heroes(loyalty ? Heroes::DAINWIN : Heroes::UNKNOWN, Race::WRLK));
    _items.push_back(new Heroes(loyalty ? Heroes::MOG : Heroes::UNKNOWN, Race::NECR));
    _items.push_back(new Heroes(loyalty ? Heroes::UNCLEIVAN : Heroes::UNKNOWN, Race::BARB));
    _items.push_back(new Heroes(loyalty ? Heroes::JOSEPH : Heroes::UNKNOWN, Race::KNGT));
    _items.push_back(new Heroes(loyalty ? Heroes::GALLAVANT : Heroes::UNKNOWN, Race::KNGT));
    _items.push_back(new Heroes(loyalty ? Heroes::ELDERIAN : Heroes::UNKNOWN, Race::WRLK));
    _items.push_back(new Heroes(loyalty ? Heroes::CEALLACH : Heroes::UNKNOWN, Race::KNGT));
    _items.push_back(new Heroes(loyalty ? Heroes::DRAKONIA : Heroes::UNKNOWN, Race::WZRD));
    _items.push_back(new Heroes(loyalty ? Heroes::MARTINE : Heroes::UNKNOWN, Race::SORC));
    _items.push_back(new Heroes(loyalty ? Heroes::JARKONAS : Heroes::UNKNOWN, Race::BARB));

    // devel
    _items.push_back(new Heroes(IS_DEVEL() ? Heroes::SANDYSANDY : Heroes::UNKNOWN, Race::WRLK));
    _items.push_back(new Heroes(Heroes::UNKNOWN, Race::KNGT));
}

void AllHeroes::clear()
{
    for (auto& it : _items)
        delete it;
    _items.clear();
}

Heroes* VecHeroes::Get(int hid) const
{
    const vector<Heroes *>& vec = _items;
    return 0 <= hid && hid < Heroes::UNKNOWN ? vec[hid] : nullptr;
}

Heroes* VecHeroes::Get(const Point& center) const
{
    auto it = _items.begin();
    for (; it != _items.end(); ++it) if ((*it)->isPosition(center)) break;
    return _items.end() != it ? *it : nullptr;
}

Heroes* AllHeroes::GetGuest(const Castle& castle) const
{
    const auto it = find_if(_items.begin(), _items.end(),
                            [&](Heroes* hero)
                            {
                                return InCastleNotGuardian(&castle, hero);
                            });
    return _items.end() != it ? *it : nullptr;
}

Heroes* AllHeroes::GetGuard(const Castle& castle) const
{
    auto it = Settings::Get().ExtCastleAllowGuardians()
                  ? find_if(_items.begin(), _items.end(),
                            [&](Heroes* hero) { return InCastleAndGuardian(&castle, hero); }
                  )
                  : _items.end();
    return _items.end() != it ? *it : nullptr;
}

Heroes* AllHeroes::GetFreeman(int race) const
{
    const Settings& conf = Settings::Get();

    int min = Heroes::UNKNOWN;
    int max = Heroes::UNKNOWN;

    switch (race)
    {
    case Race::KNGT:
        min = Heroes::LORDKILBURN;
        max = Heroes::DIMITRY;
        break;

    case Race::BARB:
        min = Heroes::THUNDAX;
        max = Heroes::ATLAS;
        break;

    case Race::SORC:
        min = Heroes::ASTRA;
        max = Heroes::LUNA;
        break;

    case Race::WRLK:
        min = Heroes::ARIE;
        max = Heroes::WRATHMONT;
        break;

    case Race::WZRD:
        min = Heroes::MYRA;
        max = Heroes::MANDIGAL;
        break;

    case Race::NECR:
        min = Heroes::ZOM;
        max = Heroes::CELIA;
        break;

    default:
        min = Heroes::LORDKILBURN;
        max = conf.ExtCastleAllowRecruitSpecialHeroes()
                  ? (conf.PriceLoyaltyVersion()
                         ? Heroes::JARKONAS
                         : Heroes::BAX)
                  : Heroes::CELIA;
        break;
    }

    vector<int> freeman_heroes;
    freeman_heroes.reserve(HEROESMAXCOUNT);

    // find freeman in race (skip: manual changes)
    for (int ii = min; ii <= max; ++ii)
        if (_items.at(ii)->isFreeman() && !_items.at(ii)->Modes(Heroes::NOTDEFAULTS)) freeman_heroes.push_back(ii);

    // not found, find any race
    if (Race::NONE != race && freeman_heroes.empty())
    {
        min = Heroes::LORDKILBURN;
        max = conf.ExtCastleAllowRecruitSpecialHeroes()
                  ? (conf.PriceLoyaltyVersion() ? Heroes::JARKONAS : Heroes::BAX)
                  : Heroes::CELIA;

        for (int ii = min; ii <= max; ++ii)
            if (_items.at(ii)->isFreeman()) freeman_heroes.push_back(ii);
    }

    // not found, all heroes busy
    if (freeman_heroes.empty())
    {
        return nullptr;
    }

    return _items.at(*Rand::Get(freeman_heroes));
}

void AllHeroes::Scoute(int colors) const
{
    for (auto it : _items)
        if (colors & it->GetColor()) it->Scoute();
}

Heroes* AllHeroes::FromJail(s32 index) const
{
    auto it = find_if(_items.begin(), _items.end(),
                      [=](Heroes* it)
                      {
                          return InJailMode(index, it);
                      });
    return _items.end() != it ? *it : nullptr;
}

bool AllHeroes::HaveTwoFreemans() const
{
    return 2 <= count_if(_items.begin(), _items.end(),
                         [](Heroes* it) { return it->isFreeman(); });
}

ByteVectorWriter& operator<<(ByteVectorWriter& msg, const VecHeroes& heroes)
{
    msg << static_cast<uint32_t>(heroes._items.size());

    for (auto heroe : heroes._items)
        msg << (heroe ? heroe->GetID() : Heroes::UNKNOWN);

    return msg;
}

ByteVectorReader& operator>>(ByteVectorReader& msg, VecHeroes& heroes)
{
    uint32_t size;
    msg >> size;

    heroes._items.resize(size, nullptr);

    for (auto& heroe : heroes._items)
    {
        uint32_t hid;
        msg >> hid;
        heroe = hid != Heroes::UNKNOWN ? world.GetHeroes(hid) : nullptr;
    }

    return msg;
}

ByteVectorWriter& operator<<(ByteVectorWriter& msg, const Heroes& hero)
{
    const HeroBase& base = hero;
    const ColorBase& col = hero;

    return msg <<
        base <<
        // heroes
        hero.name <<
        col <<
        hero.killer_color <<
        hero.experience <<
        hero.move_point_scale <<
        hero.secondary_skills <<
        hero.army <<
        hero.hid <<
        hero.portrait <<
        hero.race <<
        hero.save_maps_object <<
        hero.path <<
        hero.direction <<
        hero.sprite_index <<
        hero.patrol_center <<
        hero.patrol_square <<
        hero.visit_object;
}

enum deprecated_t
{
    AIWAITING = 0x00000002,
    HUNTER = 0x00000010,
    SCOUTER = 0x00000020,
    STUPID = 0x00000040
};

ByteVectorReader& operator>>(ByteVectorReader& msg, Heroes& hero)
{
    HeroBase& base = hero;
    ColorBase& col = hero;

    msg >> base >>
        hero.name >>
        col >>
        hero.killer_color >>
        hero.experience >>
        hero.move_point_scale >>
        hero.secondary_skills >>
        hero.army >>
        hero.hid >>
        hero.portrait >>
        hero.race >>
        hero.save_maps_object >>
        hero.path >>
        hero.direction >>
        hero.sprite_index >>
        hero.patrol_center >>
        hero.patrol_square >>
        hero.visit_object;

    hero.army.SetCommander(&hero);
    return msg;
}

ByteVectorWriter& operator<<(ByteVectorWriter& msg, const AllHeroes& heroes)
{
    msg << static_cast<uint32_t>(heroes._items.size());

    for (auto heroe : heroes._items)
        msg << *heroe;

    return msg;
}

ByteVectorReader& operator>>(ByteVectorReader& msg, AllHeroes& heroes)
{
    uint32_t size;
    msg >> size;

    heroes.clear();
    heroes._items.resize(size, nullptr);

    for (auto& heroe : heroes._items)
    {
        heroe = new Heroes();
        msg >> *heroe;
    }

    return msg;
}
