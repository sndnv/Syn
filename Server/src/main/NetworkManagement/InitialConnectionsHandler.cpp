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

#include "InitialConnectionsHandler.h"
#include <boost/thread/lock_guard.hpp>
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Security.h"
#include "../SecurityManagement/Crypto/SaltGenerator.h"
#include "../SecurityManagement/Crypto/PasswordGenerator.h"

//Protocols
#include "../../compiled/protobuf/BaseComm.pb.h"
#include "Protocols/Utilities.h"

using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::InitConnectionAdditionalData;
using NetworkManagement_Protocols::InitConnectionSetupRequest;
using NetworkManagement_Protocols::InitConenctionSetupResponse;

NetworkManagement_Handlers::InitialConnectionsHandler::InitialConnectionsHandler
(const InitialConnectionsHandlerParameters & params, Securable & parent,
 std::function<PendingInitConnectionConfigPtr (const TransientConnectionID)> cfgRetrievalHandler,
 std::function<void (const DeviceID & deviceID, const LocalPeerAuthenticationEntry &)> authDataUpdateHandler,
 Utilities::FileLoggerPtr debugLogger)
: debugLogger(debugLogger), deviceConfigRetrievalHandler(cfgRetrievalHandler),
  authenticationDataAdditionHandler(authDataUpdateHandler),
  parentNetworkManager(parent), securityManager(params.securityManager),
  active(true), requestSignatureSize(params.requestSignatureSize),
  keyExchange(params.keyExchange), defaultRandomPasswordSize(params.defaultRandomPasswordSize),
  maxRandomPasswordAttempts(params.maxRandomPasswordAttempts), localPeerID(params.localPeerID),
  localPeerPublicKey(params.localPeerPublicKey), localIPSettings(params.localIPSettings)
{}

NetworkManagement_Handlers::InitialConnectionsHandler::~InitialConnectionsHandler()
{
    active = false;

    onSetupCompleted.disconnect_all_slots();
    onSetupFailed.disconnect_all_slots();

    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    if(connectionsData.size() > 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(connectionsData.size())
                + "] connections found.");

        for(auto currentConnectionData : connectionsData)
        {
            boost::lock_guard<boost::mutex> currentDataLock(currentConnectionData.second->connectionDataMutex);
            delete currentConnectionData.second->lastPendingData;
            currentConnectionData.second->lastPendingData = nullptr;
            currentConnectionData.second->connection->disconnect();
        }

        connectionsData.clear();
    }

    if(unknownConnectionsData.size() > 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(unknownConnectionsData.size())
                + "] unknown connections found.");

        for(auto currentConnectionData : unknownConnectionsData)
        {
            boost::lock_guard<boost::mutex> currentDataLock(currentConnectionData.second->connectionDataMutex);
            delete currentConnectionData.second->lastPendingData;
            currentConnectionData.second->lastPendingData = nullptr;
            currentConnectionData.second->connection->disconnect();
        }

        unknownConnectionsData.clear();
    }
}

void NetworkManagement_Handlers::InitialConnectionsHandler::manageLocalConnection
(ConnectionPtr connection, const ConnectionID connectionID, PendingInitConnectionConfigPtr remotePeerData)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageLocalConnection) > Unable to process connection ["
                + Convert::toString(connectionID) + "] with transient ID ["
                + Convert::toString(remotePeerData->transientID) + "] for device ["
                + Convert::toString(remotePeerData->newPeerID) + "]; handler is not active.");
        
        connection->disconnect();
        return;
    }

    try
    {
        ConnectionDataPtr connectionData = createConnectionData(connection, connectionID, remotePeerData);
        CiphertextData * requestData = generateConnectionRequestData(remotePeerData->transientID, connectionData);
        connectionData->lastPendingData = requestData;
        connection->sendData(*requestData);
        connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT;

        //attaches the pending connection event handlers
        connectionData->onDisconnectEventConnection =
                connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onDisconnectHandler_PendingLocalConnections,
                                this, _1, connectionID, remotePeerData->transientID));
        
        connectionData->onDataReceivedEventConnection =
                connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onDataReceivedHandler_PendingLocalConnections,
                this, _1, _2, connectionID, remotePeerData->transientID));
        
        connectionData->onWriteResultReceivedEventConnection =
                connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections,
                                this, _1, connectionID, remotePeerData->transientID));
        
        connection->enableDataEvents();
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Request generation failed: [" + std::string(e.what()) + "].");
        terminateConnection(connectionID, remotePeerData->transientID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        terminateConnection(connectionID, remotePeerData->transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Unknown exception encountered.");
        terminateConnection(connectionID, remotePeerData->transientID);
        throw;
    }
}

