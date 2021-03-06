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

#ifndef NETWORK_MANAGEMENT_CONTAINERS_H
#define	NETWORK_MANAGEMENT_CONTAINERS_H

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include "Types.h"
#include "../../SecurityManagement/Crypto/Handlers.h"
#include "../../InstructionManagement/Sets/InstructionSet.h"

using NetworkManagement_Types::StatCounter;
using NetworkManagement_Types::CommandID;
using InstructionManagement_Sets::InstructionBasePtr;
using SecurityManagement_Crypto::SymmetricCryptoHandlerPtr;
using DatabaseManagement_Containers::DeviceDataContainerPtr;

namespace NetworkManagement_Types
{
    /** Structure for holding pending 'DATA' connection configuration. */
    struct PendingDataConnectionConfig
    {
        /** Transient connection ID. */
        TransientConnectionID transientID;
        /** Device data. */
        DeviceDataContainerPtr data;
        /** Crypto handler, for encrypted connection setup and data encryption (if enabled). */
        SymmetricCryptoHandlerPtr crypto;
        /** Denotes whether encryption is to be enabled. */
        bool encrypt;
        /** Denotes whether compression is to be enabled. */
        bool compress;
    };
    typedef boost::shared_ptr<PendingDataConnectionConfig> PendingDataConnectionConfigPtr;
    
    /** Structure for holding device IP configuration data. */
    struct DeviceIPSettings
    {
        /** Listening address for 'COMMAND' connections. */
        IPAddress commandAddress;
        /** Listening port for 'COMMAND' connections. */
        IPPort commandPort;
        /** Listening address for 'DATA' connections. */
        IPAddress dataAddress;
        /** Listening port for 'DATA' connections. */
        IPPort dataPort;
        /** Listening address for 'INIT' connections. */
        IPAddress initAddress;
        /** Listening port for 'INIT' connections. */
        IPPort initPort;
    };
    
    /**
     * Structure for holding new device connection configuration,
     * as generated by the initialization process.
     */
    struct NewDeviceConnectionParameters
    {
        /** IP configuration data. */
        DeviceIPSettings ipSettings;
        /** Raw device password. */
        std::string rawPassword;
        /** Raw peer public key. */
        std::string rawPublicKey;
        /** Key exchange type expected by the device. */
        KeyExchangeType expectedKeyExchange;
        /** Device peer type. */
        PeerType deviceType;
    };
    
    /** Structure for holding pending 'INIT' connection configuration. */
    struct PendingInitConnectionConfig
    {
        /** Shared password for the initialization process. */
        std::string initPassword;
        /** Device peer type. */
        PeerType peerType;
        /** Device ID, as generated locally. */
        DeviceID newPeerID;
        /** Transient ID associated with the connection. */
        TransientConnectionID transientID;
    };
    typedef boost::shared_ptr<PendingInitConnectionConfig> PendingInitConnectionConfigPtr;

    /** Structure for holding active connection data. */
    struct ActiveConnectionData
    {
        /** The ID of the device associated with the connection. */
        DeviceID deviceID;
        /** Active connection ID. */
        ConnectionID connectionID;
        /** Connection type. */
        ConnectionType type;
        /** Counter for the number of commands/data received/sent via the connection. */
        StatCounter eventsCounter;
        /** Device data. */
        DeviceDataContainerPtr data;
        /** Last retrieved command ID. */
        CommandID lastCommandID;
        /** Instructions waiting for responses. */
        boost::unordered_map<CommandID, InstructionBasePtr> pendingInstructions;
        /** Object mutex. */
        boost::mutex dataMutex;
    };
    typedef boost::shared_ptr<ActiveConnectionData> ActiveConnectionDataPtr;
}

#endif	/* NETWORK_MANAGEMENT_CONTAINERS_H */
