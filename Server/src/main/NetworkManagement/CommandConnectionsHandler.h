/* 
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

#ifndef COMMANDCONNECTIONSHANDLER_H
#define	COMMANDCONNECTIONSHANDLER_H

#include <atomic>
#include <string>
#include <queue>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Security.h"
#include "../Utilities/FileLogger.h"

#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Crypto/Handlers.h"
#include "../SecurityManagement/SecurityManager.h"

#include "../SessionManagement/SessionManager.h"
#include "../SessionManagement/Types/Types.h"
#include "../SessionManagement/Types/Exceptions.h"

#include "../DatabaseManagement/DatabaseManager.h"
#include "../DatabaseManagement/Types/Types.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"

#include "Types/Types.h"
#include "Types/Containers.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

#include "Connections/Connection.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

//Networking
using NetworkManagement_Connections::ConnectionPtr;
using NetworkManagement_Types::RawConnectionID;
using NetworkManagement_Types::ConnectionID;
using NetworkManagement_Types::ConnectionSetupState;
using NetworkManagement_Types::StatCounter;

//Common
using Common_Types::LogSeverity;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;

//Database
using DatabaseManagement_Containers::DeviceDataContainerPtr;

//Security
using SecurityManagement_Crypto::SaltGenerator;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;
using SecurityManagement_Crypto::RSACryptoDataContainerPtr;
using SecurityManagement_Crypto::ECDHCryptoDataContainer;
using SecurityManagement_Crypto::ECDHCryptoDataContainerPtr;
using SecurityManagement_Crypto::AsymmetricCryptoHandler;
using SecurityManagement_Crypto::AsymmetricCryptoHandlerPtr;
using SecurityManagement_Crypto::SymmetricCryptoHandler;
using SecurityManagement_Crypto::SymmetricCryptoHandlerPtr;
using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::EMPTY_PLAINTEXT_DATA;
using SecurityManagement_Types::CiphertextData;
using SecurityManagement_Types::AsymmetricCipherType;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::KeyExchangeType;
using SecurityManagement_Types::SymmetricCryptoDataGenerationRequest;
using SecurityManagement_Types::ECDHSymmetricCryptoDataGenerationRequest;
using SecurityManagement_Types::LocalPeerAuthenticationEntry;

//Sessions
using SyncServer_Core::SessionManager;
using SessionManagement_Types::InternalSessionID;
using SessionManagement_Types::INVALID_INTERNAL_SESSION_ID;

namespace NetworkManagement_Handlers
{
    /**
     * Class for managing command connections, including data encryption and decryption.
     * 
     * <code>onConnectionEstablished</code> event is fired when a connection has successfully completed its key exchange and authentication process.
     * <code>onConnectionEstablishmentFailed</code> event is fired when a connection has failed to complete its key exchange and/or authentication process.
     * <code>onCommandDataReceived</code> event is fired when new command data is received from a remote peer.
     */
    class CommandConnectionsHandler : public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure for holding <code>CommandConnectionsHandler</code> configuration data. */
            struct CommandConnectionsHandlerParameters
            {
                /** Security manager reference. */
                SecurityManager & securityManager;
                /** Session manage reference. */
                SessionManager & sessionManager;
                /** Local peer ID (as expected by other clients). */
                DeviceID localPeerID;
                /** Local peer asymmetric cryptographic data (as expected by other clients; if any). */
                AsymmetricCryptoHandlerPtr localPeerCrypto;
                /** Local peer ECDH cryptographic data (as expected by other clients; if any).*/
                ECDHCryptoDataContainerPtr localPeerECDHCryptoData;
                /** Default connection setup request signature size (in bytes). */
                RandomDataSize requestSignatureSize;
                /** Default key exchange type. */
                KeyExchangeType keyExchange;
            };
            
            /**
             * Creates a new command connection handler with the specified configuration.
             * 
             * @param params configuration data
             * @param dataRetrievalHandler function for retrieving device data
             * @param authDataRetrievalHandler function for retrieving local peer authentication data
             * @param parent parent <code>NetworkManager</code>
             * @param debugLogger logger for debugging, if any
             */
            CommandConnectionsHandler(
                    const CommandConnectionsHandlerParameters & params,
                    std::function<DeviceDataContainerPtr (const DeviceID)> dataRetrievalHandler,
                    std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authDataRetrievalHandler,
                    Securable & parent,
                    Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Disconnects all established and pending connections, and performs clean up.
             */
            ~CommandConnectionsHandler();
            
            CommandConnectionsHandler() = delete;
            CommandConnectionsHandler(const CommandConnectionsHandler&) = delete;
            CommandConnectionsHandler& operator=(const CommandConnectionsHandler&) = delete;
            
            std::string getSourceName() const override
            {
                return "CommandConnectionsHandler";
            }
            
            bool registerLoggingHandler(const std::function<void(LogSeverity, const std::string &)> handler) override
            {
                if(!dbLogHandler)
                {
                    dbLogHandler = handler;
                    return true;
                }
                else
                {
                    logMessage(LogSeverity::Error, "(CommandConnectionsHandler) > The database logging handler is already set.");
                    return false;
                }
            }
            
            /**
             * Starts the management process of the specified local connection.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             * @param deviceID device associated with the connection
             */
            void manageLocalConnection(ConnectionPtr connection, const ConnectionID connectionID, const DeviceID deviceID);
            
            /**
             * Starts the management process of the specified remote connection.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             */
            void manageRemoteConnection(ConnectionPtr connection, const ConnectionID connectionID);
            
            /**
             * Encrypts and sends the supplied data to the specified device.
             * 
             * Note: The caller can safely dispose of the plaintext data after the function returns.
             * 
             * @param deviceID the ID of the device to send the data to
             * @param plaintextData the plaintext data to be encrypted and sent
             */
            void sendData(const DeviceID deviceID, const PlaintextData & plaintextData);
            
            /**
             * Closes the established connection for the specified device.
             * 
             * @param deviceID the ID of the device to be disconnected
             */
            void closeEstablishedConnection(const DeviceID deviceID);
            
            /**
             * Attaches the supplied handler to the <code>onConnectionEstablished</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectionEstablishedEventAttach(
                std::function<
                    void (const DeviceID,
                          const ConnectionID)
                >
                function)
            {
                return onConnectionEstablished.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onConnectionEstablishmentFailed</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectionEstablishmentFailedEventAttach(
                std::function<
                    void (const DeviceID,
                          const ConnectionID)
                >
                function)
            {
                return onConnectionEstablishmentFailed.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onCommandDataReceived</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onCommandDataReceivedEventAttach(
                std::function<
                    void (const DeviceID,
                          const PlaintextData &)
                >
                function)
            {
                return onCommandDataReceived.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onEstablishedConnectionClosed</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onEstablishedConnectionClosedEventAttach(
                std::function<
                    void (const DeviceID,
                          const ConnectionID)
                >
                function)
            {
                return onEstablishedConnectionClosed.connect(function);
            }
            
        private:
            /** Structure for holding pending connection data for unknown devices. */
            struct UnknownPendingConnectionData
            {
                /** Connection pointer. */
                ConnectionPtr connection;
                /** ID of device associated with the connection (if any). */
                DeviceID deviceID;
                /** 'onDataReceived' event handler connection. */
                boost::signals2::connection onDataReceivedEventConnection;
                /** 'onDisconnect' event handler connection. */
                boost::signals2::connection onDisconnectEventConnection;
                /** 'onWriteResultReceived' event handler connection. */
                boost::signals2::connection onWriteResultReceivedEventConnection;
                /** Connection data mutex. */
                boost::mutex connectionDataMutex;
            };
            typedef boost::shared_ptr<UnknownPendingConnectionData> UnknownPendingConnectionDataPtr;
            
            /** Structure for holding pending connection data for known devices. */
            struct PendingConnectionData
            {
                /** Asymmetric cryptographic handler (if any; based on system config). */
                AsymmetricCryptoHandlerPtr asymCrypto;
                /** Symmetric cryptographic handler (if set). */
                SymmetricCryptoHandlerPtr symCrypto;
                /** ECDH cryptographic data container (if any; based on system config). */
                ECDHCryptoDataContainerPtr ecdhCrypto;
                /** Latest pending connection state. */
                ConnectionSetupState state;
                /** Symmetric cipher type selected for CEK. */
                SymmetricCipherType cipher;
                /** Symmetric cipher mode selected for CEK. */
                AuthenticatedSymmetricCipherModeType mode;
                /** Last request signature data (for verifications; if set). */
                PlaintextData requestSignatureData;
                /** Session ID associated with the connection (if set). */
                InternalSessionID sessionID;
                /** Data for the device associated with the connection. */
                DeviceDataContainerPtr deviceData;
                /** Pointer to the last pending data sent (if any). */
                CiphertextData * lastPendingData;
                /** Connection ID. */
                ConnectionID connectionID;
                /** Connection pointer. */
                ConnectionPtr connection;
                /** 'onDataReceived' event handler connection. */
                boost::signals2::connection onDataReceivedEventConnection;
                /** 'onDisconnect' event handler connection. */
                boost::signals2::connection onDisconnectEventConnection;
                /** 'onWriteResultReceived' event handler connection. */
                boost::signals2::connection onWriteResultReceivedEventConnection;
                /** Connection data mutex. */
                boost::mutex connectionDataMutex;
            };
            typedef boost::shared_ptr<PendingConnectionData> PendingConnectionDataPtr;
            
            /** Structure for holding established connection data. */
            struct EstablishedConnectionData
            {
                /** Connection ID. */
                ConnectionID connectionID;
                /** Raw connection ID. */
                RawConnectionID rawID;
                /** Connection pointer. */
                ConnectionPtr connection;
                /** Data for the device associated with the connection. */
                DeviceDataContainerPtr deviceData;
                /** Session ID associated with the connection. */
                InternalSessionID sessionID;
                /** Denotes whether the connection is bridged with another one. */
                bool bridged;
                /** Bridged connection pointer. */
                ConnectionPtr bridgeTarget;
                /** Content encryption handler. */
                SymmetricCryptoHandlerPtr cryptoHandler;
                /** Queue of pointer to data awaiting to receive send confirmations. */
                std::queue<const CiphertextData *> pendingData;
                /** 'onDataReceived' event handler connection. */
                boost::signals2::connection onDataReceivedEventConnection;
                /** 'onDisconnect' event handler connection. */
                boost::signals2::connection onDisconnectEventConnection;
                /** 'onWriteResultReceived' event handler connection. */
                boost::signals2::connection onWriteResultReceivedEventConnection;
                /** Connection data mutex. */
                boost::mutex connectionDataMutex;
            };
            typedef boost::shared_ptr<EstablishedConnectionData> EstablishedConnectionDataPtr;
            
            Utilities::FileLoggerPtr debugLogger;                                //logger for debugging
            std::function<void (LogSeverity, const std::string &)> dbLogHandler; //database log handler
            std::function<DeviceDataContainerPtr (const DeviceID)> deviceDataRetrievalHandler;
            std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authenticationDataRetrievalHandler;
            
            std::atomic<bool> active;           //denotes whether the handler is in an active state or not
            
            Securable & parentNetworkManager;   //parent
            SecurityManager & securityManager;  //security manager for making crypto requests
            SessionManager & sessionManager;    //session manager for creating new device sessions
            
            boost::mutex connectionDataMutex;
            boost::unordered_map<DeviceID, EstablishedConnectionDataPtr> establishedConnections;            //map of all established connections
            boost::unordered_map<DeviceID, PendingConnectionDataPtr> pendingConnections;                    //map of all connections waiting to be established
            boost::unordered_map<ConnectionID, UnknownPendingConnectionDataPtr> unknownPendingConnections;  //map of all pending connections from unknown sources
            
            DeviceID localPeerID;                               //default local peer ID
            AsymmetricCryptoHandlerPtr localPeerCrypto;         //local peer asymmetric cryptographic data (if any))
            ECDHCryptoDataContainerPtr localPeerECDHCryptoData; //local peer ECDH cryptographic data (if any)
            
            RandomDataSize requestSignatureSize;    //default connection request signature size (in bytes)
            KeyExchangeType keyExchange;            //default key exchange type
            
            //Events
            boost::signals2::signal<void (const DeviceID, const ConnectionID)> onConnectionEstablished;
            boost::signals2::signal<void (const DeviceID, const ConnectionID)> onConnectionEstablishmentFailed;
            boost::signals2::signal<void (const DeviceID, const PlaintextData)> onCommandDataReceived;
            boost::signals2::signal<void (const DeviceID, const ConnectionID)> onEstablishedConnectionClosed;
            
            //Stats
            std::atomic<StatCounter> sendRequestsMade{0};           //outgoing data
            std::atomic<StatCounter> sendRequestsConfirmed{0};      //outgoing data
            std::atomic<StatCounter> sendRequestsFailed{0};         //outgoing data
            std::atomic<StatCounter> totalDataObjectsReceived{0};   //incoming data
            std::atomic<StatCounter> validDataObjectsReceived{0};   //incoming data
            std::atomic<StatCounter> invalidDataObjectsReceived{0}; //incoming data
            std::atomic<StatCounter> connectionsEstablished{0};     //number of connections successfully established
            std::atomic<StatCounter> connectionsFailed{0};          //number of connections that could not be established
            
            /**
             * Verifies the supplied peer password and attempts to create a session.
             * 
             * @param peerID the ID of the peer
             * @param rawPassword the peer's password
             * @return the newly created session's ID
             */
            InternalSessionID verifyPeerPasswordAndCreateSession(
                const DeviceID peerID, const PlaintextData & rawPassword)
            {
                return sessionManager.openSession(peerID, rawPassword, SessionType::COMMAND, false);
            }
            
            /**
             * Verifies the supplied peer password and attempts to create a session.
             * 
             * @param peerID the ID of the peer (as a string)
             * @param rawPassword the peer's password
             * @return the newly created session's ID
             */
            InternalSessionID verifyPeerPasswordAndCreateSession(
                const std::string peerID, const PlaintextData & rawPassword)
            {
                return verifyPeerPasswordAndCreateSession(Convert::toDeviceID(peerID), rawPassword);
            }
            
            /**
             * Creates an entry for the specified device in the pending connections data structure
             * with all the necessary data for performing the connection establishment process.
             * 
             * @param device the ID of the device for which to create the pending data
             * @param connectionID the ID of the connection to the remote peer
             * @return a pointer to the newly created pending data
             */
            PendingConnectionDataPtr createPendingConnectionData(
                const DeviceID device, const ConnectionID connectionID);
            
            /**
             * Retrieves the pending connection data for the specified device.
             * 
             * @param device the target device ID
             * @return a pointer to the requested data
             */
            PendingConnectionDataPtr getPendingConnectionData(const DeviceID device);
            
            /**
             * Retrieves the data associated with the specified connection (for a currently unknown device).
             * 
             * @param id the ID of the connection
             * @return a pointer to the requested data
             */
            UnknownPendingConnectionDataPtr getUnknownPendingConnectionData(const ConnectionID id);
            
            /**
             * Retrieves the data associated with the specified device ID (for an established connection).
             * 
             * @param deviceID the target device ID
             * @return a pointer to the requested data
             */
            EstablishedConnectionDataPtr getEstablishedConnectionData(const DeviceID deviceID);
            
            /**
             * Generates encrypted connection request data for the specified peer.
             * 
             * The returned data can be sent over the network.
             * 
             * @param remotePeerData connection data for the remote peer
             * @throw runtime_error if the remote peer is a server but no authentication data for it was found
             *                      or if serialization fails
             * @return the data to be sent to the remote peer
             */
            CiphertextData * generateConnectionRequestData(PendingConnectionDataPtr remotePeerData);
            
            /**
             * Generates encrypted connection response data based on the supplied encrypted request.
             * 
             * The returned data can be sent over the network.
             * 
             * @param encryptedRequest the encrypted request data received from the remote peer
             * @param connectionID the ID of the connection that was established by the remote peer
             * @throw runtime_error if the connection request could not be validated
             *                      or if the remote peer is a server but no authentication data for it was found
             * @return the data to be sent to the remote peer
             */
            CiphertextData * generateConnectionResponseDataFromRequest(
                const CiphertextData & encryptedRequest, const ConnectionID connectionID);
            
            /**
             * Verifies the supplied connection response data for the specified peer.
             * 
             * After a successful call to this function, the connection can be considered
             * as established and normal command exchange can begin.
             * 
             * @param encryptedResponseData the encrypted data to be verified
             * @param remotePeerData connection data for the remote peer
             */
            void verifyConnectionResponseData(
                const CiphertextData & encryptedResponseData,
                PendingConnectionDataPtr remotePeerData);
            
            /**
             * 'onDisconnect' event handler for pending local connections.
             * 
             * @param rawID raw connection ID
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onDisconnectHandler_PendingLocalConnections(
                RawConnectionID rawID, const DeviceID deviceID,
                const ConnectionID connectionID);
            
            /**
             * 'onDataReceived' event handler for pending local connections.
             * 
             * @param data received data
             * @param remaining remaining data (should be 0)
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             * @param connection associated connection pointer
             */
            void onDataReceivedHandler_PendingLocalConnections(
                ByteData data, PacketSize remaining, const DeviceID deviceID,
                const ConnectionID connectionID, ConnectionPtr connection);
            
            /**
             * 'onWriteResultReceived' event handler for pending local connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param deviceID the associated device ID
             * @param connectionID associated connection ID
             * @param connection associated connection pointer
             */
            void onWriteResultReceivedHandler_PendingLocalConnections(
                bool received, const DeviceID deviceID,
                const ConnectionID connectionID, ConnectionPtr connection);
            
            /**
             * 'onDisconnect' event handler for pending remote connections.
             * 
             * @param rawID raw connection ID
             * @param connectionID associated connection ID
             */
            void onDisconnectHandler_PendingRemoteConnections(
                RawConnectionID rawID, const ConnectionID connectionID);
            
            /**
             * 'onDataReceived' event handler for pending remote connections.
             * 
             * @param data received data
             * @param remaining remaining data (should be 0)
             * @param connectionID associated connection ID
             * @param connection associated connection pointer
             */
            void onDataReceivedHandler_PendingRemoteConnections(
                ByteData data, PacketSize remaining,
                const ConnectionID connectionID, ConnectionPtr connection);
            
            /**
             * 'onWriteResultReceived' event handler for pending remote connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param connectionID associated connection ID
             */
            void onWriteResultReceivedHandler_PendingRemoteConnections(
                bool received, const ConnectionID connectionID);
            
            /**
             * 'onDisconnect' event handler for established connections.
             * 
             * @param rawID raw connection ID
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onDisconnectHandler_EstablishedConnections(
                RawConnectionID rawID, const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * 'onDataReceived' event handler for established connections.
             * 
             * @param encryptedData received data
             * @param remaining remaining data (should be 0)
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onDataReceivedHandler_EstablishedConnections(
                ByteData encryptedData, PacketSize remaining,
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * 'onWriteResultReceived' event handler for established connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onWriteResultReceivedHandler_EstablishedConnections(
                bool received, const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Terminates the connection for the specified connection ID.
             * 
             * @param connectionID the ID of the affected connection
             * @throw runtime_error if the specified connection cannot be found
             */
            void terminateConnection(const ConnectionID connectionID);
            
            /**
             * Terminates the connection for the specified device ID.
             * 
             * @param deviceID ID of the affected device
             */
            void terminateConnection(const DeviceID deviceID);
            
            /**
             * Logs the specified message, if the database log handler is set.
             * 
             * Note: If a debugging file logger is assigned, the message is sent to it.
             * 
             * @param severity the severity associated with the message/event
             * @param message the message to be logged
             */
            void logMessage(LogSeverity severity, const std::string & message) const
            {
                if(dbLogHandler)
                    dbLogHandler(severity, message);
                
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "CommandConnectionsHandler " + message);
            }
    };
}

#endif	/* COMMANDCONNECTIONSHANDLER_H */

