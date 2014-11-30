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

#include "Connection.h"

NetworkManagement_Connections::Connection::Connection(ConnectionParamters connectionParams, boost::asio::streambuf * externalReadBuffer, Utilities::FileLogger * debugLogger)
    : debugLogger(debugLogger), writeStrand(connectionParams.socket->get_io_service()),
      readStrand(connectionParams.socket->get_io_service()), socket(connectionParams.socket),
      connectionID(connectionParams.connectionID), localPeerType(connectionParams.localPeerType),
      connectionType(connectionParams.expectedConnection), state(ConnectionState::INVALID),
      lastSubstate(ConnectionSubstate::NONE), initiation(connectionParams.initiation)
{
    if(externalReadBuffer != nullptr)
    {//use an external buffer
        readBuffer = externalReadBuffer;
        isExternalReadBufferUsed = true;
    }
    else
    {//use an internal buffer
        readBuffer = new boost::asio::streambuf(connectionParams.readBufferSize);
        isExternalReadBufferUsed = false;
    }

    boost::asio::async_read(*socket, readBuffer->prepare(ConnectionRequest::BYTE_LENGTH), readStrand.wrap(boost::bind(&NetworkManagement_Connections::Connection::initialReadRequestHandler, this, _1, _2)));
}

NetworkManagement_Connections::Connection::Connection(ConnectionParamters connectionParams, ConnectionRequest requestParams,
                                                      boost::asio::streambuf * externalReadBuffer, Utilities::FileLogger * debugLogger)
    : debugLogger(debugLogger), writeStrand(connectionParams.socket->get_io_service()),
      readStrand(connectionParams.socket->get_io_service()), socket(connectionParams.socket),
      connectionID(connectionParams.connectionID), localPeerType(connectionParams.localPeerType),
      connectionType(connectionParams.expectedConnection), state(ConnectionState::INVALID),
      lastSubstate(ConnectionSubstate::NONE), initiation(connectionParams.initiation)
{
    if(externalReadBuffer != nullptr)
    {//use an external buffer
        readBuffer = externalReadBuffer;
        isExternalReadBufferUsed = true;
    }
    else
    {//use an internal buffer
        readBuffer = new boost::asio::streambuf(connectionParams.readBufferSize);
        isExternalReadBufferUsed = false;
    }
    
    connectionRequest = requestParams;
    boost::asio::async_write(*socket, boost::asio::buffer(connectionRequest.toBytes(), ConnectionRequest::BYTE_LENGTH), writeStrand.wrap(boost::bind(&NetworkManagement_Connections::Connection::initialWriteRequestHandler, this, _1, _2)));
}

NetworkManagement_Connections::Connection::~Connection()
{
    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (~) [" + Tools::toString(connectionID) + "] > Destruction initiated.");
    
    disconnect();
    socket.reset();
    
    //clears all events data
    std::queue<ConnectionEventID>().swap(events);
    for(auto currentEventData : eventsData)
        delete currentEventData.second;
    eventsData.clear();

    onConnect.disconnect_all_slots();
    onDisconnect.disconnect_all_slots();
    onDataReceived.disconnect_all_slots();
    onWriteResultReceived.disconnect_all_slots();
    canBeDestroyed.disconnect_all_slots();
    
    if(!isExternalReadBufferUsed)
        delete readBuffer;
    
    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (~) [" + Tools::toString(connectionID) + "] > Destruction completed.");
    
    debugLogger = nullptr;
}

void NetworkManagement_Connections::Connection::disconnect()
{
    if(closeConnection)
        return;
    
    //TODO - move to aborting the connection on the server side
    closeConnection = true;
    state = ConnectionState::CLOSED;
    socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    socket->close();
    
    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Disconnect) [" + Tools::toString(connectionID) + "] > Disconnected.");
    onDisconnectEvent();
    canBeDestroyedEvent();
}

void NetworkManagement_Connections::Connection::sendData(const ByteVector & data)
{
    if(closeConnection)
        return;
    
    boost::lock_guard<boost::mutex> pendingDataLock(writeDataMutex);
    ++pendingWriteOperations;
    if(pendingWriteOperations > 1)
        pendingWritesData.push(&data);
    else
        queueNextWrite(data);
}

void NetworkManagement_Connections::Connection::sendData(const boost::asio::streambuf & data)
{
    //TODO 
}

