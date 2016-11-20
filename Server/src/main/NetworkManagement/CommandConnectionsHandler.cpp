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

#include "CommandConnectionsHandler.h"
#include <boost/thread/lock_guard.hpp>

//Protocols
#include "../../compiled/protobuf/BaseComm.pb.h"
#include "Protocols/Utilities.h"
using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using NetworkManagement_Protocols::CommandConnectionSetupRequest;
using NetworkManagement_Protocols::CommandConnectionSetupResponse;
using NetworkManagement_Protocols::CommandConnectionSetupRequestData;

NetworkManagement_Handlers::CommandConnectionsHandler::CommandConnectionsHandler
(const CommandConnectionsHandlerParameters & params,
 std::function<DeviceDataContainerPtr (const DeviceID)> dataRetrievalHandler,
 std::function<const LocalPeerAuthenticationEntry & (const DeviceID &)> authDataRetrievalHandler,
 Securable & parent,
 Utilities::FileLoggerPtr debugLogger)
: debugLogger(debugLogger), deviceDataRetrievalHandler(dataRetrievalHandler),
  authenticationDataRetrievalHandler(authDataRetrievalHandler), active(true),
  parentNetworkManager(parent), securityManager(params.securityManager),
  sessionManager(params.sessionManager), localPeerID(params.localPeerID),
  localPeerCrypto(params.localPeerCrypto), localPeerECDHCryptoData(params.localPeerECDHCryptoData),
  requestSignatureSize(params.requestSignatureSize), keyExchange(params.keyExchange)
{}

NetworkManagement_Handlers::CommandConnectionsHandler::~CommandConnectionsHandler()
{
    active = false;

    onConnectionEstablished.disconnect_all_slots();
    onConnectionEstablishmentFailed.disconnect_all_slots();
    onCommandDataReceived.disconnect_all_slots();

    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    if(unknownPendingConnections.size() > 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(unknownPendingConnections.size())
                + "] unknown pending connections found.");

        for(auto currentUnknownConnection : unknownPendingConnections)
        {
            boost::lock_guard<boost::mutex> currentDataLock(currentUnknownConnection.second->connectionDataMutex);
            currentUnknownConnection.second->onDataReceivedEventConnection.disconnect();
            currentUnknownConnection.second->onDisconnectEventConnection.disconnect();
            currentUnknownConnection.second->onWriteResultReceivedEventConnection.disconnect();
            currentUnknownConnection.second->connection->disconnect();
        }
        
        unknownPendingConnections.clear();
    }

    if(pendingConnections.size() > 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(pendingConnections.size())
                + "] pending connections found.");

        for(auto currentPendingConnection : pendingConnections)
        {
            boost::lock_guard<boost::mutex> currentDataLock(currentPendingConnection.second->connectionDataMutex);
            delete currentPendingConnection.second->lastPendingData;
            currentPendingConnection.second->lastPendingData = nullptr;
            currentPendingConnection.second->onDataReceivedEventConnection.disconnect();
            currentPendingConnection.second->onDisconnectEventConnection.disconnect();
            currentPendingConnection.second->onWriteResultReceivedEventConnection.disconnect();
            currentPendingConnection.second->connection->disconnect();
        }
        
        pendingConnections.clear();
    }

    if(establishedConnections.size() > 0)
    {
        logMessage(LogSeverity::Info, "(~) > [" + Convert::toString(establishedConnections.size())
                + "] established connections found.");

        for(auto currentEstablishedConnection : establishedConnections)
        {
            while(currentEstablishedConnection.second->pendingData.size() > 0)
            {
                delete currentEstablishedConnection.second->pendingData.front();
                currentEstablishedConnection.second->pendingData.pop();
            }

            currentEstablishedConnection.second->onDataReceivedEventConnection.disconnect();
            currentEstablishedConnection.second->onDisconnectEventConnection.disconnect();
            currentEstablishedConnection.second->onWriteResultReceivedEventConnection.disconnect();
            currentEstablishedConnection.second->connection->disconnect();
        }
        
        establishedConnections.clear();
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::manageLocalConnection
(ConnectionPtr connection, const ConnectionID connectionID, const DeviceID deviceID)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageLocalConnection) > Unable to process connection ["
                + Convert::toString(connectionID)+ "] for device [" + Convert::toString(deviceID)
                + "]; handler is not active.");
        
        connection->disconnect();
        return;
    }

    try
    {
        //generates the connection request and sends it to the remote peer
        PendingConnectionDataPtr connectionData = createPendingConnectionData(deviceID, connectionID);
        CiphertextData * requestData = generateConnectionRequestData(connectionData);
        
        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
            connectionData->connection = connection;
            connectionData->lastPendingData = requestData;
            connection->sendData(*requestData);
            connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT;

            //attaches the pending connection event handlers
            connectionData->onDataReceivedEventConnection =
                    connection->onDataReceivedEventAttach(
                        boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_PendingLocalConnections,
                                    this, _1, _2, deviceID, connectionID, connection));
            
            connectionData->onDisconnectEventConnection =
                    connection->onDisconnectEventAttach(
                        boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_PendingLocalConnections,
                                    this, _1, deviceID, connectionID));
            
            connectionData->onWriteResultReceivedEventConnection =
                    connection->onWriteResultReceivedEventAttach(
                        boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections,
                                    this, _1, deviceID, connectionID, connection));
        }
        
        connection->enableDataEvents();
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Request generation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(deviceID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(deviceID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(manageLocalConnection) >"
                " Unknown exception encountered.");
        
        terminateConnection(deviceID);
        throw;
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::manageRemoteConnection
(ConnectionPtr connection, const ConnectionID connectionID)
{
    if(!active)
    {
        logMessage(LogSeverity::Warning, "(manageRemoteConnection) >"
                " Unable to process connection [" + Convert::toString(connectionID)
                + "]; handler is not active.");
        
        connection->disconnect();
        return;
    }

    UnknownPendingConnectionDataPtr connectionData =
            UnknownPendingConnectionDataPtr(new UnknownPendingConnectionData{connection, INVALID_DEVICE_ID});

    {
        boost::lock_guard<boost::mutex> dataLock(connectionDataMutex);
        unknownPendingConnections.insert(std::pair<ConnectionID, UnknownPendingConnectionDataPtr>(connectionID, connectionData));
    }

    //attaches the pending connection event handlers
    connectionData->onDataReceivedEventConnection =
            connection->onDataReceivedEventAttach(
                boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections,
                            this, _1, _2, connectionID, connection));
    
    connectionData->onDisconnectEventConnection =
            connection->onDisconnectEventAttach(
                boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_PendingRemoteConnections,
                            this, _1, connectionID));
    
    connectionData->onWriteResultReceivedEventConnection =
            connection->onWriteResultReceivedEventAttach(
                boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections,
                            this, _1, connectionID));
    
    connection->enableDataEvents();
}