void NetworkManagement_Handlers::InitialConnectionsHandler::manageRemoteConnection
(ConnectionPtr connection, const ConnectionID connectionID)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageRemoteConnection) >"
                " Unable to process connection [" + Convert::toString(connectionID) + "] for ["
                + connection->getRemoteAddress() + " / "
                + Convert::toString(connection->getRemotePort())
                + "]; handler is not active.");
        
        connection->disconnect();
        return;
    }

    UnknownConnectionDataPtr connectionData = createUnknownConnectionData(connection, connectionID);

    //attaches the pending connection event handlers
    connectionData->onDataReceivedEventConnection =
            connection->onDataReceivedEventAttach(
                boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections,
                            this, _1, _2, connectionID));
    
    connectionData->onDisconnectEventConnection =
            connection->onDisconnectEventAttach(
                boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onDisconnectHandler_PendingRemoteConnections,
                            this, _1, connectionID));

    connection->enableDataEvents();
}

//<editor-fold defaultstate="collapsed" desc="Data Management">
NetworkManagement_Handlers::InitialConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::createConnectionData
(ConnectionPtr connection, const ConnectionID connectionID, PendingInitConnectionConfigPtr remotePeerData)
{
    //generates new symmetric crypto data derived from the supplied password
    DerivedCryptoDataGenerationRequest derivedDataRequest(parentNetworkManager, remotePeerData->initPassword);
    SymmetricCryptoDataContainerPromisePtr promise = securityManager.postRequest(derivedDataRequest);
    SymmetricCryptoDataContainerPtr cryptoData = promise->get_future().get();
    SymmetricCryptoHandlerPtr cryptoHandler(new SymmetricCryptoHandler(cryptoData));

    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    ConnectionDataPtr newConnectionData = ConnectionDataPtr(new ConnectionData
    {
        ConnectionSetupState::INITIATED,                        //connection setup state
        remotePeerData->initPassword,                           //initial shared password
        cryptoHandler,                                          //crypto data derived from the shared password
        securityManager.getDefaultSymmetricCipher(),            //crypto config - cipher
        securityManager.getDefaultSymmetricCipherMode(),        //crypto config - mode
        securityManager.getDefaultDerivedKeyIterationsCount(),  //crypto config - key derivation iteration count
        EMPTY_PLAINTEXT_DATA,                                   //no request signature stored yet
        nullptr,                                                //no data sent yet
        connection,                                             //connection pointer
        remotePeerData->newPeerID,                              //remote peer ID (as generated on the local system)
        remotePeerData->peerType                                //remote peer type
    });

    auto connectionData = connectionsData.find(remotePeerData->transientID);
    if(connectionData == connectionsData.end())
    {
        auto result = connectionsData.insert(
            std::pair<TransientConnectionID, ConnectionDataPtr>(
                remotePeerData->transientID, newConnectionData));
        
        return result.first->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(createConnectionData) >"
                " Existing data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(remotePeerData->transientID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::createConnectionData() >"
                " Existing data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(remotePeerData->transientID) + "].");
    }
}

NetworkManagement_Handlers::InitialConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::getConnectionData
(const ConnectionID connectionID, const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    auto connectionData = connectionsData.find(transientID);
    if(connectionData != connectionsData.end())
    {
        return connectionData->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getConnectionData) >"
                " No data data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::getConnectionData() >"
                " No data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
    }
}

