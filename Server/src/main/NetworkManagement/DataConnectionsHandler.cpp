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

#include "DataConnectionsHandler.h"
#include <boost/thread/lock_guard.hpp>
#include "../Utilities/Strings/Common.h"
#include "../SecurityManagement/Crypto/SaltGenerator.h"

//Protocols
#include "../../compiled/protobuf/BaseComm.pb.h"
#include "Protocols/Utilities.h"

using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::DataConnectionSetupRequest;
using NetworkManagement_Protocols::DataConnectionSetupResponse;

NetworkManagement_Handlers::DataConnectionsHandler::DataConnectionsHandler
(const DataConnectionsHandlerParameters & params,
 std::function<PendingDataConnectionConfigPtr (const DeviceID, const TransientConnectionID)> cfgRetrievalHandler,
 std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authDataRetrievalHandler,
 Utilities::FileLoggerPtr debugLogger)
: debugLogger(debugLogger), compressor(params.compressionAccelerationLevel, params.maxDataSize),
  deviceConfigRetrievalHandler(cfgRetrievalHandler), authenticationDataRetrievalHandler(authDataRetrievalHandler),
  active(true), localPeerID(params.localPeerID), requestSignatureSize(params.requestSignatureSize), maxDataSize(params.maxDataSize)
{}

NetworkManagement_Handlers::DataConnectionsHandler::~DataConnectionsHandler()
{
    active = false;

    onConnectionEstablished.disconnect_all_slots();
    onConnectionEstablishmentFailed.disconnect_all_slots();
    onDataReceived.disconnect_all_slots();

    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    if(activeConnections.size() < 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(activeConnections.size()) + "] active connections found.");

        for(auto currentActiveConnectionsForDevice : activeConnections)
        {
            logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(currentActiveConnectionsForDevice.second.size())
                    + "] active connections found for device [" + Convert::toString(currentActiveConnectionsForDevice.first) + "].");

            for(auto currentActiveConnection : currentActiveConnectionsForDevice.second)
            {
                currentActiveConnection.second->onDataReceivedEventConnection.disconnect();
                currentActiveConnection.second->onDisconnectEventConnection.disconnect();
                currentActiveConnection.second->onWriteResultReceivedEventConnection.disconnect();

                while(currentActiveConnection.second->pendingSentData.size() > 0)
                {
                    delete currentActiveConnection.second->pendingSentData.front();
                    currentActiveConnection.second->pendingSentData.pop();
                }
            }
        }
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::manageLocalConnection
(ConnectionPtr connection, const ConnectionID connectionID, PendingDataConnectionConfigPtr config)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageLocalConnection) > Unable to process connection ["
                + Convert::toString(connectionID) + "] for device ["
                + Convert::toString(config->data->getDeviceID()) + "]; handler is not active.");
        
        connection->disconnect();
        return;
    }
    
    try
    {
        ConnectionDataPtr connectionData = createConnectionData(connectionID, config, connection);
        CiphertextData * requestData = generateConnectionRequestData(config->data->getDeviceID(), connectionData);
        connectionData->pendingSentData.push(requestData);
        connection->sendData(*requestData);
        connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT;

        //attaches the pending connection event handlers
        connectionData->onDataReceivedEventConnection =
                connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_PendingLocalConnections,
                                this, _1, _2, config->data->getDeviceID(), connectionID));
        
        connectionData->onDisconnectEventConnection =
                connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_PendingLocalConnections,
                                this, _1, config->data->getDeviceID(), connectionID, config->transientID));
        
        connectionData->onWriteResultReceivedEventConnection =
                connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections,
                                this, _1, config->data->getDeviceID(), connectionID));
        
        connection->enableDataEvents();
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Request generation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, config->data->getDeviceID());
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, config->data->getDeviceID());
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, config->data->getDeviceID());
        throw;
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::manageRemoteConnection
(ConnectionPtr connection, const ConnectionID connectionID)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageRemoteConnection) > Unable to process connection ["
                + Convert::toString(connectionID) + "] for ["
                + connection->getRemoteAddress() + " / " + Convert::toString(connection->getRemotePort())
                + "]; handler is not active.");
        connection->disconnect();
        return;
    }

    ConnectionDataPtr connectionData = createPendingConnectionData(connectionID, connection);

    //attaches the pending connection event handlers
    connectionData->onDataReceivedEventConnection =
            connection->onDataReceivedEventAttach(
                boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections,
                            this, _1, _2, connectionID));
    
    connectionData->onDisconnectEventConnection =
            connection->onDisconnectEventAttach(
                boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_PendingRemoteConnections,
                            this, _1, connectionID));

    connection->enableDataEvents();
}