void NetworkManagement_Handlers::CommandConnectionsHandler::sendData
(const DeviceID deviceID, const PlaintextData & plaintextData)
{
    if(!active)
        return;

    ++sendRequestsMade;

    EstablishedConnectionDataPtr connectionData = getEstablishedConnectionData(deviceID);

    CiphertextData * encryptedData = new CiphertextData();
    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    try
    {
        connectionData->cryptoHandler->encryptData(plaintextData, *encryptedData);
        connectionData->connection->sendData(*encryptedData);
        connectionData->pendingData.push(encryptedData);
        
        logMessage(LogSeverity::Info, "(sendData) > Data sent to device ["
                + Convert::toString(deviceID) + "].");
    }
    catch(const std::exception & e)
    {
        ++sendRequestsFailed;
        delete encryptedData;
        logMessage(LogSeverity::Error, "(sendData) >"
                " Exception encountered: [" + std::string(e.what())
                + "] while sending data to device [" + Convert::toString(deviceID) + "].");
        
        terminateConnection(deviceID);
        throw;
    }
    catch(...)
    {
        ++sendRequestsFailed;
        delete encryptedData;
        logMessage(LogSeverity::Error, "(sendData) >"
                " Unknown exception encountered while sending data to device ["
                + Convert::toString(deviceID) + "].");
        
        terminateConnection(deviceID);
        throw;
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::closeEstablishedConnection
(const DeviceID deviceID)
{
    if(!active)
        return;

    terminateConnection(deviceID);
}

//<editor-fold defaultstate="collapsed" desc="Data Management">
NetworkManagement_Handlers::CommandConnectionsHandler::PendingConnectionDataPtr
NetworkManagement_Handlers::CommandConnectionsHandler::createPendingConnectionData
(const DeviceID device, const ConnectionID connectionID)
{
    DeviceDataContainerPtr deviceData = deviceDataRetrievalHandler(device);

    AsymmetricCryptoHandlerPtr asymCryptoHandler;
    ECDHCryptoDataContainerPtr ecdhCryptoData;

    switch(deviceData->getExpectedKeyExhange())
    {
        case KeyExchangeType::RSA:
        {
            RSACryptoDataContainerPtr cryptoData =
                    RSACryptoDataContainerPtr(
                        RSACryptoDataContainer::getContainerPtrFromPublicKey(
                            deviceData->getRawPublicKey(),
                            securityManager.getDefaultKeyValidationLevel()));

            asymCryptoHandler = AsymmetricCryptoHandlerPtr(new AsymmetricCryptoHandler(cryptoData));
        } break;

        case KeyExchangeType::EC_DH:
        {
            ecdhCryptoData = ECDHCryptoDataContainerPtr(
                    ECDHCryptoDataContainer::getContainerPtrFromPublicKey(
                        deviceData->getRawPublicKey()));
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(createPendingConnectionData) >"
                    " Unexpected asymmetric cipher type encountered.");
            
            throw std::logic_error("CommandConnectionsHandler::createPendingConnectionData() >"
                    " Unexpected asymmetric cipher type encountered.");
        }
    }

    boost::lock_guard<boost::mutex> connectionDataLock(connectionDataMutex);

    PendingConnectionDataPtr newPendingData = PendingConnectionDataPtr(
            new PendingConnectionData
            {
                asymCryptoHandler,                                  //asymmetric crypto handler (if supplied; otherwise, ECDH crypto data should be set)
                SymmetricCryptoHandlerPtr(),                        //empty content encryption key (CEK) handler pointer (set at a later point)
                ecdhCryptoData,                                     //ECDH crypto data (if supplied; otherwise, the asymmetric crypto data should be set)
                ConnectionSetupState::INITIATED,                    //connection state
                securityManager.getDefaultSymmetricCipher(),        //CEK cipher type
                securityManager.getDefaultSymmetricCipherMode(),    //CEK cipher mode
                EMPTY_PLAINTEXT_DATA,                               //empty request signature data (set at a later point)
                INVALID_INTERNAL_SESSION_ID,                        //session ID (set at a later point)
                deviceData,                                         //device data
                nullptr,                                            //raw pointer to the last pending data sent
                connectionID,                                       //connection ID
                ConnectionPtr(),                                    //empty connection pointer (set at a later point)
                boost::signals2::connection(),                      //empty event connection (onDataReceived)
                boost::signals2::connection(),                      //empty event connection (onDisconnect)
                boost::signals2::connection()                       //empty event connection (onWriteResultRecieved)
            });
            
    auto result = pendingConnections.insert(std::pair<DeviceID, PendingConnectionDataPtr>(device, newPendingData));

    if(result.second)
    {
        return newPendingData;
    }
    else
    {
        logMessage(LogSeverity::Error, "(createDevicePendingConnectionData) >"
                " Pending connection data is already present for device ["
                + Convert::toString(device) + "].");
        
        throw std::logic_error("CommandConnectionsHandler::createDevicePendingConnectionData() >"
                " Pending connection data is already present for device [" + Convert::toString(device) + "].");
    }
}

NetworkManagement_Handlers::CommandConnectionsHandler::PendingConnectionDataPtr
NetworkManagement_Handlers::CommandConnectionsHandler::getPendingConnectionData
(const DeviceID device)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionDataMutex);
    auto result = pendingConnections.find(device);
    if(result != pendingConnections.end())
    {
        return result->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getPendingConnectionData) >"
                " Pending connection data not found for device ["
                + Convert::toString(device) + "].");
        
        throw std::logic_error("CommandConnectionsHandler::getPendingConnectionData() >"
                " Pending connection data not found for device ["
                + Convert::toString(device) + "].");
    }
}