NetworkManagement_Handlers::InitialConnectionsHandler::ConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::discardConnectionData
(const ConnectionID connectionID, const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    auto connectionData = connectionsData.find(transientID);
    if(connectionData != connectionsData.end())
    {
        ConnectionDataPtr result = connectionData->second;
        connectionsData.erase(connectionData);
        return result;
    }
    else
    {
        logMessage(LogSeverity::Error, "(discardConnectionData) >"
                " No data data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::discardConnectionData()"
                " > No data found for connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
    }
}

NetworkManagement_Handlers::InitialConnectionsHandler::UnknownConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::createUnknownConnectionData
(ConnectionPtr connection, const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    UnknownConnectionDataPtr newConnectionData = UnknownConnectionDataPtr(new UnknownConnectionData
    {
        connection,                     //connection pointer
        ConnectionSetupState::INITIATED,//connection setup state
        INVALID_TRANSIENT_CONNECTION_ID,//no transient ID is available yet
        nullptr                         //no data sent yet
    });

    auto connectionData = unknownConnectionsData.find(connectionID);
    if(connectionData == unknownConnectionsData.end())
    {
        auto result = unknownConnectionsData.insert(
            std::pair<ConnectionID, UnknownConnectionDataPtr>(
                connectionID, newConnectionData));
        
        return result.first->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(createUnknownConnectionData) >"
                " Existing data found for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::createUnknownConnectionData()"
                " > Existing data found for connection [" + Convert::toString(connectionID) + "].");
    }
}

NetworkManagement_Handlers::InitialConnectionsHandler::UnknownConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::getUnknownConnectionData
(const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    auto connectionData = unknownConnectionsData.find(connectionID);
    if(connectionData != unknownConnectionsData.end())
    {
        return connectionData->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getUnknownConnectionData) >"
                " No data data found for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::getUnknownConnectionData() >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
    }
}

NetworkManagement_Handlers::InitialConnectionsHandler::UnknownConnectionDataPtr
NetworkManagement_Handlers::InitialConnectionsHandler::discardUnknownConnectionData
(const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    auto connectionData = unknownConnectionsData.find(connectionID);
    if(connectionData != unknownConnectionsData.end())
    {
        UnknownConnectionDataPtr result = connectionData->second;
        unknownConnectionsData.erase(connectionData);
        return result;
    }
    else
    {
        logMessage(LogSeverity::Error, "(discardUnknownConnectionData) >"
                " No data data found for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::logic_error("InitialConnectionsHandler::discardUnknownConnectionData() >"
                " No data found for connection [" + Convert::toString(connectionID) + "].");
    }
}

const std::string NetworkManagement_Handlers::InitialConnectionsHandler::getNewServerPassword()
{
    return SecurityManagement_Crypto::PasswordGenerator::getValidRandomASCIIPassword(
            defaultRandomPasswordSize,
            boost::bind(&SyncServer_Core::SecurityManager::hashDevicePassword, &securityManager, _1),
            maxRandomPasswordAttempts);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Crypto">
MixedData * NetworkManagement_Handlers::InitialConnectionsHandler::generateConnectionRequestData
(const TransientConnectionID transientID, ConnectionDataPtr remotePeerData)
{
     //builds the request signature object
    RandomData signatureData(SecurityManagement_Crypto::SaltGenerator::getRandomSalt(requestSignatureSize));
    ConnectionSetupRequestSignature requestSignature;
    requestSignature.set_signature_size(requestSignatureSize);
    requestSignature.set_signature_data(signatureData.BytePtr(), requestSignatureSize);
    PlaintextData plaintextSignature;
    
    if(!requestSignature.SerializeToString(&plaintextSignature))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize request signature.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize request signature.");
    }
    
    remotePeerData->requestSignatureData = plaintextSignature;

    //builds the additional data object
    InitConnectionAdditionalData additionalData;
    additionalData.set_request_signature(plaintextSignature);
    additionalData.set_public_key(localPeerPublicKey);
    additionalData.set_command_address(localIPSettings.commandAddress);
    additionalData.set_command_port(localIPSettings.commandPort);
    additionalData.set_data_address(localIPSettings.dataAddress);
    additionalData.set_data_port(localIPSettings.dataPort);
    additionalData.set_init_address(localIPSettings.initAddress);
    additionalData.set_init_port(localIPSettings.initPort);
    additionalData.set_key_exchange(Convert::toString(keyExchange));
    additionalData.set_remote_peer_id(Convert::toString(remotePeerData->newPeerID));

    if(remotePeerData->remotePeerType == PeerType::SERVER)
    {
        std::string newPassword = getNewServerPassword();
        remotePeerData->serverPassword = newPassword;
        additionalData.set_password_data(newPassword);
    }
    else
    {
        additionalData.set_local_peer_id(Convert::toString(localPeerID));
    }

    //builds the setup request object
    InitConnectionSetupRequest setupRequest;
    setupRequest.set_pbkd_salt_data(
        remotePeerData->symCrypto->getCryptoData()->getSalt().BytePtr(),
        remotePeerData->symCrypto->getCryptoData()->getSalt().SizeInBytes());
    
    setupRequest.set_pbkd_key_iv(
        remotePeerData->symCrypto->getCryptoData()->getIV().BytePtr(),
        remotePeerData->symCrypto->getCryptoData()->getIV().SizeInBytes());
    
    setupRequest.set_pbkd_iterations(remotePeerData->iterationsCount);
    setupRequest.set_pbkd_sym_cipher(Convert::toString(remotePeerData->cipher));
    setupRequest.set_pbkd_sym_mode(Convert::toString(remotePeerData->mode));
    setupRequest.set_transient_id(transientID);

    //serializes and encrypts the additional data object
    PlaintextData plaintextAdditionalData;
    if(!additionalData.SerializeToString(&plaintextAdditionalData))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize additional data.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize additional data.");
    }
    
    CiphertextData encryptedAdditionalData;
    remotePeerData->symCrypto->encryptData(plaintextAdditionalData, encryptedAdditionalData);
    setupRequest.set_additional_data(encryptedAdditionalData);

    MixedData * serializedSetupRequest = new MixedData();
    if(!setupRequest.SerializeToString(serializedSetupRequest))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize setup request.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize setup request.");
    }

    return serializedSetupRequest;
}

