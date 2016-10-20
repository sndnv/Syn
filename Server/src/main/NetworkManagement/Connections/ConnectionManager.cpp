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

#include "ConnectionManager.h"
#include <boost/date_time/posix_time/posix_time.hpp>

NetworkManagement_Connections::ConnectionManager::ConnectionManager
(ConnectionManagerParameters parameters,  Utilities::FileLoggerPtr debugLogger)
: debugLogger(debugLogger), managerType(parameters.managerType), localPeerType(parameters.localPeerType),
  listeningAddress(parameters.listeningAddress), listeningPort(parameters.listeningPort),
  maxActiveConnections(parameters.maxActiveConnections),
  connectionRequestTimeout(parameters.connectionRequestTimeout),
  defaultReadBufferSize(parameters.defaultReadBufferSize),
  localEndpoint(boost::asio::ip::address::from_string(parameters.listeningAddress), listeningPort), 
  networkService(new boost::asio::io_service()), connectionAcceptor(*networkService, localEndpoint),
  disconnectedConnectionsThread(new boost::thread(&NetworkManagement_Connections::ConnectionManager::disconnectedConnectionsThreadHandler, this)),
  poolWork(new boost::asio::io_service::work(*networkService))
{
    for(unsigned long i = 0; i < parameters.initialThreadPoolSize; i++)
        threadGroup.create_thread(boost::bind(&NetworkManagement_Connections::ConnectionManager::poolThreadHandler, this));
    
    connectionAcceptor.listen();
    acceptNewConnection();
}

NetworkManagement_Connections::ConnectionManager::~ConnectionManager()
{
    logMessage(LogSeverity::Debug, "(~) Destruction initiated.");
    
    stopManager = true;
    connectionAcceptor.close();
    networkService->stop();
    
    boost::lock_guard<boost::timed_mutex> incomingConnectionDataLock(incomingConnectionDataMutex);
    incomingConnections.clear();
    
    boost::lock_guard<boost::timed_mutex> outgoingConnectionDataLock(outgoingConnectionDataMutex);
    outgoingConnections.clear();
    
    poolWork.reset();
    logMessage(LogSeverity::Debug, "(~) Waiting for all threads to terminate.");
    
    threadGroup.join_all();
    
    logMessage(LogSeverity::Debug, "(~) All threads terminated.");
    
    newDataLockCondition.notify_all();
    disconnectedConnectionsThread->join();
    
    onConnectionCreated.disconnect_all_slots();
    onConnectionInitiationFailed.disconnect_all_slots();
}

void NetworkManagement_Connections::ConnectionManager::initiateNewConnection
(IPAddress remoteAddress, IPPort port)
{
    if(stopManager)
        return;
    
    boost::asio::ip::tcp::endpoint remoteEndpoint(boost::asio::ip::address::from_string(remoteAddress), port);
    
    SocketPtr newLocalSocket(new boost::asio::ip::tcp::socket(*networkService));
    
    newLocalSocket->async_connect(remoteEndpoint, 
            boost::bind(&NetworkManagement_Connections::ConnectionManager::createLocalConnection,
                        this, _1, newLocalSocket));
}

void NetworkManagement_Connections::ConnectionManager::acceptNewConnection()
{
    if(stopManager)
        return;
    
    SocketPtr newRemoteSocket(new boost::asio::ip::tcp::socket(*networkService));
    connectionAcceptor.async_accept(*newRemoteSocket,
            boost::bind(&NetworkManagement_Connections::ConnectionManager::createRemoteConnection,
                        this, newRemoteSocket));
}

