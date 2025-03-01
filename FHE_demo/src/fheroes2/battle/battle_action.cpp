/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle.h"
#include <algorithm>
#include "settings.h"
#include "world.h"
#include "battle_command.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "battle_tower.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_interface.h"
#include "rand.h"
#include "battle_army.h"
#include <functional>
#include "battle_force.h"

void Battle::Arena::BattleProcess(Unit& attacker, Unit& defender, s32 dst, int dir)
{
    if (0 > dst) dst = defender.GetHeadIndex();

    if (dir)
    {
        if (attacker._monster.isWide())
        {
            if (!Board::isNearIndexes(attacker.GetHeadIndex(), dst))
                attacker.UpdateDirection(board._items[dst].GetPos());
            if (defender.AllowResponse())
                defender.UpdateDirection(board._items[attacker.GetHeadIndex()].GetPos());
        }
        else
        {
            attacker.UpdateDirection(board._items[dst].GetPos());
            if (defender.AllowResponse())
                defender.UpdateDirection(board._items[attacker.GetHeadIndex()].GetPos());
        }
    }
    else
        attacker.UpdateDirection(board._items[dst].GetPos());

    TargetsInfo targets = GetTargetsForDamage(attacker, defender, dst);

    if (Board::isReflectDirection(dir) != attacker.isReflect())
        attacker.UpdateDirection(board._items[dst].GetPos());

    if (interface) interface->RedrawActionAttackPart1(attacker, defender, targets);

    TargetsApplyDamage(attacker, defender, targets);
    if (interface) interface->RedrawActionAttackPart2(attacker, targets);

    const Spell spell = attacker.GetSpellMagic();

    // magic attack
    if (defender.isValid() && spell.isValid())
    {
        const string name(attacker.GetName());
        targets = GetTargetsForSpells(attacker.GetCommander(), spell, defender.GetHeadIndex());

        if (!targets._items.empty())
        {
            if (interface)
                interface->RedrawActionSpellCastPart1(spell, defender.GetHeadIndex(), nullptr, name, targets);

            // magic attack not depends from hero
            TargetsApplySpell(nullptr, spell, targets);
            if (interface) interface->RedrawActionSpellCastPart2(spell, targets);
            if (interface) interface->RedrawActionMonsterSpellCastStatus(attacker, targets._items.front());
        }
    }

    attacker.PostAttackAction(defender);
}

void Battle::Arena::ApplyAction(Command& cmd)
{
    switch (cmd.GetType())
    {
    case MSG_BATTLE_CAST:
        ApplyActionSpellCast(cmd);
        break;
    case MSG_BATTLE_ATTACK:
        ApplyActionAttack(cmd);
        break;
    case MSG_BATTLE_MOVE:
        ApplyActionMove(cmd);
        break;
    case MSG_BATTLE_SKIP:
        ApplyActionSkip(cmd);
        break;
    case MSG_BATTLE_END_TURN:
        ApplyActionEnd(cmd);
        break;
    case MSG_BATTLE_MORALE:
        ApplyActionMorale(cmd);
        break;

    case MSG_BATTLE_TOWER:
        ApplyActionTower(cmd);
        break;
    case MSG_BATTLE_CATAPULT:
        ApplyActionCatapult(cmd);
        break;

    case MSG_BATTLE_RETREAT:
        ApplyActionRetreat(cmd);
        break;
    case MSG_BATTLE_SURRENDER:
        ApplyActionSurrender(cmd);
        break;

    case MSG_BATTLE_AUTO:
        ApplyActionAutoBattle(cmd);
        break;

    default:
        break;
    }
}