void NetworkManagement_Handlers::DataConnectionsHandler::sendData
(const DeviceID deviceID, const ConnectionID connectionID, const PlaintextData & plaintextData)
{
    if(!active)
        return;

    ++sendRequestsMade;

    if(plaintextData.size() >= maxDataSize)
    {
        ++sendRequestsFailed;
        logMessage(LogSeverity::Error, "(sendData) > Cannot process data with size ["
                + Convert::toString(plaintextData.size())
                + "]; maximum is [" + Convert::toString(maxDataSize) + "].");
        
        terminateConnection(connectionID, deviceID);
        throw std::invalid_argument("DataConnectionsHandler::sendData() >"
                " Cannot process data with size [" + Convert::toString(plaintextData.size())
                + "]; maximum is [" + Convert::toString(maxDataSize) + "].");
    }

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);
    MixedData * dataToSend = new MixedData();
    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    try
    {
        if(connectionData->encryptionEnabled)
        {
            if(connectionData->compressionEnabled)
            {//compresses and encrypts the data
                ByteData compressedData;
                compressor.compressData(plaintextData, compressedData);
                connectionData->cryptoHandler->encryptData(compressedData, *dataToSend);
            }
            else
            {//only encrypts the data
                connectionData->cryptoHandler->encryptData(plaintextData, *dataToSend);
            }
        }
        else
        {
            if(connectionData->compressionEnabled)
            {//only compresses the data
                ByteData compressedData;
                compressor.compressData(plaintextData, compressedData);
                dataToSend->assign(compressedData);
            }
            else
            {//data is NOT compressed or encrypted
                dataToSend->assign(plaintextData);
            }
        }

        connectionData->connection->sendData(*dataToSend);
        connectionData->pendingSentData.push(dataToSend);
        
        logMessage(LogSeverity::Info, "(sendData) > Data sent to device ["
                + Convert::toString(deviceID) + "] on connection ["
                + Convert::toString(connectionID) + "].");
    }
    catch(const std::exception & e)
    {
        ++sendRequestsFailed;
        delete dataToSend;
        logMessage(LogSeverity::Error, "(sendData) > Exception encountered: ["
                + std::string(e.what()) + "] while sending data to device ["
                + Convert::toString(deviceID) + "] on connection ["
                + Convert::toString(connectionID) + "].");
        
        terminateConnection(connectionID, deviceID);
        connectionData->connection->disconnect();
        throw;
    }
    catch(...)
    {
        ++sendRequestsFailed;
        delete dataToSend;
        logMessage(LogSeverity::Error, "(sendData) >"
                " Unknown exception encountered while sending data to device ["
                + Convert::toString(deviceID) + "] on connection ["
                + Convert::toString(connectionID) + "].");
        
        terminateConnection(connectionID, deviceID);
        connectionData->connection->disconnect();
        throw;
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::closeConnection
(const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    terminateConnection(connectionID, deviceID);
}

//<editor-fold defaultstate="collapsed" desc="Data Management">
NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::createConnectionData
(const ConnectionID connectionID, PendingDataConnectionConfigPtr config, ConnectionPtr connection)
{
    if(!config->encrypt)
    {
        logMessage(LogSeverity::Warning, "(createConnectionData) >"
                " Encryption NOT enabled for device ["
                + Convert::toString(config->data->getDeviceID())
                + "] on connection [" + Convert::toString(connectionID) + "].");
    }
    
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    ConnectionData * newConnectionData = new ConnectionData
    {
        config->transientID,                //transient connection ID
        config->data,                       //device data
        connection,                         //connection pointer
        config->crypto,                     //symmetric crypto handler
        EMPTY_PLAINTEXT_DATA,               //last request signature data
        ConnectionSetupState::INITIATED,    //connection state
        config->encrypt,                    //encryption enabled?
        config->compress                    //compression enabled?
    };
    newConnectionData->lastPendingReceivedData.reserve(maxDataSize);

    auto deviceData = activeConnections.find(config->data->getDeviceID());
    if(deviceData == activeConnections.end())
    {//device has no existing data connections
        boost::unordered_map<ConnectionID, ConnectionDataPtr> newConnectionsMap;
        auto insertResult = activeConnections.insert(
            std::pair<DeviceID, boost::unordered_map<ConnectionID, ConnectionDataPtr>>(
                config->data->getDeviceID(), newConnectionsMap));
        
        deviceData = insertResult.first;
    }
    else if(deviceData->second.find(connectionID) != deviceData->second.end())
    {//device has an active data connection with the same ID
        delete newConnectionData;
        logMessage(LogSeverity::Error, "(createConnectionData) >"
                " Data already exists for device [" + Convert::toString(config->data->getDeviceID())
                + "] and connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::createConnectionData() >"
                " Data already exists for device [" + Convert::toString(config->data->getDeviceID())
                + "] and connection [" + Convert::toString(connectionID) + "].");
    }

    auto result = deviceData->second.insert(
        std::pair<ConnectionID, ConnectionDataPtr>(
            connectionID, ConnectionDataPtr(newConnectionData)));
    
    return result.first->second;
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::createConnectionData
(const ConnectionID connectionID, PendingDataConnectionConfigPtr config, ConnectionDataPtr pendingData)
{
    if(!config->encrypt)
    {
        logMessage(LogSeverity::Warning, "(createConnectionData) >"
                " Encryption NOT enabled for device [" + Convert::toString(config->data->getDeviceID())
                + "] on connection [" + Convert::toString(connectionID) + "].");
    }
    
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    pendingData->transientID = config->transientID;
    pendingData->deviceData = config->data;
    pendingData->cryptoHandler = config->crypto;
    pendingData->encryptionEnabled = config->encrypt;
    pendingData->compressionEnabled = config->compress;

    auto deviceData = activeConnections.find(config->data->getDeviceID());
    if(deviceData == activeConnections.end())
    {//device has no existing data connections
        boost::unordered_map<ConnectionID, ConnectionDataPtr> newConnectionsMap;
        auto insertResult = activeConnections.insert(
            std::pair<DeviceID, boost::unordered_map<ConnectionID, ConnectionDataPtr>>(
                config->data->getDeviceID(), newConnectionsMap));
        
        deviceData = insertResult.first;
    }
    else if(deviceData->second.find(connectionID) != deviceData->second.end())
    {//device has an active data connection with the same ID
        logMessage(LogSeverity::Error, "(createConnectionData) >"
                " Data already exists for device [" + Convert::toString(config->data->getDeviceID())
                + "] and connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::createConnectionData() >"
                " Data already exists for device [" + Convert::toString(config->data->getDeviceID())
                + "] and connection [" + Convert::toString(connectionID) + "].");
    }

    deviceData->second.insert(std::pair<ConnectionID, ConnectionDataPtr>(connectionID, pendingData));
    return pendingData;
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::getConnectionData
(const DeviceID deviceID, const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionDataMutex);
    auto connectionsData = activeConnections.find(deviceID);
    if(connectionsData != activeConnections.end())
    {
        auto result = connectionsData->second.find(connectionID);
        if(result != connectionsData->second.end())
        {
            return result->second;
        }
        else
        {
            logMessage(LogSeverity::Error, "(getConnectionData) >"
                    " Connection data not found for connection ["
                    + Convert::toString(connectionID) + "].");
            
            throw std::logic_error("DataConnectionsHandler::getConnectionData() >"
                    " Connection data not found for connection ["
                    + Convert::toString(connectionID) + "].");
        }
    }
    else
    {
        logMessage(LogSeverity::Error, "(getConnectionData) >"
                " Connection data not found for device ["
                + Convert::toString(deviceID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::getConnectionData()"
                " > Connection data not found for device ["
                + Convert::toString(deviceID) + "].");
    }
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::discardConnectionData
(const DeviceID deviceID, const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionDataMutex);
    auto connectionsData = activeConnections.find(deviceID);
    if(connectionsData != activeConnections.end())
    {
        auto result = connectionsData->second.find(connectionID);
        if(result != connectionsData->second.end())
        {
            ConnectionDataPtr resultData = result->second;

            connectionsData->second.erase(result);

            if(connectionsData->second.size() == 0)
            {//the device has no other active connections
                activeConnections.erase(connectionsData);
            }

            return resultData;
        }
        else
        {
            logMessage(LogSeverity::Warning, "(discardConnectionData) >"
                    " Data not found for connection [" + Convert::toString(connectionID) + "].");
            
            throw std::runtime_error("DataConnectionsHandler::discardConnectionData() >"
                    " Data not found for connection [" + Convert::toString(connectionID) + "].");
        }
    }
    else
    {
        logMessage(LogSeverity::Warning, "(discardConnectionData) >"
                " Connection data not found for device [" + Convert::toString(deviceID) + "].");
        
        throw std::runtime_error("DataConnectionsHandler::discardConnectionData()"
                " > Connection data not found for device [" + Convert::toString(deviceID) + "].");
    }
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::createPendingConnectionData
(const ConnectionID connectionID, ConnectionPtr connection)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    
    auto connectionData = pendingConnections.find(connectionID);
    if(connectionData == pendingConnections.end())
    {
        ConnectionData * newConnectionData = new ConnectionData
        {
            INVALID_TRANSIENT_CONNECTION_ID,    //no transient connection ID
            DeviceDataContainerPtr(),           //empty device data
            connection,                         //connection pointer
            SymmetricCryptoHandlerPtr(),        //empty symmetric crypto handler
            EMPTY_PLAINTEXT_DATA,               //last request signature data
            ConnectionSetupState::INITIATED,    //connection state
            false,                              //encryption not set
            false                               //compression not set
        };
        newConnectionData->lastPendingReceivedData.reserve(maxDataSize);
        
        auto insertResult = pendingConnections.insert(
            std::pair<ConnectionID, ConnectionDataPtr>(
                connectionID, ConnectionDataPtr(newConnectionData)));
        
        return insertResult.first->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(createPendingConnectionData) >"
                " Data already exists for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::createPendingConnectionData() >"
                " Data already exists for connection [" + Convert::toString(connectionID) + "].");
    }
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::getPendingConnectionData
(const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    
    auto connectionData = pendingConnections.find(connectionID);
    if(connectionData != pendingConnections.end())
    {
        return connectionData->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getPendingConnectionData) >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::getPendingConnectionData() >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
    }
}

NetworkManagement_Handlers::DataConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::DataConnectionsHandler::discardPendingConnectionData
(const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    auto connectionData = pendingConnections.find(connectionID);
    if(connectionData != pendingConnections.end())
    {
        ConnectionDataPtr result = connectionData->second;
        pendingConnections.erase(connectionData);
        return result;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getPendingConnectionData) >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::getPendingConnectionData() >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
    }
}

PendingDataConnectionConfigPtr NetworkManagement_Handlers::DataConnectionsHandler::getPendingConnectionConfig
(const DeviceID deviceID, const TransientConnectionID transientID)
{
    if(deviceConfigRetrievalHandler)
        return deviceConfigRetrievalHandler(deviceID, transientID);
    else
    {
        logMessage(LogSeverity::Error, "(getPendingConnectionConfig) >"
                " Failed to get config data; retrieval function not available.");
        
        throw std::logic_error("DataConnectionsHandler::getPendingConnectionConfig() >"
                "Failed to get config data; retrieval function not available.");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Crypto">
CiphertextData * NetworkManagement_Handlers::DataConnectionsHandler::generateConnectionRequestData
(const DeviceID remotePeerID, ConnectionDataPtr remotePeerData)
{
    //builds the request signature data
    RandomData signatureData(SecurityManagement_Crypto::SaltGenerator::getRandomSalt(requestSignatureSize));
    ConnectionSetupRequestSignature requestSignature;
    requestSignature.set_signature_size(requestSignatureSize);
    requestSignature.set_signature_data(signatureData.BytePtr(), requestSignatureSize);
    //serializes the request signature data
    PlaintextData plaintextSignature;
    if(!requestSignature.SerializeToString(&plaintextSignature))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize request signature.");
        
        throw std::runtime_error("DataConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize request signature.");
    }
    
    remotePeerData->requestSignatureData = plaintextSignature;

    //encrypts the request signature data
    CiphertextData encryptedSignature;
    remotePeerData->cryptoHandler->encryptData(plaintextSignature, encryptedSignature);

    //builds the request data
    DataConnectionSetupRequest request;
    request.set_request_signature(encryptedSignature);
    request.set_transient_id(remotePeerData->transientID);

    if(remotePeerData->deviceData->getDeviceType() == PeerType::SERVER)
    {
        auto authenticationData = authenticationDataRetrievalHandler(remotePeerID);
        request.set_peer_id(Convert::toString(authenticationData.id));
    }
    else
    {
        request.set_peer_id(Convert::toString(localPeerID));
    }

    //serializes the request data
    PlaintextData * serializedRequest = new PlaintextData();
    try
    {
        if(!request.SerializeToString(serializedRequest))
        {
            logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                    " Failed to serialize request.");
            
            throw std::runtime_error("DataConnectionsHandler::generateConnectionRequestData() >"
                    " Failed to serialize request.");
        }
    }
    catch(...)
    {
        delete serializedRequest;
        throw;
    }

    return serializedRequest;
}

CiphertextData * NetworkManagement_Handlers::DataConnectionsHandler::generateConnectionResponseDataFromRequest
(const CiphertextData & encryptedRequest, const ConnectionID connectionID)
{
    //parses the request data
    DataConnectionSetupRequest requestObject;
    requestObject.ParseFromString(encryptedRequest);

    //validates the request
    if(!requestObject.IsInitialized())
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to validate data connection setup request.");
        
        throw std::runtime_error("DataConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to validate data connection setup request.");
    }

    ConnectionDataPtr pendingPeerData = discardPendingConnectionData(connectionID);
    
    PendingDataConnectionConfigPtr pendingConnectionConfig =
            getPendingConnectionConfig(
                Convert::toDeviceID(requestObject.peer_id()), requestObject.transient_id());
    
    ConnectionDataPtr remotePeerData =
            createConnectionData(connectionID, pendingConnectionConfig, pendingPeerData);

    if(!remotePeerData->cryptoHandler)
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " No crypto handler found for [" + requestObject.peer_id()
                + " / " + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("DataConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " No crypto handler found for [" + requestObject.peer_id()
                + " / " + Convert::toString(connectionID) + "].");
    }

    //decrypts the request signature data
    PlaintextData plaintextRequestSignature;
    remotePeerData->cryptoHandler->decryptData(
        requestObject.request_signature(), plaintextRequestSignature);

    try
    {
        //verifies the request signature data
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(plaintextRequestSignature);
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to verify request signature.");
        throw;
    }

    //builds the response data
    DataConnectionSetupResponse response;
    response.set_request_signature(plaintextRequestSignature);

    //serializes the response data
    PlaintextData plaintextResponse;
    if(!response.SerializeToString(&plaintextResponse))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to serialize response.");
        
        throw std::runtime_error("DataConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to serialize response.");
    }

    //encrypts the response data
    CiphertextData * encryptedResponse = new CiphertextData();
    try
    {
        remotePeerData->cryptoHandler->encryptData(plaintextResponse, *encryptedResponse);
    }
    catch(...)
    {
        delete encryptedResponse;
        throw;
    }

    return encryptedResponse;
}

