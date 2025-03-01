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
#include "icn.h"
#include "settings.h"
#include "payment.h"
#include "world.h"
#include "text.h"
#include "race.h"
#include "game.h"
#include "luck.h"
#include "morale.h"
#include "rand.h"
#include <sstream>

namespace
{
    bool ArmyStrongestTroop(const sp<Troop>& t1, const sp<Troop>& t2)
    {
        return t1->GetDamageMin() > t2->GetDamageMin();
    }
}

enum armysize_t
{
    ARMY_FEW = 1,
    ARMY_SEVERAL = 5,
    ARMY_PACK = 10,
    ARMY_LOTS = 20,
    ARMY_HORDE = 50,
    ARMY_THRONG = 100,
    ARMY_SWARM = 250,
    ARMY_ZOUNDS = 500,
    ARMY_LEGION = 1000
};

armysize_t ArmyGetSize(uint32_t count)
{
    if (ARMY_LEGION <= count) return ARMY_LEGION;
    if (ARMY_ZOUNDS <= count) return ARMY_ZOUNDS;
    if (ARMY_SWARM <= count) return ARMY_SWARM;
    if (ARMY_THRONG <= count) return ARMY_THRONG;
    if (ARMY_HORDE <= count) return ARMY_HORDE;
    if (ARMY_LOTS <= count) return ARMY_LOTS;
    if (ARMY_PACK <= count) return ARMY_PACK;
    if (ARMY_SEVERAL <= count) return ARMY_SEVERAL;

    return ARMY_FEW;
}

string Army::TroopSizeString(const Troop& troop)
{
    string str;

    switch (ArmyGetSize(troop.GetCount()))
    {
    default:
        str = _("A few %{monster}");
        break;
    case ARMY_SEVERAL:
        str = _("Several %{monster}");
        break;
    case ARMY_PACK:
        str = _("A pack of %{monster}");
        break;
    case ARMY_LOTS:
        str = _("Lots of %{monster}");
        break;
    case ARMY_HORDE:
        str = _("A horde of %{monster}");
        break;
    case ARMY_THRONG:
        str = _("A throng of %{monster}");
        break;
    case ARMY_SWARM:
        str = _("A swarm of %{monster}");
        break;
    case ARMY_ZOUNDS:
        str = _("Zounds of %{monster}");
        break;
    case ARMY_LEGION:
        str = _("A legion of %{monster}");
        break;
    }

    StringReplace(str, "%{monster}", StringLower(troop._monster.GetMultiName()));
    return str;
}

string Army::SizeString(uint32_t size)
{
    string str_size[] = {
        _("army|Few"), _("army|Several"), _("army|Pack"), _("army|Lots"),
        _("army|Horde"), _("army|Throng"), _("army|Swarm"), _("army|Zounds"), _("army|Legion")
    };

    switch (ArmyGetSize(size))
    {
    default:
        break;
    case ARMY_SEVERAL:
        return str_size[1];
    case ARMY_PACK:
        return str_size[2];
    case ARMY_LOTS:
        return str_size[3];
    case ARMY_HORDE:
        return str_size[4];
    case ARMY_THRONG:
        return str_size[5];
    case ARMY_SWARM:
        return str_size[6];
    case ARMY_ZOUNDS:
        return str_size[7];
    case ARMY_LEGION:
        return str_size[8];
    }

    return str_size[0];
}

Troops::Troops()
= default;

Troops::~Troops()
{
    _items.clear();
}

size_t Troops::Size() const
{
    return _items.size();
}

void Troops::Assign(const Troop* it1, const Troop* it2)
{
    Clean();

    auto it = _items.begin();

    while (it != _items.end() && it1 != it2)
    {
        if (it1->IsValid()) (*it)->Set(*it1);
        ++it;
        ++it1;
    }
}

void Troops::Assign(const Troops& troops)
{
    Clean();

    auto it1 = _items.begin();
    auto it2 = troops._items.begin();

    while (it1 != _items.end() && it2 != troops._items.end())
    {
        if ((*it2)->IsValid()) (*it1)->Set(**it2);
        ++it2;
        ++it1;
    }
}

void Troops::Insert(const Troops& troops)
{
    for (const auto& it : troops._items)
        _items.push_back(make_shared<Troop>(*it));
}

void Troops::PushBack(const Monster& mons, uint32_t count)
{
    _items.push_back(make_shared<Troop>(mons, count));
}

void Troops::PopBack()
{
    if (!this->_items.empty())
    {
        _items.pop_back();
    }
}

Troop* Troops::GetTroop(size_t pos)
{
    return pos < _items.size() ? _items.at(pos).get() : nullptr;
}

