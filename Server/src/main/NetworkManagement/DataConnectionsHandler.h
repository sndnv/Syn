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

#ifndef DATACONNECTIONSHANDLER_H
#define	DATACONNECTIONSHANDLER_H

#include <atomic>
#include <vector>
#include <string>
#include <queue>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/FileLogger.h"
#include "../Utilities/Compression/CompressionHandler.h"

#include "../DatabaseManagement/Types/Types.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"

#include "Types/Types.h"
#include "Types/Containers.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

#include "../SecurityManagement/Crypto/SaltGenerator.h"
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
using NetworkManagement_Types::PendingDataConnectionConfigPtr;
using NetworkManagement_Types::INVALID_TRANSIENT_CONNECTION_ID;
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
using SecurityManagement_Crypto::SaltGenerator;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;
using SecurityManagement_Crypto::SymmetricCryptoHandler;
using SecurityManagement_Crypto::SymmetricCryptoHandlerPtr;
using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::EMPTY_PLAINTEXT_DATA;
using SecurityManagement_Types::CiphertextData;
using SecurityManagement_Types::MixedData;
using SecurityManagement_Types::RandomData;
using SecurityManagement_Types::RandomDataSize;
using SecurityManagement_Types::LocalPeerAuthenticationEntry;

//Protocols
using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::DataConnectionSetupRequest;
using NetworkManagement_Protocols::DataConnectionSetupResponse;

//Compression
using Utilities::Compression::CompressionHandler;

namespace NetworkManagement_Handlers
{
    /**
     * Class for managing data connections, including data encryption/decryption & compression/decompression.
     * 
     * <code>onConnectionEstablished</code> event is fired when a connection has successfully completed its setup process.
     * <code>onConnectionEstablishmentFailed</code> event is fired when a connection has failed to complete its establishment process.
     * <code>onDataReceived</code> event is fired when new data is received from a remote peer.
     */
    class DataConnectionsHandler : public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure for holding <code>DataConnectionsHandler</code> configuration data. */
            struct DataConnectionsHandlerParameters
            {
                /** Local peer ID (as expected by other clients). */
                DeviceID localPeerID;
                /** Default connection setup request signature size (in bytes). */
                RandomDataSize requestSignatureSize;
                /** Maximum amount of data that can be processed as a single request. */
                BufferSize maxDataSize;
                /** Compression acceleration level; see <code>Utilities::Compression::CompressionHandler</code> for more details. */
                int compressionAccelerationLevel;
            };

            /**
             * Creates a new data connection handler with the specified configuration.
             * 
             * @param params configuration data
             * @param cfgRetrievalHandler function for retrieving pending device configuration data
             * @param authDataRetrievalHandler function for retrieving local peer authentication data
             * @param debugLogger logger for debugging, if any
             */
            DataConnectionsHandler(
                    const DataConnectionsHandlerParameters & params,
                    std::function<PendingDataConnectionConfigPtr (const DeviceID, const TransientConnectionID)> cfgRetrievalHandler,
                    std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authDataRetrievalHandler,
                    Utilities::FileLogger * debugLogger = nullptr);

            /**
             * Disconnects all connections and performs clean up.
             */
            ~DataConnectionsHandler();

            DataConnectionsHandler() = delete;
            DataConnectionsHandler(const DataConnectionsHandler&) = delete;
            DataConnectionsHandler& operator=(const DataConnectionsHandler&) = delete;

            std::string getSourceName() const
            {
                return "DataConnectionsHandler";
            }
            
            bool registerLoggingHandler(const std::function<void(LogSeverity, const std::string &)> handler)
            {
                if(!dbLogHandler)
                {
                    dbLogHandler = handler;
                    return true;
                }
                else
                {
                    logMessage(LogSeverity::Error, "(DataConnectionsHandler) > The database logging handler is already set.");
                    return false;
                }
            }
            
            /**
             * Starts the management process of the specified local connection.
             * 
             * Note: The connection setup always uses encryption and never uses compression.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             * @param config connection configuration data
             */
            void manageLocalConnection(
                ConnectionPtr connection, const ConnectionID connectionID,
                PendingDataConnectionConfigPtr config);

            /**
             * Starts the management process of the specified remote connection.
             * 
             * Note: The connection setup always uses encryption and never uses compression.
             * 
             * @param connection connection pointer
             * @param connectionID connection ID
             */
            void manageRemoteConnection(ConnectionPtr connection, const ConnectionID connectionID);
            
            /**
             * Sends the supplied data to the specified device on the specified connection.
             * 
             * Notes:
             * - Whether the supplied data is encrypted and/or compressed, depends on the initial connection configuration.
             * - The caller can safely dispose of the plaintext data after the function returns.
             * 
             * @param deviceID the ID of the device to send the data to
             * @param connectionID connection ID
             * @param plaintextData the plaintext data to be encrypted and/or compressed and sent
             * @throw invalid_argument if the supplied plaintext data is larger than the maximum allowed
             */
            void sendData(
                const DeviceID deviceID, const ConnectionID connectionID,
                const PlaintextData & plaintextData);
            