void Battle::Arena::ApplyActionSpellCast(Command& cmd)
{
    const Spell spell(cmd.GetValue());
    HeroBase* current_commander = GetCurrentForce().GetCommander();

    if (current_commander && current_commander->HaveSpellBook() &&
        !current_commander->Modes(Heroes::SPELLCASTED) &&
        current_commander->CanCastSpell(spell) && spell.isCombat())
    {
        // uniq spells action
        switch (spell())
        {
        case Spell::TELEPORT:
            ApplyActionSpellTeleport(cmd);
            break;

        case Spell::EARTHQUAKE:
            ApplyActionSpellEarthQuake(cmd);
            break;

        case Spell::MIRRORIMAGE:
            ApplyActionSpellMirrorImage(cmd);
            break;

        case Spell::SUMMONEELEMENT:
        case Spell::SUMMONAELEMENT:
        case Spell::SUMMONFELEMENT:
        case Spell::SUMMONWELEMENT:
            ApplyActionSpellSummonElemental(cmd, spell);
            break;

        default:
            ApplyActionSpellDefaults(cmd, spell);
            break;
        }

        current_commander->SetModes(Heroes::SPELLCASTED);
        current_commander->SpellCasted(spell);

        // save spell for "eagle eye" capability
        usage_spells.Append(spell);
    }
    else
    {
    }
}

void Battle::Arena::ApplyActionAttack(Command& cmd)
{
    const uint32_t uid1 = cmd.GetValue();
    const uint32_t uid2 = cmd.GetValue();
    const s32 dst = cmd.GetValue();
    const int dir = cmd.GetValue();

    Unit* b1 = GetTroopUID(uid1);
    Unit* b2 = GetTroopUID(uid2);

    if (!b1 || !b1->isValid()
        || !b2 || !b2->isValid()
        || (b1->GetColor() == b2->GetColor() && !b2->Modes(SP_HYPNOTIZE)))
        return;
    // reset blind
    if (b2->Modes(SP_BLIND)) b2->ResetBlind();

    const bool handfighting = Unit::isHandFighting(*b1, *b2);
    // check position
    if (!b1->isArchers() && !handfighting)
        return;
    // attack
    BattleProcess(*b1, *b2, dst, dir);

    if (b2->isValid())
    {
        // defense answer
        if (handfighting && !b1->_monster.isHideAttack() && b2->AllowResponse())
        {
            BattleProcess(*b2, *b1);
            b2->SetResponse();
        }

        // twice attack
        if (b1->isValid() && b1->isTwiceAttack() && !b1->Modes(IS_PARALYZE_MAGIC))
        {
            BattleProcess(*b1, *b2);
        }
    }

    b1->UpdateDirection();
    b2->UpdateDirection();
}

void Battle::Arena::ApplyActionMove(Command& cmd)
{
    uint32_t uid = cmd.GetValue();
    s32 dst = cmd.GetValue();
    int path_size = cmd.GetValue();

    Unit* b = GetTroopUID(uid);
    Cell* cell = Board::GetCell(dst);

    if (!b || !b->isValid() || !cell || !cell->isPassable3(*b, false))
        return;
    Position pos2;
    const s32 head = b->GetHeadIndex();
    Position pos1 = Position::GetCorrect(*b, dst);

    // force check fly
    if (static_cast<ArmyTroop *>(b)->_monster.isFly())
    {
        b->UpdateDirection(pos1.GetRect());
        if (b->isReflect() != pos1.isReflect()) pos1.Swap();
        if (interface) interface->RedrawActionFly(*b, pos1);
        pos2 = pos1;
    }
    else
    {
        Indexes path;

        // check path
        if (0 == path_size)
        {
            path = GetPath(*b, pos1);
            cmd = Command(MSG_BATTLE_MOVE, b->GetUID(), dst, path);
        }
        else
            for (int index = 0; index < path_size; ++index)
                path.push_back(cmd.GetValue());

        if (path.empty())
        {
            return;
        }

        if (interface) interface->RedrawActionMove(*b, path);
        else if (bridge)
        {
            for (Indexes::const_iterator
                 it = path.begin(); it != path.end(); ++it)
                if (bridge->NeedAction(*b, *it)) bridge->Action(*b, *it);
        }

        if (b->_monster.isWide())
        {
            const s32 dst1 = path.back();
            const s32 dst2 = 1 < path.size() ? path[path.size() - 2] : head;

            pos2.Set(dst1, b->_monster.isWide(), RIGHT_SIDE & Board::GetDirection(dst1, dst2));
        }
        else
            pos2.Set(path.back(), false, b->isReflect());
    }

    b->SetPosition(pos2);
    b->UpdateDirection();
}