void NetworkManagement_Connections::Connection::enableLifecycleEvents()
{
    boost::unique_lock<boost::mutex> eventsLock(eventsMutex);
    if(lifecycleEventsBlocked)
    {
        lifecycleEventsBlocked = false;
        std::queue<ConnectionEventID> remainingEvents;
        std::queue<EventType> eventsToFire;
        
        //processes all pending events
        while(events.size() > 0)
        {
            ConnectionEventID currentEventID = events.front();
            
            boost::tuples::tuple<EventType, boost::any, boost::any> * currentEventData = eventsData[currentEventID];
            bool erase = true;
            
            switch(currentEventData->get<0>())
            {
                case EventType::CONNECT:                eventsToFire.push(EventType::CONNECT); break;
                case EventType::DISCONNECT:             eventsToFire.push(EventType::DISCONNECT); break;
                case EventType::CAN_BE_DESTROYED:       eventsToFire.push(EventType::CAN_BE_DESTROYED); break;
                case EventType::DATA_RECEIVED:          { remainingEvents.push(currentEventID); erase = false; } break;
                case EventType::WRITE_RESULT_RECEIVED:  { remainingEvents.push(currentEventID); erase = false; } break;
                default: debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Enable Lifecycle Events) [" + Tools::toString(connectionID) + "] > Unexpected event type encountered."); break;
            }
            
            if(erase)
            {
                eventsData.erase(currentEventID);
                delete currentEventData;
            }
            
            events.pop();
        }
        
        //puts back any events that could not be processed
        if(remainingEvents.size() > 0)
            events.swap(remainingEvents);
        
        eventsLock.unlock();
        
        //fires all eligible events
        while(eventsToFire.size() > 0)
        {
            EventType currentEvent = eventsToFire.front();
            eventsToFire.pop();
            
            switch(currentEvent)
            {
                case EventType::CONNECT:                onConnect(connectionID); break;
                case EventType::DISCONNECT:             onDisconnect(connectionID); break;
                case EventType::CAN_BE_DESTROYED:       canBeDestroyed(connectionID, initiation); break;
                default: debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Enable Lifecycle Events) [" + Tools::toString(connectionID) + "] > Unexpected event type encountered."); break;
            }
        }
    }
}

void NetworkManagement_Connections::Connection::disableLifecycleEvents()
{
    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
    if(!lifecycleEventsBlocked)
        lifecycleEventsBlocked = true;
}

void NetworkManagement_Connections::Connection::enableDataEvents()
{
    boost::unique_lock<boost::mutex> eventsLock(eventsMutex);
    if(dataEventsBlocked)
    {
        dataEventsBlocked = false;
        std::queue<ConnectionEventID> remainingEvents;
        std::queue<boost::tuples::tuple<EventType, boost::any, boost::any> *> eventsToFire;
        
        //processes all pending events
        while(events.size() > 0)
        {
            ConnectionEventID currentEventID = events.front();
            boost::tuples::tuple<EventType, boost::any, boost::any> * currentEventData = eventsData[currentEventID];
            bool erase = true;
            
            switch(currentEventData->get<0>())
            {
                case EventType::DATA_RECEIVED:          eventsToFire.push(currentEventData); break;
                case EventType::WRITE_RESULT_RECEIVED:  eventsToFire.push(currentEventData); break;
                case EventType::CONNECT:                { remainingEvents.push(currentEventID); erase = false; } break;
                case EventType::DISCONNECT:             { remainingEvents.push(currentEventID); erase = false; } break;
                case EventType::CAN_BE_DESTROYED:       { remainingEvents.push(currentEventID); erase = false; } break;
                default: debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Enable Data Events) [" + Tools::toString(connectionID) + "] > Unexpected event type encountered."); break;
            }
            
            if(erase)
            {
                eventsData.erase(currentEventID);
            }
            
            events.pop();
        }
        
        //puts back any events that could not be processed
        if(remainingEvents.size() > 0)
            events.swap(remainingEvents);
        
        eventsLock.unlock();
        
        //fires all eligible events
        while(eventsToFire.size() > 0)
        {
            boost::tuples::tuple<EventType, boost::any, boost::any> * currentEvent = eventsToFire.front();
            eventsToFire.pop();
            
            switch(currentEvent->get<0>())
            {
                case EventType::DATA_RECEIVED:          onDataReceived(boost::any_cast<ByteVector>(currentEvent->get<1>()), boost::any_cast<PacketSize>(currentEvent->get<2>())); break;
                case EventType::WRITE_RESULT_RECEIVED:  onWriteResultReceived(boost::any_cast<bool>(currentEvent->get<1>())); break;
                default: debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Enable Data Events) [" + Tools::toString(connectionID) + "] > Unexpected event type encountered."); break;
            }
            
            delete currentEvent;
        }
    }
}

