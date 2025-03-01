/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <utility>
#include <iomanip>
#include <list>
#include <array>
#include <vector>
#include <iostream>

#include "audio_music.h"

#define TAG_FORM    0x464F524D
#define TAG_XDIR    0x58444952
#define    TAG_INFO    0x494E464F
#define TAG_CAT0    0x43415420
#define TAG_XMID    0x584D4944
#define TAG_TIMB    0x54494D42
#define TAG_EVNT    0x45564E54
#define TAG_RBRN    0x5242524E
#define TAG_MTHD    0x4D546864
#define TAG_MTRK    0x4D54726B

#include "system.h"
#include "ByteVectorReader.h"
#include "ByteVectorWriter.h"

using namespace std;

struct pack_t : public pair<uint32_t, uint32_t> /* delta offset */
{
    pack_t() : pair<uint32_t, uint32_t>(0, 0)
    {
    }
};

vector<u8> packValue(uint32_t delta)
{
    const u8 c1 = delta & 0x0000007F;
    const u8 c2 = (delta & 0x00003F80) >> 7;
    const u8 c3 = (delta & 0x001FC000) >> 14;
    const u8 c4 = (delta & 0x0FE00000) >> 21;

    vector<u8> res;
    res.reserve(4);

    if (c4)
    {
        res.push_back(c4 | 0x80);
        res.push_back(c3 | 0x80);
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else if (c3)
    {
        res.push_back(c3 | 0x80);
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else if (c2)
    {
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else
        res.push_back(c1);

    return res;
}

pack_t unpackValue(const u8* ptr)
{
    const u8* p = ptr;
    pack_t res;

    while (*p & 0x80)
    {
        if (4 <= p - ptr)
        {
            H2ERROR("unpack delta mistake");
            break;
        }

        res.first |= 0x0000007F & *p;
        res.first <<= 7;
        ++p;
    }

    res.first += *p;
    res.second = p - ptr + 1;

    return res;
}

struct meta_t
{
    meta_t()
    {
    }

    meta_t(u8 c, u8 q, uint32_t d) : command(c), quantity(q), duration(d)
    {
    }

    bool operator<(const meta_t& m) const
    {
        return duration < m.duration;
    }

    void decrease_duration(uint32_t delta)
    {
        duration -= delta;
    }

    u8 command = 0;
    u8 quantity = 0;
    uint32_t duration = 0;
};

struct IFFChunkHeader
{
    uint32_t ID = 0; // 4 upper case ASCII chars, padded with 0x20 (space)
    uint32_t length = 0; // big-endian

    IFFChunkHeader(uint32_t id, uint32_t sz) : ID(id), length(sz)
    {
    }

    IFFChunkHeader() = default;
};


ByteVectorReader& operator>>(ByteVectorReader& sb, IFFChunkHeader& st)
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    return sb;
}

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const IFFChunkHeader& st)
{
    sb.putBE32(st.ID);
    sb.putBE32(st.length);
    return sb;
}

struct GroupChunkHeader
{
    uint32_t ID = 0; // 4 byte ASCII string, either 'FORM', 'CAT ' or 'LIST'
    uint32_t length = 0;
    uint32_t type = 0; // 4 byte ASCII string

    GroupChunkHeader(uint32_t id, uint32_t sz, uint32_t tp) : ID(id), length(sz), type(tp)
    {
    }

    GroupChunkHeader() = default;
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const GroupChunkHeader& st)
{
    sb.putBE32(st.ID);
    sb.putBE32(st.length);
    sb.putBE32(st.type);
    return sb;
}

ByteVectorReader& operator>>(ByteVectorReader& sb, GroupChunkHeader& st)
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    st.type = sb.getBE32();
    return sb;
}

struct XMITrack
{
    vector<u8> timb;
    vector<u8> evnt;
};

struct XMITracks : list<XMITrack>
{
};

struct XMIData
{
    XMITracks tracks;

