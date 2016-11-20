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

#include "NetworkManager.h"
#include <boost/thread/lock_guard.hpp>
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Instructions.h"

namespace Convert = Utilities::Strings;
namespace Instructions = InstructionManagement_Sets::NetworkManagerInstructions;
namespace InstructionResults = InstructionManagement_Sets::NetworkManagerInstructions::Results;

//Protocols
#include "../../compiled/protobuf/BaseComm.pb.h"

using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::CommandConnectionSetupRequest;
using NetworkManagement_Protocols::CommandConnectionSetupResponse;
using NetworkManagement_Protocols::CommandConnectionSetupRequestData;
using NetworkManagement_Protocols::DataConnectionSetupRequest;
using NetworkManagement_Protocols::DataConnectionSetupResponse;
using NetworkManagement_Protocols::Command;
using NetworkManagement_Protocols::Response;
using NetworkManagement_Protocols::Response_Status;

SyncServer_Core::NetworkManager::NetworkManager
(const NetworkManagerParameters & params, Utilities::FileLoggerPtr debugLogger)
: networkingThreadPool(params.networkThreadPoolSize, debugLogger),
  instructionsThreadPool(params.instructionsThreadPoolSize, debugLogger),
  debugLogger(debugLogger),
  databaseManager(params.databaseManager),
  securityManager(params.securityManager),
  sessionManager(params.sessionManager),
  authenticationStore(params.authenticationStore),
  initConnections(
        params.initConnectionsParams,
        *this,
        boost::bind(&ConnectionDataStore::getInitConnectionData, &this->dataStore, _1),
        boost::bind(&LocalAuthenticationDataStore::addData, &this->authenticationStore, _1, _2),
        debugLogger),
  commandConnections(
        params.commandConnectionsParams,
        boost::bind(&NetworkManager::loadCommandConnectionDeviceData, this, _1),
        boost::bind(&LocalAuthenticationDataStore::getData, &this->authenticationStore, _1),
        *this,
        debugLogger),
  dataConnections(
        params.dataConnectionsParams,
        boost::bind(static_cast<PendingDataConnectionConfigPtr
                                (ConnectionDataStore::*)(const DeviceID, const TransientConnectionID)>
                        (&ConnectionDataStore::getDataConnectionData), //pointer cast for the specific function overload
                    &this->dataStore, _1, _2), //remaining bind parameters
        boost::bind(&LocalAuthenticationDataStore::getData, &this->authenticationStore, _1),
        debugLogger),
  commandConnectionSetupTimeout(params.commandConnectionSetupTimeout),
  dataConnectionSetupTimeout(params.dataConnectionSetupTimeout),
  initConnectionSetupTimeout(params.initConnectionSetupTimeout),
  commandConnectionInactivityTimeout(params.commandConnectionInactivityTimeout),
  dataConnectionInactivityTimeout(params.dataConnectionInactivityTimeout),
  pendingConnectionDataDiscardTimeout(params.pendingConnectionDataDiscardTimeout),
  expectedDataConnectionTimeout(params.expectedDataConnectionTimeout),
  expectedInitConnectionTimeout(params.expectedInitConnectionTimeout),
  dataSent(0),
  dataReceived(0),
  commandsSent(0),
  commandsReceived(0),
  connectionsInitiated(0),
  connectionsReceived(0),
  setupsStarted(0),
  setupsCompleted(0),
  setupsPartiallyCompleted(0),
  setupsFailed(0),
  instructionsReceived(0),
  instructionsProcessed(0)
{
    onCommandDataReceivedEventConnection =
            commandConnections.onCommandDataReceivedEventAttach(
                boost::bind(&NetworkManager::onCommandDataReceivedHandler, this, _1, _2));
    onCommandConnectionEstablishedEventConnection =
            commandConnections.onConnectionEstablishedEventAttach(
                boost::bind(&NetworkManager::onCommandConnectionEstablishedHandler, this, _1, _2));
    onCommandConnectionEstablishmentFailedEventConnection =
            commandConnections.onConnectionEstablishmentFailedEventAttach(
                boost::bind(&NetworkManager::onCommandConnectionEstablishmentFailedHandler, this, _1, _2));

    onDataReceivedEventConnection =
            dataConnections.onDataReceivedEventAttach(
                boost::bind(&NetworkManager::onDataReceivedHandler, this, _1, _2, _3));
    onDataConnectionEstablishedEventConnection =
            dataConnections.onConnectionEstablishedEventAttach(
                boost::bind(&NetworkManager::onDataConnectionEstablishedHandler, this, _1, _2, _3));
    onDataConnectionEstablishmentFailedEventConnection =
            dataConnections.onConnectionEstablishmentFailedEventAttach(
                boost::bind(&NetworkManager::onDataConnectionEstablishmentFailedHandler, this, _1, _2, _3));

    onSetupCompletedEventConnection =
            initConnections.onSetupCompletedEventAttach(
                boost::bind(&NetworkManager::onInitSetupCompletedHandler, this, _1, _2, _3, _4));
    onSetupFailedEventConnection =
            initConnections.onSetupFailedEventAttach(
                boost::bind(&NetworkManager::onInitSetupFailedHandler, this, _1, _2));
}

SyncServer_Core::NetworkManager::~NetworkManager()
{
    onCommandDataReceivedEventConnection.disconnect();
    onCommandConnectionEstablishedEventConnection.disconnect();
    onCommandConnectionEstablishmentFailedEventConnection.disconnect();
    onDataReceivedEventConnection.disconnect();
    onDataConnectionEstablishedEventConnection.disconnect();
    onDataConnectionEstablishmentFailedEventConnection.disconnect();
    onSetupCompletedEventConnection.disconnect();
    onSetupFailedEventConnection.disconnect();

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);
    if(!authorizationTokens.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Authorization tokens found: ["
                + Convert::toString(authorizationTokens.size()) + "].");
        authorizationTokens.clear();
    }

    boost::lock_guard<boost::mutex> activeDataConnectionsLock(activeDataConnectionsMutex);
    if(!activeDataConnections.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Active data connections found for ["
                + Convert::toString(activeDataConnections.size()) + "] devices.");

        for(auto currentDataConnectionsSet : activeDataConnections)
        {
            logMessage(LogSeverity::Warning, "(~) > Active data connections found for device ["
                    + Convert::toString(currentDataConnectionsSet.first) + "]: ["
                    + Convert::toString(currentDataConnectionsSet.second.size()) + "].");

            for(auto currentDataConnection : currentDataConnectionsSet.second)
            {
                dataConnections.closeConnection(
                    currentDataConnectionsSet.first,
                    currentDataConnection.first);
            }
        }

        activeDataConnections.clear();
    }

    boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
    if(!activeCommandConnections.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Active command connections found: ["
                + Convert::toString(activeCommandConnections.size()) + "].");

        for(auto currentCommandConnection : activeCommandConnections)
        {
            commandConnections.closeEstablishedConnection(currentCommandConnection.first);
        }

        activeCommandConnections.clear();
    }

    if(!pendingDeviceInstructions.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Pending device instructions found: ["
                + Convert::toString(pendingDeviceInstructions.size()) + "].");
        pendingDeviceInstructions.clear();
    }

    boost::lock_guard<boost::mutex> pendingConnectionsLock(pendingConnectionsMutex);
    if(!pendingConnections.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Pending connections found: [" +
                Convert::toString(pendingConnections.size()) + "].");
        pendingConnections.clear();
    }

    //Connection Management Data
    boost::lock_guard<boost::mutex> connectionManagementDataLock(connectionManagementDataMutex);
    dataConnectionManagers.clear();
    commandConnectionManagers.clear();
    initConnectionManagers.clear();
}