const Troop* Troops::GetTroop(size_t pos) const
{
    return pos < _items.size() ? _items.at(pos).get() : nullptr;
}

void Troops::UpgradeMonsters(const Monster& m)
{
    for (const auto& it : _items)
        if (it->IsValid() && *it == m) it->_monster.Upgrade();
}

uint32_t Troops::GetCountMonsters(const Monster& m) const
{
    uint32_t c = 0;

    for (const auto& it : _items)
        if (it->IsValid() && *it == m) c += it->GetCount();

    return c;
}

bool Troops::IsValid() const
{
    return _items.end() != find_if(_items.begin(), _items.end(), [](auto& it) { return it->IsValid(); });
}

uint32_t Troops::GetCount() const
{
    return count_if(_items.begin(), _items.end(), [](auto& it) { return it->IsValid(); });
}

bool Troops::HasMonster(const Monster& mons) const
{
    return _items.end() != find_if(_items.begin(), _items.end(), [&](auto& it) { return it->isMonster(mons()); });
}

bool Troops::AllTroopsIsRace(int race) const
{
    for (const auto& it : _items)
        if (it->IsValid() && it->_monster.GetRace() != race) return false;

    return true;
}

bool Troops::CanJoinTroop(const Monster& mons) const
{
    auto it = find_if(_items.begin(), _items.end(), [&](auto& it) { return it->isMonster(mons()); });
    if (it == _items.end()) it = find_if(_items.begin(), _items.end(), [&](auto& it) { return !it->IsValid(); });

    return it != _items.end();
}

bool Troops::JoinTroop(const Monster& mons, uint32_t count)
{
    if (mons.IsValid() && count)
    {
        auto it = find_if(_items.begin(), _items.end(), [&](auto& it) { return it->isMonster(mons()); });
        if (it == _items.end()) it = find_if(_items.begin(), _items.end(), [&](auto& it) { return !it->IsValid(); });

        if (it != _items.end())
        {
            if ((*it)->IsValid())
                (*it)->SetCount((*it)->GetCount() + count);
            else
                (*it)->Set(mons, count);

            return true;
        }
    }

    return false;
}

bool Troops::JoinTroop(const Troop& troop)
{
    return troop.IsValid() && JoinTroop(troop(), troop.GetCount());
}

bool Troops::CanJoinTroops(const Troops& troops2) const
{
    if (this == &troops2)
        return false;

    Army troops1;
    troops1.m_troops.Insert(*this);

    for (const auto& it : troops2._items)
        if (it->IsValid() && !troops1.m_troops.JoinTroop(*it)) return false;

    return true;
}

void Troops::JoinTroops(Troops& troops2)
{
    if (this == &troops2)
        return;

    for (auto& _item : troops2._items)
        if (_item->IsValid())
        {
            JoinTroop(*_item);
            _item->Reset();
        }
}

uint32_t Troops::GetUniqueCount() const
{
    vector<int> monsters;
    monsters.reserve(_items.size());

    for (const auto& _item : _items)
        if (_item->IsValid()) monsters.push_back(_item->_monster.GetID());

    sort(monsters.begin(), monsters.end());
    monsters.resize(distance(monsters.begin(),
                             unique(monsters.begin(), monsters.end())));

    return monsters.size();
}


uint32_t Troops::GetAttack() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : _items)
        if (_item->IsValid())
        {
            res += static_cast<Monster *>(&_item->_monster)->GetAttack();
            ++count;
        }

    return count ? res / count : 0;
}

uint32_t Troops::GetDefense() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : _items)
        if (_item->IsValid())
        {
            res += static_cast<Monster *>(&_item->_monster)->GetDefense();
            ++count;
        }

    return count ? res / count : 0;
}

uint32_t Troops::GetHitPoints() const
{
    uint32_t res = 0;

    for (const auto& _item : _items)
        if (_item->IsValid()) res += _item->GetHitPointsTroop();

    return res;
}

uint32_t Troops::GetDamageMin() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : _items)
        if (_item->IsValid())
        {
            res += _item->GetDamageMin();
            ++count;
        }

    return count ? res / count : 0;
}

uint32_t Troops::GetDamageMax() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : _items)
        if (_item->IsValid())
        {
            res += _item->GetDamageMax();
            ++count;
        }

    return count ? res / count : 0;
}

uint32_t Troops::GetStrength() const
{
    uint32_t res = 0;

    for (const auto& _item : _items)
        if (_item->IsValid()) res += _item->GetStrength();

    return res;
}

void Troops::Clean()
{
    for_each(_items.begin(), _items.end(), [](sp<Troop>& it) { it->Reset(); });
}

