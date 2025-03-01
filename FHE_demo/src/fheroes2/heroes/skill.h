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

#include <string>
#include <vector>
#include <utility>

#include "ByteVectorReader.h"
#include "ByteVectorWriter.h"

void StringAppendModifiers(string&, int);

class Spell;

class Heroes;

namespace Skill
{
    int GetLeadershipModifiers(int level, string* strs);

    int GetLuckModifiers(int level, string* strs);

    void UpdateStats(const string&);

    namespace Level
    {
        enum
        {
            NONE = 0,
            BASIC = 1,
            ADVANCED = 2,
            EXPERT = 3
        };

        std::string String(int level);
    }

    enum class SkillT
    {
        UNKNOWN = 0,
        PATHFINDING = 1,
        ARCHERY = 2,
        LOGISTICS = 3,
        SCOUTING = 4,
        DIPLOMACY = 5,
        NAVIGATION = 6,
        LEADERSHIP = 7,
        WISDOM = 8,
        MYSTICISM = 9,
        LUCK = 10,
        BALLISTICS = 11,
        EAGLEEYE = 12,
        NECROMANCY = 13,
        ESTATES = 14,

        LEARNING = EAGLEEYE
    };

    class Secondary : public pair<SkillT, int>
    {
    public:

        Secondary();

        Secondary(SkillT skill, int level);

        void Reset();

        void Set(const Secondary&);

        void SetSkill(SkillT);

        void SetLevel(int);

        void NextLevel();

        int Level() const;

        SkillT Skill() const;

        bool isLevel(int) const;

        bool isSkill(SkillT) const;

        bool isValid() const;

        std::string GetName() const;

        string GetDescription() const;

        uint32_t GetValues() const;

        /* index sprite from SECSKILL */
        int GetIndexSprite1() const;

        /* index sprite from MINISS */
        int GetIndexSprite2() const;

        static SkillT RandForWitchsHut();

        static std::string String(SkillT);
    };

    ByteVectorReader& operator>>(ByteVectorReader&, Secondary&);

    class SecSkills
    {
    public:
        vector<Secondary> _items;
        SecSkills();

        SecSkills(int race);

        int GetLevel(SkillT skill) const;

        uint32_t GetValues(SkillT skill) const;

        void AddSkill(const Secondary&);

        void FindSkillsForLevelUp(int race, Secondary&, Secondary&) const;

        void FillMax(const Secondary&);

        Secondary* FindSkill(SkillT);

        string String() const;

        int Count() const;

        vector<Secondary>&
        ToVector();

    protected:
        friend ByteVectorWriter& operator<<(ByteVectorWriter&, const SecSkills&);

        friend ByteVectorReader& operator>>(ByteVectorReader&, SecSkills&);
    };

    ByteVectorWriter& operator<<(ByteVectorWriter&, const SecSkills&);
    ByteVectorReader& operator>>(ByteVectorReader&, SecSkills&);

    class Primary
    {
    public:
        Primary();

        virtual ~Primary() = default;

        enum
        {
            UNKNOWN = 0,
            ATTACK = 1,
            DEFENSE = 2,
            POWER = 3,
            KNOWLEDGE = 4
        };

        virtual int GetAttack() const = 0;

        virtual int GetDefense() const = 0;

        virtual int GetPower() const = 0;

        virtual int GetKnowledge() const = 0;

        virtual int GetMorale() const = 0;

        virtual int GetLuck() const = 0;

        virtual int GetRace() const = 0;

        virtual bool isCaptain() const;

        virtual bool isHeroes() const;

        int LevelUp(int race, int level);

        string StringSkills(const string&) const;

        static std::string String(int);

        static string StringDescription(int, const Heroes*);

        static int GetInitialSpell(int race);

        void ReadFrom(ByteVectorReader& msg);

    protected:
        void LoadDefaults(int type, int race);

        friend ByteVectorWriter& operator<<(ByteVectorWriter&, const Primary&);
        friend ByteVectorReader& operator>>(ByteVectorReader&, Primary&);

        int attack;
        int defense;
        int power;
        int knowledge;
    };

    ByteVectorWriter& operator<<(ByteVectorWriter&, const Primary&);

    ByteVectorReader& operator>>(ByteVectorReader&, Primary&);
}

#include "interface_itemsbar.h"

class PrimarySkillsBar : public Interface::ItemsBar<int>
{
public:
    PrimarySkillsBar(const Heroes*, bool mini);

    void SetTextOff(s32, s32);

    void RedrawBackground(const Rect&, Surface&);

    void RedrawItem(int&, const Rect&, Surface&);

    bool ActionBarSingleClick(const Point&, int&, const Rect&);

    bool ActionBarPressRight(const Point&, int&, const Rect&);

    bool ActionBarCursor(const Point&, int&, const Rect&);

    bool QueueEventProcessing(string* = nullptr);

protected:
    const Heroes* hero;
    Surface backsf;
    bool use_mini_sprite;
    vector<int> content;
    Point toff;
    string msg;
};

class SecondarySkillsBar : public Interface::ItemsBar<Skill::Secondary>
{
public:
    SecondarySkillsBar(bool mini = true, bool change = false);

    void RedrawBackground(const Rect&, Surface&);

    void RedrawItem(Skill::Secondary&, const Rect&, Surface&);

    bool ActionBarSingleClick(const Point&, Skill::Secondary&, const Rect&);

    bool ActionBarPressRight(const Point&, Skill::Secondary&, const Rect&);

    bool ActionBarCursor(const Point&, Skill::Secondary&, const Rect&);

    bool QueueEventProcessing(string* = nullptr);

protected:
    Surface backsf;
    bool use_mini_sprite;
    bool can_change;
    string msg;
};
