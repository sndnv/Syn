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

#ifndef CONNECTION_H
#define	CONNECTION_H

#include <string>

#include <queue>
#include <atomic>
#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include "../Types/Types.h"
#include "../Types/Packets.h"

#include "../../Utilities/Strings/Common.h"
#include "../../Utilities/Strings/Network.h"
#include "../../Utilities/FileLogger.h"
#include "../../Common/Types.h"

using Common_Types::Byte;
using Common_Types::ByteVector;

using NetworkManagement_Types::PeerType;
using NetworkManagement_Types::SocketPtr;
using NetworkManagement_Types::PacketSize;
using NetworkManagement_Types::BufferSize;
using NetworkManagement_Types::HeaderPacket;
using NetworkManagement_Types::ConnectionType;
using NetworkManagement_Types::ConnectionState;
using NetworkManagement_Types::ConnectionRequest;
using NetworkManagement_Types::ConnectionEventID;
using NetworkManagement_Types::ConnectionSubstate;
using NetworkManagement_Types::RawNetworkSessionID;
using NetworkManagement_Types::ConnectionInitiation;
using NetworkManagement_Types::OperationTimeoutLength;
using Common_Types::TransferredDataAmount;

namespace Convert = Utilities::Strings;

namespace NetworkManagement_Connections
{
    //TODO -> get rid of mutexes
    /**
     * Class representing a TCP connection between two endpoints.\n\n
     * 
     * Note: A connection object should always be created by a <code>ConnectionManager</code>.
     */
    class Connection 
    {
        public:
            /** Parameters structure for holding <code>Connection</code> configuration data. */
            struct ConnectionParamters
            {
                /** The type of connection to be expected from the remote peer. */
                ConnectionType expectedConnection;
                
                /** The type of the local peer. */
                PeerType localPeerType;
                
                /** The connection initiation side. */
                ConnectionInitiation initiation;
                
                /** The internal connection ID. */
                RawNetworkSessionID connectionID;
                
                /** Pointer to a valid socket object. */
                SocketPtr socket;
                
                /** The size of the buffer for incoming data. */
                BufferSize readBufferSize;
            };
            