void NetworkManagement_Handlers::DataConnectionsHandler::verifyConnectionResponseData
(const CiphertextData & encryptedResponse, ConnectionDataPtr remotePeerData)
{
    //decrypts the response data
    PlaintextData plaintextResponse;
    remotePeerData->cryptoHandler->decryptData(encryptedResponse, plaintextResponse);

    //parses the response data
    DataConnectionSetupResponse responseObject;
    responseObject.ParseFromString(plaintextResponse);

    //validates the request
    if(!responseObject.IsInitialized())
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to validate data connection setup response.");
        
        throw std::runtime_error("DataConnectionsHandler::verifyConnectionResponseData() >"
                " Failed to validate data connection setup response.");
    }

    try
    {
        //verifies the response signature
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(
            responseObject.request_signature(), remotePeerData->requestSignatureData);
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to verify request signature.");
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Local Connections">
void NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_PendingLocalConnections
(RawConnectionID rawID, const DeviceID deviceID, const ConnectionID connectionID, const TransientConnectionID transientID) 
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_PendingLocalConnections) > Terminating connection ["
            + Convert::toString(connectionID) + "] for device [" + Convert::toString(deviceID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    terminateConnection(connectionID, deviceID);
    ++connectionsFailed;
    onConnectionEstablishmentFailed(deviceID, connectionID, transientID);
}

void NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_PendingLocalConnections
(ByteData data, PacketSize remaining, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);

    try
    {
        if(remaining > 0)
        {
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                    " Cannot process connection response for device ["
                    + Convert::toString(deviceID) + "] on connection ["
                    + Convert::toString(connectionID) + "]; more data remains to be received.");
            
            throw std::runtime_error("DataConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                    " Cannot process connection response for device ["
                    + Convert::toString(deviceID) + "] on connection ["
                    + Convert::toString(connectionID) + "]; more data remains to be received.");
        }

        connectionData->connection->disableDataEvents();

        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);

            if(connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT
                && connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED)
            {
                logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for device [" + Convert::toString(deviceID) + "] on connection ["
                        + Convert::toString(connectionID) + "].");
                
                throw std::logic_error("DataConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for device [" + Convert::toString(deviceID) + "] on connection ["
                        + Convert::toString(connectionID) + "].");
            }
            
            if(connectionData->state == ConnectionSetupState::CONNECTION_REQUEST_SENT)
            {
                delete connectionData->pendingSentData.front();
                connectionData->pendingSentData.pop();
            }
            
            connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_RECEIVED;
            
            verifyConnectionResponseData(data, connectionData);
            connectionData->state = ConnectionSetupState::COMPLETED;

            //detaches the pending connection handlers
            connectionData->onDataReceivedEventConnection.disconnect();
            connectionData->onDisconnectEventConnection.disconnect();
            connectionData->onWriteResultReceivedEventConnection.disconnect();
        }

        //attaches the established connection handlers
        connectionData->onDataReceivedEventConnection =
                connectionData->connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_EstablishedConnections,
                                this, _1, _2, deviceID, connectionID));
        
        connectionData->onDisconnectEventConnection =
                connectionData->connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        connectionData->onWriteResultReceivedEventConnection =
                connectionData->connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        connectionData->connection->enableDataEvents();
        
        logMessage(LogSeverity::Info, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Setup completed for device [" + Convert::toString(deviceID)
                + "] on connection [" + Convert::toString(connectionID) + "].");
        
        ++connectionsEstablished;
        onConnectionEstablished(deviceID, connectionID, connectionData->transientID);
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Response validation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        throw;
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections
(bool received, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);

    if(!received)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Failed to send request data on connection [" + Convert::toString(connectionID)
                + "] for device [" + Convert::toString(deviceID) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        return;
    }

    try
    {
        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
        if(connectionData->state == ConnectionSetupState::CONNECTION_REQUEST_SENT)
        {
            delete connectionData->pendingSentData.front();
            connectionData->pendingSentData.pop();
            connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED;
        }
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID, connectionData->transientID);
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Remote Connections">
void NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_PendingRemoteConnections
(RawConnectionID rawID, const ConnectionID connectionID)
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_PendingRemoteConnections) >"
            " Terminating connection [" + Convert::toString(connectionID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    
    terminateConnection(connectionID);
    ++connectionsFailed;
}

void NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections
(ByteData data, PacketSize remaining, const ConnectionID connectionID)
{
    if(!active)
        return;

    if(remaining > 0)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Cannot process connection response for connection [" + Convert::toString(connectionID)
                + "]; more data remains to be received.");
        
        throw std::runtime_error("DataConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections() >"
                " Cannot process connection response for connection [" + Convert::toString(connectionID)
                + "]; more data remains to be received.");
    }

    ConnectionDataPtr connectionData = getPendingConnectionData(connectionID);

    if(!connectionData->connection->isActive())
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) > Connection ["
                + Convert::toString(connectionID) + "] is not active.");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        return;
    }

    try
    {
        //generates and send a connection response
        CiphertextData * responseData = generateConnectionResponseDataFromRequest(data, connectionID);

        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
        
        connectionData->onWriteResultReceivedEventConnection =
                connectionData->connection->onWriteResultReceivedEventAttach(
                    boost::bind(&DataConnectionsHandler::DataConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections,
                                this, _1, connectionData->deviceData->getDeviceID(), connectionID));
        
        connectionData->pendingSentData.push(responseData);
        connectionData->connection->sendData(*responseData);
        connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_SENT;
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Request validation failed: [" + std::string(e.what()) + "].");
        
        if(connectionData->deviceData)
        {
            terminateConnection(connectionID, connectionData->deviceData->getDeviceID());
        }
        else
        {
            terminateConnection(connectionID);
        }
        
        ++connectionsFailed;
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        if(connectionData->deviceData)
        {
            terminateConnection(connectionID, connectionData->deviceData->getDeviceID());
        }
        else
        {
            terminateConnection(connectionID);
        }
        
        ++connectionsFailed;
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        if(connectionData->deviceData)
        {
            terminateConnection(connectionID, connectionData->deviceData->getDeviceID());
        }
        else
        {
            terminateConnection(connectionID);
        }
        
        ++connectionsFailed;
        throw;
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections
(bool received, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);

    if(!received)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Failed to send response data for device [" + Convert::toString(deviceID)
                + "] on connection [" + Convert::toString(connectionID) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        return;
    }

    try
    {
        if(connectionData->state != ConnectionSetupState::CONNECTION_RESPONSE_SENT)
        {
            logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for device [" + Convert::toString(deviceID)
                    + "] on connection [" + Convert::toString(connectionID) + "].");
            
            throw std::logic_error("DataConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections() >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for device [" + Convert::toString(deviceID)
                    + "] on connection [" + Convert::toString(connectionID) + "].");
        }

        connectionData->connection->disableDataEvents();

        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
            delete connectionData->pendingSentData.front();
            connectionData->pendingSentData.pop();
            connectionData->state = ConnectionSetupState::COMPLETED;
        }

        //detaches the pending connection handlers
        connectionData->onDataReceivedEventConnection.disconnect();
        connectionData->onDisconnectEventConnection.disconnect();
        connectionData->onWriteResultReceivedEventConnection.disconnect();

        //attaches the established connection handlers
        connectionData->onDataReceivedEventConnection =
                connectionData->connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_EstablishedConnections,
                                this, _1, _2, deviceID, connectionID));
        
        connectionData->onDisconnectEventConnection =
                connectionData->connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        connectionData->onWriteResultReceivedEventConnection =
                connectionData->connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        connectionData->connection->enableDataEvents();
        
        logMessage(LogSeverity::Info, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Setup completed for device [" + Convert::toString(deviceID)
                + "] on connection [" + Convert::toString(connectionID) + "].");
        
        ++connectionsEstablished;
        onConnectionEstablished(deviceID, connectionID, connectionData->transientID);
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, deviceID);
        ++connectionsFailed;
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Established Connections">
void NetworkManagement_Handlers::DataConnectionsHandler::onDisconnectHandler_EstablishedConnections
(RawConnectionID rawID, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_EstablishedConnections) >"
            " Terminating connection [" + Convert::toString(connectionID)
            + "] for device [" + Convert::toString(deviceID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    
    terminateConnection(connectionID, deviceID);
    onEstablishedConnectionClosed(deviceID, connectionID);
}