void Battle::Arena::ApplyActionSkip(Command& cmd)
{
    uint32_t uid = cmd.GetValue();
    int hard = cmd.GetValue();

    Unit* battle = GetTroopUID(uid);
    if (!battle || !battle->isValid())
        return;
    if (battle->Modes(TR_MOVED))
        return;
    if (hard)
    {
        battle->SetModes(TR_HARDSKIP);
        battle->SetModes(TR_SKIPMOVE);
        battle->SetModes(TR_MOVED);
        if (Settings::Get().ExtBattleSkipIncreaseDefense()) battle->SetModes(TR_DEFENSED);
    }
    else
        battle->SetModes(battle->Modes(TR_SKIPMOVE) ? TR_MOVED : TR_SKIPMOVE);

    if (interface) interface->RedrawActionSkipStatus(*battle);
}

void Battle::Arena::ApplyActionEnd(Command& cmd)
{
    uint32_t uid = cmd.GetValue();

    Unit* battle = GetTroopUID(uid);

    if (!battle)
        return;
    if (battle->Modes(TR_MOVED))
        return;
    battle->SetModes(TR_MOVED);

    if (battle->Modes(TR_SKIPMOVE) && interface) interface->RedrawActionSkipStatus(*battle);
}

void Battle::Arena::ApplyActionMorale(Command& cmd)
{
    uint32_t uid = cmd.GetValue();
    int morale = cmd.GetValue();

    Unit* b = GetTroopUID(uid);

    if (!b || !b->isValid())
        return;
    // good morale
    if (morale && b->Modes(TR_MOVED) && b->Modes(MORALE_GOOD))
    {
        b->ResetModes(TR_MOVED);
        b->ResetModes(MORALE_GOOD);
        end_turn = false;
    }
        // bad morale
    else if (!morale && !b->Modes(TR_MOVED) && b->Modes(MORALE_BAD))
    {
        b->SetModes(TR_MOVED);
        b->ResetModes(MORALE_BAD);
        end_turn = true;
    }

    if (interface) interface->RedrawActionMorale(*b, morale);
}

void Battle::Arena::ApplyActionRetreat(Command& cmd)
{
    if (!CanRetreatOpponent(current_color))
        return;
    if (army1->GetColor() == current_color)
    {
        result_game.army1 = RESULT_RETREAT;
    }
    else if (army2->GetColor() == current_color)
    {
        result_game.army2 = RESULT_RETREAT;
    }
}

void Battle::Arena::ApplyActionSurrender(Command& cmd)
{
    if (!CanSurrenderOpponent(current_color))
        return;
    Funds cost;

    if (army1->GetColor() == current_color)
        cost.gold = army1->GetSurrenderCost();
    else if (army2->GetColor() == current_color)
        cost.gold = army2->GetSurrenderCost();

    if (!world.GetKingdom(current_color).AllowPayment(cost))
        return;
    if (army1->GetColor() == current_color)
    {
        result_game.army1 = RESULT_SURRENDER;
        world.GetKingdom(current_color).OddFundsResource(cost);
        world.GetKingdom(army2->GetColor()).AddFundsResource(cost);
    }
    else if (army2->GetColor() == current_color)
    {
        result_game.army2 = RESULT_SURRENDER;
        world.GetKingdom(current_color).OddFundsResource(cost);
        world.GetKingdom(army1->GetColor()).AddFundsResource(cost);
    }
}