MixedData *
NetworkManagement_Handlers::InitialConnectionsHandler::generateConnectionResponseDataFromRequest
(const MixedData & setupRequest, const ConnectionID connectionID)
{
    //parses the setup request object
    InitConnectionSetupRequest setupRequestObject;
    setupRequestObject.ParseFromString(setupRequest);

    //validates the request
    if(!setupRequestObject.IsInitialized())
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to validate connection setup request.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to validate connection setup request.");
    }

    PendingInitConnectionConfigPtr remotePeerData =
            deviceConfigRetrievalHandler(setupRequestObject.transient_id());
    
    UnknownConnectionDataPtr connectionData = getUnknownConnectionData(connectionID);

    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    connectionData->transientID = setupRequestObject.transient_id();

    //builds the connection's symmetric key with the supplied parameters
    IVData ivData(reinterpret_cast<const unsigned char*>(
        setupRequestObject.pbkd_key_iv().data()), setupRequestObject.pbkd_key_iv().size());
    
    SaltData saltData(reinterpret_cast<const unsigned char*>(
        setupRequestObject.pbkd_salt_data().data()), setupRequestObject.pbkd_salt_data().size());

    DerivedCryptoDataGenerationRequest derivedDataRequest
    (
        parentNetworkManager,
        remotePeerData->initPassword,
        ivData,
        saltData,
        setupRequestObject.pbkd_iterations(),
        Convert::toSymmetricCipherType(setupRequestObject.pbkd_sym_cipher()),
        Convert::toAuthenticatedSymmetricCipherModeType(setupRequestObject.pbkd_sym_mode())
    );

    SymmetricCryptoDataContainerPromisePtr promise = securityManager.postRequest(derivedDataRequest);
    SymmetricCryptoDataContainerPtr cryptoData = promise->get_future().get();
    SymmetricCryptoHandler cryptoHandler(cryptoData);

    //decrypts the remote additional data
    PlaintextData decryptedAdditionalData;
    cryptoHandler.decryptData(setupRequestObject.additional_data(), decryptedAdditionalData);

    //parses the remote additional data object
    InitConnectionAdditionalData remoteAdditionalDataObject;
    remoteAdditionalDataObject.ParseFromString(decryptedAdditionalData);

    //validates the additional data
    if(!remoteAdditionalDataObject.IsInitialized()
       || (remotePeerData->peerType == PeerType::SERVER &&
            (!remoteAdditionalDataObject.has_password_data() || !remoteAdditionalDataObject.has_remote_peer_id())))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to validate additional setup data.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to validate additional setup data.");
    }

    //verifies the received data
    try
    {
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(
            remoteAdditionalDataObject.request_signature());
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to verify request signature.");
        throw;
    }

    //builds the local additional data object
    InitConnectionAdditionalData localAdditionalDataObject;
    localAdditionalDataObject.set_request_signature(remoteAdditionalDataObject.request_signature());
    localAdditionalDataObject.set_public_key(localPeerPublicKey);
    localAdditionalDataObject.set_command_address(localIPSettings.commandAddress);
    localAdditionalDataObject.set_command_port(localIPSettings.commandPort);
    localAdditionalDataObject.set_data_address(localIPSettings.dataAddress);
    localAdditionalDataObject.set_data_port(localIPSettings.dataPort);
    localAdditionalDataObject.set_init_address(localIPSettings.initAddress);
    localAdditionalDataObject.set_init_port(localIPSettings.initPort);
    localAdditionalDataObject.set_key_exchange(Convert::toString(keyExchange));
    localAdditionalDataObject.set_remote_peer_id(Convert::toString(remotePeerData->newPeerID));

    std::string newPassword = "";
    if(remotePeerData->peerType == PeerType::SERVER)
    {
        newPassword = getNewServerPassword();
        localAdditionalDataObject.set_password_data(newPassword);
    }
    else
    {
        localAdditionalDataObject.set_local_peer_id(Convert::toString(localPeerID));
    }

    //serializes and encrypts the local additional data object
    PlaintextData plaintextAdditionalData;
    if(!localAdditionalDataObject.SerializeToString(&plaintextAdditionalData))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to serialize additional data.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to serialize additional data.");
    }
    
    CiphertextData encryptedAdditionalData;
    cryptoHandler.encryptData(plaintextAdditionalData, encryptedAdditionalData);

    //builds the response object
    InitConenctionSetupResponse responseObject;
    responseObject.set_additional_data(encryptedAdditionalData);

    //serializes the response object
    MixedData * serializedResponse = new MixedData();
    if(!responseObject.SerializeToString(serializedResponse))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to serialize response.");
        
        throw std::runtime_error("InitialConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to serialize response.");
    }

    //updates the new device parameters
    NewDeviceConnectionParameters deviceParams
    {
        DeviceIPSettings
        {
            remoteAdditionalDataObject.command_address(),
            remoteAdditionalDataObject.command_port(),
            remoteAdditionalDataObject.data_address(),
            remoteAdditionalDataObject.data_port(),
            remoteAdditionalDataObject.init_address(),
            remoteAdditionalDataObject.init_port(),
        },
        (remoteAdditionalDataObject.has_password_data()) ? remoteAdditionalDataObject.password_data() : "",
        remoteAdditionalDataObject.public_key(),
        Convert::toKeyExchangeType(remoteAdditionalDataObject.key_exchange()),
        remotePeerData->peerType
    };
    connectionData->deviceParams = deviceParams;
    connectionData->newPeerID = remotePeerData->newPeerID;

    if(remotePeerData->peerType == PeerType::SERVER)
    {//updates the server authentication data
        LocalPeerAuthenticationEntry authData
        {
            Convert::toDeviceID(remoteAdditionalDataObject.remote_peer_id()),
            newPassword
        };
        connectionData->authData = authData;
    }

    return serializedResponse;
}

