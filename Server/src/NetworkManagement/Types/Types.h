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

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <stdlib.h>
#include "../../Common/Types.h"

namespace NetworkManagement_Types
{
    typedef unsigned long RawNetworkSessionID;
    const RawNetworkSessionID INVALID_RAW_NETWORK_SESSION_ID = 0;
    
    typedef unsigned long DataSessionID;
    const DataSessionID INVALID_DATA_SESSION_ID = 0;
    
    typedef std::size_t BufferSize;
    const BufferSize INVALID_BUFFER_SIZE = 0;
    
    typedef unsigned long OperationTimeoutLength;
    
    typedef unsigned int ConnectionEventID;
    
    typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
    
    enum class PeerType { INVALID, CLIENT, SERVER };
    enum class ConnectionType { INVALID, COMMAND, DATA };
    enum class ConnectionState { INVALID, ESTABLISHED, CLOSED };
    enum class ConnectionSubstate { NONE, READING, WRITING, FAILED, DROPPED, WAITING };
    enum class ConnectionInitiation { INVALID, LOCAL, REMOTE };
    
    typedef std::size_t PacketSize;
    
}

#endif	/* NETWORK_MANAGEMENT_TYPES_H */