void Troops::UpgradeTroops(const Castle& castle)
{
    for (auto& _item : _items)
        if (_item->IsValid())
        {
            payment_t payment = _item->GetUpgradeCost();
            Kingdom& kingdom = castle.GetKingdom();

            if (castle.GetRace() == _item->_monster.GetRace() &&
                castle.isBuild(_item->_monster.GetUpgrade().GetDwelling()) &&
                kingdom.AllowPayment(payment))
            {
                kingdom.OddFundsResource(payment);
                _item->_monster.Upgrade();
            }
        }
}

Troop* Troops::GetFirstValid()
{
    auto it = find_if(_items.begin(), _items.end(), [](sp<Troop>& it) { return it->IsValid(); });
    return it == _items.end() ? nullptr : it->get();
}

Troop* Troops::GetWeakestTroop()
{
    auto first = _items.begin();
    auto last = _items.end();

    while (first != last)
    {
        if ((*first)->IsValid()) break;
        ++first;
    }

    if (first == _items.end()) return nullptr;
    auto lowest = first;

    if (first != last)
        while (++first != last)
            if ((*first)->IsValid() && Army::WeakestTroop(first->get(), lowest->get()))
                lowest = first;

    return lowest->get();
}

Troop* Troops::GetSlowestTroop()
{
    auto first = _items.begin();
    auto last = _items.end();

    while (first != last)
    {
        if ((*first)->IsValid()) break;
        ++first;
    }

    if (first == _items.end()) return nullptr;
    auto lowest = first;

    if (first != last)
        while (++first != last)
            if ((*first)->IsValid() && Army::SlowestTroop(first->get(), lowest->get()))
                lowest = first;

    return lowest->get();
}

Troops Troops::GetOptimized() const
{
    Troops result;
    result._items.reserve(_items.size());

    for (auto _item : _items)
    {
        if (!_item->IsValid()) continue;
        auto it2 = find_if(result._items.begin(), result._items.end(),
                           [&](sp<Troop>& it) { return it->isMonster(_item->_monster.GetID()); });

        if (it2 == result._items.end())
            result._items.push_back(make_shared<Troop>(*_item));
        else
            (*it2)->SetCount((*it2)->GetCount() + _item->GetCount());
    }
    return result;
}

void Troops::ArrangeForBattle(bool upgrade)
{
    Troops priority = GetOptimized();

    switch (priority._items.size())
    {
    case 1:
        {
            auto mPtr = priority._items.back();
            const Monster& m = mPtr->_monster;
            const uint32_t count = mPtr->GetCount();

            Clean();

            if (49 < count)
            {
                const uint32_t c = count / 5;
                _items.at(0)->Set(m, c);
                _items.at(1)->Set(m, c);
                _items.at(2)->Set(m, c + count - c * 5);
                _items.at(3)->Set(m, c);
                _items.at(4)->Set(m, c);

                if (upgrade && _items.at(2)->_monster.isAllowUpgrade())
                    _items.at(2)->_monster.Upgrade();
            }
            else if (20 < count)
            {
                const uint32_t c = count / 3;
                _items.at(1)->Set(m, c);
                _items.at(2)->Set(m, c + count - c * 3);
                _items.at(3)->Set(m, c);

                if (upgrade && _items.at(2)->_monster.isAllowUpgrade())
                    _items.at(2)->_monster.Upgrade();
            }
            else
                _items.at(2)->Set(m, count);
            break;
        }
    case 2:
        {
            // TODO: need modify army for 2 troops
            Assign(priority);
            break;
        }
    case 3:
        {
            // TODO: need modify army for 3 troops
            Assign(priority);
            break;
        }
    case 4:
        {
            // TODO: need modify army for 4 troops
            Assign(priority);
            break;
        }
    case 5:
        {
            // possible change orders monster
            // store
            Assign(priority);
            break;
        }
    default:
        break;
    }
}

void Troops::JoinStrongest(Troops& troops2, bool save_last)
{
    if (this == &troops2)
        return;

    auto priority = GetOptimized();
    priority._items.reserve(ARMYMAXTROOPS * 2);

    auto priority2 = troops2.GetOptimized();
    priority.Insert(priority2);

    Clean();
    troops2.Clean();

    // sort: strongest
    sort(priority._items.begin(), priority._items.end(), ArmyStrongestTroop);

    // weakest to army2
    while (_items.size() < priority._items.size())
    {
        troops2.JoinTroop(*priority._items.back());
        priority.PopBack();
    }

    // save half weak of strongest to army2
    if (save_last && !troops2.IsValid())
    {
        Troop& last = *priority._items.back();
        uint32_t count = last.GetCount() / 2;
        troops2.JoinTroop(last._monster, last.GetCount() - count);
        last.SetCount(count);
    }

    // strongest to army
    while (!priority._items.empty())
    {
        JoinTroop(*priority._items.back());
        priority.PopBack();
    }
}