void NetworkManagement_Handlers::InitialConnectionsHandler::verifyConnectionResponseData
(const MixedData & responseData, ConnectionDataPtr connectionData)
{
    //parses the setup response object
    InitConenctionSetupResponse responseObject;
    responseObject.ParseFromString(responseData);

    //validates the request
    if(!responseObject.IsInitialized())
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to validate connection setup response.");
        
        throw std::runtime_error("InitialConnectionsHandler::verifyConnectionResponseData() >"
                " Failed to validate connection setup response.");
    }

    PlaintextData decryptedAdditionalData;
    connectionData->symCrypto->decryptData(responseObject.additional_data(), decryptedAdditionalData);

    InitConnectionAdditionalData additionalDataObject;
    additionalDataObject.ParseFromString(decryptedAdditionalData);

    //validates the additional data
    if(!additionalDataObject.IsInitialized()
        || (connectionData->remotePeerType == PeerType::SERVER
            && (!additionalDataObject.has_password_data()
                || !additionalDataObject.has_remote_peer_id())))
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to validate additional setup data.");
        
        throw std::runtime_error("InitialConnectionsHandler::verifyConnectionResponseData() >"
                " Failed to validate additional setup data.");
    }

    //verifies the received data
    try
    {
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(
            additionalDataObject.request_signature(),
            connectionData->requestSignatureData);
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to verify request signature.");
        throw;
    }

    //updates the new device parameters
    NewDeviceConnectionParameters deviceParams
    {
        DeviceIPSettings
        {
            additionalDataObject.command_address(),
            additionalDataObject.command_port(),
            additionalDataObject.data_address(),
            additionalDataObject.data_port(),
            additionalDataObject.init_address(),
            additionalDataObject.init_port()
        },
        (additionalDataObject.has_password_data()) ? additionalDataObject.password_data() : "",
        additionalDataObject.public_key(),
        Convert::toKeyExchangeType(additionalDataObject.key_exchange()),
        connectionData->remotePeerType
    };
    connectionData->deviceParams = deviceParams;

    if(connectionData->remotePeerType == PeerType::SERVER)
    {//updates the server authentication data
        LocalPeerAuthenticationEntry authData
        {
            Convert::toDeviceID(additionalDataObject.remote_peer_id()),
            connectionData->serverPassword
        };
        connectionData->authData = authData;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Local Connections">
void NetworkManagement_Handlers::InitialConnectionsHandler::onDisconnectHandler_PendingLocalConnections
(RawConnectionID rawID, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_PendingLocalConnections) >"
            " Terminating connection [" + Convert::toString(connectionID)
            + "] with transient ID [" + Convert::toString(transientID)
            + "] and raw ID [" + Convert::toString(rawID) + "].");
    
    terminateConnection(connectionID, transientID);
    ++setupsFailed;
    onSetupFailed(connectionID, transientID);
}

