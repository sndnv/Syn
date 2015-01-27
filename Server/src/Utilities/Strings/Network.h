/**
 * Copyright (C) 2015 https://github.com/sndnv
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

#ifndef UTILITIES_STRINGS_NETWORK_H
#define	UTILITIES_STRINGS_NETWORK_H

#include <string>
#include <boost/unordered_map.hpp>
#include "../../NetworkManagement/Types/Types.h"
#include "../../NetworkManagement/Types/Packets.h"

using NetworkManagement_Types::ConnectionRequest;
using NetworkManagement_Types::ConnectionState;
using NetworkManagement_Types::ConnectionSubstate;
using NetworkManagement_Types::ConnectionType;
using NetworkManagement_Types::PeerType;
using NetworkManagement_Types::ConnectionInitiation;

namespace Utilities
{
    namespace Strings
    {
        struct NetworkMaps
        {
            static const boost::unordered_map<PeerType, std::string> peerTypeToString;
            static const boost::unordered_map<std::string, PeerType> stringToPeerType;
            static const boost::unordered_map<ConnectionType, std::string> connectionTypeToString;
            static const boost::unordered_map<std::string, ConnectionType> stringToConnectionType;
            static const boost::unordered_map<ConnectionState, std::string> connectionStateToString;
            static const boost::unordered_map<std::string, ConnectionState> stringToConnectionState;
            static const boost::unordered_map<ConnectionSubstate, std::string> connectionSubstateToString;
            static const boost::unordered_map<std::string, ConnectionSubstate> stringToConnectionSubstate;
            static const boost::unordered_map<ConnectionInitiation, std::string> connectionInitiationToString;
            static const boost::unordered_map<std::string, ConnectionInitiation> stringToConnectionInitiation;
        };
        
        std::string toString(PeerType var);
        std::string toString(ConnectionType var);
        std::string toString(ConnectionState var);
        std::string toString(ConnectionSubstate var);
        std::string toString(ConnectionInitiation var);
        
        PeerType toPeerType(std::string var);
        ConnectionType toConnectionType(std::string var);
        ConnectionState toConnectionState(std::string var);
        ConnectionSubstate toConnectionSubstate(std::string var);
        ConnectionInitiation toConnectionInitiation(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_NETWORK_H */