    explicit XMIData(const vector<u8>& buf)
    {
        ByteVectorReader sb(buf);

        GroupChunkHeader group;
        IFFChunkHeader iff;

        // FORM XDIR
        sb >> group;
        if (group.ID != TAG_FORM || group.type != TAG_XDIR)
        {
            H2ERROR("parse H2ERROR: " << "form xdir")
            return;
        }
        // INFO
        sb >> iff;
        if (iff.ID != TAG_INFO || iff.length != 2)
        {
            H2ERROR("parse H2ERROR: " << "info");
            return;
        }
        const int numTracks = sb.getLE16();

        // CAT XMID
        sb >> group;
        if (group.ID != TAG_CAT0 || group.type != TAG_XMID)
        {
            H2ERROR("parse H2ERROR: " << "cat xmid")
            return;
        }
        for (int track = 0; track < numTracks; ++track)
        {
            tracks.push_back(XMITrack());

            vector<u8>& timb = tracks.back().timb;
            vector<u8>& evnt = tracks.back().evnt;

            sb >> group;
            // FORM XMID
            if (group.ID != TAG_FORM || group.type != TAG_XMID)
            {
                H2ERROR("unknown tag: " << group.ID << " (expected FORM), " << group.type << " (expected XMID)");
                continue;
            }
            sb >> iff;
            // [TIMB]
            if (iff.ID == TAG_TIMB)
            {
                timb = sb.getRaw(iff.length);
                if (timb.size() != iff.length)
                {
                    H2ERROR("parse H2ERROR: " << "out of range");
                    break;
                }
                sb >> iff;
            }

            // [RBRN]
            if (iff.ID == TAG_RBRN)
            {
                sb.skip(iff.length);
                sb >> iff;
            }

            // EVNT
            if (iff.ID != TAG_EVNT)
            {
                H2ERROR("parse H2ERROR: " << "evnt");
                break;
            }

            evnt = sb.getRaw(iff.length);

            if (evnt.size() != iff.length)
            {
                H2ERROR("parse H2ERROR: " << "out of range");
                break;
            }
        }
    }

    bool isvalid() const
    {
        return !tracks.empty();
    }
};

struct MidEvent
{
    vector<u8> pack;
    std::array<u8, 4> data; // status, data1, data2, count
    //char		status;
    //std::vector<u8>	data;

    size_t size() const
    {
        return pack.size() + data[3] + 1;
    }

    MidEvent() = default;

    MidEvent(uint32_t delta, u8 st, u8 d1, u8 d2)
    {
        data[0] = st;
        data[1] = d1;
        data[2] = d2;
        data[3] = 2;
        pack = packValue(delta);
    }

    MidEvent(uint32_t delta, u8 st, u8 d1)
    {
        data[0] = st;
        data[1] = d1;
        data[2] = 0;
        data[3] = 1;
        pack = packValue(delta);
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const MidEvent& st)
{
    for (unsigned char it : st.pack)
        sb << it;
    sb << st.data[0];
    if (2 == st.data[3])
        sb << st.data[1] << st.data[2];
    else if (1 == st.data[3])
        sb << st.data[1];
    return sb;
}

struct MidEvents
{
    vector<MidEvent> _items;

    size_t count() const
    {
        return _items.size();
    }

    size_t size() const
    {
        size_t res = 0;
        for (const auto& it : _items)
            res += it.size();
        return res;
    }

    MidEvents() = default;

