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

#ifndef PACKETS_H
#define	PACKETS_H

#include <stdexcept>
#include <vector>
#include "Types.h"

using NetworkManagement_Types::PacketSize;

namespace NetworkManagement_Types
{
    /**
     * Class for representing basic connection requirements.\n
     * 
     * The request is always sent by the endpoint that initiated the connection
     * and must be validated by the receiving endpoint before any further communication
     * is allowed.
     */
    class ConnectionRequest
    {
        public:
            /** Request packet length, when converted to bytes. */
            static const unsigned int BYTE_LENGTH = 2;
            
            //TODO
            /** Current request packet version.\n
             * v.1. -> (CURRENT) Supports CLIENT and SERVER peer types; supports COMMAND and DATA connection types;\n
             * v.2. -> (FUTURE) Adds support for DEBUG mode when connecting; only if both parties are in that mode will a connection be allowed
             * v.3. -> (FUTURE) Adds support for GATEWAY and UI peer types; adds support for GATEWAY and UI connection types; */
            static const unsigned int VERSION = 1;
            
            /** Peer type of the endpoint sending the request. */
            PeerType senderPeerType;
            
            /** Type of connection to be initiated. */
            ConnectionType connectionType;

            /**
             * Attempts to convert the supplied byte data to a valid <code>ConnectionRequest</code> object.
             * 
             * @param data bytes to be converted
             * @return the newly built and validated object
             * @throws <code>std::invalid_argument</code>, if the supplied data cannot be converted to a valid object
             */
            static ConnectionRequest fromBytes(const std::vector<BYTE> & data)
            {
                ConnectionRequest result;
                
                if(data.size() != ConnectionRequest::BYTE_LENGTH)
                    throw std::invalid_argument("ConnectionRequest::fromBytes() > Unexpected data length encountered.");
                
                switch(data[0])
                {
                    case 'C': result.senderPeerType = PeerType::CLIENT; break;
                    case 'S': result.senderPeerType = PeerType::SERVER; break;
                    default: throw std::invalid_argument("ConnectionRequest::fromBytes() > Unexpected senderPeerType encountered.");
                }
                
                switch(data[1])
                {
                    case 'C': result.connectionType = ConnectionType::COMMAND; break;
                    case 'D': result.connectionType = ConnectionType::DATA; break;
                    default: throw std::invalid_argument("ConnectionRequest::fromBytes() > Unexpected connectionType encountered.");
                }
                
                if(!result.isValid())
                    throw std::invalid_argument("ConnectionRequest::fromBytes() > Cannot convert invalid object.");
                
                return result;
            }
            
            /**
             * Converts the request to bytes.
             * 
             * @return the byte representation of the request
             * @throws <code>std::invalid_argument</code>, if the conversion cannot be done
             */
            std::vector<BYTE> toBytes() const
            {
                if(!isValid())
                    throw std::invalid_argument("ConnectionRequest::toBytes() > Cannot convert invalid object.");
                
                std::vector<BYTE> result;
                
                switch(senderPeerType)
                {
                    case PeerType::CLIENT: result.push_back('C'); break;
                    case PeerType::SERVER: result.push_back('S'); break;
                    default: throw std::invalid_argument("ConnectionRequest::toBytes() > Cannot convert invalid senderPeerType.");
                }
                
                switch(connectionType)
                {
                    case ConnectionType::COMMAND:           result.push_back('C'); break;
                    case ConnectionType::DATA:              result.push_back('D'); break;
                    default: throw std::invalid_argument("ConnectionRequest::toBytes() > Cannot convert invalid connectionType.");
                }
                
                return result;
            }
            
            /**
             * Validates the request.
             * 
             * @return <code>true</code>, if the request is valid
             */
            bool isValid() const
            {
                if(senderPeerType != PeerType::CLIENT && senderPeerType != PeerType::SERVER)
                    return false;
                
                if(connectionType != ConnectionType::COMMAND && connectionType != ConnectionType::DATA)
                    return false;
                                
                return true;
            }
    };
    
    /**
     * Class for representing data transmission information.\n
     * 
     * The header is always sent prior to any data and informs the receiving endpoint
     * of what to expect.
     */
    class HeaderPacket
    {
        public:
            /** Header packet length, when converted to bytes. */
            static const std::size_t BYTE_LENGTH = sizeof(PacketSize);
            
            /**
             * Current packet version.\n
             * v.1. -> (CURRENT) Supports payload size;
             * v.2. -> (FUTURE) n/a
             */
            static const unsigned int VERSION = 1;
            
            /** Size of the data that is to be sent after the header (in bytes). */
            PacketSize payloadSize;
            
            /**
             * Attempts to convert the supplied byte data to a <code>HeaderPacket</code> object.
             * 
             * Note: Network byte order is expected for the input data.
             * 
             * @param data bytes to be converted
             * @return the newly built object
             * @throws <code>std::invalid_argument</code>, if the supplied data cannot be converted
             */
            static HeaderPacket fromNetworkBytes(const std::vector<BYTE> & data)
            {
                if(HeaderPacket::BYTE_LENGTH != data.size())
                    throw std::invalid_argument("HeaderPacket::fromNetworkBytes() > Unexpected data length encountered.");

                HeaderPacket result;
                
                BYTE nPayloadSize[data.size()];
                
                for(std::size_t i = 0; i < data.size(); i++)
                    nPayloadSize[i] = data[i];
                
                result.payloadSize = ntohl(*static_cast<PacketSize*>(static_cast<void*>(nPayloadSize)));
                
                return result;
            }

            /**
             * Converts the header to bytes.
             * 
             * Note: Network byte order is used for the output data.
             * 
             * @return the byte representation of the request
             * @throws <code>std::invalid_argument</code>, if the conversion cannot be done
             */
            std::vector<BYTE> toNetworkBytes()
            {
                auto nPayloadSize = htonl(payloadSize);
                
                if(sizeof nPayloadSize != HeaderPacket::BYTE_LENGTH)
                    throw std::invalid_argument("HeaderPacket::toNetworkBytes() > The converted payload size does not have the expected byte length.");
                
                std::vector<BYTE> result(HeaderPacket::BYTE_LENGTH);
                BYTE* rawResult = static_cast<BYTE*>(static_cast<void*>(&nPayloadSize));
                
                for(std::size_t i = 0; i < (sizeof nPayloadSize); i++)
                    result[i] = rawResult[i];
                
                return result;
            }
            
            /**
             * Converts the header to bytes and places the result in the supplied container.
             * 
             * Note: Network byte order is used for the output data.
             * 
             * @param target the container to be used for storing the result
             * @throws <code>std::invalid_argument</code>, if the conversion cannot be done
             */
            void toNetworkBytes(std::vector<BYTE> & target)
            {
                if(target.size() != HeaderPacket::BYTE_LENGTH)
                    throw std::invalid_argument("HeaderPacket::toNetworkBytes() > The target container does not have the expected storage capacity.");
                
                auto nPayloadSize = htonl(payloadSize);
                
                if(sizeof nPayloadSize != HeaderPacket::BYTE_LENGTH)
                    throw std::invalid_argument("HeaderPacket::toNetworkBytes() > The converted payload size does not have the expected byte length.");
                
                BYTE* rawResult = static_cast<BYTE*>(static_cast<void*>(&nPayloadSize));
                
                for(std::size_t i = 0; i < (sizeof nPayloadSize); i++)
                    target[i] = rawResult[i];
            }
    };
}

#endif	/* PACKETS_H */

