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

#pragma once

#include <string>
#include "button.h"
#include "dialog.h"
#include "text.h"
#include "battle_board.h"
#include "sprite.h"
#include "spell.h"
#include "localevent.h"

namespace Battle
{
    class Arena;

    class Unit;

    class Units;

    class Tower;

    class StatusListBox;

    class Cell;

    class Actions;

    struct TargetInfo;
    struct TargetsInfo;
    struct Result;

    void DialogBattleSettings();

    bool DialogBattleSurrender(const HeroBase&, uint32_t);

    enum
    {
        OP_IDLE,
        OP_SRRW,
        OP_CAST
    };

    class OpponentSprite
    {
    public:
        OpponentSprite(const Rect&, const HeroBase*, bool);

        const Rect& GetArea() const;

        void Redraw() const;

        void ResetAnimFrame(int);

        void IncreaseAnimFrame(bool loop = false);

        bool isFinishFrame() const;

        bool isStartFrame() const;

        int GetColor() const;

        const HeroBase* GetHero() const;

    private:
        const HeroBase* base;
        int icn;
        int animframe;
        int animframe_start;
        int animframe_count;
        bool reflect;
        Rect pos;
    };

    class Status : public Rect
    {
    public:
        Status();

        void SetPosition(s32, s32);

        void SetLogs(StatusListBox* logs)
        {
            listlog = logs;
        };

        void SetMessage(const string&, bool = false);

        void Redraw() const;

        const string&
        GetMessage() const;

    private:
        Text bar1;
        Text bar2;
        Sprite back1;
        Sprite back2;
        string message;
        StatusListBox* listlog;
    };

    class ArmiesOrder : public Rect
    {
    public:
        ArmiesOrder();

        void Set(const Rect&, const Units*, int);

        void Redraw(const Unit*);

        void QueueEventProcessing(string&);

    private:
        typedef pair<const Unit *, Rect> UnitPos;

        void RedrawUnit(const Rect&, const Unit&, bool, bool) const;

        const Units* orders;
        int army_color2;
        Rect area;
        Surface sf_color[3];
        vector<UnitPos> rects;
    };

    class PopupDamageInfo : public Dialog::FrameBorder
    {
    public:
        PopupDamageInfo();

        void SetInfo(const Cell*, const Unit*, const Unit*);

        void Reset();

        void Redraw(int, int);

    private:
        const Cell* cell;
        const Unit* attacker;
        const Unit* defender;
        bool redraw;
    };

    class Interface
    {
    public:
        Interface(Arena&, s32);

        ~Interface();

        void Redraw();

        void HumanTurn(const Unit&, Actions&);

        static bool NetworkTurn(Result&);

        const Rect& GetArea() const;

        void SetStatus(const string&, bool = false);

        void SetArmiesOrder(const Units*);

        void FadeArena();

        void RedrawActionAttackPart1(Unit&, Unit&, const TargetsInfo&);

        void RedrawActionAttackPart2(Unit&, TargetsInfo&);

        void RedrawActionSpellCastPart1(const Spell&, s32, const HeroBase*, const string&, const TargetsInfo&);

        void RedrawActionSpellCastPart2(const Spell&, TargetsInfo&);

        void RedrawActionResistSpell(const Unit&);

        void RedrawActionMonsterSpellCastStatus(const Unit&, const TargetInfo&);

        void RedrawActionMove(Unit&, const Indexes&);

        void RedrawActionFly(Unit&, const Position&);

        void RedrawActionMorale(Unit&, bool);

        void RedrawActionLuck(Unit&);

        void RedrawActionTowerPart1(Tower&, Unit&);

        void RedrawActionTowerPart2(Tower&, TargetInfo&);

        void RedrawActionCatapult(int);

        void RedrawActionTeleportSpell(Unit&, s32);

        void RedrawActionEarthQuakeSpell(const vector<int>&);

        void RedrawActionSummonElementalSpell(const Unit&);

        void RedrawActionMirrorImageSpell(const Unit&, const Position&);