void Troops::KeepOnlyWeakest(Troops& troops2, bool save_last)
{
    if (this == &troops2)
        return;

    Troops priority = GetOptimized();
    priority._items.reserve(ARMYMAXTROOPS * 2);

    Troops priority2 = troops2.GetOptimized();
    priority.Insert(priority2);

    Clean();
    troops2.Clean();

    // sort: strongest
    sort(priority._items.begin(), priority._items.end(), ArmyStrongestTroop);

    // weakest to army
    while (_items.size() < priority._items.size())
    {
        JoinTroop(*priority._items.back());
        priority.PopBack();
    }

    // save half weak of strongest to army
    if (save_last && !IsValid())
    {
        Troop& last = *priority._items.back();
        uint32_t count = last.GetCount() / 2;
        JoinTroop(last._monster, last.GetCount() - count);
        last.SetCount(count);
    }

    // strongest to army2
    while (!priority._items.empty())
    {
        troops2.JoinTroop(*priority._items.back());
        priority.PopBack();
    }
}

void Troops::DrawMons32LineWithScoute(s32 cx, s32 cy, uint32_t width, uint32_t first, uint32_t count, uint32_t scoute,
                                      bool shorts) const
{
    if (!IsValid()) return;
    if (0 == count) count = GetCount();

    const uint32_t chunk = width / count;
    cx += chunk / 2;

    Text text;
    text.Set(Font::SMALL);

    for (const auto& _item : _items)
    {
        if (!_item->IsValid())
            continue;
        if (0 == first && count)
        {
            const Sprite& monster = AGG::GetICN(ICN::MONS32, _item->_monster.GetSpriteIndex());

            monster.Blit(cx - monster.w() / 2, cy + 30 - monster.h());
            text.Set(Game::CountScoute(_item->GetCount(), scoute, shorts));
            text.Blit(cx - text.w() / 2, cy + 28);

            cx += chunk;
            --count;
        }
        else
            --first;
    }
}

void Troops::SplitTroopIntoFreeSlots(const Troop& troop, uint32_t slotCount)
{
    if (slotCount && slotCount <= Size() - GetCount())
    {
        uint32_t chunk = troop.GetCount() / slotCount;
        uint32_t limits = slotCount;
        vector<vector<sp<Troop>>::iterator> iters;

        for (auto it = _items.begin(); it != _items.end(); ++it)
            if (!(*it)->IsValid() && limits)
            {
                iters.push_back(it);
                (*it)->Set(troop.GetMonster(), chunk);
                --limits;
            }

        uint32_t last = troop.GetCount() - chunk * slotCount;

        for (auto& iter : iters)
        {
            if (last)
            {
                (*iter)->SetCount((*iter)->GetCount() + 1);
                --last;
            }
        }
    }
}


Army::Army(HeroBase* s) : commander(s), combat_format(true), color(Color::NONE)
{
    m_troops._items.reserve(ARMYMAXTROOPS);
    for (uint32_t ii = 0; ii < ARMYMAXTROOPS; ++ii) m_troops._items.push_back(std::make_shared<ArmyTroop>(this));
}