            /**
             * Closes the specified connection for the specified device.
             * 
             * @param deviceID the device to which the connection to be closed belongs
             * @param connectionID the connection that is to be closed
             */
            void closeConnection(const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Attaches the supplied handler to the <code>onConnectionEstablished</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectionEstablishedEventAttach(
                std::function<
                    void (const DeviceID,
                          const ConnectionID,
                          const TransientConnectionID)
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
                          const ConnectionID,
                          const TransientConnectionID)
                >
                function)
            {
                return onConnectionEstablishmentFailed.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onDataReceived</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onDataReceivedEventAttach(
                std::function<
                    void (const DeviceID,
                          const ConnectionID,
                          const PlaintextData &)
                >
                function)
            {
                return onDataReceived.connect(function);
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
            /** Structure for holding connection data. */
            struct ConnectionData
            {
                /** Transient connection ID, as agreed between the peers. */
                TransientConnectionID transientID;
                /** Data for the device associated with the connection. */
                DeviceDataContainerPtr deviceData;
                /** Connection pointer. */
                ConnectionPtr connection;
                /** Content encryption handler. */
                SymmetricCryptoHandlerPtr cryptoHandler;
                /** Last request signature data (for verifications; if set). */
                PlaintextData requestSignatureData;
                /** Last connection state. */
                ConnectionSetupState state;
                /** Denotes whether encryption has been enabled for the connection. */
                bool encryptionEnabled;
                /** Denotes whether compression has been enabled for the connection. */
                bool compressionEnabled;
                /** Pointer to the last pending data received (if any). */
                ByteData lastPendingReceivedData;
                /** Queue of pointer to data awaiting to receive send confirmations. */
                std::queue<const MixedData *> pendingSentData;
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
            
            Utilities::FileLogger * debugLogger;                                //logger for debugging
            std::function<void (LogSeverity, const std::string &)> dbLogHandler; //database log handler
            CompressionHandler compressor;                                      //data compression handler
            std::function<PendingDataConnectionConfigPtr (const DeviceID, const TransientConnectionID)> deviceConfigRetrievalHandler;
            std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authenticationDataRetrievalHandler;
            
            std::atomic<bool> active;               //denotes whether the handler is in an active state or not
            
            DeviceID localPeerID;                   //default local peer ID
            RandomDataSize requestSignatureSize;    //default connection request signature size (in bytes)
            BufferSize maxDataSize;                 //maximum amount of data that can be processed (sent/received)
            
            boost::mutex connectionDataMutex;
            boost::unordered_map<DeviceID, boost::unordered_map<ConnectionID, ConnectionDataPtr>> activeConnections;
            boost::unordered_map<ConnectionID, ConnectionDataPtr> pendingConnections;
            
            //Events
            boost::signals2::signal<void (const DeviceID, const ConnectionID, const TransientConnectionID)> onConnectionEstablished;
            boost::signals2::signal<void (const DeviceID, const ConnectionID, const TransientConnectionID)> onConnectionEstablishmentFailed;
            boost::signals2::signal<void (const DeviceID, const ConnectionID, const PlaintextData)> onDataReceived;
            boost::signals2::signal<void (const DeviceID, const ConnectionID)> onEstablishedConnectionClosed;
            
            //Stats
            std::atomic<StatCounter> sendRequestsMade;             //outgoing data
            std::atomic<StatCounter> sendRequestsConfirmed;        //outgoing data
            std::atomic<StatCounter> sendRequestsFailed;           //outgoing data
            std::atomic<StatCounter> totalDataObjectsReceived;     //incoming data
            std::atomic<StatCounter> validDataObjectsReceived;     //incoming data
            std::atomic<StatCounter> invalidDataObjectsReceived;   //incoming data
            std::atomic<StatCounter> connectionsEstablished;       //number of connections successfully established
            std::atomic<StatCounter> connectionsFailed;            //number of connections that could not be established
            
            /**
             * Creates a new connection data object based on the supplied data.
             * 
             * @param connectionID connection ID
             * @param config connection configuration data
             * @return connection data pointer
             */
            ConnectionDataPtr createConnectionData(
                const ConnectionID connectionID, PendingDataConnectionConfigPtr config,
                ConnectionPtr connection);
            
            /**
             * Updates the supplied connection data object with the supplied
             * configuration data and stores it as an active connection.
             * 
             * @param connectionID connection ID
             * @param config connection configuration data
             * @param pendingData existing pending connection data
             * @return connection data pointer
             */
            ConnectionDataPtr createConnectionData(
                const ConnectionID connectionID, PendingDataConnectionConfigPtr config,
                ConnectionDataPtr pendingData);
            
            /**
             * Retrieves the connection data associated with the specified device
             * and connection IDs.
             * 
             * @param deviceID device ID
             * @param connectionID connection ID
             * @return the retrieved connection data
             */
            ConnectionDataPtr getConnectionData(
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Discards the connection data associated with the specified device
             * and connection IDs, and returns a shared pointer to it.
             * 
             * @param deviceID device ID
             * @param connectionID connection ID
             * @throw runtime_error if no data is found for device and/ or connection
             * @return a shared pointer to the discarded data
             */
            ConnectionDataPtr discardConnectionData(
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Creates a new pending connection data object based on the supplied data.
             * 
             * @param connectionID connection ID
             * @param connection connection pointer
             * @return pending connection data pointer
             */
            ConnectionDataPtr createPendingConnectionData(
                const ConnectionID connectionID, ConnectionPtr connection);
            
            /**
             * Retrieves the pending connection data associated with the specified
             * connection ID.
             * 
             * @param connectionID connection ID
             * @return the retrieved connection data
             */
            ConnectionDataPtr getPendingConnectionData(const ConnectionID connectionID);
            
            /**
             * Discards the pending connection data associated with the specified
             * connection ID, and returns a shared pointer to it.
             * 
             * @param connectionID connection ID
             * @return a shared pointer to the discarded data
             */
            ConnectionDataPtr discardPendingConnectionData(const ConnectionID connectionID);
            
            /**
             * Retrieves pending connection configuration data from the parent
             * <code>NetworkManager</code>.
             * 
             * @param deviceID device associated with the connection
             * @param transientID transient connection ID
             * @return the requested data
             */
            PendingDataConnectionConfigPtr getPendingConnectionConfig(
                const DeviceID deviceID, const TransientConnectionID transientID);
            
            /**
             * Generates encrypted connection request data for the specified peer and connection.
             * 
             * The returned data can be sent over the network.
             * 
             * Note: The returned data includes the plaintext local peer ID.
             * 
             * @param remotePeerID the device ID of the remote peer
             * @param remotePeerData connection data for the remote peer
             * @throw runtime_error if request serialization or data retrieval fails
             * @return the data to be sent to the remote peer
             */
            CiphertextData * generateConnectionRequestData(
                const DeviceID remotePeerID, ConnectionDataPtr remotePeerData);
            
            /**
             * Generates encrypted connection response data based on the supplied encrypted request.
             * 
             * The returned data can be sent over the network.
             * 
             * @param encryptedRequest the encrypted request data received from the remote peer
             * @param connectionID the ID of the connection that was established by the remote peer
             * @throw runtime_error if the request is not valid or if response serialization fails
             * @return the data to be sent to the remote peer
             */
            CiphertextData * generateConnectionResponseDataFromRequest(
                const CiphertextData & encryptedRequest, const ConnectionID connectionID);
            
            /**
             * Verifies the supplied connection response data for the specified peer and connection.
             * 
             * @param encryptedResponse the encrypted data to be verified
             * @param remotePeerData connection data for the remote peer
             * @throw runtime_error if the response is not valid
             */
            void verifyConnectionResponseData(
                const CiphertextData & encryptedResponse, ConnectionDataPtr remotePeerData);
            
            /**
             * 'onDisconnect' event handler for pending local connections.
             * 
             * @param rawID raw connection ID
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             * @param transientID associated transient ID
             */
            void onDisconnectHandler_PendingLocalConnections(
                RawConnectionID rawID, const DeviceID deviceID,
                const ConnectionID connectionID, const TransientConnectionID transientID);
            
            /**
             * 'onDataReceived' event handler for pending local connections.
             * 
             * @param data received data
             * @param remaining remaining data (should be 0)
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onDataReceivedHandler_PendingLocalConnections(
                ByteData data, PacketSize remaining, const DeviceID deviceID,
                const ConnectionID connectionID);
            
            /**
             * 'onWriteResultReceived' event handler for pending local connections.
             * 
             * @param received set to <code>true</code>, if the data was successfully received
             * @param deviceID the associated device ID
             * @param connectionID associated connection ID
             */
            void onWriteResultReceivedHandler_PendingLocalConnections(
                bool received, const DeviceID deviceID, const ConnectionID connectionID);
            
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
             * @param deviceID the associated device ID
             * @param connectionID associated connection ID
             */
            void onWriteResultReceivedHandler_PendingRemoteConnections(
                bool received, const DeviceID deviceID, const ConnectionID connectionID);
            
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
             * @param data received data
             * @param remaining remaining data
             * @param deviceID associated device ID
             * @param connectionID associated connection ID
             */
            void onDataReceivedHandler_EstablishedConnections(
                ByteData data, PacketSize remaining, const DeviceID deviceID,
                const ConnectionID connectionID);
            
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
             * Terminates the specified connection for the specified device and
             * discards all associated data.
             * 
             * @param connectionID the connection ID
             * @param remotePeerID the device ID
             */
            void terminateConnection(
                const ConnectionID connectionID, const DeviceID remotePeerID = INVALID_DEVICE_ID);
            
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
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "DataConnectionsHandler " + message);
            }
    };
}

#endif	/* DATACONNECTIONSHANDLER_H */