void NetworkManagement_Handlers::DataConnectionsHandler::onDataReceivedHandler_EstablishedConnections
(ByteData data, PacketSize remaining, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ++totalDataObjectsReceived;
    logMessage(LogSeverity::Info, "(onDataReceivedHandler_EstablishedConnections) >"
            " Received data for device [" + Convert::toString(deviceID)
            + "] on connection [" + Convert::toString(connectionID) + "].");

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);
    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);

    if((connectionData->lastPendingReceivedData.size() + data.size() + remaining) > maxDataSize)
    {
        ++invalidDataObjectsReceived;
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                " Cannot process data with size ["
                + Convert::toString(connectionData->lastPendingReceivedData.size() + data.size() + remaining)
                + "]; maximum is [" + Convert::toString(maxDataSize) + "].");
        
        terminateConnection(connectionID, deviceID);
        return;
    }

    if(remaining > 0)
    {//the handler needs to wait for the rest of the data from the remote peer
        if(connectionData->lastPendingReceivedData.size() > 0)
        {
            connectionData->lastPendingReceivedData.append(data);
        }
        else
        {
            connectionData->lastPendingReceivedData.assign(data);
        }
    }
    else
    {//the handler has all necessary data to continue
        try
        {
            PlaintextData receivedData;
            if(connectionData->lastPendingReceivedData.size() > 0)
            {
                connectionData->lastPendingReceivedData.append(data);

                if(connectionData->encryptionEnabled)
                {//the data needs to be decrypted
                    connectionData->cryptoHandler->decryptData(
                        connectionData->lastPendingReceivedData, receivedData);
                }
                else
                {//the data was sent in plaintext
                    receivedData = connectionData->lastPendingReceivedData;
                }

                connectionData->lastPendingReceivedData.clear();
            }
            else
            {
                if(connectionData->encryptionEnabled)
                {//the data needs to be decrypted
                    connectionData->cryptoHandler->decryptData(data, receivedData);
                }
                else
                {//the data was sent in plaintext
                    receivedData = data;
                }
            }

            if(connectionData->compressionEnabled)
            {//the data needs to be decompressed
                ByteData decompressedData;
                compressor.decompressData(receivedData, decompressedData);
                onDataReceived(deviceID, connectionID, decompressedData);
            }
            else
            {//the data is not compressed
                onDataReceived(deviceID, connectionID, receivedData);
            }

            ++validDataObjectsReceived;
        }
        catch(const std::exception & e)
        {
            ++invalidDataObjectsReceived;
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                    " Exception encountered: [" + std::string(e.what())
                    + "] for device [" + Convert::toString(deviceID)
                    + "] on connection [" + Convert::toString(connectionID) + "].");
        }
        catch(...)
        {
            ++invalidDataObjectsReceived;
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                    " Unknown exception encountered for device [" + Convert::toString(deviceID)
                    + "] on connection [" + Convert::toString(connectionID) + "].");
        }
    }
}