Army::Army(const Maps::Tiles& t) : commander(nullptr), combat_format(true), color(Color::NONE)
{
    m_troops._items.reserve(ARMYMAXTROOPS);
    for (uint32_t ii = 0; ii < ARMYMAXTROOPS; ++ii) m_troops._items.push_back(std::make_shared<ArmyTroop>(this));

    if (MP2::isCaptureObject(t.GetObject()))
        color = t.QuantityColor();

    switch (t.GetObject(false))
    {
    case MP2::OBJ_PYRAMID:
        m_troops._items.at(0)->Set(Monster::ROYAL_MUMMY, 10);
        m_troops._items.at(1)->Set(Monster::VAMPIRE_LORD, 10);
        m_troops._items.at(2)->Set(Monster::ROYAL_MUMMY, 10);
        m_troops._items.at(3)->Set(Monster::VAMPIRE_LORD, 10);
        m_troops._items.at(4)->Set(Monster::ROYAL_MUMMY, 10);
        break;

    case MP2::OBJ_GRAVEYARD:
        m_troops._items.at(0)->Set(Monster::MUTANT_ZOMBIE, 100);
        m_troops.ArrangeForBattle(false);
        break;

    case MP2::OBJ_SHIPWRECK:
        m_troops._items.at(0)->Set(Monster::GHOST, t.GetQuantity2());
        m_troops.ArrangeForBattle(false);
        break;

    case MP2::OBJ_DERELICTSHIP:
        m_troops._items.at(0)->Set(Monster::SKELETON, 200);
        m_troops.ArrangeForBattle(false);
        break;

    case MP2::OBJ_ARTIFACT:
        switch (t.QuantityVariant())
        {
        case 6:
            m_troops._items.at(0)->Set(Monster::ROGUE, 50);
            break;
        case 7:
            m_troops._items.at(0)->Set(Monster::GENIE, 1);
            break;
        case 8:
            m_troops._items.at(0)->Set(Monster::PALADIN, 1);
            break;
        case 9:
            m_troops._items.at(0)->Set(Monster::CYCLOPS, 1);
            break;
        case 10:
            m_troops._items.at(0)->Set(Monster::PHOENIX, 1);
            break;
        case 11:
            m_troops._items.at(0)->Set(Monster::GREEN_DRAGON, 1);
            break;
        case 12:
            m_troops._items.at(0)->Set(Monster::TITAN, 1);
            break;
        case 13:
            m_troops._items.at(0)->Set(Monster::BONE_DRAGON, 1);
            break;
        default:
            break;
        }
        m_troops.ArrangeForBattle(false);
        break;

        //case MP2::OBJ_ABANDONEDMINE:
        //    at(0) = Troop(t);
        //    ArrangeForBattle(false);
        //    break;

    case MP2::OBJ_CITYDEAD:
        m_troops._items.at(0)->Set(Monster::ZOMBIE, 20);
        m_troops._items.at(1)->Set(Monster::VAMPIRE_LORD, 5);
        m_troops._items.at(2)->Set(Monster::POWER_LICH, 5);
        m_troops._items.at(3)->Set(Monster::VAMPIRE_LORD, 5);
        m_troops._items.at(4)->Set(Monster::ZOMBIE, 20);
        break;

    case MP2::OBJ_TROLLBRIDGE:
        m_troops._items.at(0)->Set(Monster::TROLL, 4);
        m_troops._items.at(1)->Set(Monster::WAR_TROLL, 4);
        m_troops._items.at(2)->Set(Monster::TROLL, 4);
        m_troops._items.at(3)->Set(Monster::WAR_TROLL, 4);
        m_troops._items.at(4)->Set(Monster::TROLL, 4);
        break;

    case MP2::OBJ_DRAGONCITY:
        m_troops._items.at(0)->Set(Monster::GREEN_DRAGON, 3);
        m_troops._items.at(1)->Set(Monster::RED_DRAGON, 2);
        m_troops._items.at(2)->Set(Monster::BLACK_DRAGON, 1);
        break;

    case MP2::OBJ_DAEMONCAVE:
        m_troops._items.at(0)->Set(Monster::EARTH_ELEMENT, 2);
        m_troops._items.at(1)->Set(Monster::EARTH_ELEMENT, 2);
        m_troops._items.at(2)->Set(Monster::EARTH_ELEMENT, 2);
        m_troops._items.at(3)->Set(Monster::EARTH_ELEMENT, 2);
        break;

    default:
        if (MP2::isCaptureObject(t.GetObject()))
        {
            CapturedObject& co = world.GetCapturedObject(t.GetIndex());
            Troop& troop = co.GetTroop();

            switch (co.GetSplit())
            {
            case 3:
                if (3 > troop.GetCount())
                    m_troops._items.at(0)->Set(co.GetTroop());
                else
                {
                    m_troops._items.at(0)->Set(troop(), troop.GetCount() / 3);
                    m_troops._items.at(4)->Set(troop(), troop.GetCount() / 3);
                    m_troops._items.at(2)->Set(troop(), troop.GetCount() - m_troops._items.at(4)->GetCount() -
                                               m_troops._items.at(0)->GetCount());
                }
                break;

            case 5:
                if (5 > troop.GetCount())
                    m_troops._items.at(0)->Set(co.GetTroop());
                else
                {
                    m_troops._items.at(0)->Set(troop(), troop.GetCount() / 5);
                    m_troops._items.at(1)->Set(troop(), troop.GetCount() / 5);
                    m_troops._items.at(3)->Set(troop(), troop.GetCount() / 5);
                    m_troops._items.at(4)->Set(troop(), troop.GetCount() / 5);
                    m_troops._items.at(2)->Set(troop(),
                                               troop.GetCount() - m_troops._items.at(0)->GetCount() -
                                               m_troops._items.at(1)->GetCount() -
                                               m_troops._items.at(3)->GetCount() -
                                               m_troops._items.at(4)->GetCount());
                }
                break;

            default:
                m_troops._items.at(0)->Set(co.GetTroop());
                break;
            }
        }
        else
        {
            auto* map_troop = static_cast<MapMonster *>(world.GetMapObject(t.GetObjectUID(MP2::OBJ_MONSTER)));
            Troop troop = map_troop ? map_troop->QuantityTroop() : t.QuantityTroop();

            m_troops._items.at(0)->Set(troop);
            m_troops.ArrangeForBattle(!Settings::Get().ExtWorldSaveMonsterBattle());
        }
        break;
    }
}

