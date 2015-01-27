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

#ifndef NETWORKCOMMANDMANAGER_H
#define	NETWORKCOMMANDMANAGER_H

#include <string>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "../../Utilities/Strings/Common.h"
#include "../../Utilities/Strings/Network.h"
#include "../../Utilities/FileLogger.h"
#include "../Types/Types.h"
#include "../Types/Packets.h"
#include "Connection.h"

using NetworkManagement_Types::PeerType;
using NetworkManagement_Types::SocketPtr;
using NetworkManagement_Types::BufferSize;
using NetworkManagement_Types::ConnectionRequest;
using NetworkManagement_Types::ConnectionInitiation;
using NetworkManagement_Types::OperationTimeoutLength;
using NetworkManagement_Types::RawNetworkSessionID;
using NetworkManagement_Types::INVALID_RAW_NETWORK_SESSION_ID;
using NetworkManagement_Connections::Connection;
using NetworkManagement_Connections::ConnectionPtr;
using Common_Types::IPPort;
using Common_Types::IPAddress;

namespace Convert = Utilities::Strings;

namespace NetworkManagement_Connections
{
    /**
     * Class representing basic TCP connection management.\n\n
     * 
     * * New connections to remote peers are create via <code>initiateNewConnection(IPAddress, IPPort)</code>;\n
     * * New connections from remote peers are created automatically;\n
     * 
     * * <code>onConnectionCreated</code> event is fired when either a local or a remote connection has been
     * successfully created and can be used;\n
     * * <code>onConnectionInitiationFailed</code> event is fired when an attempt to create an outgoing 
     * connection has failed;
     */
    class ConnectionManager
    {
        public:
            /** Parameters structure for holding <code>ConnectionManager</code> configuration data. */
            struct ConnectionManagerParameters
            {
                /** Manager type */
                ConnectionType managerType;
                /** Local peer type */
                PeerType localPeerType;
                /** Manager listening IP address */
                IPAddress listeningAddress;
                /** Manager listening port */
                IPPort listeningPort;
                /** Maximum number of active connections */
                unsigned int maxActiveConnections;
                /** Network IO service thread pool size */
                unsigned int initialThreadPoolSize;
                /** Connection request timeout (in seconds); set to 0 for no timeout. */
                OperationTimeoutLength connectionRequestTimeout;
                /** Default size for connection read buffers (in bytes) */
                BufferSize defaultReadBufferSize;
            };
            