NetworkManagement_Handlers::CommandConnectionsHandler::UnknownPendingConnectionDataPtr
NetworkManagement_Handlers::CommandConnectionsHandler::getUnknownPendingConnectionData
(const ConnectionID id)
{
    boost::lock_guard<boost::mutex> connectionDataLock(connectionDataMutex);
    auto result = unknownPendingConnections.find(id);
    if(result != unknownPendingConnections.end())
    {
        return result->second;
    }
    else
    {
        logMessage(LogSeverity::Error, "(getUnknownPendingConnectionData) >"
                " Pending connection data not found for connection ["
                + Convert::toString(id) + "].");
        
        throw std::logic_error("CommandConnectionsHandler::getUnknownPendingConnectionData() >"
                " Pending connection data not found for connection ["
                + Convert::toString(id) + "].");
    }
}

NetworkManagement_Handlers::CommandConnectionsHandler::EstablishedConnectionDataPtr
NetworkManagement_Handlers::CommandConnectionsHandler::getEstablishedConnectionData
(const DeviceID deviceID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    auto iterator = establishedConnections.find(deviceID);
    if(iterator != establishedConnections.end())
    {
        return iterator->second;
    }
    else
    {
        ++sendRequestsFailed;
        logMessage(LogSeverity::Error, "(getEstablishedConnectionData) >"
                " No established connection data found for device ["
                + Convert::toString(deviceID) + "].");
        
        throw std::logic_error("CommandConnectionsHandler::getEstablishedConnectionData() >"
                " No established connection data found for device ["
                + Convert::toString(deviceID) + "].");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Crypto">
CiphertextData * NetworkManagement_Handlers::CommandConnectionsHandler::generateConnectionRequestData
(PendingConnectionDataPtr remotePeerData)
{
    //builds the request signature
    RandomData signatureData(SaltGenerator::getRandomSalt(requestSignatureSize));
    ConnectionSetupRequestSignature requestSignature;
    requestSignature.set_signature_size(requestSignatureSize);
    requestSignature.set_signature_data(signatureData.BytePtr(), requestSignatureSize);
    PlaintextData plaintextSignature;
    if(!requestSignature.SerializeToString(&plaintextSignature))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize request signature.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize request signature.");
    }
    
    remotePeerData->requestSignatureData = plaintextSignature;

    //retrieves a new content encryption key (CEK)
    SymmetricCryptoDataGenerationRequest keyRequest(parentNetworkManager, remotePeerData->cipher, remotePeerData->mode);
    SymmetricCryptoDataContainerPromisePtr promise = securityManager.postRequest(keyRequest);
    SymmetricCryptoDataContainerPtr cekData = promise->get_future().get();

    remotePeerData->symCrypto = SymmetricCryptoHandlerPtr(new SymmetricCryptoHandler(cekData));

    //builds the request data
    CommandConnectionSetupRequestData requestData;
    requestData.set_sym_cipher(Convert::toString(securityManager.getDefaultSymmetricCipher()));
    requestData.set_sym_mode(Convert::toString(securityManager.getDefaultSymmetricCipherMode()));
    requestData.set_request_signature(plaintextSignature);
    requestData.set_content_encryption_key_data(cekData->getKey().BytePtr(), cekData->getKey().SizeInBytes());
    requestData.set_content_encryption_key_iv(cekData->getIV().BytePtr(), cekData->getIV().SizeInBytes());

    DeviceID peerID = localPeerID;
    if(remotePeerData->deviceData->getDeviceType() == PeerType::SERVER)
    {//resets the local peer ID, if the remote peer is a server
        auto authenticationData = authenticationDataRetrievalHandler(remotePeerData->deviceData->getDeviceID());
        requestData.set_password_data(authenticationData.plaintextPassword);
        peerID = authenticationData.id;
    }

    //serializes the request data object
    PlaintextData plaintextRequestData;
    if(!requestData.SerializeToString(&plaintextRequestData))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize request data.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize request data.");
    }
        
    //builds the actual request object
    CommandConnectionSetupRequest request;
    request.set_peer_id(Convert::toString(peerID));

    switch(keyExchange)
    {
        case KeyExchangeType::RSA:
        {
            //signs the request data with the local private key
            SignedData signedRequestData;
            localPeerCrypto->signDataWithPrivateKey(plaintextRequestData, signedRequestData);
            request.set_data(signedRequestData);
        } break;

        case KeyExchangeType::EC_DH:
        {
            //encrypts the request data with the ECDH key encryption key (KEK)
            CiphertextData encryptedRequestData;
            ECDHSymmetricCryptoDataGenerationRequest ecdhDataRequest(
                parentNetworkManager,
                localPeerECDHCryptoData->getPrivateKey(),
                remotePeerData->ecdhCrypto->getPublicKey());
            
            SymmetricCryptoDataContainerPromisePtr ecdhPromise = securityManager.postRequest(ecdhDataRequest);
            SymmetricCryptoDataContainerPtr ecdhSymmetricData = ecdhPromise->get_future().get();
            SymmetricCryptoHandler ecdhCryptoHandler(ecdhSymmetricData);
            request.set_ecdh_iv(ecdhSymmetricData->getIV().BytePtr(), ecdhSymmetricData->getIV().SizeInBytes());
            ecdhCryptoHandler.encryptData(plaintextRequestData, encryptedRequestData);
            request.set_data(encryptedRequestData);
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                    " Unexpected key exchange type encountered.");
            
            throw std::logic_error("CommandConnectionsHandler::generateConnectionRequestData() >"
                    " Unexpected key exchange type encountered.");
        }
    }

    //serializes the request object
    PlaintextData plaintextRequest;
    if(!request.SerializeToString(&plaintextRequest))
    {
        logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                " Failed to serialize request.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionRequestData() >"
                " Failed to serialize request.");
    }

    CiphertextData * securedRequest;

    switch(keyExchange)
    {
        case KeyExchangeType::RSA:
        {
            try
            {
                //encrypts the request with the remote public key
                securedRequest = new CiphertextData();
                remotePeerData->asymCrypto->encryptDataWithPublicKey(plaintextRequest, *securedRequest);
            }
            catch(...)
            {
                delete securedRequest;
                throw;
            }
        } break;

        case KeyExchangeType::EC_DH:
        {
            securedRequest = new CiphertextData(plaintextRequest);
            //no additional actions are needed (data is encrypted with KEK)
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(generateConnectionRequestData) >"
                    " Unexpected key exchange type encountered.");
            
            throw std::logic_error("CommandConnectionsHandler::generateConnectionRequestData() >"
                    " Unexpected key exchange type encountered.");
        }
    }

    return securedRequest;
}