Army::~Army()
{
    m_troops._items.clear();
}

bool Army::isFullHouse() const
{
    return m_troops.GetCount() == m_troops._items.size();
}

void Army::SetSpreadFormat(bool f)
{
    combat_format = f;
}

bool Army::isSpreadFormat() const
{
    return combat_format;
}

int Army::GetColor() const
{
    return GetCommander() ? GetCommander()->GetColor() : color;
}

void Army::SetColor(int cl)
{
    color = cl;
}

int Army::GetRace() const
{
    vector<int> races;
    races.reserve(m_troops._items.size());

    for (const auto& _item : m_troops._items)
        if (_item->IsValid()) races.push_back(_item->_monster.GetRace());

    sort(races.begin(), races.end());
    races.resize(distance(races.begin(), unique(races.begin(), races.end())));

    if (races.empty())
    {
        return Race::NONE;
    }

    return 1 < races.size() ? Race::MULT : races[0];
}

int Army::GetLuck() const
{
    return GetCommander() ? GetCommander()->GetLuck() : GetLuckModificator(nullptr);
}

int Army::GetLuckModificator(string* strs)
{
    return Luck::NORMAL;
}

int Army::GetMorale() const
{
    return GetCommander() ? GetCommander()->GetMorale() : GetMoraleModificator(nullptr);
}

// TODO:: need optimize
int Army::GetMoraleModificator(string* strs) const
{
    int result = Morale::NORMAL;

    // different race penalty
    uint32_t count = 0;
    uint32_t count_kngt = 0;
    uint32_t count_barb = 0;
    uint32_t count_sorc = 0;
    uint32_t count_wrlk = 0;
    uint32_t count_wzrd = 0;
    uint32_t count_necr = 0;
    uint32_t count_bomg = 0;
    bool ghost_present = false;

    for (const auto& _item : m_troops._items)
    {
        if (!_item->IsValid())
            continue;
        switch (_item->_monster.GetRace())
        {
        case Race::KNGT:
            ++count_kngt;
            break;
        case Race::BARB:
            ++count_barb;
            break;
        case Race::SORC:
            ++count_sorc;
            break;
        case Race::WRLK:
            ++count_wrlk;
            break;
        case Race::WZRD:
            ++count_wzrd;
            break;
        case Race::NECR:
            ++count_necr;
            break;
        case Race::NONE:
            ++count_bomg;
            break;
        default:
            break;
        }
        if (_item->_monster.GetID() == Monster::GHOST) ghost_present = true;
    }

    uint32_t r = Race::MULT;
    if (count_kngt)
    {
        ++count;
        r = Race::KNGT;
    }
    if (count_barb)
    {
        ++count;
        r = Race::BARB;
    }
    if (count_sorc)
    {
        ++count;
        r = Race::SORC;
    }
    if (count_wrlk)
    {
        ++count;
        r = Race::WRLK;
    }
    if (count_wzrd)
    {
        ++count;
        r = Race::WZRD;
    }
    if (count_necr)
    {
        ++count;
        r = Race::NECR;
    }
    if (count_bomg)
    {
        ++count;
        r = Race::NONE;
    }
    const uint32_t uniq_count = m_troops.GetUniqueCount();

    switch (count)
    {
    case 2:
    case 0:
        break;
    case 1:
        if (0 == count_necr && !ghost_present)
        {
            if (1 < uniq_count)
            {
                ++result;
                if (strs)
                {
                    string str = _("All %{race} troops +1");
                    StringReplace(str, "%{race}", Race::String(r));
                    strs->append(str);
                    strs->append("\n");
                }
            }
        }
        else
        {
            if (strs)
            {
                strs->append(_("Entire unit is undead, so morale does not apply."));
                strs->append("\n");
            }
            return 0;
        }
        break;
    case 3:
        result -= 1;
        if (strs)
        {
            strs->append(_("Troops of 3 alignments -1"));
            strs->append("\n");
        }
        break;
    case 4:
        result -= 2;
        if (strs)
        {
            strs->append(_("Troops of 4 alignments -2"));
            strs->append("\n");
        }
        break;
    default:
        result -= 3;
        if (strs)
        {
            strs->append(_("Troops of 5 alignments -3"));
            strs->append("\n");
        }
        break;
    }

    // undead in life group
    if ((1 < uniq_count && (count_necr || ghost_present) &&
        (count_kngt || count_barb || count_sorc || count_wrlk || count_wzrd || count_bomg)) ||
        // or artifact Arm Martyr
        (GetCommander() && GetCommander()->HasArtifact(Artifact::ARM_MARTYR)))
    {
        result -= 1;
        if (strs)
        {
            strs->append(_("Some undead in groups -1"));
            strs->append("\n");
        }
    }

    return result;
}