            /**
             * Creates a new <code>ConnectionManager</code> with the specified configuration data.
             * 
             * @param parameters manager configuration data
             * @param debugLogger logger for debugging, if any
             */
            ConnectionManager(ConnectionManagerParameters parameters, Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Stops all work on the networking IO service and terminates all connections.
             */
            ~ConnectionManager();
            
            ConnectionManager() = delete;                                  //No default constructor
            ConnectionManager(const ConnectionManager& orig) = delete;        //Copying not allowed (pass/access only by reference/pointer)
            ConnectionManager& operator=(const ConnectionManager&) = delete;  //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Initiates a new connection to the specified endpoint.
             * 
             * @param remoteAddress the IP address of the endpoint
             * @param port the port of the endpoint
             */
            void initiateNewConnection(IPAddress remoteAddress, IPPort port);
            
            /**
             * Attaches the supplied handler to the <code>onConnectionCreated</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectionCreatedEventAttach
            (std::function<void(ConnectionPtr, ConnectionInitiation)> function)
            {
                return onConnectionCreated.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onConnectionInitiationFailed</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onConnectionInitiationFailedEventAttach
            (std::function<void(const boost::system::error_code &)> function)
            {
                return onConnectionInitiationFailed.connect(function);
            }
            
            /** Retrieves the type of the manager.\n\n@return the manager's type */
            ConnectionType getManagerType()                         const { return managerType; }
            /** Retrieves the local peer type for all connections.\n\n@return the local peer type */
            PeerType getLocalPeerType()                             const { return localPeerType; }
            /** Retrieves the IP address on which the manager listens for new connections.\n\n@return the listening IP address */
            IPAddress getListeningAddress()                         const { return listeningAddress; }
            /** Retrieves the port on which the manager listens for new connections.\n\n@return the listening port */
            IPPort getListeningPort()                               const { return listeningPort; }
            /** Retrieves the maximum number of active connections allowed.\n\n@return the maximum number of active connections */
            unsigned int getMaxActiveConnections()                  const { return maxActiveConnections; }
            /** Retrieves the request timeout for each new remote connection.\n\n@return the timeout (in seconds) */
            OperationTimeoutLength getConnectionRequestTimeout()    const { return connectionRequestTimeout; }
            /** Retrieves the default read buffer size for new connections.\n\n@return the default read buffer size (in bytes) */
            BufferSize getDefaultReadBufferSize()                   const { return defaultReadBufferSize; }
            /** Retrieves the number of currently active incoming connections.\n\n@return the number of incoming connections */
            unsigned long getIncomingConnectionsCount()             const { return incomingConnections.size(); }
            /** Retrieves the number of currently active outgoing connections.\n\n@return the number of outgoing connections */
            unsigned long getOutgoingConnectionsCount()             const { return outgoingConnections.size(); }
            /** Retrieves the internal ID of the last connection.\n\n@return the last connection ID */
            RawNetworkSessionID getLastConnectionID()               const { return newSessionID; }
            /** Retrieves the number of closed connections waiting to be destroyed.\n\n@return the number of connections pending destruction */
            unsigned long getPendingDestroyedConnectionsCount()     const { return disconnectedConnections.size(); }
            /** Retrieves the total number of outgoing connections that have been made.\n\n@return the number of outgoing connections */
            RawNetworkSessionID getTotalOutgoingConnectionsCount()  const { return initiatedOutgoingConnections; }
            /** Retrieves the total number of incoming connections that have been made.\n\n@return the number of incoming connections */
            RawNetworkSessionID getTotalIncomingConnectionsCount()  const { return acceptedIncomingConnections; }
            
        private:
            boost::mutex sessionIDMutex;
            RawNetworkSessionID newSessionID = INVALID_RAW_NETWORK_SESSION_ID;
            Utilities::FileLogger * debugLogger; //debugging logger
            
            //Statistics
            RawNetworkSessionID initiatedOutgoingConnections = 0;   //number of initiated outgoing connections
            RawNetworkSessionID acceptedIncomingConnections = 0;    //number of accepted incoming connections
            
            //Configuration
            ConnectionType managerType;         //manager and child connections type
            PeerType localPeerType;             //local peer type
            IPAddress listeningAddress;         //listening address for the manager
            IPPort listeningPort;               //listening port for the manager
            unsigned int maxActiveConnections;  //TODO - implement //0 = unlimited
            std::atomic<OperationTimeoutLength> connectionRequestTimeout{0}; //0 = unlimited (in seconds)
            BufferSize defaultReadBufferSize;   //default read buffer size for all new connections
            
            unsigned long connectionDestructionInterval = 5; //TODO - update/rename/what
            
            //Connection Management
            boost::asio::ip::tcp::endpoint localEndpoint;       //local listening endpoint
            boost::asio::io_service networkService;             //io_service for handling networking
            boost::asio::ip::tcp::acceptor connectionAcceptor;  //acceptor for incoming connections
            
            //Deadline timers data
            boost::mutex deadlineTimerMutex;
            boost::unordered_map<RawNetworkSessionID, std::pair<ConnectionPtr, boost::asio::deadline_timer *>> timerData;
            
            //Incoming & outgoing connections containers
            boost::mutex incomingConnectionDataMutex;
            boost::unordered_map<RawNetworkSessionID, ConnectionPtr> incomingConnections;
            boost::mutex outgoingConnectionDataMutex;
            boost::unordered_map<RawNetworkSessionID, ConnectionPtr> outgoingConnections;
            
            std::vector<ConnectionPtr> disconnectedConnections; //connections waiting for destruction
            
            //Thread Management
            boost::mutex disconnectedConnectionsMutex;      //mutex for disconnected connections data sync
            boost::condition_variable newDataLockCondition; //condition variable for waiting for more data
            boost::condition_variable timedLockCondition;   //condition variable for performing timed waits
            boost::thread * disconnectedConnectionsThread;  //thread for handling connection destruction
            
            boost::shared_ptr<boost::asio::io_service::work> poolWork; //thread pool work object
            boost::thread_group threadGroup; //thread pool management group
            
            std::atomic<bool> stopManager{false}; //denotes whether the manager is to be stopped or not
            
            //Events
            boost::signals2::signal<void (ConnectionPtr, ConnectionInitiation)> onConnectionCreated;
            boost::signals2::signal<void (const boost::system::error_code &)> onConnectionInitiationFailed;
            
            /**
             * Prepares the manager for accepting a new incoming connection.
             */
            void acceptNewConnection();
            
            /**
             * Creates a new local connection with the specified socket.
             * 
             * @param error error encountered during the connection initiation, if any
             * @param localSocket the socket to be used with the new connection
             */
            void createLocalConnection(const boost::system::error_code & error,  SocketPtr localSocket);
            
            /**
             * Creates a new remote connection with the specified socket.
             * 
             * @param remoteSocket the socket to be used with the new connection
             */
            void createRemoteConnection(SocketPtr remoteSocket);
            
            /**
             * Connection destruction handler.
             * 
             * This handler is attached to the <code>canBeDestroyed</code> event
             * of all <code>Connection</code> objects.
             * 
             * Note: After the handler finishes executing, the connection with the
             * specified ID should no longer be used.
             * 
             * @param connectionID
             * @param initiation
             */
            void destroyConnection(RawNetworkSessionID connectionID, ConnectionInitiation initiation);
            
            /**
             * Connection timeout handler.
             * 
             * If the connection request for incoming connections does not arrive
             * in the required time, the connection is terminated.
             * 
             * @param timeoutError error encountered during the timeout operation, if any
             * @param connectionID the ID of the connection associated with the timeout
             */
            void timeoutConnection(const boost::system::error_code & timeoutError, RawNetworkSessionID connectionID);
            
            /**
             * Handler for <code>onConnect</code> events from new <code>Connection</code> objects.
             * 
             * @param connectionID the ID of the connection
             * @param initiation the connection initiation side
             */
            void onConnectHandler(RawNetworkSessionID connectionID, ConnectionInitiation initiation);
            
            /**
             * IO Service thread handler.
             */
            void poolThreadHandler();
            
            /**
             * Enqueues the supplied connection for destruction.
             * 
             * @param connection the connection to be destroyed
             */
            void queueConnectionForDestruction(ConnectionPtr connection);
            
            /**
             * Thread handler for destroying terminated connections.
             */
            void disconnectedConnectionsThreadHandler();
            
            /**
             * Generates a new internal connection ID.
             * 
             * Note: This should be used as the only way for retrieving new IDs.
             * 
             * @return the new ID
             */
            RawNetworkSessionID getNewSessionID()
            {
                boost::lock_guard<boost::mutex> sessionIDLock(sessionIDMutex);
                return ++newSessionID;
            }
    };
}

#endif	/* NETWORKCOMMANDMANAGER_H */

