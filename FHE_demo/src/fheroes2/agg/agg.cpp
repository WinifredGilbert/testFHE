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

#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "agg.h"
#include "icn.h"
#include "error.h"

#include "settings.h"
#include "text.h"
#include "artifact.h"
#include "game.h"
#include "ByteVectorReader.h"
#include "BinaryFileReader.h"
#include "ByteVectorWriter.h"

#include "m82.h"
#include "system.h"
#include "tools.h"
#include "palette_h2.h"
#include "til.h"
#include "xmi.h"
#include "mus.h"
#include "font.h"

#include "audio_mixer.h"
#include "audio_music.h"

#include <sstream>
#include <iostream>
#include "agg_private.h"

namespace
{

    std::vector<SDL_Color> pal_colors;
    std::vector<uint32_t> pal_colors_u32;
}



namespace AGG
{
    File heroes2_agg;
    File heroes2x_agg;

    vector<icn_cache_t> icn_cache;
    vector<til_cache_t> til_cache;

    unordered_map<int, vector<u8>> wav_cache;
    unordered_map<int, vector<u8>> mid_cache;
    vector<loop_sound_t> loop_sounds;
    unordered_map<uint32_t, fnt_cache_t> fnt_cache;

    bool memlimit_usage = true;


    up<FontTTF[]> fonts; /* small, medium */

    void LoadTTFChar(uint32_t);

    Surface GetFNT(uint32_t, uint32_t);

    const vector<u8>& GetWAV(int m82);

    const vector<u8>& GetMID(int xmi);

    void LoadWAV(int m82, vector<u8>&);

    void LoadMID(int xmi, vector<u8>&);

    bool LoadExtICN(int icn, uint32_t, bool);

    bool LoadAltICN(int icn, uint32_t, bool);

    bool LoadOrgICN(Sprite&, int icn, uint32_t, bool);

    bool LoadOrgICN(int icn, uint32_t, bool);

    void LoadICN(int icn, uint32_t, bool reflect = false);

    void SaveICN(int icn);

    bool LoadAltTIL(int til, uint32_t max);

    bool LoadOrgTIL(int til, uint32_t max);

    void LoadTIL(int til);

    void SaveTIL(int til);

    void LoadFNT();

    void ShowError();

    bool CheckMemoryLimit();

    uint32_t ClearFreeObjects();

    bool ReadDataDir();

    vector<u8> ReadICNChunk(int icn, uint32_t);

    vector<u8> ReadChunk(const string&);
}

sp<Sprite> AGG::ICNSprite::CreateSprite(bool reflect, bool shadow) const
{
    Surface res(first.GetSize(), true);
    first.Blit(res);

    if (shadow && second.isValid())
        second.Blit(res);

    return std::make_shared<Sprite>(reflect ? res.RenderReflect(2) : res, offset.x, offset.y);
}

bool AGG::ICNSprite::isValid() const
{
    return first.isValid();
}

uint32_t AGG::ClearFreeObjects()
{
    uint32_t total = 0;

    // wav cache
    for (auto& it : wav_cache)
        total += it.second.size();

    total = 0;

    // mus cache
    for (auto& it : mid_cache)
        total += it.second.size();

    total = 0;

    // fnt cache
    for (auto& it : fnt_cache)
    {
        total += it.second.sfs[0].GetMemoryUsage();
        total += it.second.sfs[1].GetMemoryUsage();
        total += it.second.sfs[2].GetMemoryUsage();
        total += it.second.sfs[3].GetMemoryUsage();
    }

    total = 0;

    // til cache
    for (auto& tils : til_cache)
    {
        for (uint32_t jj = 0; jj < tils.count; ++jj)
            if (tils.sprites)
                total += tils.sprites[jj].GetMemoryUsage();
    }
    total = 0;

    // icn cache
    uint32_t used = 0;

    for (auto& icns : icn_cache)
    {
        for (uint32_t jj = 0; jj < icns.count; ++jj)
        {
            if (icns.sprites)
            {
                Sprite& sprite1 = icns.sprites[jj];

                if (!sprite1.isRefCopy())
                {
                    total += sprite1.GetMemoryUsage();
                    sprite1.Reset();
                }
                else
                    used += sprite1.GetMemoryUsage();
            }

            if (icns.reflect)
            {
                Sprite& sprite2 = icns.reflect[jj];

                if (!sprite2.isRefCopy())
                {
                    total += sprite2.GetMemoryUsage();
                    sprite2.Reset();
                }
                else
                    used += sprite2.GetMemoryUsage();
            }
        }
    }

    return total;
}

bool AGG::CheckMemoryLimit()
{
    Settings& conf = Settings::Get();

    // memory limit trigger
    if ( 0 < conf.MemoryLimit() && memlimit_usage)
    {
        uint32_t usage = System::GetMemoryUsage();

        if (0 < usage && conf.MemoryLimit() < usage)
        {
            H2VERBOSE("settings: " << conf.MemoryLimit() << ", game usage: " << usage);
            const uint32_t freemem = ClearFreeObjects();
            H2VERBOSE("free " << freemem);

            usage = System::GetMemoryUsage();

            if (conf.MemoryLimit() < usage + 300 * 1024)
            {
                H2VERBOSE("settings: " << conf.MemoryLimit() << ", too small");
                // increase + 300Kb
                conf.SetMemoryLimit(usage + 300 * 1024);
                H2VERBOSE("settings: " << "increase limit on 300kb, current value: " << conf.MemoryLimit());
            }

            return true;
        }
    }

    return false;
}