void NetworkManagement_Connections::ConnectionManager::createLocalConnection
(const boost::system::error_code & error, SocketPtr localSocket)
{
    if(stopManager)
        return;
    
    RawConnectionID connectionID = getNewConnectionID();
    
    if(!error)
    {
        ++initiatedOutgoingConnections;
        Connection::ConnectionParamters connectionParams{managerType,
                                                         localPeerType,
                                                         ConnectionInitiation::LOCAL,
                                                         connectionID,
                                                         localSocket,
                                                         defaultReadBufferSize};
                                                         
        ConnectionRequest requestParams{localPeerType, managerType};
        ConnectionPtr newConnection(new Connection(networkService, connectionParams, requestParams, nullptr, debugLogger));
        
        newConnection->onConnectEventAttach(boost::bind(&NetworkManagement_Connections::ConnectionManager::onConnectHandler,
                                                        this, _1, ConnectionInitiation::LOCAL));
        
        newConnection->canBeDestroyedEventAttach(boost::bind(&NetworkManagement_Connections::ConnectionManager::destroyConnection,
                                                             this, _1, _2));
        
        bool done = false;
        do
        {
            boost::timed_mutex::scoped_lock connectionDataLock(outgoingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));
            
            if(connectionDataLock.owns_lock() && !stopManager)
            {
                outgoingConnections.insert(std::pair<RawConnectionID, ConnectionPtr>(connectionID, newConnection));
                done = true;
            }
            else if(stopManager)
                return;
        } while(!done);
                                                             
        newConnection->enableLifecycleEvents();
    }
    else
    {
        logMessage(LogSeverity::Debug, "(createLocalConnection) Error encountered during connection <"
            + Convert::toString(connectionID) + "> creation: " + error.message());
        
        onConnectionInitiationFailed(error);
    }
}

void NetworkManagement_Connections::ConnectionManager::createRemoteConnection(SocketPtr remoteSocket)
{
    if(stopManager)
        return;
    
    RawConnectionID connectionID = getNewConnectionID();
    ++acceptedIncomingConnections;
    
    Connection::ConnectionParamters connectionParams{managerType,
                                                     localPeerType,
                                                     ConnectionInitiation::REMOTE,
                                                     connectionID,
                                                     remoteSocket,
                                                     defaultReadBufferSize};
    
    ConnectionPtr newConnection(new Connection(networkService, connectionParams, nullptr, debugLogger));
    
    newConnection->onConnectEventAttach(boost::bind(&NetworkManagement_Connections::ConnectionManager::onConnectHandler,
                                                    this, _1, ConnectionInitiation::REMOTE));
    
    newConnection->canBeDestroyedEventAttach(boost::bind(&NetworkManagement_Connections::ConnectionManager::destroyConnection,
                                                         this, _1, _2));
    
    bool done = false;
    do
    {
        boost::timed_mutex::scoped_lock connectionDataLock(incomingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));

        if(connectionDataLock.owns_lock() && !stopManager)
        {
            incomingConnections.insert(std::pair<RawConnectionID, ConnectionPtr>(connectionID, newConnection));
            done = true;
        }
        else if(stopManager)
            return;
    } while(!done);
    
    if(connectionRequestTimeout > 0)
    {//timeout is enabled
        boost::shared_ptr<boost::asio::deadline_timer> newTimer(new boost::asio::deadline_timer(*networkService));
        
        {
            boost::lock_guard<boost::mutex> timerLock(deadlineTimerMutex);
            
            auto connectionTimerData = std::pair<ConnectionPtr, boost::shared_ptr<boost::asio::deadline_timer>>
                    (newConnection, newTimer);
            
            timerData.insert(std::pair<RawConnectionID, std::pair<ConnectionPtr, boost::shared_ptr<boost::asio::deadline_timer>>>
                    (connectionID, connectionTimerData));
        }
        
        newTimer->expires_from_now(boost::posix_time::seconds(connectionRequestTimeout));
        newTimer->async_wait(boost::bind(&NetworkManagement_Connections::ConnectionManager::timeoutConnection,
                                         this, _1, connectionID));
    }
    
    newConnection->enableLifecycleEvents();
    acceptNewConnection();
}

