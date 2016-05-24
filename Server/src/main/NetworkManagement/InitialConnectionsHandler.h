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

#ifndef INITIALCONNECTIONSHANDLER_H
#define	INITIALCONNECTIONSHANDLER_H

#include <atomic>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/FileLogger.h"

#include "../DatabaseManagement/Types/Types.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"

#include "Types/Types.h"
#include "Types/Containers.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Crypto/SaltGenerator.h"
#include "../SecurityManagement/Crypto/PasswordGenerator.h"
#include "../SecurityManagement/Crypto/Containers.h"
#include "../SecurityManagement/Crypto/Handlers.h"

#include "Connections/Connection.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

//Protocols
#include "../../../external/protobuf/BaseComm.pb.h"
#include "Protocols/Utilities.h"

//Networking
using NetworkManagement_Connections::ConnectionPtr;
using NetworkManagement_Types::RawConnectionID;
using NetworkManagement_Types::ConnectionID;
using NetworkManagement_Types::TransientConnectionID;
using NetworkManagement_Types::ConnectionSetupState;
using NetworkManagement_Types::INVALID_TRANSIENT_CONNECTION_ID;
using NetworkManagement_Types::DeviceIPSettings;
using NetworkManagement_Types::NewDeviceConnectionParameters;
using NetworkManagement_Types::PendingInitConnectionConfigPtr;
using NetworkManagement_Types::StatCounter;

//Common
using Common_Types::LogSeverity;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;
using Common_Types::ByteData;
using Common_Types::EMPTY_BYTE_DATA;

//Database
using DatabaseManagement_Containers::DeviceDataContainerPtr;

//Security
using SyncServer_Core::SecurityManager;
using SecurityManagement_Crypto::AsymmetricCryptoHandlerPtr;
using SecurityManagement_Crypto::ECDHCryptoDataContainerPtr;
using SecurityManagement_Crypto::SaltGenerator;
using SecurityManagement_Crypto::PasswordGenerator;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;
using SecurityManagement_Crypto::SymmetricCryptoHandler;
using SecurityManagement_Crypto::SymmetricCryptoHandlerPtr;
using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::EMPTY_PLAINTEXT_DATA;
using SecurityManagement_Types::CiphertextData;
using SecurityManagement_Types::MixedData;
using SecurityManagement_Types::RandomData;
using SecurityManagement_Types::LocalPeerAuthenticationEntry;

//Protocols
using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::InitConnectionAdditionalData;
using NetworkManagement_Protocols::InitConnectionSetupRequest;
using NetworkManagement_Protocols::InitConenctionSetupResponse;

namespace NetworkManagement_Handlers
{
    /**
     * Class for managing initial connections, including data encryption and decryption.
     * 
     * - <code>onSetupCompleted</code> event is fired when a connection has
     *   successfully completed the initial setup and key exchange process.
     * 
     * - <code>onSetupFailed</code> event is fired when a connection has
     *   failed to complete the initial setup and key exchange process.
     */
    class InitialConnectionsHandler : public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure for holding <code>InitialConnectionsHandlerParameters</code> configuration data. */
            struct InitialConnectionsHandlerParameters
            {
                /** Security manager reference. */
                SecurityManager & securityManager;
                /** Default connection setup request signature size (in bytes). */
                RandomDataSize requestSignatureSize;
                /** Default key exchange type. */
                KeyExchangeType keyExchange;
                /** Default length when generating new random passwords. */
                unsigned int defaultRandomPasswordSize;
                /** Maximum number of random password generation retries. */
                unsigned int maxRandomPasswordAttempts;
                /** Local peer ID (as expected by other clients). */
                DeviceID localPeerID;
                /** Raw local peer public key (shared with remote peers). */
                ByteData localPeerPublicKey;
                /** IP configuration for the local peer (shared with remote peers). */
                DeviceIPSettings localIPSettings;
            };
            