void NetworkManagement_Handlers::InitialConnectionsHandler::onDataReceivedHandler_PendingLocalConnections
(ByteData data, PacketSize remaining, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    if(!active)
        return;

    ConnectionDataPtr connectionData = getConnectionData(connectionID, transientID);

    try
    {
        if(remaining > 0)
        {
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                    " Cannot process connection response for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID)
                    + "]; more data remains to be received.");
            
            throw std::runtime_error("InitialConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                    " Cannot process connection response for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID)
                    + "]; more data remains to be received.");
        }

        connectionData->connection->disableDataEvents();

        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);

            if(connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT
                && connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED)
            {
                logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for connection [" + Convert::toString(connectionID)
                        + "] with transient ID [" + Convert::toString(transientID) + "].");
                
                throw std::logic_error("InitialConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for connection [" + Convert::toString(connectionID)
                        + "] with transient ID [" + Convert::toString(transientID) + "].");
            }

            if(connectionData->state == ConnectionSetupState::CONNECTION_REQUEST_SENT)
            {
                delete connectionData->lastPendingData;
                connectionData->lastPendingData = nullptr;
            }

            connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_RECEIVED;

            verifyConnectionResponseData(data, connectionData);
            connectionData->state = ConnectionSetupState::COMPLETED;
        }

        terminateConnection(connectionID, transientID);
        logMessage(LogSeverity::Info, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Setup completed for device [" + Convert::toString(connectionData->newPeerID)
                + "] on connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
        
        ++setupsCompleted;
        
        logMessage(LogSeverity::Debug, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Adding authentication data for local device ["
                + Convert::toString(connectionData->newPeerID)
                + "] with remote ID [" + Convert::toString(connectionData->authData.id) + "].");
        
        authenticationDataAdditionHandler(connectionData->newPeerID, connectionData->authData);
        onSetupCompleted(connectionID, connectionData->newPeerID, transientID, connectionData->deviceParams);
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Response validation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
}

void NetworkManagement_Handlers::InitialConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections
(bool received, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    if(!active)
        return;

    try
    {
        ConnectionDataPtr connectionData = getConnectionData(connectionID, transientID);

        if(!received)
        {
            logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                    " Failed to send request data on connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
            
            throw std::runtime_error("InitialConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections >"
                    " Failed to send request data on connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
        }

        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
        if(connectionData->state == ConnectionSetupState::CONNECTION_REQUEST_SENT)
        {
            delete connectionData->lastPendingData;
            connectionData->lastPendingData = nullptr;
            connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED;
        }
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID, transientID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Remote Connections">
void NetworkManagement_Handlers::InitialConnectionsHandler::onDisconnectHandler_PendingRemoteConnections
(RawConnectionID rawID, const ConnectionID connectionID)
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_PendingRemoteConnections) >"
            " Terminating connection [" + Convert::toString(connectionID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    
    terminateUnknownConnection(connectionID);
    ++setupsFailed;
}

void NetworkManagement_Handlers::InitialConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections
(ByteData data, PacketSize remaining, const ConnectionID connectionID)
{
    if(!active)
        return;

    try
    {
        if(remaining > 0)
        {
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                    " Cannot process connection response for connection ["
                    + Convert::toString(connectionID) + "]; more data remains to be received.");
            
            throw std::runtime_error("InitialConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                    " Cannot process connection response for connection [" + Convert::toString(connectionID)
                    + "]; more data remains to be received.");
        }

        //generates and send a connection response
        MixedData * responseData = generateConnectionResponseDataFromRequest(data, connectionID);
        UnknownConnectionDataPtr connectionData = getUnknownConnectionData(connectionID);

        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);

        connectionData->onWriteResultReceivedEventConnection =
                connectionData->connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::InitialConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections,
                                this, _1, connectionID, connectionData->transientID));
        
        connectionData->lastPendingData = responseData;
        connectionData->connection->sendData(*responseData);
        connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_SENT;
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Request validation failed: [" + std::string(e.what()) + "].");
        
        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, INVALID_TRANSIENT_CONNECTION_ID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, INVALID_TRANSIENT_CONNECTION_ID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, INVALID_TRANSIENT_CONNECTION_ID);
        throw;
    }
}