void NetworkManagement_Connections::ConnectionManager::timeoutConnection
(const boost::system::error_code & timeoutError, RawConnectionID connectionID)
{
    if(stopManager)
        return;
    
    boost::lock_guard<boost::mutex> timerLock(deadlineTimerMutex);
    if(timerData.find(connectionID) != timerData.end())
    {//the timer has expired
        auto currentConnectionData = timerData[connectionID];

        if(!timeoutError)
        {
            logMessage(LogSeverity::Debug, "(timeoutConnection) [" + Convert::toString(connectionID)
                + "] > The remote peer failed to send the request data in time.>");
            
            currentConnectionData.first->disconnect(); //terminates the connection
        }
        else
        {
            logMessage(LogSeverity::Debug, "(timeoutConnection) [" + Convert::toString(connectionID)
                + "] > Timeout error encountered: <" + timeoutError.message() + ">");
        }

        timerData.erase(connectionID);
    }
}

void NetworkManagement_Connections::ConnectionManager::destroyConnection
(RawConnectionID connectionID, ConnectionInitiation initiation)
{
    if(stopManager)
        return;
    
    switch(initiation)
    {
        case ConnectionInitiation::LOCAL:
        {
            bool done = false;
            do
            {
                boost::timed_mutex::scoped_lock connectionDataLock(outgoingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));

                if(connectionDataLock.owns_lock() && !stopManager)
                {
                    if(outgoingConnections.find(connectionID) != outgoingConnections.end())
                    {
                        queueConnectionForDestruction(outgoingConnections[connectionID]);
                        outgoingConnections.erase(connectionID);

                        logMessage(LogSeverity::Debug, "(destroyConnection) Outgoing connection <"
                            + Convert::toString(connectionID) + "> removed.");
                    }
                    else
                    {
                        logMessage(LogSeverity::Debug, "(destroyConnection) Outgoing connection <"
                            + Convert::toString(connectionID) + "> not found in table.");
                    }
                    
                    done = true;
                }
                else if(stopManager)
                    return;
            } while(!done);
        } break;
        
        case ConnectionInitiation::REMOTE:
        {
            bool done = false;
            do
            {
                boost::timed_mutex::scoped_lock connectionDataLock(incomingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));

                if(connectionDataLock.owns_lock() && !stopManager)
                {
                    if(incomingConnections.find(connectionID) != incomingConnections.end())
                    {
                        queueConnectionForDestruction(incomingConnections[connectionID]);
                        incomingConnections.erase(connectionID);

                        logMessage(LogSeverity::Debug, "(destroyConnection) Incoming connection <"
                            + Convert::toString(connectionID) + "> removed.");
                    }
                    else
                    {
                        logMessage(LogSeverity::Debug, "(destroyConnection) Incoming connection <"
                            + Convert::toString(connectionID) + "> not found in table.");
                    }
                    
                    done = true;
                }
                else if(stopManager)
                    return;
            } while(!done);
        } break;
        
        default:
        {
            logMessage(LogSeverity::Debug, "(destroyConnection) Invalid connection <"
                + Convert::toString(connectionID) + "> initiation encountered.");
        } break;
    }
}

void NetworkManagement_Connections::ConnectionManager::onConnectHandler
(RawConnectionID connectionID, ConnectionInitiation initiation)
{
    if(stopManager)
        return;
    
    switch(initiation)
    {
        case ConnectionInitiation::LOCAL:
        {
            ConnectionPtr currentConnection;
            
            bool done = false;
            do
            {
                boost::timed_mutex::scoped_lock connectionDataLock(outgoingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));

                if(connectionDataLock.owns_lock() && !stopManager)
                {
                    currentConnection = outgoingConnections.at(connectionID);
                    done = true;
                }
                else if(stopManager)
                    return;
            } while(!done);
            
            onConnectionCreated(currentConnection, ConnectionInitiation::LOCAL);
        } break;
        
        case ConnectionInitiation::REMOTE:
        {
            if(connectionRequestTimeout > 0)
            {
                boost::lock_guard<boost::mutex> timerLock(deadlineTimerMutex);
                if(timerData.find(connectionID) != timerData.end())
                {//there is a pending timer that has not expired yet
                    timerData.erase(connectionID);
                }
                else
                {//the timer has expired
                    logMessage(LogSeverity::Debug, "(onConnectHandler) ["
                        + Convert::toString(connectionID) + "] > Connection expired.");
                    
                    return;
                }
            }
            
            ConnectionPtr currentConnection;
            
            bool done = false;
            do
            {
                boost::timed_mutex::scoped_lock connectionDataLock(incomingConnectionDataMutex, boost::get_system_time() + boost::posix_time::milliseconds(mutexWaitInterval));

                if(connectionDataLock.owns_lock() && !stopManager)
                {
                    currentConnection = incomingConnections.at(connectionID);
                    done = true;
                }
                else if(stopManager)
                    return;
            } while(!done);
            
            onConnectionCreated(currentConnection, ConnectionInitiation::REMOTE);
        } break;
        
        default:
        {
            logMessage(LogSeverity::Debug, "(onConnectHandler) Invalid connection <"
                + Convert::toString(connectionID) + "> initiation encountered.");
        } break;
    }
}