void NetworkManagement_Connections::Connection::disableDataEvents()
{
    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
    if(!dataEventsBlocked)
        dataEventsBlocked = true;
}

void NetworkManagement_Connections::Connection::initialReadRequestHandler(const boost::system::error_code & readError, std::size_t)
{
    if(closeConnection)
    {
        --pendingHandlers;
        return;
    }
    
    if(!readError)
    {
        try
        {
            readBuffer->commit(ConnectionRequest::BYTE_LENGTH); //makes the incoming data available
            boost::asio::streambuf::const_buffers_type rawRequest = readBuffer->data(); //retrieves the incoming data
            ConnectionRequest request = ConnectionRequest::fromBytes(ByteVector(boost::asio::buffers_begin(rawRequest), boost::asio::buffers_begin(rawRequest) + ConnectionRequest::BYTE_LENGTH));
            debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Initial Read Request Handler) [" + Tools::toString(connectionID) + "] > Request data received.");
            readBuffer->consume(ConnectionRequest::BYTE_LENGTH); //removes the incoming data from the buffer
            
            if(request.connectionType == connectionType)
            {
                connectionRequest = request;
                state = ConnectionState::ESTABLISHED;
                isHeaderExpected = true;
                lastSubstate = ConnectionSubstate::WAITING;
                queueNextRead(HeaderPacket::BYTE_LENGTH);
                onConnectEvent();
            }
            else
            {
                debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Initial Read Request Handler) [" + Tools::toString(connectionID) + "] > Invalid connection type requested.");
                lastSubstate = ConnectionSubstate::FAILED;
                disconnect();
            }
        }
        catch(std::invalid_argument & ex)
        {
            debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Initial Read Request Handler) [" + Tools::toString(connectionID) + "] > Invalid request data received: <" + ex.what() + ">.");
            lastSubstate = ConnectionSubstate::FAILED;
            disconnect();
        }
        catch(...)
        {
            --pendingHandlers;
            throw;
        }
    }
    else
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Initial Read Request Handler) [" + Tools::toString(connectionID) + "] > Read error encountered: <" + readError.message() + ">");
        lastSubstate = ConnectionSubstate::FAILED;
        disconnect();
    }
    
    --pendingHandlers;
}

void NetworkManagement_Connections::Connection::initialWriteRequestHandler(const boost::system::error_code & writeError, std::size_t)
{
    if(closeConnection)
    {
        --pendingHandlers;
        return;
    }
    
    if(!writeError)
    {
        state = ConnectionState::ESTABLISHED;
        isHeaderExpected = true;
        lastSubstate = ConnectionSubstate::WAITING;
        queueNextRead(HeaderPacket::BYTE_LENGTH);
        onConnectEvent();
    }
    else
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Initial Write Request Handler) [" + Tools::toString(connectionID) + "] > Write error encountered: <" + writeError.message() + ">");
        lastSubstate = ConnectionSubstate::FAILED;
        disconnect();
    }
    
    --pendingHandlers;
}

void NetworkManagement_Connections::Connection::queueNextRead(BufferSize readSize)
{
    if(closeConnection)
        return;
    
    ++pendingHandlers;
    boost::asio::async_read(*socket, readBuffer->prepare(readSize), readStrand.wrap(boost::bind(&NetworkManagement_Connections::Connection::readHandler, this, _1, _2)));
}

void NetworkManagement_Connections::Connection::queueNextWrite(const ByteVector & data)
{
    if(closeConnection)
        return;
    
    lastSubstate = ConnectionSubstate::WRITING;

    //creates the header packet and puts it with the data in a new buffer sequence
    ByteVector headerData(HeaderPacket::BYTE_LENGTH);
    HeaderPacket{data.size()}.toNetworkBytes(headerData);
    std::vector<boost::asio::const_buffer> sendBuffer;
    sendBuffer.push_back(boost::asio::buffer(headerData, HeaderPacket::BYTE_LENGTH));
    sendBuffer.push_back(boost::asio::buffer(data, data.size()));
    
    ++pendingHandlers;
    boost::asio::async_write(*socket, sendBuffer, writeStrand.wrap(boost::bind(&NetworkManagement_Connections::Connection::writeHandler, this, _1, _2)));
}