/* read data directory */
bool AGG::ReadDataDir()
{
    Settings& conf = Settings::Get();

    ListFiles aggs = Settings::GetListFiles("data", ".agg");
    const string& other_data = conf.GetDataParams();

    if (!other_data.empty() && other_data != "data")
        aggs.Append(Settings::GetListFiles(other_data, ".agg"));

    // not found agg, exit
    if (aggs.empty()) return false;

    // attach agg files
    for (ListFiles::const_iterator
         it = aggs.begin(); it != aggs.end(); ++it)
    {
        string lower = StringLower(*it);
        if (string::npos != lower.find("heroes2.agg") && !heroes2_agg.isGood()) heroes2_agg.Open(*it);
        if (string::npos != lower.find("heroes2x.agg") && !heroes2x_agg.isGood()) heroes2x_agg.Open(*it);
    }

    if (heroes2x_agg.isGood()) conf.SetPriceLoyaltyVersion();

    return heroes2_agg.isGood();
}

vector<u8> AGG::ReadChunk(const string& key)
{
    if (heroes2x_agg.isGood())
    {
        const vector<u8>& buf = heroes2x_agg.Read(key);
        if (!buf.empty()) return buf;
    }

    return heroes2_agg.Read(key);
}

/* load manual ICN object */
bool AGG::LoadExtICN(int icn, uint32_t index, bool reflect)
{
    // for animation sprite need update count for ICN::AnimationFrame
    uint32_t count = 0;
    const Settings& conf = Settings::Get();

    switch (icn)
    {
    case ICN::BOAT12:
        count = 1;
        break;
    case ICN::BATTLESKIP:
    case ICN::BATTLEWAIT:
    case ICN::BATTLEAUTO:
    case ICN::BATTLESETS:
    case ICN::BUYMAX:
    case ICN::BTNBATTLEONLY:
    case ICN::BTNGIFT:
    case ICN::BTNMIN:
    case ICN::BTNCONFIG:
        count = 2;
        break;
    case ICN::FOUNTAIN:
        count = 2;
        break;
    case ICN::TREASURE:
        count = 2;
        break;
    case ICN::CSLMARKER:
        count = 3;
        break;
    case ICN::TELEPORT1:
    case ICN::TELEPORT2:
    case ICN::TELEPORT3:
        count = 8;
        break;
    case ICN::YELLOW_FONT:
    case ICN::YELLOW_SMALFONT:
        count = 96;
        break;
    case ICN::ROUTERED:
        count = 145;
        break;

    default:
        break;
    }

    // not modify sprite
    if (0 == count) return false;

    icn_cache_t& v = icn_cache[icn];

    if (nullptr == v.sprites)
    {
        v.sprites = new Sprite[count];
        v.reflect = new Sprite[count];
        v.count = count;
    }

    // simple modify
    if (index < count)
    {
        Sprite& sprite = reflect ? v.reflect[index] : v.sprites[index];

        memlimit_usage = false;

        switch (icn)
        {
        case ICN::BTNBATTLEONLY:
            LoadOrgICN(sprite, ICN::BTNNEWGM, 2 + index, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 55, 14), 15, 13, sprite);
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 55, 14), 70, 13, sprite);
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 55, 14), 42, 28, sprite);
            // ba
            GetICN(ICN::BTNCMPGN, index).Blit(Rect(41, 28, 28, 14), 30, 13, sprite);
            // tt
            GetICN(ICN::BTNNEWGM, index).Blit(Rect(25, 13, 13, 14), 57, 13, sprite);
            GetICN(ICN::BTNNEWGM, index).Blit(Rect(25, 13, 13, 14), 70, 13, sprite);
            // le
            GetICN(ICN::BTNNEWGM, 6 + index).Blit(Rect(97, 21, 13, 14), 83, 13, sprite);
            GetICN(ICN::BTNNEWGM, 6 + index).Blit(Rect(86, 21, 13, 14), 96, 13, sprite);
            // on
            GetICN(ICN::BTNDCCFG, 4 + index).Blit(Rect(44, 21, 31, 14), 40, 28, sprite);
            // ly
            GetICN(ICN::BTNHOTST, index).Blit(Rect(47, 21, 13, 13), 71, 28, sprite);
            GetICN(ICN::BTNHOTST, index).Blit(Rect(72, 21, 13, 13), 84, 28, sprite);
            break;

        case ICN::BTNCONFIG:
            LoadOrgICN(sprite, ICN::SYSTEM, 11 + index, false);
            // config
            GetICN(ICN::BTNDCCFG, 4 + index).Blit(Rect(30, 20, 80, 16), 8, 5, sprite);
            break;

        case ICN::BTNGIFT:
            LoadOrgICN(sprite,
                       Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST,
                       17 + index, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 72, 15), 6, 4, sprite);
            // G
            GetICN(ICN::BTNDCCFG, 4 + index).Blit(Rect(94, 20, 15, 15), 20, 4, sprite);
            // I
            GetICN(ICN::BTNDCCFG, 4 + index).Blit(Rect(86, 20, 9, 15), 36, 4, sprite);
            // F
            GetICN(ICN::BTNDCCFG, 4 + index).Blit(Rect(74, 20, 13, 15), 46, 4, sprite);
            // T
            GetICN(ICN::BTNNEWGM, index).Blit(Rect(25, 13, 13, 14), 60, 5, sprite);
            break;

        case ICN::BTNMIN:
            // max
            LoadOrgICN(sprite, ICN::RECRUIT, index + 4, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 33, 15), 30, 4, sprite);
            // add: IN
            GetICN(ICN::APANEL, 4 + index).Blit(Rect(23, 20, 25, 15), 30, 4, sprite);
            break;

        case ICN::BUYMAX:
            LoadOrgICN(sprite, ICN::WELLXTRA, index, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(10, 6, 52, 14), 6, 2, sprite);
            // max
            GetICN(ICN::RECRUIT, 4 + index).Blit(Rect(12, 6, 50, 12), 7, 3, sprite);
            break;

        case ICN::BATTLESKIP:
            {
                LoadOrgICN(sprite, ICN::TEXTBAR, 4 + index, false);
                // clean
                GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(3, 8, 43, 14), 3, 1, sprite);
                // skip
                GetICN(ICN::TEXTBAR, index).Blit(Rect(3, 8, 43, 14), 3, 0, sprite);
            }
            break;

        case ICN::BATTLEAUTO:
            LoadOrgICN(sprite, ICN::TEXTBAR, 0 + index, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(4, 8, 43, 13), 3, 10, sprite);
            //
            GetICN(ICN::TEXTBAR, 4 + index).Blit(Rect(5, 2, 40, 12), 4, 11, sprite);
            break;

        case ICN::BATTLESETS:
            LoadOrgICN(sprite, ICN::TEXTBAR, 0 + index, false);
            // clean
            GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(4, 8, 43, 13), 3, 10, sprite);
            //
            GetICN(ICN::ADVBTNS, 14 + index).Blit(Rect(5, 5, 26, 26), 10, 6, sprite);
            break;

        case ICN::BATTLEWAIT:
            {
                LoadOrgICN(sprite, ICN::TEXTBAR, 4 + index, false);
                // clean
                GetICN(ICN::SYSTEM, 11 + index).Blit(Rect(3, 8, 43, 14), 3, 1, sprite);
                // wait
                Surface dst = Sprite::ScaleQVGASurface(
                    GetICN(ICN::ADVBTNS, 8 + index).GetSurface(Rect(5, 4, 28, 28)));
                dst.Blit((sprite.w() - dst.w()) / 2, 2, sprite);
            }
            break;

        case ICN::BOAT12:
            {
                LoadOrgICN(sprite, ICN::ADVMCO, 28 + index, false);
                sprite.SetSurface(Sprite::ScaleQVGASurface(sprite));
            }
            break;

        case ICN::CSLMARKER:
            // sprite: not allow build: complete, not today, all builds (white)
            LoadOrgICN(sprite, ICN::LOCATORS, 24, false);

            // sprite: not allow build: builds requires
            if (1 == index)
                sprite.ChangeColorIndex(0x0A, 0xD6);
            else
                // sprite: not allow build: lack resources (green)
                if (2 == index)
                    sprite.ChangeColorIndex(0x0A, 0xDE);
            break;

        default:
            break;
        }

        memlimit_usage = true;
    }

    // change color
    for (uint32_t ii = 0; ii < count; ++ii)
    {
        Sprite& sprite = reflect ? v.reflect[ii] : v.sprites[ii];

        switch (icn)
        {
        case ICN::TELEPORT1:
            LoadOrgICN(sprite, ICN::OBJNMUL2, 116, false);
            sprite.ChangeColorIndex(0xEE, 0xEE + ii / 2);
            break;

        case ICN::TELEPORT2:
            LoadOrgICN(sprite, ICN::OBJNMUL2, 119, false);
            sprite.ChangeColorIndex(0xEE, 0xEE + ii);
            break;

        case ICN::TELEPORT3:
            LoadOrgICN(sprite, ICN::OBJNMUL2, 122, false);
            sprite.ChangeColorIndex(0xEE, 0xEE + ii);
            break;

        case ICN::FOUNTAIN:
            LoadOrgICN(sprite, ICN::OBJNMUL2, 15, false);
            sprite.ChangeColorIndex(0xE8, 0xE8 - ii);
            sprite.ChangeColorIndex(0xE9, 0xE9 - ii);
            sprite.ChangeColorIndex(0xEA, 0xEA - ii);
            sprite.ChangeColorIndex(0xEB, 0xEB - ii);
            sprite.ChangeColorIndex(0xEC, 0xEC - ii);
            sprite.ChangeColorIndex(0xED, 0xED - ii);
            sprite.ChangeColorIndex(0xEE, 0xEE - ii);
            sprite.ChangeColorIndex(0xEF, 0xEF - ii);
            break;

        case ICN::TREASURE:
            LoadOrgICN(sprite, ICN::OBJNRSRC, 19, false);
            sprite.ChangeColorIndex(0x0A, ii ? 0x00 : 0x0A);
            sprite.ChangeColorIndex(0xC2, ii ? 0xD6 : 0xC2);
            sprite.ChangeColorIndex(0x64, ii ? 0xDA : 0x64);
            break;

        case ICN::ROUTERED:
            LoadOrgICN(sprite, ICN::ROUTE, ii, false);
            sprite.ChangeColorIndex(0x55, 0xB0);
            sprite.ChangeColorIndex(0x5C, 0xB7);
            sprite.ChangeColorIndex(0x60, 0xBB);
            break;

        case ICN::YELLOW_FONT:
            LoadOrgICN(sprite, ICN::FONT, ii, false);
            sprite.ChangeColorIndex(0x0A, 0xDA);
            sprite.ChangeColorIndex(0x0B, 0xDA);
            sprite.ChangeColorIndex(0x0C, 0xDA);
            sprite.ChangeColorIndex(0x0D, 0xDA);
            sprite.ChangeColorIndex(0x0E, 0xDB);
            sprite.ChangeColorIndex(0x0F, 0xDB);
            sprite.ChangeColorIndex(0x10, 0xDB);
            sprite.ChangeColorIndex(0x11, 0xDB);
            sprite.ChangeColorIndex(0x12, 0xDB);
            sprite.ChangeColorIndex(0x13, 0xDB);
            sprite.ChangeColorIndex(0x14, 0xDB);
            break;

        case ICN::YELLOW_SMALFONT:
            LoadOrgICN(sprite, ICN::SMALFONT, ii, false);
            sprite.ChangeColorIndex(0x0A, 0xDA);
            sprite.ChangeColorIndex(0x0B, 0xDA);
            sprite.ChangeColorIndex(0x0C, 0xDA);
            sprite.ChangeColorIndex(0x0D, 0xDA);
            sprite.ChangeColorIndex(0x0E, 0xDB);
            sprite.ChangeColorIndex(0x0F, 0xDB);
            sprite.ChangeColorIndex(0x10, 0xDB);
            sprite.ChangeColorIndex(0x11, 0xDB);
            sprite.ChangeColorIndex(0x12, 0xDB);
            sprite.ChangeColorIndex(0x13, 0xDB);
            sprite.ChangeColorIndex(0x14, 0xDB);
            break;

        default:
            break;
        }
    }

    return true;
}