void SyncServer_Core::NetworkManager::postAuthorizationToken(const AuthorizationTokenPtr token)
{
    if(NetworkManagerAdminInstructionTarget::getType() != token->getAuthorizedSet()
        && NetworkManagerStateInstructionTarget::getType() != token->getAuthorizedSet()
        && NetworkManagerConnectionLifeCycleInstructionTarget::getType() != token->getAuthorizedSet()
        && NetworkManagerConnectionBridgingInstructionTarget::getType() != token->getAuthorizedSet())
    {
        throw std::logic_error("NetworkManager::postAuthorizationToken() > The token with ID ["
                + Convert::toString(token->getID()) + "] is not for the expected instruction sets.");
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    if(authorizationTokens.find(token->getID()) == authorizationTokens.end())
    {
        authorizationTokens.insert({token->getID(), token});
    }
    else
    {
        throw std::logic_error("NetworkManager::postAuthorizationToken() > A token with ID ["
                + Convert::toString(token->getID()) + "] is already present.");
    }
}

bool SyncServer_Core::NetworkManager::registerInstructionSet
(InstructionSetPtr<NetworkManagerAdminInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            throw std::logic_error("NetworkManager::registerInstructionSet() >"
                    " Instruction set [NetworkManagerAdminInstructionType] not implemented.");
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                    " Exception encountered: <" + std::string(ex.what()) + ">");

            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                " The supplied set is not initialised.");
        return false;
    }
}


bool SyncServer_Core::NetworkManager::registerInstructionSet
(InstructionSetPtr<NetworkManagerUserInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            throw std::logic_error("NetworkManager::registerInstructionSet() >"
                    " Instruction set [NetworkManagerUserInstructionType] not implemented.");
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                    " Exception encountered: <"
                    + std::string(ex.what()) + ">");

            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                " The supplied set is not initialised.");
        return false;
    }
}

bool SyncServer_Core::NetworkManager::registerInstructionSet
(InstructionSetPtr<NetworkManagerStateInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            throw std::logic_error("NetworkManager::registerInstructionSet() >"
                    " Instruction set [NetworkManagerStateInstructionType] not implemented.");
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                    " Exception encountered: <" + std::string(ex.what()) + ">");

            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                " The supplied set is not initialised.");
        return false;
    }
}

bool SyncServer_Core::NetworkManager::registerInstructionSet
(InstructionSetPtr<NetworkManagerConnectionLifeCycleInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            set->bindInstructionHandler(
                NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION,
                lifeCycleOpenDataConnectionHandlerBind);

            set->bindInstructionHandler(
                NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION,
                lifeCycleOpenInitConnectionHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                    " Exception encountered: <" + std::string(ex.what()) + ">");

            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                " The supplied set is not initialised.");
        return false;
    }
}

bool SyncServer_Core::NetworkManager::registerInstructionSet
(InstructionSetPtr<NetworkManagerConnectionBridgingInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            throw std::logic_error("NetworkManager::registerInstructionSet() >"
                    " Instruction set [NetworkManagerConnectionBridgingInstructionType] not implemented.");
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                    " Exception encountered: <" + std::string(ex.what()) + ">");

            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) >"
                " The supplied set is not initialised.");
        return false;
    }
}

//<editor-fold defaultstate="collapsed" desc="Handlers - Connection Managers">
ConnectionManagerID SyncServer_Core::NetworkManager::startConnectionManager
(ConnectionManager::ConnectionManagerParameters params)
{
    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    boost::unordered_map<ConnectionManagerID, ConnectionManagerPtr> * managers = nullptr;
    switch(params.managerType)
    {
        case ConnectionType::COMMAND: managers = &commandConnectionManagers; break;
        case ConnectionType::DATA: managers = &dataConnectionManagers; break;
        case ConnectionType::INIT: managers = &initConnectionManagers; break;
        default: throw std::logic_error("NetworkManager::startConnectionManager() >"
                " Unexpected manager type encountered [" + Convert::toString(params.managerType) + "].");
    }

    for(const std::pair<ConnectionManagerID, ConnectionManagerPtr> & currentManager : *managers)
    {
        if(currentManager.second->getListeningPort() == params.listeningPort
            && currentManager.second->getListeningAddress().compare(params.listeningAddress) == 0)
        {
            throw std::logic_error("NetworkManager::startConnectionManager() >"
                    " Another connection manager is already listening on ["
                    + params.listeningAddress + ":"
                    + Convert::toString(params.listeningPort) + "].");
        }
    }

    ConnectionManagerID managerID = ++lastManagerID;
    ConnectionManagerPtr newManager(new ConnectionManager(params, debugLogger));
    newManager->onConnectionCreatedEventAttach(
            boost::bind(&NetworkManager::onConnectionCreatedHandler, this, _1, _2, managerID));
    newManager->onConnectionInitiationFailedEventAttach(
            boost::bind(&NetworkManager::onConnectionInitiationFailedHandler, this, _1, managerID));
    
    managers->insert(std::pair<ConnectionManagerID, ConnectionManagerPtr>(managerID, newManager));

    return managerID;
}