            /**
             * Creates a new initial connection handler with the specified configuration.
             * 
             * @param params configuration data
             * @param parent parent <code>NetworkManager</code>
             * @param cfgRetrievalHandler function reference for device configuration retrieval
             * @param authDataUpdateHandler function reference for updating local peer authentication data
             * @param debugLogger logger for debugging, if any
             */
            InitialConnectionsHandler(
                    const InitialConnectionsHandlerParameters & params,
                    Securable & parent,
                    std::function<PendingInitConnectionConfigPtr (const TransientConnectionID)> cfgRetrievalHandler,
                    std::function<void (const DeviceID & deviceID, const LocalPeerAuthenticationEntry &)> authDataAdditionHandler,
                    Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Disconnects all connections, and performs clean up.
             */
            ~InitialConnectionsHandler();

            InitialConnectionsHandler() = delete;
            InitialConnectionsHandler(const InitialConnectionsHandler&) = delete;
            InitialConnectionsHandler& operator=(const InitialConnectionsHandler&) = delete;
            
            std::string getSourceName() const override
            {
                return "InitialConnectionsHandler";
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
                    logMessage(LogSeverity::Error, "(InitialConnectionsHandler) >"
                            " The database logging handler is already set.");
                    return false;
                }
            }
            
            /**
             * Starts the management process of the specified local connection.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             * @param remotePeerData configuration data for the remote peer
             */
            void manageLocalConnection(
                ConnectionPtr connection, const ConnectionID connectionID,
                PendingInitConnectionConfigPtr remotePeerData);
            
            /**
             * Starts the management process of the specified remote connection.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             */
            void manageRemoteConnection(
                ConnectionPtr connection, const ConnectionID connectionID);
            
            /**
             * Attaches the supplied handler to the <code>onSetupCompleted</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onSetupCompletedEventAttach(
                std::function<
                    void (const ConnectionID,
                          const DeviceID,
                          const TransientConnectionID,
                          const NewDeviceConnectionParameters &)
                >
                function)
            {
                return onSetupCompleted.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onSetupFailed</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onSetupFailedEventAttach(
                std::function<
                    void (const ConnectionID,
                          const TransientConnectionID)
                >
                function)
            {
                return onSetupFailed.connect(function);
            }
            
        private:
            /** Structure for holding connection data. */
            struct ConnectionData
            {
                /** Last connection state. */
                ConnectionSetupState state;
                /** Password to be used for the PBKD function. */
                std::string initPassword;
                /** Symmetric cryptographic handler (if set). */
                SymmetricCryptoHandlerPtr symCrypto;
                /** Symmetric cipher type. */
                SymmetricCipherType cipher;
                /** Symmetric cipher mode. */
                AuthenticatedSymmetricCipherModeType mode;
                /** The number of iterations for the PBKD function that generates the sym keys. */
                unsigned int iterationsCount;
                /** Last request signature data (for verifications; if set). */
                PlaintextData requestSignatureData;
                /** Pointer to the last pending data sent (if any). */
                CiphertextData * lastPendingData;
                /** Connection pointer. */
                ConnectionPtr connection;
                /** New device ID associated with the remote peer. */
                DeviceID newPeerID;
                /** The type of the remote peer */
                PeerType remotePeerType;
                /** Password supplied to remote server (if remote peer is server). */
                std::string serverPassword;
                /** New device connection parameters (if set). */
                NewDeviceConnectionParameters deviceParams;
                /** New server authentication data (if set). */
                LocalPeerAuthenticationEntry authData;
                /** 'onDataReceived' event handler connection. */
                boost::signals2::connection onDataReceivedEventConnection;
                /** 'onDisconnect' event handler connection. */
                boost::signals2::connection onDisconnectEventConnection;
                /** 'onWriteResultReceived' event handler connection. */
                boost::signals2::connection onWriteResultReceivedEventConnection;
                /** Connection data mutex. */
                boost::mutex connectionDataMutex;
            };
            typedef boost::shared_ptr<ConnectionData> ConnectionDataPtr;
            