bool AGG::LoadAltICN(int icn, uint32_t index, bool reflect)
{
    return false;
}

void AGG::SaveICN(int icn)
{
}

vector<u8> AGG::ReadICNChunk(int icn, uint32_t index)
{
    // hard fix artifact "ultimate stuff" sprite for loyalty version
    if (ICN::ARTIFACT == icn &&
        Artifact(Artifact::ULTIMATE_STAFF).IndexSprite64() == index && heroes2x_agg.isGood())
    {
        return heroes2x_agg.Read(ICN::GetString(icn));
    }

    return ReadChunk(ICN::GetString(icn));
}

struct ICNHeader
{
    ICNHeader() : offsetX(0), offsetY(0), width(0), height(0), type(0), offsetData(0)
    {
    }

    u16 offsetX;
    u16 offsetY;
    u16 width;
    u16 height;
    u8 type;
    uint32_t offsetData;
};

ByteVectorReader& operator>>(ByteVectorReader& st, ICNHeader& icn)
{
    icn.offsetX = st.getLE16();
    icn.offsetY = st.getLE16();
    icn.width = st.getLE16();
    icn.height = st.getLE16();
    icn.type = st.Get8();
    icn.offsetData = st.getLE32();

    return st;
}

void AGG::RenderICNSprite(int icn, uint32_t index, const Rect& srt, const Point& dpt, Surface& dst)
{
    ICNSprite res = RenderICNSprite(icn, index);
    res.first.Blit(srt, dpt, dst);
}

