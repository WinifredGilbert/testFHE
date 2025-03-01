/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *   Copyright (C) 2009 by Josh Matthews  <josh@joshmatthews.net>          *
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

#include "bitmodes.h"
#include "skill.h"
#include "spell_book.h"
#include "artifact.h"
#include "position.h"
#include "players.h"

#include "ByteVectorReader.h"
#include "ByteVectorWriter.h"

class Castle;
class Army;

enum
{
    MDF_NONE,
    MDF_ATTACK,
    MDF_DEFENSE,
    MDF_POWER,
    MDF_KNOWLEDGE,
    MDF_MORALE,
    MDF_LUCK
};

enum
{
    PORT_BIG = 1,
    PORT_MEDIUM = 2,
    PORT_SMALL = 3
};

class HeroBase : public Skill::Primary, public MapPosition, public BitModes, public Control
{
public:
    HeroBase(int type, int race);

    HeroBase();

    enum
    {
        UNDEFINED,
        CAPTAIN,
        HEROES
    };

    virtual const string& GetName() const = 0;

    virtual int GetColor() const = 0;

    virtual int GetControl() const = 0;

    virtual bool isValid() const = 0;

    virtual const Army& GetArmy() const = 0;

    virtual Army& GetArmy() = 0;

    virtual uint32_t GetMaxSpellPoints() const = 0;

    virtual int GetLevelSkill(Skill::SkillT skill) const = 0;

    virtual uint32_t GetSecondaryValues(Skill::SkillT skill) const = 0;

    virtual void ActionAfterBattle() = 0;

    virtual void ActionPreBattle() = 0;

    virtual const Castle* inCastle() const = 0;

    virtual void PortraitRedraw(s32, s32, int type, Surface&) const = 0;

    virtual int GetType() const = 0;

    bool isCaptain() const;

    bool isHeroes() const;

    int GetAttackModificator(string* = nullptr) const;

    int GetDefenseModificator(string* = nullptr) const;

    int GetPowerModificator(string* = nullptr) const;

    int GetKnowledgeModificator(string* = nullptr) const;

    int GetMoraleModificator(string* = nullptr) const;

    int GetLuckModificator(string* = nullptr) const;

    uint32_t GetSpellPoints() const;

    bool HaveSpellPoints(const Spell&) const;

    bool CanCastSpell(const Spell&, string* = nullptr) const;

    bool CanTeachSpell(const Spell&) const;

    bool CanLearnSpell(const Spell&) const;

    bool CanTranscribeScroll(const Artifact&) const;

    void TranscribeScroll(const Artifact&);

    void SpellCasted(const Spell&);

    void SetSpellPoints(uint32_t);

    void EditSpellBook();

    Spell OpenSpellBook(int filter, bool) const;

    bool HaveSpellBook() const;

    bool HaveSpell(const Spell&, bool skip_bag = false) const;

    void AppendSpellToBook(const Spell&, bool without_wisdom = false);

    void AppendSpellsToBook(const SpellStorage&, bool without_wisdom = false);

    bool SpellBookActivate();

    BagArtifacts& GetBagArtifacts();

    const BagArtifacts& GetBagArtifacts() const;

    uint32_t HasArtifact(const Artifact&) const;

    bool PickupArtifact(const Artifact&);

    void LoadDefaults(int type, int race);

    void ReadFrom(ByteVectorReader& msg);

protected:
    friend ByteVectorWriter& operator<<(ByteVectorWriter&, const HeroBase&);
    friend ByteVectorReader& operator>>(ByteVectorReader&, HeroBase&);

    uint32_t magic_point;
    uint32_t move_point;

    SpellBook spell_book;
    BagArtifacts bag_artifacts;
};

ByteVectorWriter& operator<<(ByteVectorWriter&, const HeroBase&);


ByteVectorReader& operator>>(ByteVectorReader&, HeroBase&);