void SyncServer_Core::NetworkManager::stopConnectionManager(ConnectionManagerID id, ConnectionType type)
{
    switch(type)
    {
        case ConnectionType::COMMAND: stopCommandConnectionManager(id); break;
        case ConnectionType::DATA: stopDataConnectionManager(id); break;
        case ConnectionType::INIT: stopInitConnectionManager(id); break;
        default: throw std::logic_error("NetworkManager::stopConnectionManager() >"
                " Unexpected manager type encountered [" + Convert::toString(type) + "].");
    }
}

void SyncServer_Core::NetworkManager::stopInitConnectionManager(ConnectionManagerID id)
{
    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = initConnectionManagers.find(id);
    if(manager == initConnectionManagers.end())
    {
        throw std::logic_error("NetworkManager::stopInitConnectionManager() >"
                " No initial connection manager with ID [" + Convert::toString(id)
                + "] was found.");
    }
    else
    {
        initConnectionManagers.erase(manager);
    }
}

void SyncServer_Core::NetworkManager::stopCommandConnectionManager(ConnectionManagerID id)
{
    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = commandConnectionManagers.find(id);
    if(manager == commandConnectionManagers.end())
    {
        throw std::logic_error("NetworkManager::stopCommandConnectionManager() >"
                " No command connection manager with ID [" + Convert::toString(id)
                + "] was found.");
    }
    else
    {
        commandConnectionManagers.erase(manager);
    }
}

void SyncServer_Core::NetworkManager::stopDataConnectionManager(ConnectionManagerID id)
{
    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = dataConnectionManagers.find(id);
    if(manager == dataConnectionManagers.end())
    {
        throw std::logic_error("NetworkManager::stopDataConnectionManager() >"
                " No data connection manager with ID [" + Convert::toString(id)
                + "] was found.");
    }
    else
    {
        dataConnectionManagers.erase(manager);
    }
}
//</editor-fold>

void SyncServer_Core::NetworkManager::sendInstruction
(const ConnectionManagerID managerID, const DeviceID device, const InstructionBasePtr instruction)
{
    bool isConnectionAvailable = false;
    CommandID newCommandID = INVALID_COMMAND_ID;

    {
        boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
        auto activeConnection = activeCommandConnections.find(device);
        if(activeConnection != activeCommandConnections.end())
        {
            boost::lock_guard<boost::mutex> connectionDataLock(activeConnection->second->dataMutex);
            ++activeConnection->second->eventsCounter;
            isConnectionAvailable =  true;
            newCommandID = ++activeConnection->second->lastCommandID;
        }
        else
        {
            auto pendingDataQueue = pendingDeviceInstructions.find(device);
            if(pendingDataQueue != pendingDeviceInstructions.end())
            {
                pendingDataQueue->second.push_back(instruction);
            }
            else
            {
                std::deque<InstructionBasePtr> instructionQueue;
                instructionQueue.push_back(instruction);
                pendingDeviceInstructions.insert(
                        std::pair<DeviceID, std::deque<InstructionBasePtr>>
                        (device, instructionQueue));
            }
        }
    }

    if(isConnectionAvailable)
    {
        const CommandConverter::CommandData commandData =
            converter.serializeCommand(instruction, device, newCommandID);
        
        commandConnections.sendData(device, commandData.serializedData);
        ++commandsSent;
    }
    else if(!dataStore.isCommandConnectionDataAvailable(device))
    {
        initiateCommandConnection(managerID, device);
        networkingThreadPool.assignTimedTask(
                boost::bind(&NetworkManager::pendingDeviceInstructionsDiscardTimeoutHandler, this, device),
                pendingConnectionDataDiscardTimeout);
    }
}

//<editor-fold defaultstate="collapsed" desc="Handlers - Connection Setup">
void SyncServer_Core::NetworkManager::initiateDeviceSetupProcess(
    const ConnectionManagerID managerID, const IPAddress initAddress, const IPPort initPort,
    const std::string & sharedPassword, const PeerType remotePeerType, const DeviceID remotePeerID,
    const TransientConnectionID transientID)
{
    dataStore.addInitConnectionData(
            initAddress,
            initPort,
            PendingInitConnectionConfigPtr(
                new PendingInitConnectionConfig{sharedPassword, remotePeerType, remotePeerID, transientID}));

    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = initConnectionManagers.find(managerID);
    if(manager == initConnectionManagers.end())
    {
        throw std::invalid_argument("NetworkManager::initiateDeviceSetupProcess() >"
                " No init connection manager with ID ["
                + Convert::toString(managerID) + "] was found.");
    }
    else
    {
        manager->second->initiateNewConnection(initAddress, initPort);
    }

    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::initConnectionDataDiscardTimeoutHandler, this, transientID),
            pendingConnectionDataDiscardTimeout);
}

void SyncServer_Core::NetworkManager::waitForDeviceSetupProcess(
    const std::string & sharedPassword, const PeerType remotePeerType,
    const DeviceID remotePeerID, const TransientConnectionID transientID)
{
    dataStore.addInitConnectionData(
            INVALID_IP_ADDRESS,
            INVALID_IP_PORT,
            PendingInitConnectionConfigPtr(
                new PendingInitConnectionConfig{sharedPassword, remotePeerType, remotePeerID, transientID}));

    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::initConnectionDataDiscardTimeoutHandler, this, transientID),
            expectedInitConnectionTimeout);
}

void SyncServer_Core::NetworkManager::initiateCommandConnection
(const ConnectionManagerID managerID, const DeviceID targetDevice)
{
    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = commandConnectionManagers.find(managerID);
    if(manager == commandConnectionManagers.end())
    {
        throw std::invalid_argument("NetworkManager::initiateNewCommandConnection() >"
                " No command connection manager with ID [" + Convert::toString(managerID)
                + "] was found.");
    }
    else
    {
        DeviceDataContainerPtr targetDeviceData = databaseManager.Devices().getDevice(targetDevice);
        dataStore.addCommandConnectionData(targetDeviceData);
        manager->second->initiateNewConnection(
            targetDeviceData->getDeviceCommandAddress(),
            targetDeviceData->getDeviceCommandPort());
    }

    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::commandConnectionDataDiscardTimeoutHandler, this, targetDevice),
            pendingConnectionDataDiscardTimeout);
}

