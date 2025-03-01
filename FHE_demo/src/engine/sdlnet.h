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

#ifndef SDLNET_H
#define SDLNET_H

#include "types.h"

#ifdef WITH_NET
#include "SDL_net.h"

namespace Network
{
    bool Init();
    void Quit();
    bool ResolveHost(IPaddress&, const char*, u16);
    const char* GetError();

    class Socket
    {
    public:
        Socket();
        explicit Socket(TCPsocket);
        ~Socket();

        void Assign(TCPsocket);

        bool Ready() const;

        bool Recv(char*, int);
        bool Send(const char*, int);

        bool Recv32(uint32_t&);
        bool Send32(const uint32_t&);

        bool Recv16(u16&);
        bool Send16(const u16&);

        uint32_t Host() const;
        u16 Port() const;

        bool Open(IPaddress&);
        bool isValid() const;
        void Close();

    protected:
        Socket(const Socket&);
        Socket& operator=(const Socket&);

        TCPsocket sd;
        SDLNet_SocketSet sdset;
        size_t status;
    };

    class Server : public Socket
    {
    public:
        Server();

        TCPsocket Accept() const;
    };
}
#endif

#endif