std::string joinValues(const std::vector<u8>& body, int maxSize)
{
    std::string result = std::to_string(body.size()) + ": ";
    const int maxIndex = std::min((int)body.size(), maxSize);
    bool isFirst = true;

    for (int i = 0; i < maxIndex; i++)
    {
        if (isFirst)
        {
            isFirst = false;
        }
        else
        {
            result += ", ";
        }
        result += std::to_string(body[i]);
    }
    return result;
}

AGG::ICNSprite AGG::RenderICNSprite(int icn, uint32_t index)
{
    ICNSprite res;
    const vector<u8> body = ReadICNChunk(icn, index);
    if (body.empty())
    {
        return res;
    }

    ByteVectorReader st(body);

    const auto count = st.getLE16();
    if (index >= count)
    {
        return res;
    }
    const auto blockSize = st.getLE32();

    if (index) st.skip(index * 13);

    ICNHeader header1;
    st >> header1;

    uint32_t sizeData = 0;
    if (index + 1 != count)
    {
        ICNHeader header2;
        st >> header2;
        sizeData = header2.offsetData - header1.offsetData;
    }
    else
        sizeData = blockSize - header1.offsetData;

    // start render
    const Size sz = Size(header1.width, header1.height);

    const u8* buf = &body[6 + header1.offsetData];
    const u8* max = buf + sizeData;

    res.offset = Point(header1.offsetX, header1.offsetY);
    Surface& sf1 = res.first;
    Surface& sf2 = res.second;

    sf1.Set(sz.w, sz.h, false);

    const uint32_t shadowCol = RGBA::packRgba(0, 0, 0, 0x40);

    uint32_t c = 0;
    Point pt(0, 0);
    sf1.Lock();
    while (true)
    {
        auto cur = *buf;
        // 0x00 - end line
        if (0 == *buf)
        {
            ++pt.y;
            pt.x = 0;
            ++buf;
        }
        else
            // 0x7F - count data
            if (0x80 > *buf)
            {
                c = *buf;
                ++buf;
                while (c-- && buf < max)
                {
                    DrawPointFast(sf1, pt.x, pt.y, *buf);
                    ++pt.x;
                    ++buf;
                }
            }
            else
                // 0x80 - end data
                if (0x80 == *buf)
                {
                    break;
                }
                else
                    // 0xBF - skip data
                    if (0xC0 > *buf)
                    {
                        pt.x += *buf - 0x80;
                        ++buf;
                    }
                    else
                        // 0xC0 - shadow
                        if (0xC0 == *buf)
                        {
                            ++buf;
                            c = *buf % 4 ? *buf % 4 : *++buf;

                            if (sf1.depth() == 8) // skip alpha
                            {
                                pt.x += c;
                                c = 0;
                            }
                            else
                            {
                                if (!sf2.isValid())
                                {
                                    sf2.Set(sz.w, sz.h, true);
                                    sf2.Lock();
                                }
                                while (c--)
                                {
                                    sf2.SetPixel4(pt.x, pt.y, shadowCol);
                                    ++pt.x;
                                }
                            }
                            ++buf;
                        }
                        else
                            // 0xC1
                            if (0xC1 == *buf)
                            {
                                ++buf;
                                c = *buf;
                                ++buf;
                                while (c--)
                                {
                                    DrawPointFast(sf1, pt.x, pt.y, *buf);
                                    ++pt.x;
                                }
                                ++buf;
                            }
                            else
                            {
                                c = *buf - 0xC0;
                                ++buf;
                                while (c--)
                                {
                                    DrawPointFast(sf1, pt.x, pt.y, *buf);
                                    ++pt.x;
                                }
                                ++buf;
                            }
        if (buf >= max)
        {
            break;
        }
    }

    // fix air elem sprite
    if (icn == ICN::AELEM &&
        res.first.w() > 3 && res.first.h() > 3)
    {
        res.first.RenderContour(RGBA(0, 0x84, 0xe0)).Blit(-1, -1, res.first);
    }
    sf1.Unlock();
    if (sf2.isValid())
    {
        sf2.Unlock();
    }
    return res;
}