void SyncServer_Core::NetworkManager::initiateDataConnection(
    const ConnectionManagerID managerID, const TransientConnectionID transientID,
    DeviceDataContainerPtr data, SymmetricCryptoHandlerPtr crypto, bool encrypt, bool compress)
{
    dataStore.addDataConnectionData(
        PendingDataConnectionConfigPtr(
            new PendingDataConnectionConfig{transientID, data, crypto, encrypt, compress}));

    boost::lock_guard<boost::mutex> connectionManagementLock(connectionManagementDataMutex);
    auto manager = dataConnectionManagers.find(managerID);
    if(manager == dataConnectionManagers.end())
    {
        throw std::invalid_argument("NetworkManager::initiateDataConnection() >"
                " No data connection manager with ID ["
                + Convert::toString(managerID) + "] was found.");
    }
    else
    {
        manager->second->initiateNewConnection(
            data->getDeviceDataAddress(),
            data->getDeviceDataPort());
    }

    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::dataConnectionDataDiscardTimeoutHandler,
                        this, data->getDeviceID(), transientID),
            pendingConnectionDataDiscardTimeout);
}

void SyncServer_Core::NetworkManager::waitForDataConnection(
    const TransientConnectionID transientID, DeviceDataContainerPtr data,
    SymmetricCryptoHandlerPtr crypto, bool encrypt, bool compress)
{
    dataStore.addDataConnectionData(
        PendingDataConnectionConfigPtr(
            new PendingDataConnectionConfig{transientID, data, crypto, encrypt, compress}));
            
    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::dataConnectionDataDiscardTimeoutHandler,
                        this, data->getDeviceID(), transientID),
            expectedDataConnectionTimeout);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Misc">
DeviceDataContainerPtr SyncServer_Core::NetworkManager::loadCommandConnectionDeviceData
(const DeviceID targetDevice)
{
    DeviceDataContainerPtr targetDeviceData = databaseManager.Devices().getDevice(targetDevice);
    dataStore.addCommandConnectionData(targetDeviceData);
    return targetDeviceData;
}

InstructionBasePtr SyncServer_Core::NetworkManager::retrievePendingInstruction
(ActiveConnectionDataPtr connectionData, const CommandID commandID)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->dataMutex);
    auto instructionData = connectionData->pendingInstructions.find(commandID);
    if(instructionData != connectionData->pendingInstructions.end())
    {
        return instructionData->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(retrievePendingInstruction) >"
                " No pending instruction found for command ["
                + Convert::toString(commandID) + "] for device ["
                + Convert::toString(connectionData->deviceID) + "].");
        
        throw std::runtime_error("NetworkManager::retrievePendingInstruction()"
                " > No pending instruction found for command ["
                + Convert::toString(commandID) + "] for device ["
                + Convert::toString(connectionData->deviceID) + "].");
    }
}

void SyncServer_Core::NetworkManager::enqueueRemoteInstructionResultProcessing
(const DeviceID deviceID, std::function<const PlaintextData (void)> responseSerializationFunction)
{
    auto resultProcessingFunction = [this, deviceID, responseSerializationFunction]()
    {
        const PlaintextData responseData = responseSerializationFunction();
        commandConnections.sendData(deviceID, responseData);
    };

    instructionsThreadPool.assignTask(resultProcessingFunction);
}

void SyncServer_Core::NetworkManager::setPendingConnectionState
(const ConnectionID id, ConnectionSetupState state)
{
    boost::lock_guard<boost::mutex> pendingConnectionsLock(pendingConnectionsMutex);
    auto connectionState = pendingConnections.find(id);
    if(connectionState != pendingConnections.end())
    {
        connectionState->second = state;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Timeouts">
void SyncServer_Core::NetworkManager::pendingConnectionTimeoutHandler
(const ConnectionID id, ConnectionPtr connection)
{
    boost::lock_guard<boost::mutex> pendingConnectionsLock(pendingConnectionsMutex);
    auto connectionState = pendingConnections.find(id);
    if(connectionState != pendingConnections.end())
    {
        if(connectionState->second != ConnectionSetupState::COMPLETED
           && connectionState->second != ConnectionSetupState::FAILED)
        {
            logMessage(LogSeverity::Error, "(pendingConnectionTimeoutHandler) >"
                    " Connection setup for [" + Convert::toString(id) + "] ("
                    + Convert::toString(connection->getConnectionType())
                    + ") not completed in time; disconnecting.");
            
            connection->disconnect();
        }

        pendingConnections.erase(connectionState);
    }
    else
    {
        logMessage(LogSeverity::Error, "(pendingConnectionTimeoutHandler) >"
                " No connection state data found for [" + Convert::toString(id)
                + "] (" + Convert::toString(connection->getConnectionType()) + ").");
        
        connection->disconnect();
        
        throw std::logic_error("NetworkManager::pendingConnectionTimeoutHandler() >"
                " No connection state data found for [" + Convert::toString(id)
                + "] (" + Convert::toString(connection->getConnectionType()) + ").");
    }
}

void SyncServer_Core::NetworkManager::activeConnectionTimeoutHandler
(ActiveConnectionDataPtr connectionData, const StatCounter lastEventsCount)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->dataMutex);
    if(lastEventsCount == connectionData->eventsCounter)
    {//a connection is considered inactive if no new events were received between handler invocations
        logMessage(LogSeverity::Debug, "(activeConnectionTimeoutHandler) >"
                " No events received for connection ["
                + Convert::toString(connectionData->connectionID) + "].");

        switch(connectionData->type)
        {
            case ConnectionType::COMMAND:
                commandConnections.closeEstablishedConnection(connectionData->deviceID); break;
                
            case ConnectionType::DATA:
                dataConnections.closeConnection(connectionData->deviceID, connectionData->connectionID); break;
                
            default:
            {
                logMessage(LogSeverity::Error, "(activeConnectionTimeoutHandler) >"
                        " Unexpected connection type [" + Convert::toString(connectionData->type)
                        + "] encountered for connection ["
                        + Convert::toString(connectionData->connectionID) + "].");
                
                throw std::logic_error("NetworkManager::activeConnectionTimeoutHandler() >"
                        " Unexpected connection type [" + Convert::toString(connectionData->type)
                        + "] encountered for connection ["
                        + Convert::toString(connectionData->connectionID) + "].");
            }
        }
    }
    else
    {
        switch(connectionData->type)
        {
            case ConnectionType::COMMAND:
            {
                networkingThreadPool.assignTimedTask(
                        boost::bind(&NetworkManager::activeConnectionTimeoutHandler,
                                    this, connectionData, connectionData->eventsCounter),
                        commandConnectionInactivityTimeout);
            } break;

            case ConnectionType::DATA:
            {
                networkingThreadPool.assignTimedTask(
                        boost::bind(&NetworkManager::activeConnectionTimeoutHandler,
                                    this, connectionData, connectionData->eventsCounter),
                        dataConnectionInactivityTimeout);
            } break;

            default:
            {
                logMessage(LogSeverity::Error, "(activeConnectionTimeoutHandler) >"
                        " Unexpected connection type [" + Convert::toString(connectionData->type)
                        + "] encountered for connection ["
                        + Convert::toString(connectionData->connectionID) + "].");
                
                throw std::logic_error("NetworkManager::activeConnectionTimeoutHandler()"
                        " > Unexpected connection type [" + Convert::toString(connectionData->type)
                        + "] encountered for connection ["
                        + Convert::toString(connectionData->connectionID) + "].");
            }
        }
    }
}

