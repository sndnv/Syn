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

#ifndef NETWORKMANAGER_H
#define	NETWORKMANAGER_H

#include <vector>
#include <string>
#include <deque>

#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>

#include "../Common/Types.h"
#include "../Utilities/FileLogger.h"
#include "../Utilities/ThreadPool.h"
#include "../SecurityManagement/Crypto/Handlers.h"
#include "../SecurityManagement/Crypto/LocalAuthenticationDataStore.h"
#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Types/SecurityTokens.h"
#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Types/Exceptions.h"
#include "../SecurityManagement/Interfaces/Securable.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../SessionManagement/SessionManager.h"
#include "../SessionManagement/Types/Types.h"
#include "../SessionManagement/Types/Exceptions.h"
#include "../DatabaseManagement/Types/Types.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"

#include "../InstructionManagement/Interfaces/InstructionSource.h"
#include "../InstructionManagement/Interfaces/InstructionTarget.h"

#include "Types/Types.h"
#include "Types/Containers.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

#include "CommandConverter.h"
#include "CommandConnectionsHandler.h"
#include "DataConnectionsHandler.h"
#include "InitialConnectionsHandler.h"
#include "ConnectionDataStore.h"
#include "Connections/Connection.h"
#include "Connections/ConnectionManager.h"

#include "../InstructionManagement/Sets/NetworkManagerInstructionSet.h"

//Exceptions
using SecurityManagement_Types::InvalidAuthorizationTokenException;

//Networking
using NetworkManagement_Handlers::CommandConverter;
using NetworkManagement_Handlers::InitialConnectionsHandler;
using NetworkManagement_Handlers::DataConnectionsHandler;
using NetworkManagement_Handlers::CommandConnectionsHandler;
using NetworkManagement_Handlers::ConnectionDataStore;
using NetworkManagement_Connections::Connection;
using NetworkManagement_Connections::ConnectionPtr;
using NetworkManagement_Connections::ConnectionManager;
using NetworkManagement_Connections::ConnectionManagerPtr;
using NetworkManagement_Types::ConnectionManagerID;
using NetworkManagement_Types::INVALID_CONNECTION_MANAGER_ID;
using NetworkManagement_Types::RawConnectionID;
using NetworkManagement_Types::ConnectionID;
using NetworkManagement_Types::INVALID_CONNECTION_ID;
using NetworkManagement_Types::ConnectionSetupState;
using NetworkManagement_Types::TransientConnectionID;
using NetworkManagement_Types::INVALID_TRANSIENT_CONNECTION_ID;
using NetworkManagement_Types::PendingDataConnectionConfig;
using NetworkManagement_Types::PendingDataConnectionConfigPtr;
using NetworkManagement_Types::NewDeviceConnectionParameters;
using NetworkManagement_Types::PendingInitConnectionConfig;
using NetworkManagement_Types::PendingInitConnectionConfigPtr;
using NetworkManagement_Types::ActiveConnectionData;
using NetworkManagement_Types::ActiveConnectionDataPtr;
using NetworkManagement_Types::StatCounter;
using NetworkManagement_Types::CommandID;
using NetworkManagement_Types::INVALID_COMMAND_ID;

//Common
using Common_Types::LogSeverity;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::INVALID_USER_ID;
using Common_Types::INVALID_DEVICE_ID;

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
using SecurityManagement_Crypto::LocalAuthenticationDataStore;
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::INVALID_TOKEN_ID;
using SecurityManagement_Types::AuthorizationTokenPtr;
using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::CiphertextData;
using SecurityManagement_Types::AsymmetricCipherType;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::KeyExchangeType;

//Sessions
using SessionManagement_Types::InternalSessionID;
using SessionManagement_Types::INVALID_INTERNAL_SESSION_ID;

//Instructions
using InstructionManagement_Sets::InstructionBasePtr;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Sets::InstructionSetPtr;
using InstructionManagement_Types::NetworkManagerAdminInstructionType;
using InstructionManagement_Types::NetworkManagerUserInstructionType;
using InstructionManagement_Types::NetworkManagerStateInstructionType;
using InstructionManagement_Types::NetworkManagerConnectionLifeCycleInstructionType;
using InstructionManagement_Types::NetworkManagerConnectionBridgingInstructionType;

