/**
 * Copyright (C) 2016 https://github.com/sndnv
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

#include "../../BasicSpec.h"
#include "../../Fixtures.h"
#include <atomic>
#include "../../../main/NetworkManagement/Types/Types.h"
#include "../../../main/NetworkManagement/Connections/ConnectionManager.h"
#include "../../../main/Utilities/FileLogger.h"

using NetworkManagement_Connections::ConnectionManager;

SCENARIO("Connection managers are created and can handle incoming/outgoing connections",
         "[ConnectionManager][Connections][NetworkManagement]")
{
    GIVEN("a source and a target ConnectionManager")
    {
        Utilities::FileLoggerParameters sourceLoggerParams{"test_data/ConnectionManager_source.log", 32*1024*1024, Utilities::FileLogSeverity::Debug};
        Utilities::FileLoggerParameters targetLoggerParams{"test_data/ConnectionManager_target.log", 32*1024*1024, Utilities::FileLogSeverity::Debug};
        Utilities::FileLoggerParameters testPoolLoggerParams{"test_data/ConnectionManager_testPool.log", 32*1024*1024, Utilities::FileLogSeverity::Debug};
        Utilities::FileLoggerPtr testPoolLogger(new Utilities::FileLogger(testPoolLoggerParams));
        Utilities::FileLoggerPtr sourceLogger(new Utilities::FileLogger(sourceLoggerParams));
        Utilities::FileLoggerPtr targetLogger(new Utilities::FileLogger(targetLoggerParams));

        Utilities::ThreadPool testPool(2, testPoolLogger);

        unsigned int connectionsToRequest = 1000;
        unsigned int maxWaitAttempts = 6;
        unsigned int defaultWaitTime = 5;
        ByteData sourceToTaretData = "SOURCE->TARGET";
        ByteData targetToSourceData = "TARGET->SOURCE";
        std::string localAddress = "127.0.0.1";
        unsigned int localPort = 19001;
        std::string remoteAddress = "127.0.0.1";
        unsigned int remotePort = 19002;

        ConnectionManager::ConnectionManagerParameters sourceParameters
        {
            ConnectionType::COMMAND,    //ConnectionType managerType;
            PeerType::SERVER,           //PeerType localPeerType;
            localAddress,               //IPAddress listeningAddress;
            localPort,                  //IPPort listeningPort;
            0,                          //unsigned int maxActiveConnections;
            2,                          //unsigned int initialThreadPoolSize;
            0,                          //OperationTimeoutLength connectionRequestTimeout;
            512                         //BufferSize defaultReadBufferSize;
        };

        ConnectionManager::ConnectionManagerParameters targetParameters
        {
            ConnectionType::COMMAND,    //ConnectionType managerType;
            PeerType::SERVER,           //PeerType localPeerType;
            remoteAddress,              //IPAddress listeningAddress;
            remotePort,                 //IPPort listeningPort;
            0,                          //unsigned int maxActiveConnections;
            2,                          //unsigned int initialThreadPoolSize;
            0,                          //OperationTimeoutLength connectionRequestTimeout;
            512                         //BufferSize defaultReadBufferSize;
        };

        ConnectionManager sourceManager(sourceParameters, sourceLogger);
        ConnectionManager targetManager(targetParameters, targetLogger);

        std::atomic<unsigned int> connectionsInitiated(0);
        std::atomic<unsigned int> connectionsAccepted(0);
        std::atomic<unsigned int> connectionsFailed(0);
        std::atomic<unsigned int> dataSentCount(0);
        std::atomic<unsigned int> dataReceivedCount(0);

        auto sourceSuccessHandler = [&](ConnectionPtr connection, ConnectionInitiation init)
        {
            ++connectionsInitiated;

            auto writeResultHandler = [&dataSentCount](bool result)
            {
                ++dataSentCount;
            };

            auto dataReceivedHandler = [&dataReceivedCount](ByteData data, PacketSize remainingData)
            {
                ++dataReceivedCount;
            };

            connection->onWriteResultReceivedEventAttach(writeResultHandler);
            connection->onDataReceivedEventAttach(dataReceivedHandler);
            connection->enableDataEvents();

            connection->sendData(sourceToTaretData);
        };

        auto sourceFailureHandler = [&](const boost::system::error_code & msg)
        {
            ++connectionsFailed;
        };

        auto targetSuccessHandler = [&](ConnectionPtr connection, ConnectionInitiation init)
        {
            ++connectionsAccepted;

            auto writeResultHandler = [connection, &dataSentCount, &testPool, testPoolLogger](bool result)
            {
                ++dataSentCount;

                auto disconnectTask = [connection, testPoolLogger]()
                {
                    try
                    {
                        waitFor(0.1);
                        connection->disconnect();
                    }
                    catch(std::exception & e)
                    {
                        testPoolLogger->logMessage(FileLogSeverity::Debug, "Exception encountered for connection ["
                            + Utilities::Strings::toString(connection->getID()) + "] :[" + e.what() + "]");
                        throw;
                    }
                };
                testPool.assignTask(disconnectTask);
            };

            auto dataReceivedHandler = [connection, &dataReceivedCount, &targetToSourceData](ByteData data, PacketSize remainingData)
            {
                connection->sendData(targetToSourceData);
                ++dataReceivedCount;
            };

            connection->onWriteResultReceivedEventAttach(writeResultHandler);
            connection->onDataReceivedEventAttach(dataReceivedHandler);
            connection->enableDataEvents();
        };

        auto targetFailureHandler = [&connectionsFailed](const boost::system::error_code & msg)
        {
            ++connectionsFailed;
        };

        sourceManager.onConnectionCreatedEventAttach(sourceSuccessHandler);
        sourceManager.onConnectionInitiationFailedEventAttach(sourceFailureHandler);
        targetManager.onConnectionCreatedEventAttach(targetSuccessHandler);
        targetManager.onConnectionInitiationFailedEventAttach(targetFailureHandler);

        CHECK(sourceManager.getManagerType() == ConnectionType::COMMAND);
        CHECK(sourceManager.getLocalPeerType() == PeerType::SERVER);
        CHECK(sourceManager.getListeningAddress() == localAddress);
        CHECK(sourceManager.getListeningPort() == localPort);
        CHECK(sourceManager.getMaxActiveConnections() == 0);
        CHECK(sourceManager.getConnectionRequestTimeout() == 0);
        CHECK(sourceManager.getDefaultReadBufferSize() == 512);
        CHECK(sourceManager.getIncomingConnectionsCount() == 0);
        CHECK(sourceManager.getOutgoingConnectionsCount() == 0);
        CHECK(sourceManager.getLastConnectionID() == 0);
        CHECK(sourceManager.getPendingDestroyedConnectionsCount() == 0);
        CHECK(sourceManager.getTotalOutgoingConnectionsCount() == 0);
        CHECK(sourceManager.getTotalIncomingConnectionsCount() == 0);

        CHECK(targetManager.getManagerType() == ConnectionType::COMMAND);
        CHECK(targetManager.getLocalPeerType() == PeerType::SERVER);
        CHECK(targetManager.getListeningAddress() == remoteAddress);
        CHECK(targetManager.getListeningPort() == remotePort);
        CHECK(targetManager.getMaxActiveConnections() == 0);
        CHECK(targetManager.getConnectionRequestTimeout() == 0);
        CHECK(targetManager.getDefaultReadBufferSize() == 512);
        CHECK(targetManager.getIncomingConnectionsCount() == 0);
        CHECK(targetManager.getOutgoingConnectionsCount() == 0);
        CHECK(targetManager.getLastConnectionID() == 0);
        CHECK(targetManager.getPendingDestroyedConnectionsCount() == 0);
        CHECK(targetManager.getTotalOutgoingConnectionsCount() == 0);
        CHECK(targetManager.getTotalIncomingConnectionsCount() == 0);

        WHEN("new connections are requested for a valid target")
        {
            for(unsigned int i = 0; i < connectionsToRequest; i++)
            {
                sourceManager.initiateNewConnection(remoteAddress, remotePort);
                if((i % 100) == 0)
                {
                    waitFor(0.5);
                }
            }

            unsigned int currentWaitAttempts = 0;
            while(connectionsInitiated != connectionsToRequest && maxWaitAttempts != currentWaitAttempts)
            {
                ++currentWaitAttempts;
                waitFor(defaultWaitTime);
            }

            THEN("they are initiated successfully, can send/receive data and can be disconnected")
            {
                CHECK(connectionsInitiated == connectionsToRequest);
                CHECK(connectionsAccepted == connectionsToRequest);
                CHECK(connectionsFailed == 0);
                CHECK(dataSentCount == 2*connectionsToRequest);
                CHECK(dataSentCount == dataReceivedCount);

                CHECK(sourceManager.getTotalOutgoingConnectionsCount() == connectionsToRequest);
                CHECK(sourceManager.getTotalIncomingConnectionsCount() == 0);
                CHECK(targetManager.getTotalOutgoingConnectionsCount() == 0);
                CHECK(targetManager.getTotalIncomingConnectionsCount() == connectionsToRequest);
            }
        }

        WHEN("new connections are requested for an invalid target")
        {
            for(unsigned int i = 0; i < connectionsToRequest; i++)
            {
                sourceManager.initiateNewConnection("127.1.2.3", i+50000);
            }
            unsigned int currentWaitAttempts = 0;
            while(connectionsInitiated != connectionsToRequest && maxWaitAttempts != currentWaitAttempts)
            {
                ++currentWaitAttempts;
                waitFor(defaultWaitTime);
            }

            THEN("they fail to be initiated")
            {
                CHECK(connectionsInitiated == 0);
                CHECK(connectionsAccepted == 0);
                CHECK(connectionsFailed == connectionsToRequest);
                CHECK(dataSentCount == 0);
                CHECK(dataSentCount == 0);

                CHECK(sourceManager.getTotalOutgoingConnectionsCount() == 0);
                CHECK(sourceManager.getTotalIncomingConnectionsCount() == 0);
                CHECK(targetManager.getTotalOutgoingConnectionsCount() == 0);
                CHECK(targetManager.getTotalIncomingConnectionsCount() == 0);
            }
        }
    }
}