bool AGG::LoadOrgICN(Sprite& sp, int icn, uint32_t index, bool reflect)
{
    ICNSprite icnSprite = RenderICNSprite(icn, index);

    if (!icnSprite.isValid())
        return false;
    const auto picSp = icnSprite.CreateSprite(reflect, !ICN::SkipLocalAlpha(icn));
    sp = *picSp;
    return true;
}

bool AGG::LoadOrgICN(int icn, uint32_t index, bool reflect)
{
    icn_cache_t& v = icn_cache[icn];

    if (nullptr == v.sprites)
    {
        const vector<u8>& body = ReadChunk(ICN::GetString(icn));

        if (body.empty())
            return false;
        ByteVectorReader bvr(body);
        v.count = bvr.getLE16();
        v.sprites = new Sprite[v.count];
        v.reflect = new Sprite[v.count];
        if (v.count == 0)
        {
            return true;
        }
    }

    Sprite& sp = reflect ? v.reflect[index] : v.sprites[index];

    return LoadOrgICN(sp, icn, index, reflect);
}

/* load ICN object */
void AGG::LoadICN(int icn, uint32_t index, bool reflect)
{
    icn_cache_t& v = icn_cache[icn];

    // need load
    if ((!reflect || (v.reflect
            && (index >= v.count || v.reflect[index].isValid())))
        && (reflect || (v.sprites &&
            (index >= v.count || v.sprites[index].isValid()))))
        return;
    const Settings& conf = Settings::Get();

    // load from images dir
    if (conf.UseAltResource() && LoadAltICN(icn, index, reflect))
        return;
    // load modify sprite
    if (LoadExtICN(icn, index, reflect))
        return;
    //load origin sprite
    if (!LoadOrgICN(icn, index, reflect))
        Error::Except(__FUNCTION__, "load icn");
}

/* return ICN sprite */
Sprite AGG::GetICN(int icn, uint32_t index, bool reflect)
{
    Sprite result;

    if (icn >= static_cast<int>(icn_cache.size()))
    {
        return result;
    }
    icn_cache_t& v = icn_cache[icn];

    // out of range?
    if (v.count && index >= v.count)
    {
        index = 0;
    }

    // need load?
    if (0 == v.count ||
        ((reflect && (!v.reflect || !v.reflect[index].isValid())) || (!v.sprites || !v.sprites[index].isValid())))
    {
        CheckMemoryLimit();
        LoadICN(icn, index, reflect);
    }

    result = reflect ? v.reflect[index] : v.sprites[index];


    return result;
}