uint32_t Army::GetAttack() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : m_troops._items)
    {
        if (!_item->IsValid())
            continue;
        res += _item->_monster.GetAttack();
        ++count;
    }

    return count ? res / count : 0;
}

uint32_t Army::GetDefense() const
{
    uint32_t res = 0;
    uint32_t count = 0;

    for (const auto& _item : m_troops._items)
    {
        if (!_item->IsValid())
            continue;
        res += _item->_monster.GetDefense();
        ++count;
    }

    return count ? res / count : 0;
}

void Army::Reset(bool soft)
{
    m_troops.Clean();

    if (!commander || !commander->isHeroes())
        return;
    const Monster mons1(commander->GetRace(), DWELLING_MONSTER1);

    if (soft)
    {
        const Monster mons2(commander->GetRace(), DWELLING_MONSTER2);

        switch (Rand::Get(1, 3))
        {
        case 1:
            m_troops.JoinTroop(mons1, 3 * mons1.GetGrown());
            break;
        case 2:
            m_troops.JoinTroop(mons2, mons2.GetGrown() + mons2.GetGrown() / 2);
            break;
        default:
            m_troops.JoinTroop(mons1, 2 * mons1.GetGrown());
            m_troops.JoinTroop(mons2, mons2.GetGrown());
            break;
        }
    }
    else
    {
        m_troops.JoinTroop(mons1, 1);
    }
}

void Army::SetCommander(HeroBase* c)
{
    commander = c;
}

HeroBase* Army::GetCommander()
{
    return !commander || (commander->isCaptain() && !commander->isValid()) ? nullptr : commander;
}

const Castle* Army::inCastle() const
{
    return commander ? commander->inCastle() : nullptr;
}

const HeroBase* Army::GetCommander() const
{
    return !commander || (commander->isCaptain() && !commander->isValid()) ? nullptr : commander;
}

int Army::GetControl() const
{
    return commander ? commander->GetControl() : color == Color::NONE ? CONTROL_AI : Players::GetPlayerControl(color);
}

string Army::String() const
{
    ostringstream os;

    os << "color(" << Color::String(commander ? commander->GetColor() : color) << "), ";

    if (GetCommander())
        os << "commander(" << GetCommander()->GetName() << "), ";

    os << ": ";

    for (const auto& _item : m_troops._items)
        if (_item->IsValid())
            os << dec << _item->GetCount() << " " << _item->GetName() << ", ";

    return os.str();
}

void Army::JoinStrongestFromArmy(Army& army2)
{
    bool save_last = army2.commander && army2.commander->isHeroes();
    m_troops.JoinStrongest(army2.m_troops, save_last);
}

void Army::KeepOnlyWeakestTroops(Army& army2)
{
    bool save_last = commander && commander->isHeroes();
    m_troops.KeepOnlyWeakest(army2.m_troops, save_last);
}

uint32_t Army::ActionToSirens()
{
    uint32_t res = 0;

    for (auto& _item : m_troops._items)
    {
        if (!_item->IsValid()) continue;
        const uint32_t kill = _item->GetCount() * 30 / 100;

        if (!kill) continue;
        _item->SetCount(_item->GetCount() - kill);
        res += kill * static_cast<Monster *>(&_item->_monster)->GetHitPoints();
    }
    return res;
}

bool Army::TroopsStrongerEnemyTroops(const Troops& troops1, const Troops& troops2)
{
    if (!troops2.IsValid()) return true;

    const int a1 = troops1.GetAttack();
    const int d1 = troops1.GetDefense();
    double r1 = 0;

    const int a2 = troops2.GetAttack();
    const int d2 = troops2.GetDefense();
    double r2 = 0;

    if (a1 > d2)
        r1 = 1 + 0.1 * static_cast<float>(min(a1 - d2, 20));
    else
        r1 = 1 + 0.05 * static_cast<float>(min(d2 - a1, 14));

    if (a2 > d1)
        r2 = 1 + 0.1 * static_cast<float>(min(a2 - d1, 20));
    else
        r2 = 1 + 0.05 * static_cast<float>(min(d1 - a2, 14));

    const uint32_t s1 = troops1.GetStrength();
    const uint32_t s2 = troops2.GetStrength();

    const float h1 = troops1.GetHitPoints();
    const float h2 = troops2.GetHitPoints();


    r1 *= s1 / h2;
    r2 *= s2 / h1;

    return static_cast<s32>(r1) > static_cast<s32>(r2);
}

