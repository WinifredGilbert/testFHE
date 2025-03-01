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

#include <string>
#include "agg.h"
#include "world.h"
#include "cursor.h"
#include "settings.h"
#include "dialog.h"
#include "text.h"
#include "race.h"
#include "game.h"
#include "statusbar.h"
#include "buildinginfo.h"
#include "icn.h"

int Castle::DialogBuyHero(const Heroes* hero) const
{
    if (!hero) return Dialog::CANCEL;

    const int system = Settings::Get().ExtGameEvilInterface() ? ICN::SYSTEME : ICN::SYSTEM;

    Display& display = Display::Get();
    Cursor& cursor = Cursor::Get();
    cursor.Hide();

    const int spacer = 10;
    const Sprite& portrait_frame = AGG::GetICN(ICN::SURRENDR, 4);

    Text text(_("Recruit Hero"), Font::BIG);

    uint32_t count = hero->GetCountArtifacts();
    if (hero->HasArtifact(Artifact::MAGIC_BOOK)) count--;

    string str = _("%{name} is a level %{value} %{race}");

    // FIXME: It is necessary to consider locale features for numerals (with getext).
    if (count)
    {
        str += " ";
        str += count > 1 ? _(" with %{count} artifacts") : _(" with one artifact");
    }

    StringReplace(str, "%{name}", hero->GetName());
    StringReplace(str, "%{value}", hero->GetLevel());
    StringReplace(str, "%{race}", Race::String(hero->GetRace()));
    StringReplace(str, "%{count}", count);

    TextBox box2(str, Font::BIG, BOXAREA_WIDTH);

    Resource::BoxSprite rbs(PaymentConditions::RecruitHero(hero->GetLevel()), BOXAREA_WIDTH);

    Dialog::FrameBox box(text.h() + spacer + portrait_frame.h() + spacer + box2.h() + spacer + rbs.GetArea().h, true);
    const Rect& box_rt = box.GetArea();
    LocalEvent& le = LocalEvent::Get();
    Point dst_pt;

    dst_pt.x = box_rt.x + (box_rt.w - text.w()) / 2;
    dst_pt.y = box_rt.y;
    text.Blit(dst_pt);

    //portrait and frame
    dst_pt.x = box_rt.x + (box_rt.w - portrait_frame.w()) / 2;
    dst_pt.y = dst_pt.y + text.h() + spacer;
    portrait_frame.Blit(dst_pt);

    dst_pt.x = dst_pt.x + 5;
    dst_pt.y = dst_pt.y + 5;
    hero->PortraitRedraw(dst_pt.x, dst_pt.y, PORT_BIG, display);

    dst_pt.x = box_rt.x;
    dst_pt.y = dst_pt.y + portrait_frame.h() + spacer;
    box2.Blit(dst_pt);

    rbs.SetPos(dst_pt.x, dst_pt.y + box2.h() + spacer);
    rbs.Redraw();

    dst_pt.x = box_rt.x;
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 1).h();
    Button button1(dst_pt.x + 50, dst_pt.y, system, 1, 2);

    if (!AllowBuyHero(*hero))
    {
        button1.Press();
        button1.SetDisable(true);
    }

    dst_pt.x = box_rt.x + box_rt.w - AGG::GetICN(system, 3).w();
    dst_pt.y = box_rt.y + box_rt.h - AGG::GetICN(system, 3).h();
    Button button2(dst_pt.x, dst_pt.y, system, 3, 4);

    button1.Draw();
    button2.Draw();

    cursor.Show();
    display.Flip();

    // message loop
    while (le.HandleEvents())
    {
        le.MousePressLeft(button1) ? button1.PressDraw() : button1.ReleaseDraw();
        le.MousePressLeft(button2) ? button2.PressDraw() : button2.ReleaseDraw();

        if (button1.isEnable() &&
            (le.MouseClickLeft(button1) ||
                HotKeyPressEvent(Game::EVENT_DEFAULT_READY)))
            return Dialog::OK;

        if (le.MouseClickLeft(button2) ||
            HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT))
            break;
    }

    return Dialog::CANCEL;
}

int Castle::DialogBuyCastle(bool buttons) const
{
    castle::BuildingInfo info(*this, BUILD_CASTLE);
    return info.DialogBuyBuilding(buttons) ? Dialog::OK : Dialog::CANCEL;
}