/* return count of sprites from specific ICN */
uint32_t AGG::GetICNCount(int icn)
{
    if (icn_cache[icn].count == 0) GetICN(icn, 0);
    return icn_cache[icn].count;
}


int AGG::PutICN(const Sprite& sprite, bool init_reflect)
{
    icn_cache_t v;

    v.count = 1;
    v.sprites = new Sprite[1];
    v.sprites[0] = sprite;

    if (init_reflect)
    {
        v.reflect = new Sprite[1];
        v.reflect[0] = Sprite(sprite.RenderReflect(2), sprite.x(), sprite.y());
    }

    icn_cache.push_back(v);
    return icn_cache.size() - 1;
}

bool AGG::LoadAltTIL(int til, uint32_t max)
{
    return false;
}

void AGG::SaveTIL(int til)
{
}

bool AGG::LoadOrgTIL(int til, uint32_t max)
{
    vector<u8> body = ReadChunk(TIL::GetString(til));

    if (body.empty())
        return false;
    ByteVectorReader st(body);

    const uint32_t count = st.getLE16();
    const uint32_t width = st.getLE16();
    const uint32_t height = st.getLE16();

    const uint32_t tile_size = width * height;
    const uint32_t body_size = 6 + count * tile_size;

    til_cache_t& v = til_cache[til];

    // check size
    if (body.size() == body_size && count <= max)
    {
        for (uint32_t ii = 0; ii < count; ++ii)
            v.sprites[ii] = Surface(&body[6 + ii * tile_size], width, height, 1, false);

        return true;
    }

    return false;
}

/* load TIL object to AGG::Cache */
void AGG::LoadTIL(int til)
{
    til_cache_t& v = til_cache[til];

    if (v.sprites != nullptr)
        return;
    uint32_t max = 0;

    switch (til)
    {
    case TIL::CLOF32:
        max = 4;
        break;
    case TIL::GROUND32:
        max = 432;
        break;
    case TIL::STON:
        max = 36;
        break;
    default:
        break;
    }

    v.count = max * 4; // rezerve for rotate sprites
    v.sprites = new Surface[v.count];

    const Settings& conf = Settings::Get();

    // load from images dir
    if (!conf.UseAltResource() || !LoadAltTIL(til, max))
    {
        if (!LoadOrgTIL(til, max))
            Error::Except(__FUNCTION__, "load til");
    }
}

/* return TIL surface from AGG::Cache */
Surface AGG::GetTIL(int til, uint32_t index, uint32_t shape)
{
    Surface result;

    if (til < static_cast<int>(til_cache.size()))
    {
        til_cache_t& v = til_cache[til];

        if (0 == v.count) LoadTIL(til);
        uint32_t index2 = index;

        if (shape)
        {
            switch (til)
            {
            case TIL::STON:
                index2 += 36 * (shape % 4);
                break;
            case TIL::CLOF32:
                index2 += 4 * (shape % 4);
                break;
            case TIL::GROUND32:
                index2 += 432 * (shape % 4);
                break;
            default:
                break;
            }
        }

        if (index2 >= v.count)
        {
            index2 = 0;
        }

        Surface& surface = v.sprites[index2];

        if (shape && !surface.isValid())
        {
            const Surface& src = v.sprites[index];

            if (src.isValid())
                surface = src.RenderReflect(shape);
        }


        result = surface;
    }

    return result;
}

/* load 82M object to AGG::Cache in Audio::CVT */
void AGG::LoadWAV(int m82, std::vector<u8>& v)
{
    const Settings& conf = Settings::Get();

    if (conf.UseAltResource())
    {
        std::string name = StringLower(M82::GetString(m82));
        const std::string prefix_sounds = System::ConcatePath("files", "sounds");

        // ogg
        StringReplace(name, ".82m", ".ogg");
        std::string sound = Settings::GetLastFile(prefix_sounds, name);
        v = FileUtils::readFileBytes(sound);

        if (v.empty())
        {
            // find mp3
            StringReplace(name, ".82m", ".mp3");
            sound = Settings::GetLastFile(prefix_sounds, name);

            v = FileUtils::readFileBytes(sound);
        }

        if (!v.empty()) return;
    }

    const vector<u8>& body = ReadChunk(M82::GetString(m82));

    if (body.empty())
        return;
    // create WAV format
    ByteVectorWriter wavHeader(44);
    wavHeader.putLE32(0x46464952); // RIFF
    wavHeader.putLE32(body.size() + 0x24); // size
    wavHeader.putLE32(0x45564157); // WAVE
    wavHeader.putLE32(0x20746D66); // FMT
    wavHeader.putLE32(0x10); // size_t
    wavHeader.putLE16(0x01); // format
    wavHeader.putLE16(0x01); // channels
    wavHeader.putLE32(22050); // samples
    wavHeader.putLE32(22050); // byteper
    wavHeader.putLE16(0x01); // align
    wavHeader.putLE16(0x08); // bitsper
    wavHeader.putLE32(0x61746164); // DATA
    wavHeader.putLE32(body.size()); // size

    v.reserve(body.size() + 44);
    auto vecData = wavHeader.data();
    v.assign(vecData.begin(), vecData.end());
    v.insert(v.begin() + 44, body.begin(), body.end());
}