            /**
             * Creates a new incoming connection object with the specified configuration.
             * 
             * Note: To be used for incoming connections only.
             * 
             * @param connectionParams connection configuration data
             * @param externalReadBuffer externally supplied read buffer, if any
             * @param debugLogger logger for debugging, if any
             */
            Connection(ConnectionParamters connectionParams,
                       boost::asio::streambuf * externalReadBuffer = nullptr,
                       Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Creates a new outgoing connection object with the specified configuration.
             * 
             * Note: To be used for outgoing connections only.
             * 
             * @param connectionParams connection configuration data
             * @param requestParams request parameters to be sent to the remote peer
             * @param externalReadBuffer externally supplied read buffer, if any
             * @param debugLogger logger for debugging, if any
             */
            Connection(ConnectionParamters connectionParams,
                       ConnectionRequest requestParams,
                       boost::asio::streambuf * externalReadBuffer = nullptr,
                       Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Terminates the connection (if still active), removes all signal handlers and performs general cleanup.
             * 
             * Warning: Any pending events are dropped.
             */
            virtual ~Connection();
            
            Connection() = delete;                                  //No default constructor
            Connection(const Connection& orig) = delete;            //Copying not allowed (pass/access only by reference/pointer)
            Connection& operator=(const Connection& orig) = delete; //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Severs the communication done via the connection.
             * 
             * After a call to this method, the connection can no longer be
             * used for sending/receiving data and becomes eligible for destruction.
             */
            void disconnect();
            
            /**
             * Sends the supplied data to the associated remote peer.
             * 
             * Warning: The caller is responsible for retaining the data until
             * the write operation is complete.
             * 
             * Note: If a write operation is currently running, the new request
             * will be enqueued and at a later time; otherwise, the write operation
             * will be started immediately.
             * 
             * @param data the data to be sent
             */
            void sendData(const ByteVector & data);
            
            /**
             * Sends the supplied data to the associated remote peer.
             * 
             * Note: Not implemented.
             * 
             * @param data the data to be sent
             */
            void sendData(const boost::asio::streambuf & data); //TODO
            
            //Event Management
            /**
             * Retrieves the state of the connection life cycle events handling.
             * 
             * @return <code>true</code>, if life cycle events are blocked
             */
            bool areLifecycleEventsBlocked()            const { return lifecycleEventsBlocked; }
            
            /**
             * Retrieves the state of the data events handling.
             * 
             * @return <code>true</code>, if data events are blocked
             */
            bool areDataEventsBlocked()                 const { return dataEventsBlocked; }
            
            /**
             * Enables all life cycle events.
             * 
             * Note: All currently pending events will be fired within this call.
             */
            void enableLifecycleEvents();
            
            /**
             * Disables all life cycle events.
             */
            void disableLifecycleEvents();
            
            /**
             * Enables all data events.
             * 
             * Note: All currently pending events will be fired within this call.
             */
            void enableDataEvents();
            
            /**
             * Disables all data events.
             */
            void disableDataEvents();
            
            //Connection Info
            /** Retrieves the internal ID associated with this connection.\n\n@return the connection ID */
            RawNetworkSessionID getID()                 const { return connectionID; }
            /** Retrieves the local peer type.\n\n@return the local peer type */
            PeerType getLocalPeerType()                 const { return localPeerType; }
            /** Retrieves the connection type.\n\n@return the connection type */
            ConnectionType getConnectionType()          const { return connectionType; }
            /** Retrieves the amount of data sent via the connection (in bytes).\n\n@return bytes sent */
            TransferredDataAmount getBytesSent()        const { return sent; }
            /** Retrieves the amount of data received via the connection (in bytes).\n\n@return bytes received */
            TransferredDataAmount getBytesReceived()    const { return received; }
            /** Retrieves the current connection state.\n\n@return the connection state */
            ConnectionState getState()                  const { return state; }
            /** Retrieves the current connection substate.\n\n@return the connection substate */
            ConnectionSubstate getLastSubstate()        const { return lastSubstate; }
            /** Retrieves the connection initiation.\n\n@return the connection initiation */
            ConnectionInitiation getInitiation()        const { return initiation; }
            /** Retrieves the state of the connection.\n\n@return <code>true</code>, if the connection is established */
            bool isActive()                             const { return (state == ConnectionState::ESTABLISHED); }
            /** Retrieves the number of currently pending handlers.\n\n@return the number of handlers */
            unsigned int getPendingHandlersNumber()     const { return pendingHandlers; }
            
            //Signals
            /**
             * Attaches the supplied handler to the <code>onConnect</code> event.
             * 
             * Note: This is a life cycle event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectEventAttach
            (std::function<void(RawNetworkSessionID)> function)
            {
                return onConnect.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onDisconnect</code> event.
             * 
             * Note: This is a life cycle event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onDisconnectEventAttach
            (std::function<void(RawNetworkSessionID)> function)
            {
                return onDisconnect.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onDataReceived</code> event.
             * 
             * Note: This is a data event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onDataReceivedEventAttach
            (std::function<void(ByteVector, PacketSize)> function)
            {
                return onDataReceived.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onWriteResultReceived</code> event.
             * 
             * Note: This is a data event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onWriteResultReceivedEventAttach
            (std::function<void(bool)> function)
            {
                return onWriteResultReceived.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>canBeDestroyed</code> event.
             * 
             * Note: This is a life cycle event.
             * 
             * Note: This is used by the parent <code>ConnectionManager</code> to handle
             * the life cycle of the connection and after it gets notified of this
             * event, the connection should be considered unusable/destroyed.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection canBeDestroyedEventAttach
            (std::function<void(RawNetworkSessionID, ConnectionInitiation)> function)
            {
                return canBeDestroyed.connect(function);
            }
            
        private:
            //Data - Reading
            bool isExternalReadBufferUsed;              //denotes whether an external read buffer is to be used
            boost::asio::streambuf * readBuffer;        //read buffer pointer (external or internal)
            bool isHeaderExpected;                      //denotes whether the next read operation should expect a header or data
            PacketSize remainingBytes = 0;              //the number of bytes remaining to be read for the current read operation
            TransferredDataAmount received = 0;         //received data (in bytes); Note: header transmission is not included
            
            //Data - Writing
            boost::mutex writeDataMutex;                //pending operations data mutex
            unsigned int pendingWriteOperations = 0;    //number of currently pending write operations
            std::queue<const ByteVector *>  pendingWritesData; //the data for all currently pending write operations
            TransferredDataAmount sent = 0;             //send data (in bytes); Note: header transmission is not included
            
            //Utils
            Utilities::FileLogger * debugLogger;        //debugging logger
            boost::asio::io_service::strand writeStrand;//synchronisation strand for write operations
            boost::asio::io_service::strand readStrand; //synchronisation strand for read operations
            
            //Connection
            SocketPtr socket;                           //socket pointer
            RawNetworkSessionID connectionID;           //internal connection ID
            PeerType localPeerType;                     //local peer type
            ConnectionRequest connectionRequest;        //connection request data
            ConnectionType connectionType;              //connection type
            ConnectionState state;                      //connection tate
            ConnectionSubstate lastSubstate;            //connection substate
            ConnectionInitiation initiation;            //connection initiation
            
            std::atomic<unsigned int> pendingHandlers{1};//number of pending handlers
            std::atomic<bool> closeConnection{false};   //denotes whether the connection has been closed
            
            /**
             * Read handler for the initial connection request data.
             * 
             * @param readError read error, if any
             * @param unused
             */
            void initialReadRequestHandler(const boost::system::error_code & readError, std::size_t);
            
            /**
             * Write handler for the initial connection request data.
             * 
             * @param writeError write error, if any
             * @param unused
             */
            void initialWriteRequestHandler(const boost::system::error_code & writeError, std::size_t);
            
            /**
             * Starts the next read operation, waiting for the specified number of bytes to be received.
             * 
             * @param readSize the amount of data to wait for
             */
            void queueNextRead(BufferSize readSize);
            
            /**
             * Starts the next write operation, with the specified data to send.
             * 
             * @param data the data to be sent
             */
            void queueNextWrite(const ByteVector & data);
            
            /**
             * Read handler for all incoming data.
             * 
             * @param readError read error, if any
             * @param bytesRead amount of data read (in bytes)
             */
            void readHandler(const boost::system::error_code & readError, std::size_t bytesRead);
            
            /**
             * Write handler for all outgoing data.
             * 
             * @param writeError write error, if any
             * @param bytesSent the amount of data sent (in bytes)
             */
            void writeHandler(const boost::system::error_code & writeError, std::size_t bytesSent);
            
            //Events
            enum class EventType { INVALID, CONNECT, DISCONNECT, DATA_RECEIVED, WRITE_RESULT_RECEIVED, CAN_BE_DESTROYED };
            std::atomic<bool> lifecycleEventsBlocked{true}; //denotes whether life cycle events are blocked
            std::atomic<bool> dataEventsBlocked{true};      //denotes whether data events are blocked
            boost::mutex eventsMutex;                       //events mutex
            ConnectionEventID queuedEventID = 0;            //last event ID
            std::queue<ConnectionEventID> events;           //pending events queue
            boost::unordered_map<ConnectionEventID, boost::tuples::tuple<EventType, boost::any, boost::any> *> eventsData;
            
            boost::signals2::signal<void (RawNetworkSessionID)> onConnect;      //life cycle event
            boost::signals2::signal<void (RawNetworkSessionID)> onDisconnect;   //life cycle event
            boost::signals2::signal<void (ByteVector, PacketSize)> onDataReceived; //data event
            boost::signals2::signal<void (bool)> onWriteResultReceived;                   //data event
            //NOTE: this is used internally and the object can be assumed invalid, after a single handler finishes executing
            boost::signals2::signal<void (RawNetworkSessionID, ConnectionInitiation)> canBeDestroyed; //life cycle event
            
            //<editor-fold defaultstate="collapsed" desc="Event Functions">
            /**
             * Internal <code>onConnect</code> event dispatcher function.\n
             * 
             * If events are enabled, the <code>onConnect</code> event is fired directly;
             * otherwise, it is enqueued, for later processing.
             */
            void onConnectEvent()
            {
                {
                    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
                    if(lifecycleEventsBlocked)
                    {
                        queueEvent(EventType::CONNECT);
                        return;
                    }
                }
                
                onConnect(connectionID);
            }
            
            /**
             * Internal <code>onDisconnect</code> event dispatcher function.\n
             * 
             * If events are enabled, the <code>onDisconnect</code> event is fired directly;
             * otherwise, it is enqueued, for later processing.
             */
            void onDisconnectEvent()
            {
                {
                    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
                    if(lifecycleEventsBlocked)
                    {
                        queueEvent(EventType::DISCONNECT);
                        return;
                    }
                }
                
                onDisconnect(connectionID);
            }
            
            /**
             * Internal <code>onDataReceived</code> event dispatcher function.\n
             * 
             * If events are enabled, the <code>onDataReceived</code> event is fired directly;
             * otherwise, it is enqueued, for later processing.
             * 
             * @param data the bytes returned by the last read operation
             * @param remainingData the number of bytes remaining to be read
             */
            void onDataReceivedEvent(ByteVector data, PacketSize remainingData)
            {
                {
                    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
                    if(dataEventsBlocked)
                    {
                        queueEvent(EventType::DATA_RECEIVED, data, remainingData);
                        return;
                    }
                }
                
                onDataReceived(data, remainingData);
            }
            
            /**
             * Internal <code>onWriteResultReceived</code> event dispatcher function.\n
             * 
             * If events are enabled, the <code>onWriteResultReceived</code> event is fired directly;
             * otherwise, it is enqueued, for later processing.
             * 
             * @param writeResult the result of the last write operation
             */
            void onWriteResultReceivedEvent(bool writeResult)
            {
                {
                    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
                    if(dataEventsBlocked)
                    {
                        queueEvent(EventType::WRITE_RESULT_RECEIVED, writeResult);
                        return;
                    }
                }
                
                onWriteResultReceived(writeResult);
            }
            
            /**
             * Internal <code>canBeDestroyed</code> event dispatcher function.\n
             * 
             * If events are enabled, the <code>canBeDestroyed</code> event is fired directly;
             * otherwise, it is enqueued, for later processing.
             */
            void canBeDestroyedEvent()
            {
                {
                    boost::lock_guard<boost::mutex> eventsLock(eventsMutex);
                    if(lifecycleEventsBlocked)
                    {
                        queueEvent(EventType::CAN_BE_DESTROYED);
                        return;
                    }
                }
                
                canBeDestroyed(connectionID, initiation);
            }
            
            /**
             * Enqueues the supplied event data, for later processing.
             * 
             * @param event the type of the event
             * @param parameter first event parameter, if any
             * @param additionalParameter second event parameter, if any
             */
            void queueEvent(EventType event, boost::any parameter = 0, boost::any additionalParameter = 0)
            {
                events.push(queuedEventID);
                boost::tuples::tuple<EventType, boost::any, boost::any> * currentEventData = 
                        new boost::tuples::tuple<EventType, boost::any, boost::any>(event, parameter, additionalParameter);
                eventsData.insert(std::pair<ConnectionEventID, boost::tuples::tuple<EventType, boost::any, boost::any> *>(queuedEventID, currentEventData));
                queuedEventID++;
            }
            //</editor-fold>
    };
    
    typedef boost::shared_ptr<NetworkManagement_Connections::Connection> ConnectionPtr;
}
#endif	/* CONNECTION_H */