void NetworkManagement_Handlers::InitialConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections
(bool received, const ConnectionID connectionID, const TransientConnectionID transientID)
{
    if(!active)
        return;

    try
    {
        UnknownConnectionDataPtr connectionData = getUnknownConnectionData(connectionID);

        if(!received)
        {
            logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                    " Failed to send response data for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
            
            throw std::runtime_error("InitialConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections() >"
                    " Failed to send response data for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
        }

        if(connectionData->state != ConnectionSetupState::CONNECTION_RESPONSE_SENT)
        {
            logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
            
            throw std::logic_error("InitialConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections() >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for connection [" + Convert::toString(connectionID)
                    + "] with transient ID [" + Convert::toString(transientID) + "].");
        }

        connectionData->onDataReceivedEventConnection.disconnect();
        connectionData->onWriteResultReceivedEventConnection.disconnect();

        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
            delete connectionData->lastPendingData;
            connectionData->lastPendingData = nullptr;
            connectionData->state = ConnectionSetupState::COMPLETED;
        }

        logMessage(LogSeverity::Info, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Setup completed for device [" + Convert::toString(connectionData->newPeerID)
                + "] on connection [" + Convert::toString(connectionID)
                + "] with transient ID [" + Convert::toString(transientID) + "].");
        
        ++setupsCompleted;
        
        logMessage(LogSeverity::Debug, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Adding authentication data for local device [" + Convert::toString(connectionData->newPeerID)
                + "] with remote ID [" + Convert::toString(connectionData->authData.id) + "].");
        
        authenticationDataAdditionHandler(connectionData->newPeerID, connectionData->authData);
        onSetupCompleted(connectionID, connectionData->newPeerID, transientID, connectionData->deviceParams);
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");

        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        terminateUnknownConnection(connectionID);
        ++setupsFailed;
        onSetupFailed(connectionID, transientID);
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Cleanup">
void NetworkManagement_Handlers::InitialConnectionsHandler::terminateConnection
(const ConnectionID connectionID, const TransientConnectionID transientID)
{
    ConnectionDataPtr connectionData = discardConnectionData(connectionID, transientID);

    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    connectionData->onDisconnectEventConnection.disconnect();
    connectionData->onDataReceivedEventConnection.disconnect();
    connectionData->onWriteResultReceivedEventConnection.disconnect();
    connectionData->connection->disconnect();
    delete connectionData->lastPendingData;
    connectionData->lastPendingData = nullptr;
}

void NetworkManagement_Handlers::InitialConnectionsHandler::terminateUnknownConnection
(const ConnectionID connectionID)
{
    UnknownConnectionDataPtr connectionData = discardUnknownConnectionData(connectionID);

    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    connectionData->onDisconnectEventConnection.disconnect();
    connectionData->onDataReceivedEventConnection.disconnect();
    connectionData->onWriteResultReceivedEventConnection.disconnect();
    connectionData->connection->disconnect();
    delete connectionData->lastPendingData;
    connectionData->lastPendingData = nullptr;
}
//</editor-fold>