/* load XMI object */
void AGG::LoadMID(int xmi, vector<u8>& v)
{
    const vector<u8> body = ReadChunk(XMI::GetString(xmi));

    if (!body.empty())
        v = Music::Xmi2Mid(body);
}

/* return CVT */
const vector<u8>& AGG::GetWAV(int m82)
{
    vector<u8>& v = wav_cache[m82];
    if (Mixer::isValid() && v.empty()) LoadWAV(m82, v);
    return v;
}

/* return MID */
const vector<u8>& AGG::GetMID(int xmi)
{
    vector<u8>& v = mid_cache[xmi];
    if (Mixer::isValid() && v.empty()) LoadMID(xmi, v);
    return v;
}

void AGG::LoadLOOPXXSounds(const vector<int>& vols)
{
    const Settings& conf = Settings::Get();

    if (!conf.Sound())
        return;
    // set volume loop sounds
    for (auto itv = vols.begin(); itv != vols.end(); ++itv)
    {
        const int vol = *itv;
        int m82 = M82::GetLOOP00XX(distance(vols.begin(), itv));
        if (M82::UNKNOWN == m82) continue;

        // find loops
        const auto itl = find(loop_sounds.begin(), loop_sounds.end(), m82);

        if (itl != loop_sounds.end())
        {
            // unused and free
            if (0 == vol)
            {
                if (Mixer::isPlaying((*itl).channel))
                {
                    Mixer::Pause((*itl).channel);
                    Mixer::Volume((*itl).channel, Mixer::MaxVolume() * conf.SoundVolume() / 10);
                    Mixer::Stop((*itl).channel);
                }
                (*itl).sound = M82::UNKNOWN;
            }
                // used and set vols
            else if (Mixer::isPlaying((*itl).channel))
            {
                Mixer::Pause((*itl).channel);
                Mixer::Volume((*itl).channel, vol * conf.SoundVolume() / 10);
                Mixer::Resume((*itl).channel);
            }
        }
        else
            // new sound
            if (0 != vol)
            {
                const vector<u8>& v = GetWAV(m82);
                int ch = Mixer::Play(&v[0], v.size(), -1, true);

                if (0 <= ch)
                {
                    Mixer::Pause(ch);
                    Mixer::Volume(ch, vol * conf.SoundVolume() / 10);
                    Mixer::Resume(ch);

                    // find unused
                    auto itl = find(loop_sounds.begin(), loop_sounds.end(),
                                    static_cast<int>(M82::UNKNOWN));

                    if (itl != loop_sounds.end())
                    {
                        (*itl).sound = m82;
                        (*itl).channel = ch;
                    }
                    else
                        loop_sounds.emplace_back(m82, ch);
                }
            }
    }
}

/* wrapper Audio::Play */
void AGG::PlaySound(int m82)
{
    const Settings& conf = Settings::Get();

    if (!conf.Sound()) return;
    const vector<u8>& v = GetWAV(m82);
    const int ch = Mixer::Play(&v[0], v.size(), -1, false);
    Mixer::Pause(ch);
    Mixer::Volume(ch, Mixer::MaxVolume() * conf.SoundVolume() / 10);
    Mixer::Resume(ch);
}

/* wrapper Audio::Play */
void AGG::PlayMusic(int mus, bool loop)
{
    const Settings& conf = Settings::Get();

    if (!conf.Music() || MUS::UNUSED == mus || MUS::UNKNOWN == mus ||
        (Game::CurrentMusic() == mus && Music::isPlaying()))
        return;

    Game::SetCurrentMusic(mus);
    const string prefix_music = System::ConcatePath("files", "music");

    if (conf.MusicExt())
    {
        string filename = Settings::GetLastFile(prefix_music, MUS::GetString(mus));

        if (!System::IsFile(filename))
            filename.clear();

        if (filename.empty())
        {
            filename = Settings::GetLastFile(prefix_music, MUS::GetString(mus, true));

            if (!System::IsFile(filename))
            {
                StringReplace(filename, ".ogg", ".mp3");

                if (!System::IsFile(filename))
                {
                    filename.clear();
                }
            }
        }

        if (!filename.empty())
            Music::Play(filename, loop);
    }
    else
#ifdef WITH_AUDIOCD
        if(conf.MusicCD() && Cdrom::IsValid())
        {
            Cdrom::Play(mus, loop);
        }
        else
#endif
        if (conf.MusicMIDI())
        {
            const int xmi = XMI::FromMUS(mus);
            if (XMI::UNKNOWN != xmi)
            {
#ifdef WITH_MIXER
                const std::vector<u8>& v = GetMID(xmi);
                if (!v.empty()) Music::Play(v, loop);
#else
            string mid = XMI::GetString(xmi);
            StringReplace(mid, ".XMI", ".MID");
            const string file = System::ConcatePath(Settings::GetWriteableDir("music"), mid);

            if (!System::IsFile(file))
            {
                SaveMemToFile(GetMID(xmi), file);
            }

            Music::Play(file, loop);
#endif
            }
        }
}