void SyncServer_Core::NetworkManager::pendingDeviceInstructionsDiscardTimeoutHandler
(const DeviceID device)
{
    boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
    auto pendingInstructions = pendingDeviceInstructions.find(device);
    if(pendingInstructions != pendingDeviceInstructions.end())
    {
        logMessage(LogSeverity::Warning, "(pendingDeviceInstructionsDiscardTimeoutHandler) >"
                " Discarding [" + Convert::toString(pendingInstructions->second.size())
                + "] pending instructions for [" + Convert::toString(device) + "].");
        pendingDeviceInstructions.erase(pendingInstructions);
    }
}

void SyncServer_Core::NetworkManager::initConnectionDataDiscardTimeoutHandler
(const TransientConnectionID transientID)
{
    if(dataStore.discardInitConnectionData(transientID))
    {
        logMessage(LogSeverity::Warning, "(initConnectionDataDiscardTimeoutHandler) >"
                " Discarded 'INIT' data for transient connection ["
                + Convert::toString(transientID) + "].");
    }
}

void SyncServer_Core::NetworkManager::commandConnectionDataDiscardTimeoutHandler
(const DeviceID device)
{
    if(dataStore.discardCommandConnectionData(device))
    {
        logMessage(LogSeverity::Warning, "(commandConnectionDataDiscardTimeoutHandler) >"
                " Discarded 'COMMAND' data for device [" + Convert::toString(device) + "].");
    }
}