CiphertextData *
NetworkManagement_Handlers::CommandConnectionsHandler::generateConnectionResponseDataFromRequest
(const CiphertextData & encryptedRequest, const ConnectionID connectionID)
{
    PlaintextData plaintextRequest;

    switch(keyExchange)
    {
        case KeyExchangeType::RSA:
        {
            localPeerCrypto->decryptDataWithPrivateKey(encryptedRequest, plaintextRequest);
        } break;

        case KeyExchangeType::EC_DH:
        {
            //no actions are needed (data is encrypted with KEK)
            plaintextRequest = encryptedRequest;
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                    " Unexpected key exchange type encountered.");
            
            throw std::logic_error("CommandConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                    " Unexpected key exchange type encountered.");
        }
    }

    //parses the request object
    CommandConnectionSetupRequest requestObject;
    requestObject.ParseFromString(plaintextRequest);

    //validates the request
    if(!requestObject.IsInitialized()
       || (keyExchange == KeyExchangeType::EC_DH && !requestObject.has_ecdh_iv()))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to validate connection setup request.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to validate connection setup request.");
    }

    //creates and retrieves the various data objects
    PendingConnectionDataPtr remotePeerData = createPendingConnectionData(
            Convert::toDeviceID(requestObject.peer_id()), connectionID);
    
    UnknownPendingConnectionDataPtr unknownRemotePeerData =
            getUnknownPendingConnectionData(connectionID);
    
    unknownRemotePeerData->deviceID = Convert::toDeviceID(requestObject.peer_id());

    PlaintextData plaintextRequestData;
    switch(keyExchange)
    {
        case KeyExchangeType::RSA:
        {
            localPeerCrypto->verifyAndRecoverDataWithPublicKey(
                requestObject.data(), plaintextRequestData);
        } break;

        case KeyExchangeType::EC_DH:
        {
            //decrypts the request data with the KEK
            IVData kekIV(reinterpret_cast<const unsigned char*>(requestObject.ecdh_iv().data()), requestObject.ecdh_iv().size());
            
            ECDHSymmetricCryptoDataGenerationRequest ecdhDataRequest(
                parentNetworkManager,
                localPeerECDHCryptoData->getPrivateKey(),
                remotePeerData->ecdhCrypto->getPublicKey(),
                kekIV);
            
            SymmetricCryptoDataContainerPromisePtr ecdhPromise = securityManager.postRequest(ecdhDataRequest);
            SymmetricCryptoDataContainerPtr ecdhSymmetricData = ecdhPromise->get_future().get();
            SymmetricCryptoHandler ecdhCryptoHandler(ecdhSymmetricData);
            ecdhCryptoHandler.decryptData(requestObject.data(), plaintextRequestData);
        } break;

        default:
        {
            logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                    " Unexpected key exchange type encountered.");
            
            throw std::logic_error("CommandConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                    " Unexpected key exchange type encountered.");
        }
    }

    //parses the request data object
    CommandConnectionSetupRequestData requestDataObject;
    requestDataObject.ParseFromString(plaintextRequestData);

    if(!requestDataObject.IsInitialized() || !requestDataObject.has_password_data())
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to validate connection setup request data.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to validate connection setup request data.");
    }

    //verifies the received data
    try
    {
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(
            requestDataObject.request_signature());
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to verify request signature.");
        throw;
    }
    
    remotePeerData->sessionID = verifyPeerPasswordAndCreateSession(
            requestObject.peer_id(), requestDataObject.password_data());

    //builds the content encryption key (CEK)
    KeyData cek(reinterpret_cast<const unsigned char*>(
        requestDataObject.content_encryption_key_data().data()),
        requestDataObject.content_encryption_key_data().size());
    
    IVData cekIV(reinterpret_cast<const unsigned char*>(
        requestDataObject.content_encryption_key_iv().data()),
        requestDataObject.content_encryption_key_iv().size());
    
    SymmetricCryptoDataGenerationRequest keyRequest
            (
                parentNetworkManager,
                Convert::toSymmetricCipherType(requestDataObject.sym_cipher()),
                Convert::toAuthenticatedSymmetricCipherModeType(requestDataObject.sym_mode()),
                cek,
                cekIV
            );
    SymmetricCryptoDataContainerPromisePtr promise = securityManager.postRequest(keyRequest);
    SymmetricCryptoDataContainerPtr cekData = promise->get_future().get();

    remotePeerData->symCrypto = SymmetricCryptoHandlerPtr(new SymmetricCryptoHandler(cekData));

    //builds the response object
    CommandConnectionSetupResponse response;
    response.set_request_signature(requestDataObject.request_signature());

    if(remotePeerData->deviceData->getDeviceType() == PeerType::SERVER)
    {//sets the password data field, if the remote peer is a server
        auto authenticationData =
            authenticationDataRetrievalHandler(Convert::toDeviceID(requestObject.peer_id()));
        
        response.set_password_data(authenticationData.plaintextPassword);
    }

    //serializes the response object
    PlaintextData plaintextResponse;
    if(!response.SerializeToString(&plaintextResponse))
    {
        logMessage(LogSeverity::Error, "(generateConnectionResponseDataFromRequest) >"
                " Failed to serialize response.");
        
        throw std::runtime_error("CommandConnectionsHandler::generateConnectionResponseDataFromRequest() >"
                " Failed to serialize response.");
    }

    //encrypts the response object
    CiphertextData * encryptedResponse = new CiphertextData();
    try
    {
        remotePeerData->symCrypto->encryptData(plaintextResponse, *encryptedResponse);
    }
    catch(...)
    {
        delete encryptedResponse;
        throw;
    }

    return encryptedResponse;
}