void Battle::Arena::TargetsApplyDamage(Unit& attacker, Unit& defender, TargetsInfo& targets)
{
    for (auto& target : targets._items)
    {
        if (!target.defender) continue;
        target.killed = target.defender->ApplyDamage(attacker, target.damage);
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForDamage(Unit& attacker, Unit& defender, s32 dst) const
{
    TargetsInfo targets;
    targets._items.reserve(8);

    Unit* enemy = nullptr;
    Cell* cell = nullptr;
    TargetInfo res;

    // first target
    res.defender = &defender;
    res.damage = attacker.GetDamage(defender);
    targets._items.push_back(res);

    // long distance attack
    if (attacker._monster.isDoubleCellAttack())
    {
        const int dir = Board::GetDirection(attacker.GetHeadIndex(), dst);
        if (!defender._monster.isWide() || 0 == ((RIGHT | LEFT) & dir))
        {
            if (nullptr != (cell = Board::GetCell(dst, dir)) &&
                nullptr != (enemy = cell->GetUnit()) && enemy != &defender)
            {
                res.defender = enemy;
                res.damage = attacker.GetDamage(*enemy);
                targets._items.push_back(res);
            }
        }
    }
    else
        // around hydra
        if (attacker.GetID() == Monster::HYDRA)
        {
            vector<Unit *> v;
            v.reserve(8);

            const Indexes around = Board::GetAroundIndexes(attacker);

            for (int it : around)
            {
                if (nullptr != (enemy = Board::GetCell(it)->GetUnit()) &&
                    enemy != &defender && enemy->GetColor() != attacker.GetColor())
                {
                    res.defender = enemy;
                    res.damage = attacker.GetDamage(*enemy);
                    targets._items.push_back(res);
                }
            }
        }
        else
            // lich cloud damages
            if ((attacker.GetID() == Monster::LICH ||
                attacker.GetID() == Monster::POWER_LICH) && !attacker.isHandFighting())
            {
                const Indexes around = Board::GetAroundIndexes(defender.GetHeadIndex());

                for (int it : around)
                {
                    if (nullptr != (enemy = Board::GetCell(it)->GetUnit()) && enemy != &defender)
                    {
                        res.defender = enemy;
                        res.damage = attacker.GetDamage(*enemy);
                        targets._items.push_back(res);
                    }
                }
            }

    return targets;
}

void Battle::Arena::TargetsApplySpell(const HeroBase* hero, const Spell& spell, TargetsInfo& targets)
{
    for (auto& target : targets._items)
    {
        if (!target.defender) continue;
        target.defender->ApplySpell(spell, hero, target);
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForSpells(const HeroBase* hero, const Spell& spell, s32 dst)
{
    TargetsInfo targets;
    targets._items.reserve(8);

    TargetInfo res;
    Unit* target = GetTroopBoard(dst);

    // from spells
    switch (spell())
    {
    case Spell::CHAINLIGHTNING:
    case Spell::COLDRING:
        // skip center
        target = nullptr;
        break;

    default:
        break;
    }

    // first target
    if (target && target->AllowApplySpell(spell, hero))
    {
        res.defender = target;
        targets._items.push_back(res);
    }

    // resurrect spell? get target from graveyard
    if (nullptr == target && GraveyardAllowResurrect(dst, spell))
    {
        target = GetTroopUID(graveyard.GetLastTroopUID(dst));

        if (target && target->AllowApplySpell(spell, hero))
        {
            res.defender = target;
            targets._items.push_back(res);
        }
    }
    else
        // check other spells
        switch (spell())
        {
        case Spell::CHAINLIGHTNING:
            {
                Indexes trgts;
                trgts.reserve(12);
                trgts.push_back(dst);

                // find targets
                for (uint32_t ii = 0; ii < 3; ++ii)
                {
                    const Indexes reslt = board.GetNearestTroopIndexes(dst, &trgts);
                    if (reslt.empty()) break;
                    trgts.push_back(reslt.size() > 1 ? *Rand::Get(reslt) : reslt.front());
                }

                // save targets
                for (auto
                     it = trgts.begin(); it != trgts.end(); ++it)
                {
                    Unit* target = GetTroopBoard(*it);

                    if (target)
                    {
                        res.defender = target;
                        // store temp priority for calculate damage
                        res.damage = distance(trgts.begin(), it);
                        targets._items.push_back(res);
                    }
                }
            }
            break;

            // check abroads
        case Spell::FIREBALL:
        case Spell::METEORSHOWER:
        case Spell::COLDRING:
        case Spell::FIREBLAST:
            {
                const Indexes positions = Board::GetDistanceIndexes(dst, spell == Spell::FIREBLAST ? 2 : 1);

                for (int position : positions)
                {
                    Unit* target = GetTroopBoard(position);
                    if (target && target->AllowApplySpell(spell, hero))
                    {
                        res.defender = target;
                        targets._items.push_back(res);
                    }
                }

                // unique
                targets._items.resize(
                    distance(targets._items.begin(),
                             unique(targets._items.begin(), targets._items.end())));
            }
            break;

            // check all troops
        case Spell::DEATHRIPPLE:
        case Spell::DEATHWAVE:
        case Spell::ELEMENTALSTORM:
        case Spell::HOLYWORD:
        case Spell::HOLYSHOUT:
        case Spell::ARMAGEDDON:
        case Spell::MASSBLESS:
        case Spell::MASSCURE:
        case Spell::MASSCURSE:
        case Spell::MASSDISPEL:
        case Spell::MASSHASTE:
        case Spell::MASSSHIELD:
        case Spell::MASSSLOW:
            {
                for (auto& it : board._items)
                {
                    target = it.GetUnit();
                    if (target && target->AllowApplySpell(spell, hero))
                    {
                        res.defender = target;
                        targets._items.push_back(res);
                    }
                }

                // unique
                targets._items.resize(distance(targets._items.begin(),
                                               unique(targets._items.begin(), targets._items.end())));
            }
            break;

        default:
            break;
        }

    // remove resistent magic troop
    auto it = targets._items.begin();
    while (it != targets._items.end())
    {
        const uint32_t resist = (*it).defender->GetMagicResist(spell, hero ? hero->GetPower() : 0);

        if (0 < resist && 100 > resist && resist >= Rand::Get(1, 100))
        {
            if (interface) interface->RedrawActionResistSpell(*(*it).defender);

            // erase(it)
            if (it + 1 != targets._items.end()) swap(*it, targets._items.back());
            targets._items.pop_back();
        }
        else ++it;
    }

    return targets;
}

void Battle::Arena::ApplyActionTower(Command& cmd)
{
    uint32_t type = cmd.GetValue();
    uint32_t uid = cmd.GetValue();

    Tower* tower = GetTower(type);
    Unit* b2 = GetTroopUID(uid);

    if (!b2 || !b2->isValid() || !tower)
        return;
    TargetInfo target;
    target.defender = b2;
    target.damage = tower->GetDamage(*b2);

    if (interface) interface->RedrawActionTowerPart1(*tower, *b2);
    target.killed = b2->ApplyDamage(*tower, target.damage);
    if (interface) interface->RedrawActionTowerPart2(*tower, target);

    if (b2->Modes(SP_BLIND)) b2->ResetBlind();
}

void Battle::Arena::ApplyActionCatapult(Command& cmd)
{
    if (!catapult)
        return;
    uint32_t shots = cmd.GetValue();

    while (shots--)
    {
        uint32_t target = cmd.GetValue();
        uint32_t damage = cmd.GetValue();

        if (!target)
            continue;
        if (interface) interface->RedrawActionCatapult(target);
        SetCastleTargetValue(target, GetCastleTargetValue(target) - damage);
    }
}

void Battle::Arena::ApplyActionAutoBattle(Command& cmd)
{
    const int color = cmd.GetValue();

    if (current_color != color)
        return;
    if (auto_battle & color)
    {
        if (interface) interface->SetStatus(_("Set auto battle off"), true);
        auto_battle &= ~color;
    }
    else
    {
        if (interface) interface->SetStatus(_("Set auto battle on"), true);
        auto_battle |= color;
    }
}

void Battle::Arena::ApplyActionSpellSummonElemental(Command& cmd, const Spell& spell) const
{
    Unit* elem = CreateElemental(spell);
    if (interface) interface->RedrawActionSummonElementalSpell(*elem);
}

void Battle::Arena::ApplyActionSpellDefaults(Command& cmd, const Spell& spell)
{
    const HeroBase* current_commander = GetCurrentCommander();
    if (!current_commander) return;

    s32 dst = cmd.GetValue();

    TargetsInfo targets = GetTargetsForSpells(current_commander, spell, dst);
    if (interface)
        interface->RedrawActionSpellCastPart1(spell, dst, current_commander, current_commander->GetName(), targets);

    TargetsApplySpell(current_commander, spell, targets);
    if (interface) interface->RedrawActionSpellCastPart2(spell, targets);
}

void Battle::Arena::ApplyActionSpellTeleport(Command& cmd)
{
    const s32 src = cmd.GetValue();
    const s32 dst = cmd.GetValue();

    Unit* b = GetTroopBoard(src);
    const Spell spell(Spell::TELEPORT);

    if (!b)
        return;
    Position pos = Position::GetCorrect(*b, dst);
    if (b->isReflect() != pos.isReflect()) pos.Swap();

    if (interface) interface->RedrawActionTeleportSpell(*b, pos.GetHead()->GetIndex());

    b->SetPosition(pos);
    b->UpdateDirection();
}

void Battle::Arena::ApplyActionSpellEarthQuake(Command& cmd)
{
    const vector<int> targets = GetCastleTargets();

    if (interface) interface->RedrawActionEarthQuakeSpell(targets);

    // FIXME: Arena::ApplyActionSpellEarthQuake: check hero spell power

    // apply random damage
    if (0 != board._items[8].GetObject()) board._items[8].SetObject(Rand::Get(board._items[8].GetObject()));
    if (0 != board._items[29].GetObject()) board._items[29].SetObject(Rand::Get(board._items[29].GetObject()));
    if (0 != board._items[73].GetObject()) board._items[73].SetObject(Rand::Get(board._items[73].GetObject()));
    if (0 != board._items[96].GetObject()) board._items[96].SetObject(Rand::Get(board._items[96].GetObject()));

    if (towers[0] && towers[0]->isValid() && Rand::Get(1)) towers[0]->SetDestroy();
    if (towers[2] && towers[2]->isValid() && Rand::Get(1)) towers[2]->SetDestroy();
}

void Battle::Arena::ApplyActionSpellMirrorImage(Command& cmd)
{
    s32 who = cmd.GetValue();
    Unit* b = GetTroopBoard(who);

    if (!b)
        return;
    Indexes distances = Board::GetDistanceIndexes(b->GetHeadIndex(), 4);

    const std::function<bool(s32, s32)> SortingDistance = [=](s32 index1, s32 index2)
    {
        auto center = b->GetHeadIndex();
        return Board::GetDistance(center, index1) < Board::GetDistance(center, index2);
    };

    sort(distances.begin(), distances.end(), SortingDistance);

    const auto it = find_if(distances.begin(), distances.end(),
                            [&](s32 dist) { return Board::isValidMirrorImageIndex(dist, b); });

    for (auto& distance : distances)
    {
        const Cell* cell = Board::GetCell(distance);
        if (cell && cell->isPassable3(*b, true)) break;
    }

    if (it != distances.end())
    {
        const Position pos = Position::GetCorrect(*b, *it);
        const s32 dst = pos.GetHead()->GetIndex();
        if (interface) interface->RedrawActionMirrorImageSpell(*b, pos);
        Unit* mirror = CreateMirrorImage(*b, dst);
        if (mirror) mirror->SetPosition(pos);
    }
    else
    {
        if (interface) interface->SetStatus(_("spell failed!"), true);
    }
}