        void RedrawActionSkipStatus(const Unit&);

        void RedrawActionRemoveMirrorImage(const Unit&);

        void RedrawBridgeAnimation(bool down);

    private:
        void HumanBattleTurn(const Unit&, Actions&, string&);

        void HumanCastSpellTurn(const Unit&, Actions&, string&);

        void RedrawBorder() const;

        void RedrawCover();

        void RedrawCoverStatic(Surface&) const;

        static void RedrawLowObjects(s32, Surface&);

        static void RedrawHighObjects(s32);

        void RedrawCastle1(const Castle&, Surface&) const;

        void RedrawCastle2(const Castle&, s32) const;

        void RedrawCastle3(const Castle&) const;

        void RedrawKilled() const;

        void RedrawInterface();

        void RedrawOpponents() const;

        void RedrawOpponentsFlags() const;

        void RedrawArmies() const;

        void RedrawTroopSprite(const Unit&) const;

        static void RedrawTroopCount(const Unit&);

        void RedrawPocketControls() const;

        void RedrawActionWincesKills(TargetsInfo&);

        void RedrawActionArrowSpell(const Unit&);

        void RedrawActionColdRaySpell(Unit&);

        void RedrawActionDisruptingRaySpell(Unit&);

        void RedrawActionBloodLustSpell(Unit&);

        void RedrawActionColdRingSpell(s32, const TargetsInfo&);

        void RedrawActionElementalStormSpell(const TargetsInfo&);

        void RedrawActionArmageddonSpell(const TargetsInfo&);

        void RedrawActionResurrectSpell(Unit&, const Spell&);

        void RedrawActionLightningBoltSpell(Unit&);

        void RedrawActionChainLightningSpell(const TargetsInfo&);

        void RedrawTroopFrameAnimation(Unit&);

        void RedrawTroopWithFrameAnimation(Unit&, int, int, bool);

        void RedrawTargetsWithFrameAnimation(s32, const TargetsInfo&, int, int);

        void RedrawTargetsWithFrameAnimation(const TargetsInfo&, int, int, bool);

        bool IdleTroopsAnimation() const;

        void CheckGlobalEvents(LocalEvent&);

        void ProcessingHeroDialogResult(int, Actions&);

        void EventAutoSwitch(const Unit&, Actions&);

        void EventShowOptions();

        void ButtonAutoAction(const Unit&, Actions&);

        void ButtonSettingsAction();

        void ButtonSkipAction(Actions&);

        void ButtonWaitAction(Actions&);

        void MouseLeftClickBoardAction(uint32_t, const Cell&, Actions&);

        void MousePressRightBoardAction(uint32_t, const Cell&, Actions&);

        int GetBattleCursor(string&) const;

        int GetBattleSpellCursor(string&) const;

        int GetAllowSwordDirection(uint32_t) const;

        void CreateDamageInfoPopup(s32, s32, const Unit&, const Unit&);

        Arena& arena;
        Dialog::FrameBorder border;
        Surface sf_hexagon;
        Surface sf_shadow;
        Surface sf_cursor;
        Surface sf_cover;

        int icn_cbkg;
        int icn_frng;

        Button btn_auto;
        Button btn_settings;
        Button btn_skip;
        Button btn_wait;
        Status status;

        up<OpponentSprite> opponent1;
        up<OpponentSprite> opponent2;

        Rect rectBoard;
        Spell humanturn_spell;
        bool humanturn_exit;
        bool humanturn_redraw;
        uint32_t animation_flags_frame;
        int catapult_frame;

        const Unit* b_current;
        const Unit* b_move;
        const Unit* b_fly;
        const Sprite* b_current_sprite;
        uint32_t b_current_alpha;
        Point p_move;
        Point p_fly;

        s32 index_pos;
        s32 teleport_src;
        Rect pocket_book;
        Rect main_tower;

        up<StatusListBox> listlog;
        uint32_t turn;

        PopupDamageInfo popup;
        ArmiesOrder armies_order;
    };
}