void NetworkManagement_Handlers::CommandConnectionsHandler::verifyConnectionResponseData
(const CiphertextData & encryptedResponseData, PendingConnectionDataPtr remotePeerData)
{
    //decrypts the response data
    PlaintextData plaintextResponse;
    remotePeerData->symCrypto->decryptData(encryptedResponseData, plaintextResponse);

    //parses the response data object
    CommandConnectionSetupResponse responseObject;
    responseObject.ParseFromString(plaintextResponse);

    if(!responseObject.IsInitialized() || !responseObject.has_password_data())
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to validate connection setup response data.");
        
        throw std::runtime_error("CommandConnectionsHandler::verifyConnectionResponseData() >"
                " Failed to validate connection setup response data.");
    }

    //verifies the received data
    try
    {
        NetworkManagement_Protocols::Utilities::verifyRequestSignature(
            responseObject.request_signature(), remotePeerData->requestSignatureData);
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(verifyConnectionResponseData) >"
                " Failed to verify request signature.");
        throw;
    }
    
    remotePeerData->requestSignatureData.clear();
    remotePeerData->sessionID =
            verifyPeerPasswordAndCreateSession(
                remotePeerData->deviceData->getDeviceID(), responseObject.password_data());
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Local Connections">
void NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_PendingLocalConnections
(RawConnectionID rawID, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;
    
    logMessage(LogSeverity::Info, "(onDisconnectHandler_PendingLocalConnections) >"
            " Terminating connection for device [" + Convert::toString(deviceID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    terminateConnection(deviceID);
    ++connectionsFailed;
    onConnectionEstablishmentFailed(deviceID, connectionID);
}

void NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_PendingLocalConnections
(ByteData data, PacketSize remaining, const DeviceID deviceID, const ConnectionID connectionID, ConnectionPtr connection)
{
    if(!active)
        return;
    
    try
    {
        if(remaining > 0)
        {
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                    " Cannot process connection response for device [" + Convert::toString(deviceID)
                    + "]; more data remains to be received.");
            
            throw std::runtime_error("CommandConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                    " Cannot process connection response for device [" + Convert::toString(deviceID)
                    + "]; more data remains to be received.");
        }

        PendingConnectionDataPtr connectionData = getPendingConnectionData(deviceID);
        connection->disableDataEvents();

        EstablishedConnectionDataPtr establishedConnectionData;
        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);

            if(connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT
                && connectionData->state != ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED)
            {
                logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for device [" + Convert::toString(deviceID) + "].");
                
                throw std::logic_error("CommandConnectionsHandler::onDataReceivedHandler_PendingLocalConnections() >"
                        " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                        + "] for device [" + Convert::toString(deviceID) + "].");
            }

            connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_RECEIVED;
            delete connectionData->lastPendingData;
            connectionData->lastPendingData = nullptr;
            verifyConnectionResponseData(data, connectionData);
            connectionData->state = ConnectionSetupState::COMPLETED;

            //builds the established connection data
            establishedConnectionData = EstablishedConnectionDataPtr(new EstablishedConnectionData
            {
                connectionData->connectionID,   //connection ID
                connection->getID(),            //raw connection ID
                connection,                     //connection pointer
                connectionData->deviceData,     //device data
                connectionData->sessionID,      //session ID
                false,                          //bridged connection?
                ConnectionPtr(),                //bridge target connection
                connectionData->symCrypto       //content encryption handler
            });

            {
                boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
                establishedConnections.insert(
                    std::pair<DeviceID, EstablishedConnectionDataPtr>(
                        deviceID, establishedConnectionData));
                
                pendingConnections.erase(deviceID);
            }

            //detaches the pending connection handlers
            connectionData->onDataReceivedEventConnection.disconnect();
            connectionData->onDisconnectEventConnection.disconnect();
            connectionData->onWriteResultReceivedEventConnection.disconnect();
        }

        //attaches the established connection handlers
        establishedConnectionData->onDataReceivedEventConnection =
                connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_EstablishedConnections,
                                this, _1, _2, deviceID, connectionID));
        
        establishedConnectionData->onDisconnectEventConnection =
                connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        establishedConnectionData->onWriteResultReceivedEventConnection =
                connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections,
                                this, _1, deviceID, connectionID));
        
        connection->enableDataEvents();
        
        logMessage(LogSeverity::Info, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Setup completed for device [" + Convert::toString(deviceID) + "].");
        
        ++connectionsEstablished;
        onConnectionEstablished(deviceID, connectionID);
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Response validation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        throw;
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_PendingLocalConnections
(bool received, const DeviceID deviceID, const ConnectionID connectionID, ConnectionPtr connection)
{
    if(!active)
        return;
    
    if(!received)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Failed to send request data to [" + Convert::toString(deviceID) + "].");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        return;
    }

    try
    {
        PendingConnectionDataPtr connectionData = getPendingConnectionData(deviceID);
        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
        if(connectionData->state == ConnectionSetupState::CONNECTION_REQUEST_SENT)
        {
            delete connectionData->lastPendingData;
            connectionData->lastPendingData = nullptr;
            connectionData->state = ConnectionSetupState::CONNECTION_REQUEST_SENT_CONFIRMED;
        }
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingLocalConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(deviceID);
        ++connectionsFailed;
        onConnectionEstablishmentFailed(deviceID, connectionID);
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Pending Remote Connections">
void NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_PendingRemoteConnections
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

void NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections
(ByteData data, PacketSize remaining, const ConnectionID connectionID, ConnectionPtr connection)
{
    if(!active)
        return;
    
    if(remaining > 0)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Cannot process connection response for connection ["
                + Convert::toString(connectionID) + "]; more data remains to be received.");
        
        throw std::runtime_error("CommandConnectionsHandler::onDataReceivedHandler_PendingRemoteConnections() >"
                " Cannot process connection response for connection [" + Convert::toString(connectionID)
                + "]; more data remains to be received.");
    }

    UnknownPendingConnectionDataPtr unknownConnectionData = getUnknownPendingConnectionData(connectionID);

    if(!unknownConnectionData->connection->isActive())
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Connection [" + Convert::toString(connectionID) + "] is not active.");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        return;
    }

    try
    {
        //generates and send a connection response
        CiphertextData * responseData = generateConnectionResponseDataFromRequest(data, connectionID);
        PendingConnectionDataPtr connectionData = getPendingConnectionData(unknownConnectionData->deviceID);

        boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
        connectionData->connection = connection;
        connectionData->lastPendingData = responseData;
        connection->sendData(*responseData);
        connectionData->state = ConnectionSetupState::CONNECTION_RESPONSE_SENT;
    }
    catch(const std::runtime_error & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Request validation failed: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        return;
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        throw;
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections
(bool received, const ConnectionID connectionID)
{
    if(!active)
        return;
    
    UnknownPendingConnectionDataPtr unknownConnectionData = getUnknownPendingConnectionData(connectionID);

    if(!received)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Failed to send response data to [" + Convert::toString(unknownConnectionData->deviceID) + "].");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        return;
    }

    try
    {
        PendingConnectionDataPtr connectionData = getPendingConnectionData(unknownConnectionData->deviceID);

        if(connectionData->state != ConnectionSetupState::CONNECTION_RESPONSE_SENT)
        {
            logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for device [" + Convert::toString(unknownConnectionData->deviceID) + "].");
            
            throw std::logic_error("CommandConnectionsHandler::onWriteResultReceivedHandler_PendingRemoteConnections() >"
                    " Unexpected connection state encountered [" + Convert::toString(connectionData->state)
                    + "] for device [" + Convert::toString(unknownConnectionData->deviceID) + "].");
        }

        unknownConnectionData->connection->disableDataEvents();

        EstablishedConnectionDataPtr establishedConnectionData;
        {
            boost::lock_guard<boost::mutex> dataLock(connectionData->connectionDataMutex);
            delete connectionData->lastPendingData;
            connectionData->lastPendingData = nullptr;
            connectionData->state = ConnectionSetupState::COMPLETED;

            //builds the established connection data
            establishedConnectionData = EstablishedConnectionDataPtr(new EstablishedConnectionData
            {
                connectionID,                               //connection ID
                unknownConnectionData->connection->getID(), //raw connection ID
                unknownConnectionData->connection,          //connection pointer
                connectionData->deviceData,                 //device data
                connectionData->sessionID,                  //session ID
                false,                                      //bridged connection?
                ConnectionPtr(),                            //bridge target connection
                connectionData->symCrypto                   //content encryption handler
            });

            {
                boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
                establishedConnections.insert(
                    std::pair<DeviceID, EstablishedConnectionDataPtr>(
                        unknownConnectionData->deviceID, establishedConnectionData));
                
                unknownPendingConnections.erase(connectionID);
                pendingConnections.erase(unknownConnectionData->deviceID);
            }
        }

        //detaches the pending connection handlers
        unknownConnectionData->onDataReceivedEventConnection.disconnect();
        unknownConnectionData->onDisconnectEventConnection.disconnect();
        unknownConnectionData->onWriteResultReceivedEventConnection.disconnect();

        //attaches the established connection handlers
        establishedConnectionData->onDataReceivedEventConnection =
                unknownConnectionData->connection->onDataReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_EstablishedConnections,
                                this, _1, _2, unknownConnectionData->deviceID, connectionID));
        
        establishedConnectionData->onDisconnectEventConnection =
                unknownConnectionData->connection->onDisconnectEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_EstablishedConnections,
                                this, _1, unknownConnectionData->deviceID, connectionID));
        
        establishedConnectionData->onWriteResultReceivedEventConnection =
                unknownConnectionData->connection->onWriteResultReceivedEventAttach(
                    boost::bind(&NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections,
                                this, _1, unknownConnectionData->deviceID, connectionID));
        
        unknownConnectionData->connection->enableDataEvents();
        
        logMessage(LogSeverity::Info, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Setup completed for device [" + Convert::toString(unknownConnectionData->deviceID)
                + "] on connection [" + Convert::toString(connectionID) + "].");
        
        ++connectionsEstablished;
        onConnectionEstablished(unknownConnectionData->deviceID, connectionID);
    }
    catch(const std::exception & e)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Exception encountered: [" + std::string(e.what()) + "].");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        throw;
    }
    catch(...)
    {
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_PendingRemoteConnections) >"
                " Unknown exception encountered.");
        
        terminateConnection(connectionID);
        ++connectionsFailed;
        throw;
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Handlers - Established Connections">
void NetworkManagement_Handlers::CommandConnectionsHandler::onDisconnectHandler_EstablishedConnections
(RawConnectionID rawID, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    logMessage(LogSeverity::Info, "(onDisconnectHandler_EstablishedConnections) >"
            " Terminating connection for device [" + Convert::toString(deviceID)
            + "] with raw ID [" + Convert::toString(rawID) + "].");
    terminateConnection(deviceID);
    onEstablishedConnectionClosed(deviceID, connectionID);
}