namespace SyncServer_Core
{
    //<editor-fold defaultstate="collapsed" desc="Instruction Targets">
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class NetworkManagerAdminInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<NetworkManagerAdminInstructionType>
    {
        public:
            virtual ~NetworkManagerAdminInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_ADMIN;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class NetworkManagerUserInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<NetworkManagerUserInstructionType>
    {
        public:
            virtual ~NetworkManagerUserInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_USER;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class NetworkManagerStateInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<NetworkManagerStateInstructionType>
    {
        public:
            virtual ~NetworkManagerStateInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_STATE;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class NetworkManagerConnectionLifeCycleInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<NetworkManagerConnectionLifeCycleInstructionType>
    {
        public:
            virtual ~NetworkManagerConnectionLifeCycleInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class NetworkManagerConnectionBridgingInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<NetworkManagerConnectionBridgingInstructionType>
    {
        public:
            virtual ~NetworkManagerConnectionBridgingInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_CONNECTION_BRIDGING;
            }
    };
    //</editor-fold>
    
    /**
     * Class for managing networking-related activities.
     * 
     * Handles serialization/parsing, compression/decompression,
     * encryption/decryption and all connection setup processes.
     */
    class NetworkManager final
    : public SecurityManagement_Interfaces::Securable,
      public NetworkManagerAdminInstructionTarget,
      public NetworkManagerUserInstructionTarget,
      public NetworkManagerStateInstructionTarget,
      public NetworkManagerConnectionLifeCycleInstructionTarget,
      public NetworkManagerConnectionBridgingInstructionTarget,
      public InstructionManagement_Interfaces::InstructionSource,
      public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure for holding <code>NetworkManager</code> configuration data. */
            struct NetworkManagerParameters
            {
                /** Number of threads to create in the network handling thread pool. */
                unsigned int networkThreadPoolSize;
                /** Number of threads to create in the instruction handling thread pool. */
                unsigned int instructionsThreadPoolSize;
                /** Reference to a database manager instance. */
                DatabaseManager & databaseManager;
                /** Reference to a security manager instance. */
                SecurityManager & securityManager;
                /** Reference to a session manager instance. */
                SessionManager & sessionManager;
                /** Reference to a local authentication data store. */
                LocalAuthenticationDataStore & authenticationStore;
                /** Parameters for the 'INIT' connections handler. */
                InitialConnectionsHandler::InitialConnectionsHandlerParameters initConnectionsParams;
                /** Parameters for the 'COMMAND' connections handler. */
                CommandConnectionsHandler::CommandConnectionsHandlerParameters commandConnectionsParams;
                /** Parameters for the 'DATA' connections handler. */
                DataConnectionsHandler::DataConnectionsHandlerParameters dataConnectionsParams;
                /** Time to wait before setting a 'COMMAND' connection setup as failed (in seconds). */
                Seconds commandConnectionSetupTimeout;
                /** Time to wait before setting a 'DATA' connection setup as failed (in seconds). */
                Seconds dataConnectionSetupTimeout;
                /** Time to wait before setting an 'INIT' connection setup as failed (in seconds). */
                Seconds initConnectionSetupTimeout;
                /** Time to wait before dropping a 'COMMAND' connection due to inactivity (in seconds). */
                Seconds commandConnectionInactivityTimeout;
                /** Time to wait before dropping a 'DATA' connection due to inactivity (in seconds). */
                Seconds dataConnectionInactivityTimeout;
                /** Time to wait before discarding pending connection data (in seconds). */
                Seconds pendingConnectionDataDiscardTimeout;
                /** Time to wait for a 'DATA' connection setup to be initiated (in seconds). */
                Seconds expectedDataConnectionTimeout;
                /** Time to wait for an 'INIT' connection setup to be initiated (in seconds). */
                Seconds expectedInitConnectionTimeout;
            };
            
            /**
             * Constructs a new network manager object with the specified configuration.
             * 
             * 
             * @param params the manager configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             */
            NetworkManager(const NetworkManagerParameters & params, Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Stops all networking activity and clears all related data.
             */
            ~NetworkManager();
            
            NetworkManager() = delete;
            NetworkManager(const NetworkManager&) = delete;
            NetworkManager& operator=(const NetworkManager&) = delete;
            
            void postAuthorizationToken(const AuthorizationTokenPtr token) override;
            
            SecurityManagement_Types::SecurableComponentType getComponentType() const override
            {
                return SecurityManagement_Types::SecurableComponentType::NETWORK_MANAGER;
            }
            
            bool registerInstructionSet(InstructionSetPtr<NetworkManagerAdminInstructionType> set) const override;
            bool registerInstructionSet(InstructionSetPtr<NetworkManagerUserInstructionType> set) const override;
            bool registerInstructionSet(InstructionSetPtr<NetworkManagerStateInstructionType> set) const override;
            bool registerInstructionSet(InstructionSetPtr<NetworkManagerConnectionLifeCycleInstructionType> set) const override;
            bool registerInstructionSet(InstructionSetPtr<NetworkManagerConnectionBridgingInstructionType> set) const override;
            
            bool registerInstructionHandler(const std::function<void(InstructionBasePtr, AuthorizationTokenPtr)> handler) override
            {
                if(!processInstruction)
                {
                    processInstruction = handler;
                    return true;
                }
                else
                {
                    logMessage(LogSeverity::Error, "(registerInstructionHandler) >"
                            " The instruction handler has already been set.");
                    return false;
                }
            }
            
            std::vector<InstructionSetType> getRequiredInstructionSetTypes() override
            {
                return std::vector<InstructionManagement_Types::InstructionSetType>(
                {
                    InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE
                });
            }
            
            std::string getSourceName() const override
            {
                return "NetworkManager";
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
                    logMessage(LogSeverity::Error, "(registerLoggingHandler) >"
                            " The database logging handler has already been set.");
                    return false;
                }
            }
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Connection Managers">
            /**
             * Starts a new connection manager with the supplied parameters.
             * 
             * @param params the new connection manager configuration
             * @return the ID associated with the new manager
             */
            ConnectionManagerID startConnectionManager(ConnectionManager::ConnectionManagerParameters params);
            
            /**
             * Stops the connection manager with the specified ID.
             * 
             * @param id the ID of the manager to be stopped
             * @param type the type of the manager
             */
            void stopConnectionManager(ConnectionManagerID id, ConnectionType type);
            
            /**
             * Stops the initial connection manager with the specified ID.
             * 
             * @param id the ID of the manager to be stopped
             */
            void stopInitConnectionManager(ConnectionManagerID id);
            
            /**
             * Stops the command connection manager with the specified ID.
             * 
             * @param id the ID of the manager to be stopped
             */
            void stopCommandConnectionManager(ConnectionManagerID id);
            
            /**
             * Stops the data connection manager with the specified ID.
             * 
             * @param id the ID of the manager to be stopped
             */
            void stopDataConnectionManager(ConnectionManagerID id);
            //</editor-fold>
            
            /**
             * Sends the supplied instruction to the specified device using
             * the specified manager.
             * 
             * @param managerID the manager to use for the connection
             *                  (if no connection is active)
             * @param device the ID of the target device
             * @param instruction the instruction to be sent
             */
            void sendInstruction(
                const ConnectionManagerID managerID, const DeviceID device,
                const InstructionBasePtr instruction);
            
            /**
             * Sends the supplied plaintext data to the specified device over
             * the specified connection.
             * 
             * Note: Not fully supported.
             * 
             * @param device the ID of the target device
             * @param connection the ID of the connection to be used
             * @param data the data to be sent
             */
            void sendData(
                const DeviceID device, const ConnectionID connection,
                const PlaintextData & data)
            {
                dataConnections.sendData(device, connection, data);
                ++dataSent;
                throw std::logic_error("NetworkManager::sendData() >"
                        " Operation not fully supported.");
            }
            
            /**
             * Retrieves a new transient connection ID.
             * 
             * @return the requested ID
             */
            TransientConnectionID getNewTransientID()
            {
                return ++lastTransientID;
            }
            
            StatCounter getCommandsReceived() const         { return commandsReceived; }
            StatCounter getCommandsSent() const             { return commandsSent; }
            StatCounter getConnectionsInitiated() const     { return connectionsInitiated; }
            StatCounter getConnectionsReceived() const      { return connectionsReceived; }
            StatCounter getDataReceived() const             { return dataReceived; }
            StatCounter getDataSent() const                 { return dataSent; }
            StatCounter getSetupsCompleted() const          { return setupsCompleted; }
            StatCounter getSetupsFailed() const             { return setupsFailed; }
            StatCounter getSetupsPartiallyCompleted() const { return setupsPartiallyCompleted; }
            StatCounter getSetupsStarted() const            { return setupsStarted; }
            unsigned long getInstructionsProcessed() const  { return instructionsProcessed; }
            unsigned long getInstructionsReceived() const   { return instructionsReceived; }
            
        private:
            Utilities::ThreadPool networkingThreadPool;     //thread pool for networking tasks
            Utilities::ThreadPool instructionsThreadPool;   //thread pool for processing instructions
            Utilities::FileLoggerPtr debugLogger; //logger for debugging
            std::function<void (LogSeverity, const std::string &)> dbLogHandler; //database log handler
            
            //Required Managers
            DatabaseManager & databaseManager;
            SecurityManager & securityManager;
            SessionManager & sessionManager;
            LocalAuthenticationDataStore & authenticationStore;
            
            //Connection Management Data
            boost::mutex connectionManagementDataMutex;
            ConnectionManagerID lastManagerID = INVALID_CONNECTION_MANAGER_ID;
            boost::unordered_map<ConnectionManagerID, ConnectionManagerPtr> dataConnectionManagers;
            boost::unordered_map<ConnectionManagerID, ConnectionManagerPtr> commandConnectionManagers;
            boost::unordered_map<ConnectionManagerID, ConnectionManagerPtr> initConnectionManagers;
            
            //Connection Data
            ConnectionDataStore dataStore;
            
            boost::mutex pendingConnectionsMutex;
            boost::unordered_map<ConnectionID, ConnectionSetupState> pendingConnections;
            
            boost::mutex activeDataConnectionsMutex;
            boost::unordered_map<DeviceID, boost::unordered_map<ConnectionID, ActiveConnectionDataPtr>> activeDataConnections;
            boost::mutex activeCommandConnectionsMutex;
            boost::unordered_map<DeviceID, ActiveConnectionDataPtr> activeCommandConnections;
            boost::unordered_map<DeviceID, std::deque<InstructionBasePtr>> pendingDeviceInstructions;
            
            //Connection Handlers
            CommandConverter converter;
            InitialConnectionsHandler initConnections;
            CommandConnectionsHandler commandConnections;
            DataConnectionsHandler dataConnections;
            
            boost::signals2::connection onCommandDataReceivedEventConnection;
            boost::signals2::connection onCommandConnectionEstablishedEventConnection;
            boost::signals2::connection onCommandConnectionEstablishmentFailedEventConnection;
            boost::signals2::connection onDataReceivedEventConnection;
            boost::signals2::connection onDataConnectionEstablishedEventConnection;
            boost::signals2::connection onDataConnectionEstablishmentFailedEventConnection;
            boost::signals2::connection onSetupCompletedEventConnection;
            boost::signals2::connection onSetupFailedEventConnection;
            
            //Connection Counters
            std::atomic<ConnectionID> lastConnectionID{INVALID_CONNECTION_ID};
            std::atomic<TransientConnectionID> lastTransientID{INVALID_TRANSIENT_CONNECTION_ID};
            
            //Timeout Settings
            Seconds commandConnectionSetupTimeout;
            Seconds dataConnectionSetupTimeout;
            Seconds initConnectionSetupTimeout;
            Seconds commandConnectionInactivityTimeout;
            Seconds dataConnectionInactivityTimeout;
            Seconds pendingConnectionDataDiscardTimeout;
            Seconds expectedDataConnectionTimeout;
            Seconds expectedInitConnectionTimeout;
            
            //Stats
            std::atomic<StatCounter> dataSent;
            std::atomic<StatCounter> dataReceived;
            std::atomic<StatCounter> commandsSent;
            std::atomic<StatCounter> commandsReceived;
            std::atomic<StatCounter> connectionsInitiated;
            std::atomic<StatCounter> connectionsReceived;
            std::atomic<StatCounter> setupsStarted;
            std::atomic<StatCounter> setupsCompleted;
            std::atomic<StatCounter> setupsPartiallyCompleted;
            std::atomic<StatCounter> setupsFailed;
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Connection Setup">
            /**
             * Initiates a device setup process.
             * 
             * @param managerID ID of the manager to be used for the connection
             * @param initAddress target device 'INIT' IP address
             * @param initPort target device 'INIT' IP port
             * @param sharedPassword secret shared between both peers
             * @param remotePeerType type of the remote peer
             * @param remotePeerID device ID for the remote peer (as identified by the local system)
             * @param transientID transient ID associated with the setup
             * @throw invalid_argument if the specified manager could not be found
             */
            void initiateDeviceSetupProcess(
                const ConnectionManagerID managerID, const IPAddress initAddress,
                const IPPort initPort, const std::string & sharedPassword,
                const PeerType remotePeerType, const DeviceID remotePeerID,
                const TransientConnectionID transientID);
            
            /**
             * Prepares the required data for an expected device setup process.
             * 
             * @param sharedPassword secret shared between both peers
             * @param remotePeerType type of the remote peer
             * @param remotePeerID device ID for the remote peer (as identified by the local system)
             * @param transientID transient ID associated with the setup
             */
            void waitForDeviceSetupProcess(
                const std::string & sharedPassword, const PeerType remotePeerType,
                const DeviceID remotePeerID, const TransientConnectionID transientID);
            
            /**
             * Initiates a command connection setup.
             * 
             * @param managerID ID of the manager to be used for the connection
             * @param targetDevice target device ID
             * @throw invalid_argument if the specified manager could not be found
             */
            void initiateCommandConnection(
                const ConnectionManagerID managerID, const DeviceID targetDevice);
            
            /**
             * Initiates a data connection setup.
             * 
             * Note: All connection setup data is always encrypted.
             * 
             * @param managerID ID of the manager to be used for the connection
             * @param transientID transient connection ID associated with the setup
             * @param data target device data
             * @param crypto crypto data for the connection setup process
             *              (and, if configured, for the subsequent data transfers)
             * @param encrypt denotes whether encryption should be enabled
             * @param compress denotes whether compression should be enabled
             * @throw invalid_argument if the specified manager could not be found
             */
            void initiateDataConnection(
                const ConnectionManagerID managerID, const TransientConnectionID transientID,
                DeviceDataContainerPtr data, SymmetricCryptoHandlerPtr crypto,
                bool encrypt, bool compress);
            
            /**
             * Prepares the required data for an expected data connection setup process.
             * 
             * @param transientID transient connection ID associated with the setup
             * @param data target device data
             * @param crypto crypto data for the connection setup process
             *              (and, if configured, for the subsequent data transfers)
             * @param encrypt denotes whether encryption should be enabled
             * @param compress denotes whether compression should be enabled
             */
            void waitForDataConnection(
                const TransientConnectionID transientID, DeviceDataContainerPtr data,
                SymmetricCryptoHandlerPtr crypto, bool encrypt, bool compress);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Misc">
            /**
             * Loads data for the specified device into the data store.
             * 
             * @param targetDevice the ID of the device to have its data
             *        retrieved and stored in the data store
             * @return the data associated with the device
             */
            DeviceDataContainerPtr loadCommandConnectionDeviceData(const DeviceID targetDevice);
            
            /**
             * Retrieves pending instructions based on their command IDs.
             * 
             * Note: Used for setting their results (as returned by remote peers).
             * 
             * @param connectionData active connection data
             * @param commandID command ID associated with the instruction (during serialization)
             * @throw runtime_error if the requested instruction could not be found
             * @return the requested instruction
             */
            InstructionBasePtr retrievePendingInstruction(
                ActiveConnectionDataPtr connectionData, const CommandID commandID);
            
            /**
             * Adds a new task into the instructions thread pool for processing
             * (waiting for) the result from an instruction sent to a remote peer.
             * 
             * @param deviceID the ID of the device associated with the instruction
             * @param responseSerializationFunction response serialization function
             */
            void enqueueRemoteInstructionResultProcessing(
                const DeviceID deviceID,
                std::function<const PlaintextData (void)> responseSerializationFunction);
            
            /**
             * Sets the state of the specified connection.
             * 
             * @param id the ID of the connection
             * @param state the new connection state
             */
            void setPendingConnectionState(const ConnectionID id, ConnectionSetupState state);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Timeouts">
            /**
             * Timeout handler for pending connections.
             * 
             * Fired by thread pool when timeout expires.
             * 
             * @param id connection ID
             * @param connection connection pointer
             */
            void pendingConnectionTimeoutHandler(
                const ConnectionID id, ConnectionPtr connection);
            
            /**
             * Timeout handler for active connections.
             * 
             * Closes connections that have not had any recent activity.
             * 
             * @param connectionData active connection data
             * @param lastEventsCount the number of event when the handler was last run
             */
            void activeConnectionTimeoutHandler(
                ActiveConnectionDataPtr connectionData, const StatCounter lastEventsCount);
            
            /**
             * Timeout handler for discarding pending device instructions.
             * 
             * @param device the ID of the associated device
             */
            void pendingDeviceInstructionsDiscardTimeoutHandler(const DeviceID device);
            
            /**
             * Timeout handler for discarding pending 'INIT' connection data.
             * 
             * @param transientID associated transient connection ID
             */
            void initConnectionDataDiscardTimeoutHandler(
                const TransientConnectionID transientID);
            
            /**
             * Timeout handler for discarding pending 'COMMAND' connection data.
             * 
             * @param device associated device ID
             */
            void commandConnectionDataDiscardTimeoutHandler(const DeviceID device);
            
            /**
             * Timeout handler for discarding pending 'DATA' connection data.
             * 
             * @param device associated device ID
             * @param transientID associated transient connection ID
             */
            void dataConnectionDataDiscardTimeoutHandler(
                const DeviceID device, const TransientConnectionID transientID);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Connection Setup Results">
            /**
             * Event handler for processing established 'COMMAND' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             */
            void onCommandConnectionEstablishedHandler(
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Event handler for processing failed 'COMMAND' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             */
            void onCommandConnectionEstablishmentFailedHandler(
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Event handler for processing established 'DATA' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             * @param transientID the transient ID associated with the connection
             */
            void onDataConnectionEstablishedHandler(
                const DeviceID deviceID, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
            /**
             * Event handler for processing failed 'DATA' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             * @param transientID the transient ID associated with the connection
             */
            void onDataConnectionEstablishmentFailedHandler(
                const DeviceID deviceID, const ConnectionID connectionID,
                const TransientConnectionID transientID);
            
            /**
             * Event handler for processing completed 'INIT' processes.
             * 
             * @param connectionID the ID of the connection
             * @param deviceID the ID of the device associated with the connection
             * @param transientID the transient ID associated with the connection
             * @param deviceConfig device configuration data
             */
            void onInitSetupCompletedHandler(
                const ConnectionID connectionID, const DeviceID deviceID,
                const TransientConnectionID transientID,
                const NewDeviceConnectionParameters & deviceConfig);
            
            /**
             * Event handler for processing failed 'INIT' processes.
             * 
             * @param connectionID the ID of the connection
             * @param transientID the transient ID associated with the connection
             */
            void onInitSetupFailedHandler(
                const ConnectionID connectionID, const TransientConnectionID transientID);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Closed Connections">
            /**
             * Event handler for processing termination of established 'COMMAND' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             */
            void onEstablishedCommandConnectionClosed(
                const DeviceID deviceID, const ConnectionID connectionID);
            
            /**
             * Event handler for processing termination of established 'DATA' connections.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             */
            void onEstablishedDataConnectionClosed(
                const DeviceID deviceID, const ConnectionID connectionID);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Data Received">
            /**
             * Event handler for processing received 'COMMAND' data.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param data the received plaintext data
             */
            void onCommandDataReceivedHandler(
                const DeviceID deviceID, const PlaintextData data);
            
            /**
             * Event handler for processing received 'DATA' data.
             * 
             * Note: Not supported.
             * 
             * @param deviceID the ID of the device associated with the connection
             * @param connectionID the ID of the connection
             * @param data the received plaintext data
             */
            void onDataReceivedHandler(
                const DeviceID deviceID, const ConnectionID connectionID,
                const PlaintextData data)
            {
                ++dataReceived;
                throw std::logic_error("NetworkManager::onDataReceivedHandler() >"
                        " Operation not supported.");
            }
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Connection Managers">
            /**
             * Event handler for newly created connections (local and remote).
             * 
             * @param connection connection pointer
             * @param initiation denotes which peer initiated the connection
             * @param managerID associated manager ID
             */
            void onConnectionCreatedHandler(
                ConnectionPtr connection, ConnectionInitiation initiation,
                ConnectionManagerID managerID);
            
            /**
             * Event handler for failed local connection initiations.
             * 
             * @param error the error that was encountered during initiation
             * @param managerID associated manager ID
             */
            void onConnectionInitiationFailedHandler(
                const boost::system::error_code & error, ConnectionManagerID managerID);
            //</editor-fold>
            
            /**
             * Retrieves a new connection ID.
             * 
             * @return the requested ID
             */
            ConnectionID getNewConnectionID()
            {
                return ++lastConnectionID;
            }
            
            //Instruction Management
            boost::mutex instructionDataMutex;      //instruction data mutex
            boost::unordered_map<TokenID, AuthorizationTokenPtr> authorizationTokens; //expected authorization tokens
            unsigned long instructionsReceived;     //number of instructions received by manager
            unsigned long instructionsProcessed;    //number of instructions processed by manager
            //function for sending instructions to system components
            std::function<
                void(InstructionManagement_Sets::InstructionBasePtr,
                     SecurityManagement_Types::AuthorizationTokenPtr)
            >
            processInstruction;
            
            /**
             * Sets an exception with the specified message in the supplied 
             * instruction's promise.
             * 
             * Note: Always sets <code>std::runtime_error</code> exception.
             * 
             * @param message the message for the exception
             * @param instruction the instruction in which the exception is to be set
             */
            void throwInstructionException(
                const std::string & message,
                InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType> instruction)
            {
                try
                {
                    boost::throw_exception(std::runtime_error(message));
                }
                catch(const std::runtime_error &)
                {
                    instruction->getPromise().set_exception(boost::current_exception());
                    return;
                }
            }
            
            //<editor-fold defaultstate="collapsed" desc="Handlers - Instructions">
            void lifeCycleOpenDataConnectionHandler(
                InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType> instruction);
            void lifeCycleOpenInitConnectionHandler(
                InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType> instruction);
            
            //Instruction Handlers Function Binds
            std::function<void(InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType>)>
            lifeCycleOpenDataConnectionHandlerBind =
                boost::bind(&NetworkManager::lifeCycleOpenDataConnectionHandler, this, _1);
            
            std::function<void(InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType>)>
            lifeCycleOpenInitConnectionHandlerBind =
                boost::bind(&NetworkManager::lifeCycleOpenInitConnectionHandler, this, _1);
            //</editor-fold>
            
            /**
             * Verifies the supplied authentication token.
             * 
             * Note: The token is removed the the list of expected authorization tokens
             * 
             * @param token the token to be verified
             * 
             * @throw InvalidAuthorizationTokenException if an invalid token is encountered
             */
            void verifyAuthorizationToken(AuthorizationTokenPtr token);
            
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
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "NetworkManager " + message);
            }
    };
}

#endif	/* NETWORKMANAGER_H */