void SyncServer_Core::NetworkManager::dataConnectionDataDiscardTimeoutHandler
(const DeviceID device, const TransientConnectionID transientID)
{
    if(dataStore.discardDataConnectionData(device, transientID))
    {
        logMessage(LogSeverity::Warning, "(initConnectionDataDiscardTimeoutHandler) >"
                " Discarded 'DATA' data for device [" + Convert::toString(device)
                + "] and transient connection [" + Convert::toString(transientID) + "].");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Connection Setup Results">
void SyncServer_Core::NetworkManager::onCommandConnectionEstablishedHandler
(const DeviceID deviceID, const ConnectionID connectionID)
{
    setPendingConnectionState(connectionID, ConnectionSetupState::COMPLETED);

    DeviceDataContainerPtr deviceData = dataStore.getCommandConnectionData(deviceID);
    ActiveConnectionDataPtr connectionData(
        new ActiveConnectionData
        {
            deviceID,                   //DeviceID
            connectionID,               //ConnectionID
            ConnectionType::COMMAND,    //ConnectionType
            0,                          //StatCounter
            deviceData,                 //DeviceDataContainerPtr
            INVALID_COMMAND_ID          //CommandID
        });

    boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
    
    auto result = activeCommandConnections.insert(
        std::pair<DeviceID, ActiveConnectionDataPtr>(deviceID, connectionData));
    if(!result.second)
    {
        logMessage(LogSeverity::Error, "(onCommandConnectionEstablishedHandler) >"
                " Failed to set connection [" + Convert::toString(connectionID)
                + "] for device [" + Convert::toString(deviceID) + "] as established.");
        
        throw std::logic_error("NetworkManager::onCommandConnectionEstablishedHandler() >"
                " Failed to set connection [" + Convert::toString(connectionID)
                + "] for device [" + Convert::toString(deviceID) + "] as established.");
    }
    else
    {
        networkingThreadPool.assignTimedTask(
                boost::bind(&NetworkManager::activeConnectionTimeoutHandler, this, connectionData, 0),
                commandConnectionInactivityTimeout);
    }

    if(!dataStore.discardCommandConnectionData(deviceID))
    {
        logMessage(LogSeverity::Warning, "(onCommandConnectionEstablishedHandler) >"
                " Failed to discard pending data for device [" + Convert::toString(deviceID) + "].");
    }

    auto pendingInstructions = pendingDeviceInstructions.find(deviceID);
    if(pendingInstructions != pendingDeviceInstructions.end())
    {
        for(auto currentInstruction : pendingInstructions->second)
        {
            CommandID newCommandID = ++connectionData->lastCommandID;
            
            const CommandConverter::CommandData commandData =
                converter.serializeCommand(currentInstruction, deviceID, newCommandID);
            
            connectionData->pendingInstructions.insert(
                std::pair<CommandID, InstructionBasePtr>(newCommandID, currentInstruction));
            
            commandConnections.sendData(deviceID, commandData.serializedData);
            ++commandsSent;
        }

        pendingDeviceInstructions.erase(pendingInstructions);
    }
}

void SyncServer_Core::NetworkManager::onCommandConnectionEstablishmentFailedHandler
(const DeviceID deviceID, const ConnectionID connectionID)
{
    setPendingConnectionState(connectionID, ConnectionSetupState::FAILED);

    if(!dataStore.discardCommandConnectionData(deviceID))
    {
        logMessage(LogSeverity::Warning, "(onCommandConnectionEstablishmentFailedHandler) >"
                " Failed to discard pending data for device [" + Convert::toString(deviceID) + "].");
    }

    boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
    auto pendingInstructions = pendingDeviceInstructions.find(deviceID);
    if(pendingInstructions != pendingDeviceInstructions.end())
    {
        logMessage(LogSeverity::Warning, "(onCommandConnectionEstablishmentFailedHandler) >"
                " Discarding [" + Convert::toString(pendingInstructions->second.size())
                + "] pending commands for device [" + Convert::toString(deviceID) + "].");

        pendingDeviceInstructions.erase(pendingInstructions);
    }
}

void SyncServer_Core::NetworkManager::onDataConnectionEstablishedHandler
(const DeviceID deviceID, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    setPendingConnectionState(connectionID, ConnectionSetupState::COMPLETED);

    DeviceDataContainerPtr deviceData = databaseManager.Devices().getDevice(deviceID);
    ActiveConnectionDataPtr connectionData(
        new ActiveConnectionData
        {
            deviceID,               //DeviceID
            connectionID,           //ConnectionID
            ConnectionType::DATA,   //ConnectionType
            0,                      //StatCounter
            deviceData,             //DeviceDataContainerPtr
            INVALID_COMMAND_ID      //CommandID
        });

    boost::lock_guard<boost::mutex> activeDataConnectionsLock(activeDataConnectionsMutex);
    auto deviceDataConnections = activeDataConnections.find(deviceID);
    if(deviceDataConnections == activeDataConnections.end())
    {
        boost::unordered_map<ConnectionID, ActiveConnectionDataPtr> deviceConnectionsMap;
        
        deviceConnectionsMap.insert(
            std::pair<ConnectionID, ActiveConnectionDataPtr>(connectionID, connectionData));
        
        activeDataConnections.insert(
            std::pair<DeviceID, boost::unordered_map<ConnectionID, ActiveConnectionDataPtr>>
                (deviceID, deviceConnectionsMap));
    }
    else
    {
        auto result = deviceDataConnections->second.insert(
            std::pair<ConnectionID, ActiveConnectionDataPtr>(connectionID, connectionData));

        if(!result.second)
        {
            logMessage(LogSeverity::Error, "(onDataConnectionEstablishedHandler) >"
                    " Failed to set connection [" + Convert::toString(connectionID)
                    + "] for device [" + Convert::toString(deviceID) + "] as established.");
            
            throw std::logic_error("NetworkManager::onDataConnectionEstablishedHandler() >"
                    " Failed to set connection [" + Convert::toString(connectionID)
                    + "] for device [" + Convert::toString(deviceID) + "] as established.");
        }
    }

    networkingThreadPool.assignTimedTask(
            boost::bind(&NetworkManager::activeConnectionTimeoutHandler, this, connectionData, 0),
            dataConnectionInactivityTimeout);
}

void SyncServer_Core::NetworkManager::onDataConnectionEstablishmentFailedHandler
(const DeviceID deviceID, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    setPendingConnectionState(connectionID, ConnectionSetupState::FAILED);

    if(!dataStore.discardDataConnectionData(deviceID, transientID))
    {
        logMessage(LogSeverity::Warning, "(onDataConnectionEstablishmentFailedHandler) >"
                " Failed to discard pending data for device [" + Convert::toString(deviceID)
                + "] and transient connection [" + Convert::toString(transientID) + "].");
    }
}

void SyncServer_Core::NetworkManager::onInitSetupCompletedHandler(
    const ConnectionID connectionID, const DeviceID deviceID,
    const TransientConnectionID transientID,
    const NewDeviceConnectionParameters & deviceConfig)
{
    ++setupsCompleted;
    
    setPendingConnectionState(connectionID, ConnectionSetupState::COMPLETED);
    DeviceDataContainerPtr deviceData = databaseManager.Devices().getDevice(deviceID);
    deviceData->setDeviceCommandAddress(deviceConfig.ipSettings.commandAddress);
    deviceData->setDeviceCommandPort(deviceConfig.ipSettings.commandPort);
    deviceData->setDeviceDataAddress(deviceConfig.ipSettings.dataAddress);
    deviceData->setDeviceDataPort(deviceConfig.ipSettings.dataPort);
    deviceData->setDeviceInitAddress(deviceConfig.ipSettings.initAddress);
    deviceData->setDeviceInitPort(deviceConfig.ipSettings.initPort);

    try
    {
        deviceData->resetPassword(securityManager.hashDevicePassword(deviceConfig.rawPassword));
    }
    catch(const InvalidPassswordException & e)
    {
        ++setupsPartiallyCompleted;
        logMessage(LogSeverity::Error, "(onInitSetupCompletedHandler) >"
                " Invalid password supplied on connection [" + Convert::toString(connectionID)
                + "] for device [" + Convert::toString(deviceID) + "]: [" + e.what() + "].");
    }

    deviceData->resetRawPublicKey(deviceConfig.rawPublicKey, deviceConfig.expectedKeyExchange);
    databaseManager.Devices().updateDevice(deviceData);

    if(!dataStore.discardInitConnectionData(transientID))
    {
        logMessage(LogSeverity::Warning, "(onInitSetupCompletedHandler) >"
                " Failed to discard pending data for transient connection ["
                + Convert::toString(transientID) + "].");
    }
}

void SyncServer_Core::NetworkManager::onInitSetupFailedHandler
(const ConnectionID connectionID, const TransientConnectionID transientID)
{
    ++setupsFailed;
    
    setPendingConnectionState(connectionID, ConnectionSetupState::FAILED);

    if(!dataStore.discardInitConnectionData(transientID))
    {
        logMessage(LogSeverity::Warning, "(onInitSetupFailedHandler) >"
                " Failed to discard pending data for transient connection ["
                + Convert::toString(transientID) + "].");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Closed Connections">
void SyncServer_Core::NetworkManager::onEstablishedCommandConnectionClosed
(const DeviceID deviceID, const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
    auto deviceData = activeCommandConnections.find(deviceID);
    if(deviceData != activeCommandConnections.end())
    {
        activeCommandConnections.erase(deviceData);
    }
    else
    {
        logMessage(LogSeverity::Error, "(onEstablishedCommandConnectionClosed) >"
                " No data found for device [" + Convert::toString(deviceID) + "].");
        
        throw std::logic_error("NetworkManager::onEstablishedCommandConnectionClosed() >"
                " No data found for device [" + Convert::toString(deviceID) + "].");
    }
}

void SyncServer_Core::NetworkManager::onEstablishedDataConnectionClosed
(const DeviceID deviceID, const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> activeDataConnectionsLock(activeDataConnectionsMutex);
    auto deviceData = activeDataConnections.find(deviceID);
    if(deviceData != activeDataConnections.end())
    {
        auto connectionData = deviceData->second.find(connectionID);
        if(connectionData != deviceData->second.end())
        {
            deviceData->second.erase(connectionData);

            if(deviceData->second.empty())
            {
                activeDataConnections.erase(deviceData);
            }
        }
        else
        {
            logMessage(LogSeverity::Error, "(onEstablishedDataConnectionClosed) >"
                    " No data found for connection [" + Convert::toString(connectionID) + "].");
            
            throw std::logic_error("NetworkManager::onEstablishedDataConnectionClosed() >"
                    " No data found for connection [" + Convert::toString(connectionID) + "].");
        }
    }
    else
    {
        logMessage(LogSeverity::Error, "(onEstablishedDataConnectionClosed) >"
                " No data found for device [" + Convert::toString(deviceID) + "].");
        
        throw std::logic_error("NetworkManager::onEstablishedDataConnectionClosed() >"
                " No data found for device [" + Convert::toString(deviceID) + "].");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Data Received">
void SyncServer_Core::NetworkManager::onCommandDataReceivedHandler
(const DeviceID deviceID, const PlaintextData data)
{
    ActiveConnectionDataPtr connectionData;
    {
        boost::lock_guard<boost::mutex> activeCommandConnectionsLock(activeCommandConnectionsMutex);
        auto activeConnection = activeCommandConnections.find(deviceID);
        if(activeConnection != activeCommandConnections.end())
        {
            boost::lock_guard<boost::mutex> connectionDataLock(activeConnection->second->dataMutex);
            connectionData = activeConnection->second;
            ++connectionData->eventsCounter;
        }
        else
        {
            logMessage(LogSeverity::Error, "(onCommandDataReceivedHandler) >"
                    " No active connection data found for device [" + Convert::toString(deviceID) + "].");
            
            throw std::logic_error("NetworkManager::onCommandDataReceivedHandler() >"
                    " No active connection data found for device [" + Convert::toString(deviceID) + "].");
        }
    }

    try
    {
        CommandConverter::CommandData parsedCommand =
                converter.parseCommand(
                    data,
                    deviceID,
                    boost::bind(&NetworkManager::enqueueRemoteInstructionResultProcessing,
                                this, deviceID, _1));
        
        AuthorizationRequest request(
                connectionData->data->getDeviceOwner(),
                deviceID,
                *this,
                SecurableComponentType::NETWORK_MANAGER,
                parsedCommand.instruction);

        AuthorizationTokenPromisePtr promise = securityManager.postRequest(request);

        try
        {
            AuthorizationTokenPtr token = promise->get_future().get();
            processInstruction(parsedCommand.instruction, token);
            ++commandsReceived;
        }
        catch(const std::exception & e)
        {
            logMessage(LogSeverity::Error, "(onCommandDataReceivedHandler) >"
                    " Exception encountered while retrieving"
                    " instruction authorization for device ["
                    + Convert::toString(deviceID) + "]: [" + e.what() + "].");
        }
    }
    catch(const std::runtime_error &)
    {
        try
        {
            converter.parseResponse(
                    data,
                    deviceID,
                    boost::bind(&NetworkManager::retrievePendingInstruction, 
                                this, connectionData, _1));
        }
        catch(const std::runtime_error & e)
        {
            logMessage(LogSeverity::Error, "(onCommandDataReceivedHandler) >"
                    " Invalid command/response data received for device ["
                    + Convert::toString(deviceID) + "].");
        }
        catch(const std::exception & e)
        {
            logMessage(LogSeverity::Error, "(onCommandDataReceivedHandler) >"
                    " Exception encountered while parsing response data for device ["
                    + Convert::toString(deviceID) + "]: [" + e.what() + "].");
            throw;
        }
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onCommandDataReceivedHandler) >"
                " Exception encountered while parsing command data for device ["
                + Convert::toString(deviceID) + "]: [" + e.what() + "].");
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Connection Managers">
void SyncServer_Core::NetworkManager::onConnectionCreatedHandler
(ConnectionPtr connection, ConnectionInitiation initiation, ConnectionManagerID managerID)
{
    ConnectionID newConnectionID = getNewConnectionID();

    {
        boost::lock_guard<boost::mutex> pendingConnectionsLock(pendingConnectionsMutex);
        auto result = pendingConnections.insert(
            std::pair<ConnectionID, ConnectionSetupState>(newConnectionID, ConnectionSetupState::INITIATED));
        
        if(!result.second)
        {
            logMessage(LogSeverity::Error, "(onConnectionCreatedHandler) >"
                    " Failed to add pending connection state for ["
                    + Convert::toString(newConnectionID) + "].");
            
            throw std::logic_error("NetworkManager::onConnectionCreatedHandler() >"
                    " Failed to add pending connection state for ["
                    + Convert::toString(newConnectionID) + "].");
        }
    }

    switch(initiation)
    {
        case ConnectionInitiation::LOCAL:
        {
            switch(connection->getConnectionType())
            {
                case ConnectionType::COMMAND:
                {
                    DeviceDataContainerPtr deviceData =
                            dataStore.getCommandConnectionData(
                                connection->getRemoteAddress(),
                                connection->getRemotePort());
                    
                    commandConnections.manageLocalConnection(
                        connection,
                        newConnectionID,
                        deviceData->getDeviceID());
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            commandConnectionSetupTimeout);
                } break;

                case ConnectionType::DATA:
                {
                    PendingDataConnectionConfigPtr config =
                            dataStore.getDataConnectionData(
                                connection->getRemoteAddress(),
                                connection->getRemotePort());
                    
                    dataConnections.manageLocalConnection(
                        connection,
                        newConnectionID,
                        config);
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            dataConnectionSetupTimeout);
                } break;

                case ConnectionType::INIT:
                {
                    PendingInitConnectionConfigPtr config =
                            dataStore.getInitConnectionData(
                                connection->getRemoteAddress(),
                            connection->getRemotePort());
                    
                    initConnections.manageLocalConnection(
                        connection,
                        newConnectionID,
                        config);
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            initConnectionSetupTimeout);
                } break;

                default:
                {
                    logMessage(LogSeverity::Error, "(onConnectionCreatedHandler) >"
                            " Unexpected connection type encountered ["
                            + Convert::toString(connection->getConnectionType())
                            + "] for connection [" + Convert::toString(newConnectionID)
                            + "] from manager [" + Convert::toString(managerID) + "].");
                    
                    throw std::logic_error("NetworkManager::onConnectionCreatedHandler() >"
                            " Unexpected connection type encountered ["
                            + Convert::toString(connection->getConnectionType())
                            + "] for connection [" + Convert::toString(newConnectionID)
                            + "] from manager [" + Convert::toString(managerID) + "].");
                }
            }

            ++connectionsInitiated;
        } break;

        case ConnectionInitiation::REMOTE:
        {
            switch(connection->getConnectionType())
            {
                case ConnectionType::COMMAND:
                {
                    commandConnections.manageRemoteConnection(connection, newConnectionID);
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            commandConnectionSetupTimeout);
                } break;

                case ConnectionType::DATA:
                {
                    dataConnections.manageRemoteConnection(connection, newConnectionID);
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            dataConnectionSetupTimeout);
                } break;

                case ConnectionType::INIT:
                {
                    initConnections.manageRemoteConnection(connection, newConnectionID);
                    
                    networkingThreadPool.assignTimedTask(
                            boost::bind(&NetworkManager::pendingConnectionTimeoutHandler,
                                        this, newConnectionID, connection),
                            initConnectionSetupTimeout);
                } break;

                default:
                {
                    logMessage(LogSeverity::Error, "(onConnectionCreatedHandler) >"
                            " Unexpected connection type encountered ["
                            + Convert::toString(connection->getConnectionType())
                            + "] for connection [" + Convert::toString(newConnectionID)
                            + "] from manager [" + Convert::toString(managerID) + "].");
                    
                    throw std::logic_error("NetworkManager::onConnectionCreatedHandler() >"
                            " Unexpected connection type encountered ["
                            + Convert::toString(connection->getConnectionType())
                            + "] for connection [" + Convert::toString(newConnectionID)
                            + "] from manager [" + Convert::toString(managerID) + "].");
                }
            }

            ++connectionsReceived;
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(onConnectionCreatedHandler) >"
                    " Unexpected initiation type encountered ["
                    + Convert::toString(initiation) + "] for connection ["
                    + Convert::toString(newConnectionID) + "] from manager ["
                    + Convert::toString(managerID) + "].");
            
            throw std::logic_error("NetworkManager::onConnectionCreatedHandler() >"
                    " Unexpected initiation type encountered ["
                    + Convert::toString(initiation) + "] for connection ["
                    + Convert::toString(newConnectionID) + "] from manager ["
                    + Convert::toString(managerID) + "].");
        }
    }
}