void NetworkManagement_Handlers::CommandConnectionsHandler::onDataReceivedHandler_EstablishedConnections
(ByteData encryptedData, PacketSize remaining, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    ++totalDataObjectsReceived;
    logMessage(LogSeverity::Info, "(onDataReceivedHandler_EstablishedConnections) >"
            " Received data for device [" + Convert::toString(deviceID) + "].");

    if(remaining > 0)
    {
        ++invalidDataObjectsReceived;
        logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                " Cannot process data for device [" + Convert::toString(deviceID)
                + "]; more data remains to be received.");
    }
    else
    {
        try
        {
            EstablishedConnectionDataPtr connectionData = getEstablishedConnectionData(deviceID);
            PlaintextData plaintextData;
            connectionData->cryptoHandler->decryptData(encryptedData, plaintextData);

            ++validDataObjectsReceived;
            onCommandDataReceived(deviceID, plaintextData);
        }
        catch(const std::exception & e)
        {
            ++invalidDataObjectsReceived;
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                    " Exception encountered: [" + std::string(e.what()) + "] for device ["
                    + Convert::toString(deviceID) + "].");
        }
        catch(...)
        {
            ++invalidDataObjectsReceived;
            logMessage(LogSeverity::Error, "(onDataReceivedHandler_EstablishedConnections) >"
                    " Unknown exception encountered for device [" + Convert::toString(deviceID) + "].");
        }
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::onWriteResultReceivedHandler_EstablishedConnections
(bool received, const DeviceID deviceID, const ConnectionID connectionID)
{
    if(!active)
        return;

    EstablishedConnectionDataPtr connectionData = getEstablishedConnectionData(deviceID);

    if(!received)
    {
        ++sendRequestsFailed;
        logMessage(LogSeverity::Error, "(onWriteResultReceivedHandler_EstablishedConnections) >"
                " Failed to send data to [" + Convert::toString(deviceID) + "].");
    }
    else
    {
        ++sendRequestsConfirmed;
    }

    boost::lock_guard<boost::mutex> connectionDataLock(connectionData->connectionDataMutex);
    delete connectionData->pendingData.front();
    connectionData->pendingData.pop();
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Cleanup">
void NetworkManagement_Handlers::CommandConnectionsHandler::terminateConnection
(const ConnectionID connectionID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);

    auto unknownConnectionData = unknownPendingConnections.find(connectionID);
    if(unknownConnectionData != unknownPendingConnections.end())
    {
        auto pendingConnectionData = pendingConnections.find(unknownConnectionData->second->deviceID);
        if(pendingConnectionData != pendingConnections.end())
        {
            boost::lock_guard<boost::mutex> localDataLock(pendingConnectionData->second->connectionDataMutex);
            delete pendingConnectionData->second->lastPendingData;
            pendingConnectionData->second->lastPendingData = nullptr;
            pendingConnectionData->second->onDataReceivedEventConnection.disconnect();
            pendingConnectionData->second->onDisconnectEventConnection.disconnect();
            pendingConnectionData->second->onWriteResultReceivedEventConnection.disconnect();
            pendingConnections.erase(pendingConnectionData);
        }

        auto establishedConnectionData = establishedConnections.find(unknownConnectionData->second->deviceID);
        if(establishedConnectionData != establishedConnections.end())
        {
            while(establishedConnectionData->second->pendingData.size() > 0)
            {
                delete establishedConnectionData->second->pendingData.front();
                establishedConnectionData->second->pendingData.pop();
            }
            
            establishedConnectionData->second->onDataReceivedEventConnection.disconnect();
            establishedConnectionData->second->onDisconnectEventConnection.disconnect();
            establishedConnectionData->second->onWriteResultReceivedEventConnection.disconnect();
            
            establishedConnections.erase(establishedConnectionData);
        }

        unknownConnectionData->second->onDataReceivedEventConnection.disconnect();
        unknownConnectionData->second->onDisconnectEventConnection.disconnect();
        unknownConnectionData->second->onWriteResultReceivedEventConnection.disconnect();
        unknownConnectionData->second->connection->disconnect();
        unknownPendingConnections.erase(connectionID);
    }
    else
    {
        logMessage(LogSeverity::Error, "(cleanupAfterConnectionTermination) >"
                " Failed to find data for connection [" + Convert::toString(connectionID) + "].");
        
        throw std::runtime_error("CommandConnectionsHandler::cleanupAfterConnectionTermination(ConnectionID) >"
                " Failed to find data for connection [" + Convert::toString(connectionID) + "].");
    }
}