void Army::DrawMons32LineWithScoute(const Troops& troops, s32 cx, s32 cy, uint32_t width, uint32_t first,
                                    uint32_t count, uint32_t scoute)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, scoute, false);
}

/* draw MONS32 sprite in line, first valid = 0, count = 0 */
void Army::DrawMons32Line(const Troops& troops, s32 cx, s32 cy, uint32_t width, uint32_t first, uint32_t count)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, Skill::Level::EXPERT, false);
}

void Army::DrawMons32LineShort(const Troops& troops, s32 cx, s32 cy, uint32_t width, uint32_t first, uint32_t count)
{
    troops.DrawMons32LineWithScoute(cx, cy, width, first, count, Skill::Level::EXPERT, true);
}

JoinCount Army::GetJoinSolution(const Heroes& hero, const Maps::Tiles& tile, const Troop& troop)
{
    auto* map_troop = static_cast<MapMonster *>(world.GetMapObject(tile.GetObjectUID(MP2::OBJ_MONSTER)));
    const uint32_t ratios = troop.IsValid() ? hero.GetArmy().m_troops.GetHitPoints() / troop.GetHitPointsTroop() : 0;
    const bool check_free_stack = true;
    // (hero.GetArmy().GetCount() < hero.GetArmy().size() || hero.GetArmy().HasMonster(troop)); // set force, see Dialog::ArmyJoinWithCost, http://sourceforge.net/tracker/?func=detail&aid=3567985&group_id=96859&atid=616183
    const bool check_extra_condition = !hero.HasArtifact(Artifact::HIDEOUS_MASK) &&
        Morale::NORMAL <= hero.GetMorale();

    const bool join_skip = map_troop ? map_troop->JoinConditionSkip() : tile.MonsterJoinConditionSkip();
    const bool join_free = map_troop ? map_troop->JoinConditionFree() : tile.MonsterJoinConditionFree();
    // force join for campain and others...
    const bool join_force = map_troop ? map_troop->JoinConditionForce() : tile.MonsterJoinConditionForce();

    if (!join_skip &&
        check_free_stack && ((check_extra_condition && ratios >= 2) || join_force))
    {
        if (join_free || join_force)
            return {JOIN_FREE, troop.GetCount()};
        if (hero.HasSecondarySkill(Skill::SkillT::DIPLOMACY))
        {
            // skill diplomacy
            const uint32_t to_join = Monster::GetCountFromHitPoints(troop._monster,
                                                                    troop.GetHitPointsTroop() *
                                                                    hero.GetSecondaryValues(Skill::SkillT::DIPLOMACY)
                                                                    /
                                                                    100);

            if (to_join)
                return {JOIN_COST, to_join};
        }
    }
    else if (ratios >= 5)
    {
        // ... surely flee before us
        if (!hero.isControlAI() ||
            Rand::Get(0, 10) < 5)
            return {JOIN_FLEE, 0};
    }

    return JoinCount(JOIN_NONE, 0);
}

bool Army::WeakestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetDamageMax() < t2->GetDamageMax();
}

bool Army::StrongestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetDamageMin() > t2->GetDamageMin();
}

bool Army::SlowestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetSpeed() < t2->GetSpeed();
}

bool Army::FastestTroop(const Troop* t1, const Troop* t2)
{
    return t1->GetSpeed() > t2->GetSpeed();
}

void Army::SwapTroops(Troop& t1, Troop& t2)
{
    swap(t1, t2);
}

bool Army::SaveLastTroop() const
{
    return commander && commander->isHeroes() && 1 == m_troops.GetCount();
}

ByteVectorWriter& operator<<(ByteVectorWriter& msg, const Army& army)
{
    msg << static_cast<uint32_t>(army.m_troops._items.size());

    // Army: fixed size
    for (const auto& _item : army.m_troops._items)
        msg << *_item;

    return msg << army.combat_format << army.color;
}


ByteVectorReader& operator>>(ByteVectorReader& msg, Army& army)
{
    uint32_t armysz;
    msg >> armysz;

    for (auto& _item : army.m_troops._items)
        msg >> *_item;

    msg >> army.combat_format >> army.color;

    // set army
    for (auto it = army.m_troops._items.begin(); it != army.m_troops._items.end(); ++it)
    {
        auto* troop = dynamic_cast<ArmyTroop *>(it->get());
        if (troop) troop->SetArmy(army);
    }

    // set later from owner (castle, heroes)
    army.commander = nullptr;

    return msg;
}
