/**
 * Copyright (C) 2014 https://github.com/sndnv
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETWORK_MANAGEMENT_TYPES_H
#define	NETWORK_MANAGEMENT_TYPES_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <stdlib.h>
#include "../../Common/Types.h"

namespace NetworkManagement_Types
{
    typedef unsigned long RawConnectionID;
    const RawConnectionID INVALID_RAW_CONNECTION_ID = 0;
    
    typedef unsigned long long ConnectionID;
    const RawConnectionID INVALID_CONNECTION_ID = 0;
    
    typedef unsigned long TransientConnectionID;
    const TransientConnectionID INVALID_TRANSIENT_CONNECTION_ID = 0;
    
    typedef unsigned int ConnectionManagerID;
    const ConnectionManagerID INVALID_CONNECTION_MANAGER_ID = 0;
    
    typedef unsigned int CommandID;
    const CommandID INVALID_COMMAND_ID = 0;
    
    typedef std::size_t BufferSize;
    const BufferSize INVALID_BUFFER_SIZE = 0;
    
    typedef unsigned long OperationTimeoutLength;
    
    typedef unsigned int ConnectionEventID;
    
    typedef unsigned long StatCounter;
    
    typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
    
    enum class PeerType { INVALID, CLIENT, SERVER };
    enum class ConnectionType { INVALID, COMMAND, DATA, INIT };
    enum class ConnectionState { INVALID, ESTABLISHED, CLOSED };
    enum class ConnectionSubstate { NONE, READING, WRITING, FAILED, DROPPED, WAITING };
    enum class ConnectionInitiation { INVALID, LOCAL, REMOTE };
    enum class ConnectionSetupState
    {
        INVALID,
        INITIATED,
        CONNECTION_REQUEST_SENT, CONNECTION_REQUEST_SENT_CONFIRMED, CONNECTION_REQUEST_RECEIVED,
        CONNECTION_RESPONSE_SENT, CONNECTION_RESPONSE_SENT_CONFIRMED, CONNECTION_RESPONSE_RECEIVED,
        FAILED, COMPLETED
    };
    
    typedef std::size_t PacketSize;
}

#endif	/* NETWORK_MANAGEMENT_TYPES_H */

