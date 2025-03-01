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

#include "agg.h"
#include "text.h"
#include "settings.h"
#include "cursor.h"
#include "button.h"
#include "artifact.h"
#include "dialog.h"
#include "icn.h"

int Dialog::ArtifactInfo(const string& hdr, const string& msg, const Artifact& art, int buttons)
{
    Sprite borderInfo = AGG::GetICN(ICN::RESOURCE, 7);
    Sprite& border = borderInfo;
    const Sprite& artifact = AGG::GetICN(ICN::ARTIFACT, art.IndexSprite64());
    Surface image = border.GetSurface();
    border.Blit(image);
    artifact.Blit(5, 5, image);
    border.SetAlphaMod(210);
    string ext = msg;
    ext.append("\n");
    ext.append(" ");
    ext.append("\n");
    ext.append(art.GetDescription());

    return SpriteInfo(hdr, ext, image, buttons);
}

int Dialog::SpriteInfo(const string& header, const string& message, Surface& sprite, int buttons)
{
    Display& display = Display::Get();

    // cursor
    Cursor& cursor = Cursor::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    TextBox box1(header, Font::YELLOW_BIG, BOXAREA_WIDTH);
    TextBox box2(message, Font::BIG, BOXAREA_WIDTH);
    const int spacer = 10;

    FrameBox box(box1.h() + spacer + box2.h() + spacer + sprite.h(), buttons);
    Rect pos = box.GetArea();

    if (!header.empty()) box1.Blit(pos);
    pos.y += box1.h() + spacer;

    if (!message.empty()) box2.Blit(pos);
    pos.y += box2.h() + spacer;

    // blit sprite
    pos.x = box.GetArea().x + (pos.w - sprite.w()) / 2;
    sprite.Blit(pos.x, pos.y, display);

    LocalEvent& le = LocalEvent::Get();

    ButtonGroups btnGroups(box.GetArea(), buttons);
    btnGroups.Draw();

    cursor.Show();
    display.Flip();

    // message loop
    int result = ZERO;

    while (result == ZERO && le.HandleEvents())
    {
        if (!buttons && !le.MousePressRight()) break;
        result = btnGroups.QueueEventProcessing();
    }

    cursor.Hide();
    return result;
}