void NetworkManagement_Connections::Connection::readHandler(const boost::system::error_code & readError, std::size_t bytesRead)
{
    if(closeConnection)
    {
        --pendingHandlers;
        return;
    }
    
    if(!readError)
    {
        lastSubstate = ConnectionSubstate::READING;
        
        readBuffer->commit(bytesRead); //makes the incoming data available
        boost::asio::streambuf::const_buffers_type rawData = readBuffer->data(); //retrieves the incoming data
        ByteVector data(boost::asio::buffers_begin(rawData), boost::asio::buffers_begin(rawData) + bytesRead);
        readBuffer->consume(bytesRead); //removes the incoming data from the buffer
        
        //TODO - optimise?
        if(isHeaderExpected)
        {//the incoming data should represent a header
            HeaderPacket header = HeaderPacket::fromNetworkBytes(data);
            isHeaderExpected = false;
            
            if(header.payloadSize > readBuffer->max_size())
            {//too much data for the buffer was sent; needs to be split into several reads
                remainingBytes = header.payloadSize;
                queueNextRead(readBuffer->max_size());
            }
            else if(header.payloadSize == 0)
            {//empty header was received
                debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Read Handler) [" + Tools::toString(connectionID) + "] > Header with payload size '0' encountered.");
                isHeaderExpected = true;
                queueNextRead(HeaderPacket::BYTE_LENGTH);
            }
            else
            {//the data is going to fit in the buffer
                queueNextRead(header.payloadSize);
            }
        }
        else
        {//data
            received += bytesRead;
            
            if(remainingBytes > 0)
            {//expecting more data
                remainingBytes -= bytesRead;
                
                if(remainingBytes > readBuffer->max_size())
                {//more read operations are needed
                    queueNextRead(readBuffer->max_size());
                }
                else if(remainingBytes == 0)
                {//no more data to read
                    isHeaderExpected = true;
                    queueNextRead(HeaderPacket::BYTE_LENGTH);
                }
                else
                {//last read operation
                    queueNextRead(remainingBytes);
                }
            }
            else
            {//all data was read
                isHeaderExpected = true;
                queueNextRead(HeaderPacket::BYTE_LENGTH);
            }
            
            onDataReceivedEvent(data, remainingBytes);
        }
        
        lastSubstate = ConnectionSubstate::WAITING;
    }
    else if(readError == boost::asio::error::eof || readError == boost::asio::error::connection_reset || readError == boost::asio::error::connection_aborted)
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Read Handler) [" + Tools::toString(connectionID) + "] > Connection terminated by remote peer.");
        lastSubstate = ConnectionSubstate::DROPPED;
        disconnect();
    }
    else
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Read Handler) [" + Tools::toString(connectionID) + "] > Read error encountered: <" + readError.message() + ">.");
        lastSubstate = ConnectionSubstate::FAILED;
        disconnect();
    }
    
    --pendingHandlers;
}

void NetworkManagement_Connections::Connection::writeHandler(const boost::system::error_code & writeError, std::size_t bytesSent)
{
    if(closeConnection)
    {
        --pendingHandlers;
        return;
    }
    
    if(!writeError)
    {
        sent += (bytesSent - HeaderPacket::BYTE_LENGTH);
        onWriteResultReceivedEvent(true);
        lastSubstate = ConnectionSubstate::WAITING;
        
        {//checks if there are pending operations and starts the next one
            boost::lock_guard<boost::mutex> pendingDataLock(writeDataMutex);
            --pendingWriteOperations;
            if(pendingWriteOperations > 0)
            {
                queueNextWrite(*pendingWritesData.front());
                pendingWritesData.pop();
            }
        }
    }
    else if(writeError == boost::asio::error::eof || writeError == boost::asio::error::connection_reset || writeError == boost::asio::error::connection_aborted)
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Write Handler) [" + Tools::toString(connectionID) + "] > Connection terminated by remote peer (?).");
        lastSubstate = ConnectionSubstate::DROPPED;
        onWriteResultReceivedEvent(false);
        disconnect();
    }
    else
    {
        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "Connection / " + Tools::toString(connectionType) + " (Write Handler) [" + Tools::toString(connectionID) + "] > Write error encountered: <" + writeError.message() + ">.");
        lastSubstate = ConnectionSubstate::FAILED;
        onWriteResultReceivedEvent(false);
        disconnect();
    }
    
    --pendingHandlers;
}