uint32_t Castle::OpenTown()
{
    Display& display = Display::Get();
    Cursor& cursor = Cursor::Get();
    cursor.Hide();

    Dialog::FrameBorder background(Size(640, 480));

    const Point& cur_pt = background.GetArea();
    Point dst_pt(cur_pt);

    AGG::GetICN(ICN::CASLWIND, 0).Blit(dst_pt);

    // hide captain options
    if (!(building & BUILD_CAPTAIN))
    {
        dst_pt.x = 530;
        dst_pt.y = 163;
        const Rect rect(dst_pt, 110, 84);
        dst_pt.x += cur_pt.x;
        dst_pt.y += cur_pt.y;

        AGG::GetICN(ICN::STONEBAK, 0).Blit(rect, dst_pt);
    }

    // draw castle sprite
    dst_pt.x = cur_pt.x + 460;
    dst_pt.y = cur_pt.y + 5;
    DrawImageCastle(dst_pt, display);

    // castle name
    Text text(GetName(), Font::SMALL);
    text.Blit(cur_pt.x + 536 - text.w() / 2, cur_pt.y + 1);

    //
    castle::BuildingInfo dwelling1(*this, DWELLING_MONSTER1);
    dwelling1.SetPos(cur_pt.x + 5, cur_pt.y + 2);
    dwelling1.Redraw();

    castle::BuildingInfo dwelling2(*this, DWELLING_MONSTER2);
    dwelling2.SetPos(cur_pt.x + 149, cur_pt.y + 2);
    dwelling2.Redraw();

    castle::BuildingInfo dwelling3(*this, DWELLING_MONSTER3);
    dwelling3.SetPos(cur_pt.x + 293, cur_pt.y + 2);
    dwelling3.Redraw();

    castle::BuildingInfo dwelling4(*this, DWELLING_MONSTER4);
    dwelling4.SetPos(cur_pt.x + 5, cur_pt.y + 77);
    dwelling4.Redraw();

    castle::BuildingInfo dwelling5(*this, DWELLING_MONSTER5);
    dwelling5.SetPos(cur_pt.x + 149, cur_pt.y + 77);
    dwelling5.Redraw();

    castle::BuildingInfo dwelling6(*this, DWELLING_MONSTER6);
    dwelling6.SetPos(cur_pt.x + 293, cur_pt.y + 77);
    dwelling6.Redraw();

    // mage guild
    building_t level = BUILD_NOTHING;
    switch (GetLevelMageGuild())
    {
    case 0:
        level = BUILD_MAGEGUILD1;
        break;
    case 1:
        level = BUILD_MAGEGUILD2;
        break;
    case 2:
        level = BUILD_MAGEGUILD3;
        break;
    case 3:
        level = BUILD_MAGEGUILD4;
        break;
    default:
        level = BUILD_MAGEGUILD5;
        break;
    }
    castle::BuildingInfo buildingMageGuild(*this, level);
    buildingMageGuild.SetPos(cur_pt.x + 5, cur_pt.y + 157);
    buildingMageGuild.Redraw();

    // tavern
    castle::BuildingInfo buildingTavern(*this, BUILD_TAVERN);
    buildingTavern.SetPos(cur_pt.x + 149, cur_pt.y + 157);
    buildingTavern.Redraw();

    // thieves guild
    castle::BuildingInfo buildingThievesGuild(*this, BUILD_THIEVESGUILD);
    buildingThievesGuild.SetPos(cur_pt.x + 293, cur_pt.y + 157);
    buildingThievesGuild.Redraw();

    // shipyard
    castle::BuildingInfo buildingShipyard(*this, BUILD_SHIPYARD);
    buildingShipyard.SetPos(cur_pt.x + 5, cur_pt.y + 232);
    buildingShipyard.Redraw();

    // statue
    castle::BuildingInfo buildingStatue(*this, BUILD_STATUE);
    buildingStatue.SetPos(cur_pt.x + 149, cur_pt.y + 232);
    buildingStatue.Redraw();

    // marketplace
    castle::BuildingInfo buildingMarketplace(*this, BUILD_MARKETPLACE);
    buildingMarketplace.SetPos(cur_pt.x + 293, cur_pt.y + 232);
    buildingMarketplace.Redraw();

    // well
    castle::BuildingInfo buildingWell(*this, BUILD_WELL);
    buildingWell.SetPos(cur_pt.x + 5, cur_pt.y + 307);
    buildingWell.Redraw();

    // wel2
    castle::BuildingInfo buildingWel2(*this, BUILD_WEL2);
    buildingWel2.SetPos(cur_pt.x + 149, cur_pt.y + 307);
    buildingWel2.Redraw();

    // spec
    castle::BuildingInfo buildingSpec(*this, BUILD_SPEC);
    buildingSpec.SetPos(cur_pt.x + 293, cur_pt.y + 307);
    buildingSpec.Redraw();

    // left turret
    castle::BuildingInfo buildingLTurret(*this, BUILD_LEFTTURRET);
    buildingLTurret.SetPos(cur_pt.x + 5, cur_pt.y + 387);
    buildingLTurret.Redraw();

    // right turret
    castle::BuildingInfo buildingRTurret(*this, BUILD_RIGHTTURRET);
    buildingRTurret.SetPos(cur_pt.x + 149, cur_pt.y + 387);
    buildingRTurret.Redraw();

    // moat
    castle::BuildingInfo buildingMoat(*this, BUILD_MOAT);
    buildingMoat.SetPos(cur_pt.x + 293, cur_pt.y + 387);
    buildingMoat.Redraw();

    // captain
    castle::BuildingInfo buildingCaptain(*this, BUILD_CAPTAIN);
    buildingCaptain.SetPos(cur_pt.x + 444, cur_pt.y + 165);
    buildingCaptain.Redraw();

    // combat format
    const Sprite& spriteSpreadArmyFormat = AGG::GetICN(ICN::HSICONS, 9);
    const Sprite& spriteGroupedArmyFormat = AGG::GetICN(ICN::HSICONS, 10);
    const Rect rectSpreadArmyFormat(cur_pt.x + 550, cur_pt.y + 220, spriteSpreadArmyFormat.w(),
                                    spriteSpreadArmyFormat.h());
    const Rect rectGroupedArmyFormat(cur_pt.x + 585, cur_pt.y + 220, spriteGroupedArmyFormat.w(),
                                     spriteGroupedArmyFormat.h());
    const string descriptionSpreadArmyFormat(
        _(
            "'Spread' combat formation spreads your armies from the top to the bottom of the battlefield, with at least one empty space between each army."
        ));
    const string descriptionGroupedArmyFormat(
        _("'Grouped' combat formation bunches your army toget her in the center of your side of the battlefield."));
    const Point pointSpreadArmyFormat(rectSpreadArmyFormat.x - 1, rectSpreadArmyFormat.y - 1);
    const Point pointGroupedArmyFormat(rectGroupedArmyFormat.x - 1, rectGroupedArmyFormat.y - 1);

    SpriteMove cursorFormat(AGG::GetICN(ICN::HSICONS, 11));

    if (isBuild(BUILD_CAPTAIN))
    {
        text.Set(_("Attack Skill") + string(" "), Font::SMALL);
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y = cur_pt.y + 168;
        text.Blit(dst_pt);

        text.Set(Int2Str(captain.GetAttack()));
        dst_pt.x += 90;
        text.Blit(dst_pt);

        text.Set(_("Defense Skill") + string(" "));
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit(dst_pt);

        text.Set(Int2Str(captain.GetDefense()));
        dst_pt.x += 90;
        text.Blit(dst_pt);

        text.Set(_("Spell Power") + string(" "));
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit(dst_pt);

        text.Set(Int2Str(captain.GetPower()));
        dst_pt.x += 90;
        text.Blit(dst_pt);

        text.Set(_("Knowledge") + string(" "));
        dst_pt.x = cur_pt.x + 535;
        dst_pt.y += 12;
        text.Blit(dst_pt);

        text.Set(Int2Str(captain.GetKnowledge()));
        dst_pt.x += 90;
        text.Blit(dst_pt);

        spriteSpreadArmyFormat.Blit(rectSpreadArmyFormat.x, rectSpreadArmyFormat.y);
        spriteGroupedArmyFormat.Blit(rectGroupedArmyFormat.x, rectGroupedArmyFormat.y);

        cursorFormat.Move(army.isSpreadFormat() ? pointSpreadArmyFormat : pointGroupedArmyFormat);
    }

    Kingdom& kingdom = GetKingdom();

    Heroes* hero1 = kingdom.GetRecruits().GetHero1();
    Heroes* hero2 = kingdom.GetLastLostHero() && kingdom.GetLastLostHero() != hero1
                        ? kingdom.GetLastLostHero()
                        : kingdom.GetRecruits().GetHero2();

    string not_allow1_msg, not_allow2_msg;
    const bool allow_buy_hero1 = hero1 ? AllowBuyHero(*hero1, &not_allow1_msg) : false;
    const bool allow_buy_hero2 = hero2 ? AllowBuyHero(*hero2, &not_allow2_msg) : false;

    // first hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 260;
    const Rect rectHero1(dst_pt, 102, 93);
    if (hero1)
    {
        hero1->PortraitRedraw(dst_pt.x, dst_pt.y, PORT_BIG, display);
    }
    else
        display.FillRect(rectHero1, ColorBlack);
    // indicator
    if (!allow_buy_hero1)
    {
        dst_pt.x += 83;
        dst_pt.y += 75;
        AGG::GetICN(ICN::TOWNWIND, 12).Blit(dst_pt);
    }

    // second hero
    dst_pt.x = cur_pt.x + 443;
    dst_pt.y = cur_pt.y + 362;
    const Rect rectHero2(dst_pt, 102, 94);
    if (hero2)
    {
        hero2->PortraitRedraw(dst_pt.x, dst_pt.y, PORT_BIG, display);
    }
    else
        display.FillRect(rectHero2, ColorBlack);
    // indicator
    if (!allow_buy_hero2)
    {
        dst_pt.x += 83;
        dst_pt.y += 75;
        AGG::GetICN(ICN::TOWNWIND, 12).Blit(dst_pt);
    }

    // bottom bar
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y + 461;
    const Sprite& bar = AGG::GetICN(ICN::CASLBAR, 0);
    bar.Blit(dst_pt);

    StatusBar statusBar;
    statusBar.SetCenter(dst_pt.x + bar.w() / 2, dst_pt.y + 11);

    // redraw resource panel
    RedrawResourcePanel(cur_pt);

    // button exit
    dst_pt.x = cur_pt.x + 554;
    dst_pt.y = cur_pt.y + 428;
    Button buttonExit(dst_pt.x, dst_pt.y, ICN::SWAPBTN, 0, 1);

    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent& le = LocalEvent::Get();

    // message loop
    while (le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();

        if (le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;

        // click left
        if (le.MouseCursor(dwelling1.GetArea()) && dwelling1.QueueEventProcessing()) return dwelling1();
        if (le.MouseCursor(dwelling2.GetArea()) && dwelling2.QueueEventProcessing()) return dwelling2();
        if (le.MouseCursor(dwelling3.GetArea()) && dwelling3.QueueEventProcessing()) return dwelling3();
        if (le.MouseCursor(dwelling4.GetArea()) && dwelling4.QueueEventProcessing()) return dwelling4();
        if (le.MouseCursor(dwelling5.GetArea()) && dwelling5.QueueEventProcessing()) return dwelling5();
        if (le.MouseCursor(dwelling6.GetArea()) && dwelling6.QueueEventProcessing()) return dwelling6();
        if (le.MouseCursor(buildingMageGuild.GetArea()) && buildingMageGuild.QueueEventProcessing())
            return buildingMageGuild();
        if (le.MouseCursor(buildingTavern.GetArea()) && buildingTavern.QueueEventProcessing())
            return Race::NECR == race ? BUILD_SHRINE : BUILD_TAVERN;
        if (le.MouseCursor(buildingThievesGuild.GetArea()) && buildingThievesGuild.QueueEventProcessing())
            return BUILD_THIEVESGUILD;
        if (le.MouseCursor(buildingShipyard.GetArea()) && buildingShipyard.QueueEventProcessing())
            return BUILD_SHIPYARD;
        if (le.MouseCursor(buildingStatue.GetArea()) && buildingStatue.QueueEventProcessing()) return BUILD_STATUE;
        if (le.MouseCursor(buildingMarketplace.GetArea()) && buildingMarketplace.QueueEventProcessing())
            return BUILD_MARKETPLACE;
        if (le.MouseCursor(buildingWell.GetArea()) && buildingWell.QueueEventProcessing()) return BUILD_WELL;
        if (le.MouseCursor(buildingWel2.GetArea()) && buildingWel2.QueueEventProcessing()) return BUILD_WEL2;
        if (le.MouseCursor(buildingSpec.GetArea()) && buildingSpec.QueueEventProcessing()) return BUILD_SPEC;
        if (le.MouseCursor(buildingLTurret.GetArea()) && buildingLTurret.QueueEventProcessing())
            return BUILD_LEFTTURRET;
        if (le.MouseCursor(buildingRTurret.GetArea()) && buildingRTurret.QueueEventProcessing())
            return BUILD_RIGHTTURRET;
        if (le.MouseCursor(buildingMoat.GetArea()) && buildingMoat.QueueEventProcessing()) return BUILD_MOAT;
        if (le.MouseCursor(buildingCaptain.GetArea()) && buildingCaptain.QueueEventProcessing())
            return BUILD_CAPTAIN;
        if (hero1 && le.MouseClickLeft(rectHero1) &&
            Dialog::OK == DialogBuyHero(hero1))
        {
            RecruitHero(hero1);

            return BUILD_NOTHING;
        }
        if (hero2 && le.MouseClickLeft(rectHero2) &&
            Dialog::OK == DialogBuyHero(hero2))
        {
            RecruitHero(hero2);

            return BUILD_NOTHING;
        }
        if (isBuild(BUILD_CAPTAIN))
        {
            if (le.MouseClickLeft(rectSpreadArmyFormat) && !army.isSpreadFormat())
            {
                cursor.Hide();
                cursorFormat.Move(pointSpreadArmyFormat);
                cursor.Show();
                display.Flip();
                army.SetSpreadFormat(true);
            }
            else if (le.MouseClickLeft(rectGroupedArmyFormat) && army.isSpreadFormat())
            {
                cursor.Hide();
                cursorFormat.Move(pointGroupedArmyFormat);
                cursor.Show();
                display.Flip();
                army.SetSpreadFormat(false);
            }
        }

        // right
        if (le.MousePressRight(rectSpreadArmyFormat))
            Dialog::Message(_("Spread Formation"), descriptionSpreadArmyFormat, Font::BIG);
        else if (le.MousePressRight(rectGroupedArmyFormat))
            Dialog::Message(_("Grouped Formation"), descriptionGroupedArmyFormat, Font::BIG);
        else if (hero1 && le.MousePressRight(rectHero1))
        {
            hero1->OpenDialog(true);
            cursor.Show();
            display.Flip();
        }
        else if (hero2 && le.MousePressRight(rectHero2))
        {
            hero2->OpenDialog(true);
            cursor.Show();
            display.Flip();
        }

        // status info
        if (le.MouseCursor(dwelling1.GetArea())) dwelling1.SetStatusMessage(statusBar);
        else if (le.MouseCursor(dwelling2.GetArea())) dwelling2.SetStatusMessage(statusBar);
        else if (le.MouseCursor(dwelling3.GetArea())) dwelling3.SetStatusMessage(statusBar);
        else if (le.MouseCursor(dwelling4.GetArea())) dwelling4.SetStatusMessage(statusBar);
        else if (le.MouseCursor(dwelling5.GetArea())) dwelling5.SetStatusMessage(statusBar);
        else if (le.MouseCursor(dwelling6.GetArea())) dwelling6.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingMageGuild.GetArea())) buildingMageGuild.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingTavern.GetArea())) buildingTavern.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingThievesGuild.GetArea())) buildingThievesGuild.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingShipyard.GetArea())) buildingShipyard.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingStatue.GetArea())) buildingStatue.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingMarketplace.GetArea())) buildingMarketplace.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingWell.GetArea())) buildingWell.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingWel2.GetArea())) buildingWel2.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingSpec.GetArea())) buildingSpec.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingLTurret.GetArea())) buildingLTurret.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingRTurret.GetArea())) buildingRTurret.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingMoat.GetArea())) buildingMoat.SetStatusMessage(statusBar);
        else if (le.MouseCursor(buildingCaptain.GetArea())) buildingCaptain.SetStatusMessage(statusBar);
        else if (hero1 && le.MouseCursor(rectHero1))
        {
            if (!allow_buy_hero1)
                statusBar.ShowMessage(not_allow1_msg);
            else
            {
                string str = _("Recruit %{name} the %{race}");
                StringReplace(str, "%{name}", hero1->GetName());
                StringReplace(str, "%{race}", Race::String(hero1->GetRace()));
                statusBar.ShowMessage(str);
            }
        }
        else if (hero2 && le.MouseCursor(rectHero2))
        {
            if (!allow_buy_hero2)
                statusBar.ShowMessage(not_allow2_msg);
            else
            {
                string str = _("Recruit %{name} the %{race}");
                StringReplace(str, "%{name}", hero2->GetName());
                StringReplace(str, "%{race}", Race::String(hero2->GetRace()));
                statusBar.ShowMessage(str);
            }
        }
        else if (le.MouseCursor(rectSpreadArmyFormat))
            statusBar.ShowMessage(_("Set garrison combat formation to 'Spread'"));
        else if (le.MouseCursor(rectGroupedArmyFormat))
            statusBar.ShowMessage(_("Set garrison combat formation to 'Grouped'"));
        else
            // clear all
            statusBar.ShowMessage(_("Castle Options"));
    }

    return BUILD_NOTHING;
}
