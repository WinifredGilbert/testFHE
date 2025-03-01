#include "ByteVectorWriter.h"
#include <string>

ByteVectorWriter::ByteVectorWriter(int sz)
    : _isBigEndian(false)
{
    _data.reserve(sz);
}

void ByteVectorWriter::putLE16(u16 v)
{
    put8(v);
    put8(v >> 8);
}

void ByteVectorWriter::putBE16(u16 v)
{
    put8(v >> 8);
    put8(v);
}

void ByteVectorWriter::putLE32(uint32_t v)
{
    putLE16(v);
    putLE16(v >> 16);
}

void ByteVectorWriter::putBE32(uint32_t v)
{
    putBE16(v >> 16);
    putBE16(v);
}

void ByteVectorWriter::SetBigEndian(bool value)
{
    _isBigEndian = value;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const char& v)
{
    put8(v);
    return *this;
}

void ByteVectorWriter::put32(uint32_t v)
{
    _isBigEndian ? putBE32(v) : putLE32(v);
}

void ByteVectorWriter::put16(u16 v)
{
    _isBigEndian ? putBE16(v) : putLE16(v);
}

ByteVectorWriter& ByteVectorWriter::operator<<(const bool& v)
{
    put8(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const u8& v)
{
    put8(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const s8& v)
{
    put8(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const u16& v)
{
    put16(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const s16& v)
{
    put16(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const u32& v)
{
    put32(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const s32& v)
{
    put32(v);
    return *this;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const float& v)
{
    const auto intpart = static_cast<s32>(v);
    const float decpart = (v - intpart) * 100000000;
    return *this << intpart << static_cast<s32>(decpart);
}

ByteVectorWriter& ByteVectorWriter::operator<<(const Size& v)
{
    return *this << v.w << v.h;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const Point& v)
{
    return *this << v.x << v.y;
}

ByteVectorWriter& ByteVectorWriter::operator<<(const std::string& v)
{
    put32(v.size());

    for (char it : v)
    {
        const u8 val = it;
        put8(val);
    }

    return *this;
}