void NetworkManagement_Handlers::DataConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections
(bool received, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ConnectionDataPtr connectionData = getConnectionData(deviceID, connectionID);

    if(!received)
    {
        ++sendRequestsFailed;
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_EstablishedConnections) >"
                " Failed to send data to [" + Convert::toString(deviceID)
                + "] on connection [" + Convert::toString(connectionID) + "].");
    }
    else
    {
        ++sendRequestsConfirmed;
    }

    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    delete connectionData->pendingSentData.front();
    connectionData->pendingSentData.pop();
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Cleanup">
void NetworkManagement_Handlers::DataConnectionsHandler::terminateConnection
(const ConnectionID connectionID, const DeviceID remotePeerID)
{
    try
    {
        ConnectionDataPtr connectionData = (remotePeerID == INVALID_DEVICE_ID)
                ? discardPendingConnectionData(connectionID)
                : discardConnectionData(remotePeerID, connectionID);

        boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
        connectionData->onDataReceivedEventConnection.disconnect();
        connectionData->onDisconnectEventConnection.disconnect();
        connectionData->onWriteResultReceivedEventConnection.disconnect();
        connectionData->connection->disconnect();

        while(connectionData->pendingSentData.size() > 0)
        {
            delete connectionData->pendingSentData.front();
            connectionData->pendingSentData.pop();
        }
    }
    catch(const std::runtime_error &)
    {}
}
//</editor-fold>