void NetworkManagement_Connections::ConnectionManager::poolThreadHandler()
{
    if(stopManager)
        return;
    
    logMessage(LogSeverity::Debug, "(poolThreadHandler) Thread <"
        + Convert::toString(boost::this_thread::get_id()) + "> started.");

    while(!stopManager)
    {
        try
        {
            networkService->run(); //the current threads starts work on the networking service
        }
        catch(std::exception & ex)
        {
            logMessage(LogSeverity::Debug, "(poolThreadHandler) Exception encountered in thread <"
                + Convert::toString(boost::this_thread::get_id()) + ">: [" + ex.what() + "]");
        }
    }
    
    logMessage(LogSeverity::Debug, "(poolThreadHandler) Thread <"
        + Convert::toString(boost::this_thread::get_id()) + "> stopped.");
}

void NetworkManagement_Connections::ConnectionManager::queueConnectionForDestruction(ConnectionPtr connection)
{
    boost::lock_guard<boost::mutex> queueLock(disconnectedConnectionsMutex);
    
    disconnectedConnections.push_back(connection);

    newDataLockCondition.notify_all();
}

void NetworkManagement_Connections::ConnectionManager::disconnectedConnectionsThreadHandler()
{
    logMessage(LogSeverity::Debug, "(disconnectedConnectionsThreadHandler) > Started.");
    
    while(!stopManager)
    {
        boost::unique_lock<boost::mutex> queueLock(disconnectedConnectionsMutex);
        
        if(disconnectedConnections.size() > 0)
        {
            logMessage(LogSeverity::Debug, "(disconnectedConnectionsThreadHandler) > Working with <"
                + Convert::toString(disconnectedConnections.size()) + "> connections.");
            
            std::vector<ConnectionPtr> remainingConnections;
            
            for(ConnectionPtr currentConnection : disconnectedConnections)
            {
                if(currentConnection->getPendingHandlersNumber() > 0)
                    remainingConnections.push_back(currentConnection);
            }
            
            logMessage(LogSeverity::Debug, "(disconnectedConnectionsThreadHandler) > Waiting for <"
                + Convert::toString(remainingConnections.size()) + "> connections with pending handlers.");
            
            disconnectedConnections.clear();
            disconnectedConnections.swap(remainingConnections);
            
            boost::system_time nextWakeup =
                    boost::get_system_time() + boost::posix_time::seconds(connectionDestructionInterval);
            
            while(!stopManager && timedLockCondition.timed_wait(queueLock, nextWakeup))
            {
                logMessage(LogSeverity::Debug, "(disconnectedConnectionsThreadHandler) > Exited wait without timer expiration.");
            }
        }
        else
        {
            logMessage(LogSeverity::Error, "(disconnectedConnectionsThreadHandler) > No connections found; thread will sleep until more are added.");
            
            newDataLockCondition.wait(queueLock);
        }
    }
    
    logMessage(LogSeverity::Debug, "(disconnectedConnectionsThreadHandler) > Stopped.");
}