void AGG::LoadTTFChar(uint32_t ch)
{
    const Settings& conf = Settings::Get();
    const RGBA white(0xFF, 0xFF, 0xFF);
    const RGBA yellow(0xFF, 0xFF, 0x00);
    const RGBA gray(0x7F, 0x7F, 0x7F);

    // small
    fnt_cache[ch].sfs[0] = fonts[0].RenderUnicodeChar(ch, white, !conf.FontSmallRenderBlended());
    fnt_cache[ch].sfs[1] = fonts[0].RenderUnicodeChar(ch, yellow, !conf.FontSmallRenderBlended());

    // medium
    fnt_cache[ch].sfs[2] = fonts[1].RenderUnicodeChar(ch, white, !conf.FontNormalRenderBlended());
    fnt_cache[ch].sfs[3] = fonts[1].RenderUnicodeChar(ch, yellow, !conf.FontNormalRenderBlended());
    // gray
    fnt_cache[ch].sfs[4] = fonts[0].RenderUnicodeChar(ch, gray, !conf.FontSmallRenderBlended());
    fnt_cache[ch].sfs[5] = fonts[1].RenderUnicodeChar(ch, gray, !conf.FontNormalRenderBlended());
}

void AGG::LoadFNT()
{
    const auto& conf = Settings::Get();

    if (!fnt_cache.empty())
        return;
    const std::string letters =
        "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    auto unicode = StringUTF8_to_UNICODE(letters);

    for (auto& it : unicode)
        LoadTTFChar(it);
}

uint32_t AGG::GetFontHeight(bool small)
{
    return small ? fonts[0].Height() : fonts[1].Height();
}

/* return letter sprite */
Surface AGG::GetUnicodeLetter(uint32_t ch, uint32_t ft)
{
    const bool ttf_valid = fonts[0].isValid() && fonts[1].isValid();

    if (!ttf_valid)
        return GetLetter(ch, ft);

    if (!fnt_cache[ch].sfs[0].isValid()) LoadTTFChar(ch);

    const auto& surfaces = fnt_cache[ch].sfs;
    switch (ft)
    {
    case Font::YELLOW_SMALL:
        return surfaces[1];
    case Font::BIG:
        return surfaces[2];
    case Font::YELLOW_BIG:
        return surfaces[3];
    case Font::SHADDOW:
        return surfaces[4];
    case Font::SHADDOW_BIG:
        return surfaces[5];
    default:
        break;
    }

    return surfaces[0];
}

Surface AGG::GetLetter(uint32_t ch, uint32_t ft)
{
    switch (ft)
    {
    case Font::YELLOW_BIG:
        return GetICN(ICN::YELLOW_FONT, ch - 0x20);
    case Font::YELLOW_SMALL:
        return GetICN(ICN::YELLOW_SMALFONT, ch - 0x20);
    case Font::BIG:
        return GetICN(ICN::FONT, ch - 0x20);
    case Font::SMALL:
        return GetICN(ICN::SMALFONT, ch - 0x20);

    default:
        break;
    }

    return GetICN(ICN::SMALFONT, ch - 0x20);
}

void AGG::ResetMixer()
{
    Mixer::Reset();
    loop_sounds.clear();
    loop_sounds.reserve(7);
}

void AGG::ShowError()
{
}

bool AGG::Init()
{
    // read data dir
    if (!ReadDataDir())
    {
        //ShowError();
        //return false;
    }

    Settings& conf = Settings::Get();
    const std::string prefix_fonts = System::ConcatePath("files", "fonts");
    const std::string font1 = Settings::GetLastFile(prefix_fonts, conf.FontsNormal());
    const std::string font2 = Settings::GetLastFile(prefix_fonts, conf.FontsSmall());

    fonts = std::unique_ptr<FontTTF[]>(new FontTTF[2]);

    if (conf.Unicode())
    {
        if (!fonts[1].Open(font1, conf.FontsNormalSize()) ||
            !fonts[0].Open(font2, conf.FontsSmallSize()))
            conf.SetUnicode(false);
    }

    icn_cache.reserve(ICN::LASTICN + 256);
    icn_cache.resize(ICN::LASTICN);

    til_cache.resize(TIL::LASTTIL);

    fillPalette(pal_colors, pal_colors_u32);

    Surface::SetDefaultPalette(&pal_colors[0], pal_colors.size());

    // load font
    LoadFNT();

    return true;
}

void AGG::Quit()
{
    for (auto& icns : icn_cache)
    {
        delete[] icns.sprites;
        icns.sprites = nullptr;
        delete[] icns.reflect;
        icns.reflect = nullptr;
    }
    icn_cache.clear();

    for (auto& tils : til_cache)
    {
        delete[] tils.sprites;
    }

    til_cache.clear();
    wav_cache.clear();
    mid_cache.clear();
    loop_sounds.clear();
    fnt_cache.clear();
    pal_colors.clear();
    fonts = nullptr;
}

RGBA AGG::GetPaletteColor(uint32_t index)
{
    return index < pal_colors.size()
               ? RGBA(pal_colors[index].r, pal_colors[index].g, pal_colors[index].b)
               : RGBA(0, 0, 0);
}

void AGG::DrawPointFast(Surface& srf, int x, int y, u8 palette)
{
    const auto palColor = pal_colors_u32[palette];
    srf.SetPixel4(x, y, palColor);
}
