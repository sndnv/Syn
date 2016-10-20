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

#include "../BasicSpec.h"
#include "../Fixtures.h"
#include "../InstructionManagement/TestInstructionSource.h"
#include "../../main/NetworkManagement/NetworkManager.h"
#include "../../main/SecurityManagement/Crypto/LocalAuthenticationDataStore.h"

namespace Results = InstructionManagement_Sets::NetworkManagerInstructions::Results;

SCENARIO("A network manager is created and can successfully handle connections",
         "[NetworkManager][NetworkManagement]")
{
    GIVEN("a source and a target NetworkManager")
    {
        unsigned int maxWaitAttempts = 6;
        unsigned int defaultWaitTime = 5;
        
        Utilities::FileLoggerParameters securityLoggerParams_source
        {
            "test_data/NetworkManagement_SecurityManager_source.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters securityLoggerParams_target
        {
            "test_data/NetworkManagement_SecurityManager_target.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters sessionLoggerParams_source
        {
            "test_data/NetworkManagement_SessionManager_source.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters sessionLoggerParams_target
        {
            "test_data/NetworkManagement_SessionManager_target.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters networkLoggerParams_source
        {
            "test_data/NetworkManager_source.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters networkLoggerParams_target
        {
            "test_data/NetworkManager_target.log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr
                sourceSecurityLogger(new Utilities::FileLogger(securityLoggerParams_source)),
                targetSecurityLogger(new Utilities::FileLogger(securityLoggerParams_target)),
                sourceSessionLogger(new Utilities::FileLogger(sessionLoggerParams_source)),
                targetSessionLogger(new Utilities::FileLogger(sessionLoggerParams_target)),
                sourceNetworkLogger(new Utilities::FileLogger(networkLoggerParams_source)),
                targetNetworkLogger(new Utilities::FileLogger(networkLoggerParams_target));
        
        std::vector<InstructionSetType> expectedInstructionSets;
        expectedInstructionSets.push_back(InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE);
        
        SyncServer_Core::InstructionDispatcher * sourceDispatcher = Testing::Fixtures::createInstructionDispatcher(expectedInstructionSets);
        SyncServer_Core::InstructionDispatcher * targetDispatcher = Testing::Fixtures::createInstructionDispatcher(expectedInstructionSets);
        SyncServer_Core::DatabaseManager * dbManager = Testing::Fixtures::createDatabaseManager();
        SyncServer_Core::SecurityManager * sourceSecManager = Testing::Fixtures::createSecurityManager(sourceDispatcher, dbManager, sourceSecurityLogger);
        SyncServer_Core::SecurityManager * targetSecManager = Testing::Fixtures::createSecurityManager(sourceDispatcher, dbManager, targetSecurityLogger);
        SyncServer_Core::SessionManager * sourceSessManager = Testing::Fixtures::createSessionManager(dbManager, sourceSecManager, sourceSessionLogger);
        SyncServer_Core::SessionManager * targetSessManager = Testing::Fixtures::createSessionManager(dbManager, targetSecManager, targetSessionLogger);
        
        NetworkManagement_Types::DeviceIPSettings sourceSettings
        {
            "127.0.0.1",    //command address
            9001,           //command port
            "127.0.0.1",    //data address
            8001,           //data port
            "127.0.0.1",    //init address
            7001            //init port
        };
        
        NetworkManagement_Types::DeviceIPSettings targetSettings
        {
            "127.0.0.1",    //command address
            9002,           //command port
            "127.0.0.1",    //data address
            8002,           //data port
            "127.0.0.1",    //init address
            7002            //init port
        };
        
        SecurityManagement_Crypto::LocalAuthenticationDataStore sourceAuthStore, targetAuthStore;
        
        KeyGenerator keyGenerator
        {
            {PasswordDerivationFunction::PBKDF2_SHA512, 1000, 32, 16, 16},
            {SymmetricCipherType::AES, AuthenticatedSymmetricCipherModeType::EAX, 12, 32, 32},
            {1024, 2048, EllipticCurveType::P256R1, AsymmetricKeyValidationLevel::FULL_3}
        };
        
        auto sourceManagerData = Testing::Fixtures::createNetworkManager(
                sourceDispatcher,
                dbManager,
                sourceSecManager,
                sourceSessManager,
                &sourceAuthStore,
                sourceSettings,
                keyGenerator,
                sourceNetworkLogger);
        
        auto targetManagerData = Testing::Fixtures::createNetworkManager(
                targetDispatcher,
                dbManager,
                targetSecManager,
                targetSessManager,
                &targetAuthStore,
                targetSettings,
                keyGenerator,
                targetNetworkLogger);
        
        SyncServer_Core::NetworkManager * sourceManager = sourceManagerData.manager;
        SyncServer_Core::NetworkManager * targetManager = targetManagerData.manager;
        
        std::string targetUser_rawPass = "passw0rd", sourceDevice_rawPass = "PassW0rd1", targetDevice_rawPass = "PassW0rd2";
        
        PasswordData targetUserPass(sourceSecManager->hashUserPassword(targetUser_rawPass));
        UserDataContainerPtr targetUser(new UserDataContainer("TEST_USER_1", targetUserPass, UserAccessLevel::ADMIN, false));
        targetUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        
        PasswordData sourceDevicePass(sourceSecManager->hashDevicePassword(sourceDevice_rawPass));
        DeviceDataContainerPtr sourceDevice(new DeviceDataContainer("SOURCE_DEVICE_1", sourceDevicePass, targetUser->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        sourceDevice->setDeviceInitAddress(sourceSettings.initAddress);
        sourceDevice->setDeviceInitPort(sourceSettings.initPort);
        
        PasswordData targetDevicePass(sourceSecManager->hashDevicePassword(targetDevice_rawPass));
        DeviceDataContainerPtr targetDevice(new DeviceDataContainer("TARGET_DEVICE_1", targetDevicePass, targetUser->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        targetDevice->setDeviceInitAddress(targetSettings.initAddress);
        targetDevice->setDeviceInitPort(targetSettings.initPort);
    
        dbManager->Users().addUser(targetUser);
        dbManager->Devices().addDevice(sourceDevice);
        dbManager->Devices().addDevice(targetDevice);
        
        std::vector<InstructionManagement_Types::InstructionSetType> sourceRequiredSets(
        {
            InstructionManagement_Types::InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE
        });
        
        Testing::TestInstructionSource testSourceForTarget(true, sourceRequiredSets);
        Testing::TestInstructionSource testSourceForSource(true, sourceRequiredSets);
        targetDispatcher->registerInstructionSource(testSourceForTarget);
        sourceDispatcher->registerInstructionSource(testSourceForSource);
        
        CHECK(sourceManager->getCommandsReceived() == 0);
        CHECK(sourceManager->getCommandsSent() == 0);
        CHECK(sourceManager->getConnectionsInitiated() == 0);
        CHECK(sourceManager->getConnectionsReceived() == 0);
        CHECK(sourceManager->getDataReceived() == 0);
        CHECK(sourceManager->getDataSent() == 0);
        CHECK(sourceManager->getSetupsCompleted() == 0);
        CHECK(sourceManager->getSetupsFailed() == 0);
        CHECK(sourceManager->getSetupsPartiallyCompleted() == 0);
        CHECK(sourceManager->getSetupsStarted() == 0);
        CHECK(sourceManager->getInstructionsReceived() == 0);
        CHECK(sourceManager->getInstructionsProcessed() == 0);
        
        CHECK(targetManager->getCommandsReceived() == 0);
        CHECK(targetManager->getCommandsSent() == 0);
        CHECK(targetManager->getConnectionsInitiated() == 0);
        CHECK(targetManager->getConnectionsReceived() == 0);
        CHECK(targetManager->getDataReceived() == 0);
        CHECK(targetManager->getDataSent() == 0);
        CHECK(targetManager->getSetupsCompleted() == 0);
        CHECK(targetManager->getSetupsFailed() == 0);
        CHECK(targetManager->getSetupsPartiallyCompleted() == 0);
        CHECK(targetManager->getSetupsStarted() == 0);
        CHECK(targetManager->getInstructionsReceived() == 0);
        CHECK(targetManager->getInstructionsProcessed() == 0);
        
        WHEN("new connection managers are started")
        {
            ConnectionManager::ConnectionManagerParameters testParams_1
            {
                ConnectionType::COMMAND,    //ConnectionType managerType;
                PeerType::SERVER,           //PeerType localPeerType;
                "127.0.0.1",                //IPAddress listeningAddress;
                19001,                      //IPPort listeningPort;
                5,                          //unsigned int maxActiveConnections;
                6,                          //unsigned int initialThreadPoolSize;
                7,                          //OperationTimeoutLength connectionRequestTimeout;
                8                           //BufferSize defaultReadBufferSize;
            };

            ConnectionManager::ConnectionManagerParameters testParams_2
            {
                ConnectionType::DATA,       //ConnectionType managerType;
                PeerType::SERVER,           //PeerType localPeerType;
                "127.0.0.1",                //IPAddress listeningAddress;
                18001,                      //IPPort listeningPort;
                1,                          //unsigned int maxActiveConnections;
                1,                          //unsigned int initialThreadPoolSize;
                1,                          //OperationTimeoutLength connectionRequestTimeout;
                1                           //BufferSize defaultReadBufferSize;
            };

            ConnectionManager::ConnectionManagerParameters testParams_3
            {
                ConnectionType::INIT,       //ConnectionType managerType;
                PeerType::SERVER,           //PeerType localPeerType;
                "127.0.0.1",                //IPAddress listeningAddress;
                17001,                      //IPPort listeningPort;
                1,                          //unsigned int maxActiveConnections;
                2,                          //unsigned int initialThreadPoolSize;
                3,                          //OperationTimeoutLength connectionRequestTimeout;
                4                           //BufferSize defaultReadBufferSize;
            };
            
            auto managerID_1 = targetManager->startConnectionManager(testParams_1);
            auto managerID_2 = targetManager->startConnectionManager(testParams_2);
            auto managerID_3 = targetManager->startConnectionManager(testParams_3);

            THEN("they can be successfully stopped")
            {
                CHECK_NOTHROW(targetManager->stopCommandConnectionManager(managerID_1));
                CHECK_NOTHROW(targetManager->stopDataConnectionManager(managerID_2));
                CHECK_NOTHROW(targetManager->stopInitConnectionManager(managerID_3));
            }
            
            delete targetManager;
            delete sourceManager;
            delete sourceSessManager;
            delete targetSessManager;
            delete sourceSecManager;
            delete targetSecManager;
            delete dbManager;
            delete sourceDispatcher;
            delete targetDispatcher;
        }
        
        AND_WHEN("an initialization is requested for a device")
        {
            std::string sharedSecret = "shared_test_passw0rd";
            auto sharedTransientID = sourceManager->getNewTransientID();
            
            boost::shared_ptr<InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenInitConnection> setupInstructionToTarget(
                new InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenInitConnection(
                    sharedSecret,
                    PeerType::SERVER,
                    sourceDevice->getDeviceID(),
                    sharedTransientID));

            boost::shared_ptr<InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenInitConnection> setupInstructionToSource(
                new InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenInitConnection(
                    sourceManagerData.initManagerID,
                    targetDevice->getDeviceInitAddress(),
                    targetDevice->getDeviceInitPort(),
                    sharedSecret,
                    PeerType::SERVER,
                    targetDevice->getDeviceID(),
                    sharedTransientID));

            sourceSessManager->openSession("TEST_USER_1", targetUser_rawPass, SessionType::COMMAND, true);
            targetSessManager->openSession("TEST_USER_1", targetUser_rawPass, SessionType::COMMAND, true);

            AuthorizationRequest requestForTarget(targetUser->getUserID(), *targetManager, SecurableComponentType::NETWORK_MANAGER, setupInstructionToTarget);
            AuthorizationTokenPromisePtr authorizationFromTarget = targetSecManager->postRequest(requestForTarget);
            testSourceForTarget.runInstruction(setupInstructionToTarget, authorizationFromTarget);
            auto setupInstructionToTargetResult = boost::dynamic_pointer_cast<Results::LifeCycleOpenInitConnection>(setupInstructionToTarget->getFuture().get());

            AuthorizationRequest requestForSource(targetUser->getUserID(), *sourceManager, SecurableComponentType::NETWORK_MANAGER, setupInstructionToSource);
            AuthorizationTokenPromisePtr authorizationFromSource = sourceSecManager->postRequest(requestForSource);
            testSourceForSource.runInstruction(setupInstructionToSource, authorizationFromSource);
            auto setupInstructionToSourceResult = boost::dynamic_pointer_cast<Results::LifeCycleOpenInitConnection>(setupInstructionToSource->getFuture().get());
            
            THEN("it is successfully completed and commands/data can be sent/received")
            {
                CHECK(setupInstructionToTargetResult->result);
                CHECK(setupInstructionToSourceResult->result);
                
                unsigned int currentWaitAttempts = 0;
                while((sourceManager->getSetupsCompleted() != 1 || targetManager->getSetupsCompleted() != 1)
                        && maxWaitAttempts != currentWaitAttempts)
                {
                    ++currentWaitAttempts;
                    waitFor(defaultWaitTime);
                }
                
                CHECK(sourceManager->getSetupsStarted() == 1);
                CHECK(sourceManager->getSetupsCompleted() == 1);
                CHECK(sourceManager->getInstructionsReceived() == 1);
                CHECK(sourceManager->getInstructionsProcessed() == 1);
                CHECK(sourceManager->getSetupsFailed() == 0);
                CHECK(sourceManager->getSetupsPartiallyCompleted() == 0);
                
                CHECK(targetManager->getSetupsStarted() == 1);
                CHECK(targetManager->getSetupsCompleted() == 1);
                CHECK(targetManager->getInstructionsReceived() == 1);
                CHECK(targetManager->getInstructionsProcessed() == 1);
                CHECK(targetManager->getSetupsFailed() == 0);
                CHECK(targetManager->getSetupsPartiallyCompleted() == 0);
                
                //TODO - re-enable
//                auto cryptoData = keyGenerator.getSymmetricCryptoData();
//                auto newSharedTransientID = sourceManager->getNewTransientID();
//    
//                boost::shared_ptr<InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenDataConnection> instructionToTarget(
//                    new InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenDataConnection(
//                        INVALID_CONNECTION_MANAGER_ID,
//                        newSharedTransientID,
//                        sourceDevice->getDeviceID(),
//                        cryptoData->getKey(),
//                        cryptoData->getIV(),
//                        keyGenerator.getDefaultSymmetricCipher(),
//                        keyGenerator.getDefaultSymmetricCipherMode(),
//                        true,   //encrypt?
//                        true)); //compress?
//
//                boost::shared_ptr<InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenDataConnection> instructionToSource(
//                    new InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenDataConnection(
//                        sourceManagerData.dataManagerID,
//                        newSharedTransientID,
//                        targetDevice->getDeviceID(),
//                        cryptoData->getKey(),
//                        cryptoData->getIV(),
//                        keyGenerator.getDefaultSymmetricCipher(),
//                        keyGenerator.getDefaultSymmetricCipherMode(),
//                        true,   //encrypt?
//                        true)); //compress?
//
//                sourceManager->sendInstruction(sourceManagerData.cmdManagerID, targetDevice->getDeviceID(), instructionToTarget);
//                auto instructionToTargetResult = boost::dynamic_pointer_cast<Results::LifeCycleOpenDataConnection>(instructionToTarget->getFuture().get());
//                
//                CHECK(instructionToTargetResult->result);
//                
//                currentWaitAttempts = 0;
//                while(targetManager->getInstructionsProcessed() != 2
//                        && maxWaitAttempts != currentWaitAttempts)
//                {
//                    ++currentWaitAttempts;
//                    waitFor(defaultWaitTime);
//                }
//                
//                CHECK(targetManager->getInstructionsReceived() == 2);
//                CHECK(targetManager->getInstructionsProcessed() == 2);
//                
//                AuthorizationRequest newRequestForSource(
//                    targetUser->getUserID(),
//                    targetDevice->getDeviceID(),
//                    *sourceManager,
//                    SecurableComponentType::NETWORK_MANAGER,
//                    instructionToSource);
//                AuthorizationTokenPromisePtr newAuthorizationFromSource = sourceSecManager->postRequest(newRequestForSource);
//                testSourceForSource.runInstruction(instructionToSource, newAuthorizationFromSource);
//                auto instructionToSourceResult = boost::dynamic_pointer_cast<Results::LifeCycleOpenDataConnection>(instructionToSource->getFuture().get());
//                
//                CHECK(instructionToSourceResult->result);
//                
//                currentWaitAttempts = 0;
//                while(sourceManager->getInstructionsProcessed() != 2
//                        && maxWaitAttempts != currentWaitAttempts)
//                {
//                    ++currentWaitAttempts;
//                    waitFor(defaultWaitTime);
//                }
//                
//                CHECK(sourceManager->getInstructionsReceived() == 2);
//                CHECK(sourceManager->getInstructionsProcessed() == 2);
//                
//                std::string dataForTarget = "some TEST data 123456 !@#$%^&*()_!";
//                CHECK_THROWS_AS(sourceManager->sendData(targetDevice->getDeviceID(), 0, dataForTarget), std::logic_error);
//                
//                currentWaitAttempts = 0;
//                while((sourceManager->getDataSent() != 1 || targetManager->getDataReceived() != 1)
//                        && maxWaitAttempts != currentWaitAttempts)
//                {
//                    ++currentWaitAttempts;
//                    waitFor(defaultWaitTime);
//                }
//                
//                CHECK(sourceManager->getCommandsReceived() == 0);
//                CHECK(sourceManager->getCommandsSent() == 1);
//                CHECK(sourceManager->getConnectionsInitiated() == 3);
//                CHECK(sourceManager->getConnectionsReceived() == 0);
//                CHECK(sourceManager->getDataReceived() == 0);
//                CHECK(sourceManager->getDataSent() == 1);
//                CHECK(sourceManager->getSetupsCompleted() == 1);
//                CHECK(sourceManager->getSetupsFailed() == 0);
//                CHECK(sourceManager->getSetupsPartiallyCompleted() == 0);
//                CHECK(sourceManager->getSetupsStarted() == 1);
//                CHECK(sourceManager->getInstructionsReceived() == 2);
//                CHECK(sourceManager->getInstructionsProcessed() == 2);
//
//                CHECK(targetManager->getCommandsReceived() == 1);
//                CHECK(targetManager->getCommandsSent() == 0);
//                CHECK(targetManager->getConnectionsInitiated() == 0);
//                CHECK(targetManager->getConnectionsReceived() == 3);
//                CHECK(targetManager->getDataReceived() == 1);
//                CHECK(targetManager->getDataSent() == 0);
//                CHECK(targetManager->getSetupsCompleted() == 1);
//                CHECK(targetManager->getSetupsFailed() == 0);
//                CHECK(targetManager->getSetupsPartiallyCompleted() == 0);
//                CHECK(targetManager->getSetupsStarted() == 1);
//                CHECK(targetManager->getInstructionsReceived() == 2);
//                CHECK(targetManager->getInstructionsProcessed() == 2);
            }
            
            delete targetManager;
            delete sourceManager;
            delete sourceSessManager;
            delete targetSessManager;
            delete sourceSecManager;
            delete targetSecManager;
            delete dbManager;
            delete sourceDispatcher;
            delete targetDispatcher;
        }
    }
}