    explicit MidEvents(const XMITrack& t)
    {
        const u8* ptr = &t.evnt[0];
        const u8* end = ptr + t.evnt.size();

        uint32_t delta = 0;
        list<meta_t> notesoff;

        while (ptr && ptr < end)
        {
            // insert event: note off
            if (delta)
            {
                // sort duration
                notesoff.sort();

                auto it1 = notesoff.begin();
                const auto it2 = notesoff.end();
                uint32_t delta2 = 0;

                // apply delta
                for (; it1 != it2; ++it1)
                {
                    if ((*it1).duration <= delta)
                    {
                        // note off
                        _items.emplace_back((*it1).duration - delta2, (*it1).command, (*it1).quantity, 0x7F);
                        delta2 += (*it1).duration - delta2;
                    }
                }

                // remove end notes
                while (!notesoff.empty() && notesoff.front().duration <= delta)
                    notesoff.pop_front();

                // fixed delta
                if (delta2) delta -= delta2;

                // decrease duration
                for (auto& it : notesoff)
                    it.decrease_duration(delta);
            }

            // interval
            if (*ptr < 128)
            {
                delta += *ptr;
                ++ptr;
            }
            else
                // command
            {
                // end
                if (0xFF == *ptr && 0x2F == *(ptr + 1))
                {
                    _items.emplace_back(delta, *ptr, *(ptr + 1), *(ptr + 2));
                    break;
                }
                switch (*ptr >> 4)
                {
                    // meta
                case 0x0F:
                    {
                        const pack_t pack = unpackValue(ptr + 2);
                        ptr += pack.first + pack.second + 1;
                        delta = 0;
                    }
                    break;

                    // key pressure
                case 0x0A:
                    // control change
                case 0x0B:
                    // pitch bend
                case 0x0E:
                    {
                        _items.emplace_back(delta, *ptr, *(ptr + 1), *(ptr + 2));
                        ptr += 3;
                        delta = 0;
                    }
                    break;

                    // note off
                case 0x08:
                    // note on
                case 0x09:
                    {
                        _items.emplace_back(delta, *ptr, *(ptr + 1), *(ptr + 2));
                        pack_t pack = unpackValue(ptr + 3);
                        notesoff.emplace_back(*ptr - 0x10, *(ptr + 1), pack.first);
                        ptr += 3 + pack.second;
                        delta = 0;
                    }
                    break;

                    // program change
                case 0x0C:
                    // chanel pressure
                case 0x0D:
                    {
                        _items.emplace_back(delta, *ptr, *(ptr + 1));
                        ptr += 2;
                        delta = 0;
                    }
                    break;

                    // unused command
                default:
                    _items.emplace_back(0, 0xFF, 0x2F, 0);
                    H2ERROR("unknown st: 0x" << std::setw(2) << std::setfill('0') << std::hex <<
                        static_cast<int>(*ptr) << ", ln: "
                        << static_cast<int>(&t.evnt[0] + t.evnt.size() - ptr));
                    break;
                }
            }
        }
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const MidEvents& st)
{
    for (const auto& it : st._items)
        sb << it;
    return sb;
}

struct MidTrack
{
    IFFChunkHeader mtrk;
    MidEvents events;

    MidTrack() : mtrk(TAG_MTRK, 0)
    {
    }

    explicit MidTrack(const XMITrack& t) : mtrk(TAG_MTRK, 0), events(t)
    {
        mtrk.length = events.size();
    }

    size_t size() const
    {
        return sizeof mtrk + events.size();
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const MidTrack& st)
{
    sb << st.mtrk;
    sb << st.events;
    return sb;
}

struct MidTracks
{
    vector<MidTrack> _items;

    size_t count() const
    {
        return _items.size();
    }

    size_t size() const
    {
        size_t res = 0;
        for (const auto& it : _items)
            res += it.size();
        return res;
    }

    MidTracks() = default;

    explicit MidTracks(const XMITracks& tracks)
    {
        for (const auto& track : tracks)
            _items.emplace_back(track);
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const MidTracks& st)
{
    for (const auto& it : st._items)
        sb << it;
    return sb;
}

struct MidData
{
    IFFChunkHeader mthd;
    int format;
    int ppqn;
    MidTracks tracks;

    MidData() : mthd(TAG_MTHD, 6), format(0), ppqn(0)
    {
    }

    MidData(const XMITracks& t, int p) : mthd(TAG_MTHD, 6), format(0), ppqn(p), tracks(t)
    {
    }
};

ByteVectorWriter& operator<<(ByteVectorWriter& sb, const MidData& st)
{
    sb << st.mthd;
    sb.putBE16(st.format);
    sb.putBE16(st.tracks.count());
    sb.putBE16(st.ppqn);
    sb << st.tracks;
    return sb;
}

vector<u8> Music::Xmi2Mid(const vector<u8>& buf)
{
    XMIData xmi(buf);
    ByteVectorWriter sb(16 * 4096);

    if (xmi.isvalid())
    {
        const MidData mid(xmi.tracks, 64);
        sb << mid;
    }

    return sb.data();
}
