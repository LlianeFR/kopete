// aimchatsession.cpp

// Copyright (C)  2005  Matt Rogers <mattr@kde.org>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA
// 02110-1301, USA.


#include "aimchatsession.h"
#include "kopetecontact.h"
#include "kopetechatsessionmanager.h"
#include "kopeteprotocol.h"

AIMChatSession::AIMChatSession( const Kopete::Contact* user,  Kopete::ContactPtrList others,
                                  Kopete::Protocol* protocol )
    : Kopete::ChatSession( user, others, protocol, "AIMChatSession" )
{
    Kopete::ChatSessionManager::self()->registerChatSession( this );
    setInstance( protocol->instance() );
    setMayInvite( false );
}

QString AIMChatSession::roomName() const
{

    return m_roomName;
}

void AIMChatSession::setRoomName( const QString& room )
{
    m_roomName = room;
}

Oscar::WORD AIMChatSession::exchange() const
{
    return m_exchange;
}

void AIMChatSession::setExchange( Oscar::WORD exchange )
{
    m_exchange = exchange;
}

const QString AIMChatSession::displayName()
{
    return m_roomName;
}