            /** Structure for holding unknown connection data. */
            struct UnknownConnectionData
            {
                /** Connection pointer. */
                ConnectionPtr connection;
                /** Last connection state. */
                ConnectionSetupState state;
                /** Transient connection ID, if available. */
                TransientConnectionID transientID;
                /** Pointer to the last pending data sent (if any). */
                CiphertextData * lastPendingData;
                /** New device ID associated with the remote peer. */
                DeviceID newPeerID;
                /** New device connection parameters (if set). */
                NewDeviceConnectionParameters deviceParams;
                /** New server authentication data (if set). */
                LocalPeerAuthenticationEntry authData;
                /** 'onDataReceived' event handler connection. */
                boost::signals2::connection onDataReceivedEventConnection;
                /** 'onDisconnect' event handler connection. */
                boost::signals2::connection onDisconnectEventConnection;
                /** 'onWriteResultReceived' event handler connection. */
                boost::signals2::connection onWriteResultReceivedEventConnection;
                /** Connection data mutex. */
                boost::mutex connectionDataMutex;
            };
            typedef boost::shared_ptr<UnknownConnectionData> UnknownConnectionDataPtr;
            
            Utilities::FileLogger * debugLogger;                                    //logger for debugging
            std::function<void (LogSeverity, const std::string &)> dbLogHandler;    //database log handler
            //handler for retrieving device configuration data from the parent network manager
            std::function<PendingInitConnectionConfigPtr (const TransientConnectionID)> deviceConfigRetrievalHandler;
            std::function<void (const DeviceID & deviceID, const LocalPeerAuthenticationEntry &)> authenticationDataAdditionHandler;
            
            Securable & parentNetworkManager;   //parent
            SecurityManager & securityManager;  //security manager for making crypto requests
            
            std::atomic<bool> active; //denotes whether the handler is in an active state or not
            
            boost::mutex connectionDataMutex;
            boost::unordered_map<TransientConnectionID, ConnectionDataPtr> connectionsData;     //holds local connections data
            boost::unordered_map<ConnectionID, UnknownConnectionDataPtr> unknownConnectionsData;//holds remote connections data
            
            RandomDataSize requestSignatureSize;    //default connection request signature size (in bytes)
            KeyExchangeType keyExchange;            //default key exchange type
            unsigned int defaultRandomPasswordSize; //default size for random password generation
            unsigned int maxRandomPasswordAttempts; //maximum number of attempts for random password generation
            
            DeviceID localPeerID;               //default local peer ID
            ByteData localPeerPublicKey;        //local peer asymmetric cryptographic data (shared with remote peers)
            DeviceIPSettings localIPSettings;   //IP settings for the local peer (shared with remote peers)
            
            //Events
            boost::signals2::signal<
                void (const ConnectionID,
                      const DeviceID,
                      const TransientConnectionID,
                      const NewDeviceConnectionParameters &)
            >
            onSetupCompleted;
            
            boost::signals2::signal<
                void (const ConnectionID,
                      const TransientConnectionID)
            >
            onSetupFailed;
            
            //Stats
            std::atomic<StatCounter> setupsCompleted;
            std::atomic<StatCounter> setupsFailed;
            
            /**
             * Creates an entry for the specified connection in the local connections
             * data structure with all the necessary data for performing the initial
             * setup process.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             * @param remotePeerData configuration data for the remote peer
             * @return a pointer to the newly created data
             */
            ConnectionDataPtr createConnectionData(
                ConnectionPtr connection, const ConnectionID connectionID,
                PendingInitConnectionConfigPtr remotePeerData);
            
            /**
             * Retrieves the data associated with the specified transient ID.
             * 
             * @param connectionID connection ID (for info/debugging purposes)
             * @param transientID the transient ID associated with the requested data
             * @return a pointer to the requested data
             */
            ConnectionDataPtr getConnectionData(
                const ConnectionID connectionID, const TransientConnectionID transientID);
            
            /**
             * Discards the connection data associated with the specified transient ID
             * and returns a shared pointer to it.
             * 
             * @param connectionID connection ID (for info/debugging purposes)
             * @param transientID the transient ID associated with the data to be discarded
             * @return a pointer to the discarded data
             */
            ConnectionDataPtr discardConnectionData(
                const ConnectionID connectionID, const TransientConnectionID transientID);
            