void NetworkManagement_Handlers::CommandConnectionsHandler::terminateConnection
(const DeviceID deviceID)
{
    boost::lock_guard<boost::mutex> globalDataLock(connectionDataMutex);
    
    auto pendingConnectionData = pendingConnections.find(deviceID);
    if(pendingConnectionData != pendingConnections.end())
    {
        delete pendingConnectionData->second->lastPendingData;
        pendingConnectionData->second->lastPendingData = nullptr;
        pendingConnectionData->second->onDataReceivedEventConnection.disconnect();
        pendingConnectionData->second->onDisconnectEventConnection.disconnect();
        pendingConnectionData->second->onWriteResultReceivedEventConnection.disconnect();
        if(pendingConnectionData->second->connection)
            pendingConnectionData->second->connection->disconnect();
        
        pendingConnections.erase(pendingConnectionData);
    }

    auto establishedConnectionData = establishedConnections.find(deviceID);
    if(establishedConnectionData != establishedConnections.end())
    {
        while(establishedConnectionData->second->pendingData.size() > 0)
        {
            delete establishedConnectionData->second->pendingData.front();
            establishedConnectionData->second->pendingData.pop();
        }
        
        establishedConnectionData->second->onDataReceivedEventConnection.disconnect();
        establishedConnectionData->second->onDisconnectEventConnection.disconnect();
        establishedConnectionData->second->onWriteResultReceivedEventConnection.disconnect();
        if(establishedConnectionData->second->connection)
            establishedConnectionData->second->connection->disconnect();
        
        establishedConnections.erase(establishedConnectionData);
    }
}
//</editor-fold>