void SyncServer_Core::NetworkManager::onConnectionInitiationFailedHandler
(const boost::system::error_code & error, ConnectionManagerID managerID)
{
    logMessage(LogSeverity::Error, "(onConnectionInitiationFailedHandler) >"
            " Connection initiation for manager [" + Convert::toString(managerID)
            + "] failed: [" + error.message() + "].");
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Instructions">
void SyncServer_Core::NetworkManager::lifeCycleOpenDataConnectionHandler
(InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::LifeCycleOpenDataConnection> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::LifeCycleOpenDataConnection>(instruction);

    if(actualInstruction)
    {
        //builds the content encryption key (CEK)
        SymmetricCryptoDataGenerationRequest keyRequest
                (
                    *this,
                    actualInstruction->cipherType,
                    actualInstruction->cipherMode,
                    actualInstruction->key,
                    actualInstruction->iv
                );

        SymmetricCryptoDataContainerPromisePtr promise = securityManager.postRequest(keyRequest);

        //retrieves the required data
        DeviceDataContainerPtr deviceData = databaseManager.Devices().getDevice(actualInstruction->deviceID);
        SymmetricCryptoDataContainerPtr cekData = promise->get_future().get();
        SymmetricCryptoHandlerPtr cryptoHandler = SymmetricCryptoHandlerPtr(new SymmetricCryptoHandler(cekData));

        if(actualInstruction->managerID == INVALID_CONNECTION_MANAGER_ID)
        {
            waitForDataConnection(
                    actualInstruction->transientID,
                    deviceData,
                    cryptoHandler,
                    actualInstruction->encrypt,
                    actualInstruction->compress);
        }
        else
        {
            initiateDataConnection(
                    actualInstruction->managerID,
                    actualInstruction->transientID,
                    deviceData,
                    cryptoHandler,
                    actualInstruction->encrypt,
                    actualInstruction->compress);
        }
        
        resultValue = true;
    }

    auto result = boost::shared_ptr<InstructionResults::LifeCycleOpenDataConnection>(
        new InstructionResults::LifeCycleOpenDataConnection{resultValue});

    instruction->getPromise().set_value(result);
}

void SyncServer_Core::NetworkManager::lifeCycleOpenInitConnectionHandler
(InstructionPtr<NetworkManagerConnectionLifeCycleInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::LifeCycleOpenInitConnection> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::LifeCycleOpenInitConnection>(instruction);

    if(actualInstruction)
    {
        ++setupsStarted;
        if(actualInstruction->managerID == INVALID_CONNECTION_MANAGER_ID)
        {
            waitForDeviceSetupProcess(
                    actualInstruction->sharedPassword,
                    actualInstruction->remotePeerType,
                    actualInstruction->remotePeerID,
                    actualInstruction->transientID);
        }
        else
        {
            initiateDeviceSetupProcess(
                    actualInstruction->managerID,
                    actualInstruction->initAddress,
                    actualInstruction->initPort,
                    actualInstruction->sharedPassword,
                    actualInstruction->remotePeerType,
                    actualInstruction->remotePeerID,
                    actualInstruction->transientID);
        }
        
        resultValue = true;
    }

    auto result = boost::shared_ptr<InstructionResults::LifeCycleOpenInitConnection>(
        new InstructionResults::LifeCycleOpenInitConnection{resultValue});

    instruction->getPromise().set_value(result);
}
//</editor-fold>

void SyncServer_Core::NetworkManager::verifyAuthorizationToken(AuthorizationTokenPtr token)
{
    ++instructionsReceived;
    if(!token)
    {
       throw InvalidAuthorizationTokenException("NetworkManager::verifyAuthorizationToken() > "
                "An empty token was supplied."); 
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    auto requestedToken = authorizationTokens.find(token->getID());
    if(requestedToken != authorizationTokens.end())
    {
        if(*(requestedToken->second) == *token
            && (token->getAuthorizedSet() == NetworkManagerAdminInstructionTarget::getType()
                || token->getAuthorizedSet() == NetworkManagerStateInstructionTarget::getType()
                || token->getAuthorizedSet() == NetworkManagerConnectionLifeCycleInstructionTarget::getType()
                || token->getAuthorizedSet() == NetworkManagerConnectionBridgingInstructionTarget::getType()))
        {
            authorizationTokens.erase(requestedToken);
            ++instructionsProcessed;
        }
        else
        {
            throw InvalidAuthorizationTokenException("NetworkManager::verifyAuthorizationToken() > "
                    "The supplied token [" + Convert::toString(token->getID())
                    + "] does not match the one expected by the manager.");
        }
    }
    else
    {
        throw InvalidAuthorizationTokenException("NetworkManager::verifyAuthorizationToken() > "
                "The supplied token [" + Convert::toString(token->getID()) + "] was not found.");
    }
}