            /**
             * Creates an entry for the specified connection in the remote connections
             * data structure with all the necessary data for performing the initial
             * setup process.
             * 
             * @param connection connection pointer
             * @param connectionID the connection ID associated with the data to be created
             * @return a pointer to newly created data
             */
            UnknownConnectionDataPtr createUnknownConnectionData(
                ConnectionPtr connection, const ConnectionID connectionID);
            
            /**
             * Retrieves the data associated with the specified connection ID.
             * 
             * @param connectionID the connection ID associated with the requested data
             * @return a pointer to the requested data
             */
            UnknownConnectionDataPtr getUnknownConnectionData(const ConnectionID connectionID);
            
            /**
             * Discards the connection data associated with the specified connection ID
             * and returns a shared pointer to it.
             * 
             * @param connectionID the connection ID associated with the data to be discarded
             * @return a pointer to the discarded data
             */
            UnknownConnectionDataPtr discardUnknownConnectionData(const ConnectionID connectionID);
            
            /**
             * Attempts to generate a new server password.
             * 
             * @return the generated password
             */
            const std::string getNewServerPassword();
            
            /**
             * Generates connection request data for the specified peer.
             * 
             * The returned data can be sent over the network.
             * 
             * @param transientID the transient ID associated with the connection
             * @param remotePeerData connection data for the remote peer
             * @throw runtime_error if request data serialization fails
             * @return the data to be sent to the remote peer
             */
            MixedData * generateConnectionRequestData(
                const TransientConnectionID transientID, ConnectionDataPtr remotePeerData);
            
            /**
             * Generates connection response data based on the supplied request.
             * 
             * The returned data can be sent over the network.
             * 
             * @param setupRequest the request data received from the remote peer
             * @param connectionID the ID of the connection that was established by the remote peer
             * @throw runtime_error if any of the received data fails validation or if response serialization fails
             * @return the data to be sent to the remote peer
             */
            MixedData * generateConnectionResponseDataFromRequest(
                const MixedData & setupRequest, const ConnectionID connectionID);
            
            /**
             * Verifies the supplied connection response data for the specified peer.
             * 
             * @param responseData the data to be verified
             * @param connectionData connection data for the remote peer
             * @throw runtime_error if any of the received data fails validation
             */
            void verifyConnectionResponseData(
                const MixedData & responseData, ConnectionDataPtr connectionData);
            
            /**
             * 'onDisconnect' event handler for pending local connections.
             * 
             * @param rawID raw connection ID
             * @param connectionID associated connection ID
             * @param transientID associated transient ID
             */
            void onDisconnectHandler_PendingLocalConnections(
                RawConnectionID rawID, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
            /**
             * 'onDataReceived' event handler for pending local connections.
             * 
             * @param data received data
             * @param remaining remaining data (should be 0)
             * @param connectionID associated connection ID
             * @param transientID associated transient ID
             */
            void onDataReceivedHandler_PendingLocalConnections(
                ByteData data, PacketSize remaining, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
            /**
             * 
             * 'onWriteResultReceived' event handler for pending local connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param connectionID associated connection ID
             * @param transientID associated transient ID
             */
            void onWriteResultReceivedHandler_PendingLocalConnections(
                bool received, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
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
             */
            void onDataReceivedHandler_PendingRemoteConnections(
                ByteData data, PacketSize remaining, const ConnectionID connectionID);
            
            /**
             * 'onWriteResultReceived' event handler for pending remote connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param connectionID associated connection ID
             * @param transientID associated transient ID
             */
            void onWriteResultReceivedHandler_PendingRemoteConnections(
                bool received, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
            /**
             * Terminates the specified local connection.
             * 
             * @param connectionID the ID of the connection to be terminated
             * @param transientID the transient ID of the connection to be terminated
             */
            void terminateConnection(
                const ConnectionID connectionID, const TransientConnectionID transientID);
            
            /**
             * Terminates the specified remote connection.
             * 
             * @param connectionID the ID of the connection to be terminated
             */
            void terminateUnknownConnection(const ConnectionID connectionID);
            
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
                
                if(debugLogger != nullptr)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "InitialConnectionsHandler " + message);
            }
    };
}

#endif	/* INITIALCONNECTIONSHANDLER_H */

