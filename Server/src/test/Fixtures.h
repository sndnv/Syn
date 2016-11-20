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

#ifndef FIXTURES_H
#define	FIXTURES_H

#include <vector>
#include <boost/uuid/uuid_generators.hpp>
#include "../main/Utilities/FileLogger.h"
#include "../main/DatabaseManagement/Interfaces/DatabaseAbstractionLayer.h"
#include "../main/DatabaseManagement/DALCache.h"
#include "../main/DatabaseManagement/DALQueue.h"
#include "../main/DatabaseManagement/DatabaseManager.h"
#include "../main/DatabaseManagement/DALs/DebugDAL.h"
#include "../main/EntityManagement/DatabaseLogger.h"
#include "../main/EntityManagement/DeviceManager.h"
#include "../main/EntityManagement/UserManager.h"
#include "../main/InstructionManagement/InstructionDispatcher.h"
#include "../main/SecurityManagement/SecurityManager.h"
#include "../main/SessionManagement/SessionManager.h"
#include "../main/NetworkManagement/NetworkManager.h"
#include "InstructionManagement/TestInstructionSource.h"
#include "InstructionManagement/TestInstructionTarget.h"

using DatabaseManagement_Types::DatabaseManagerOperationMode;
using DatabaseManagement_Types::DatabaseFailureAction;

namespace Testing
{
    class Fixtures
    {
        public:
            static SyncServer_Core::DatabaseManager * createDatabaseManager()
            {
                Utilities::FileLoggerParameters loggerParams
                {
                    "./DatabaseManager.log",  //logFilePath
                    32*1024*1024,           //maximumFileSize
                    Utilities::FileLogSeverity::Debug
                };

                SyncServer_Core::DatabaseManagement::DALQueue::DALQueueParameters dqParams
                {
                    DatabaseManagerOperationMode::PRPW,     //dbMode
                    DatabaseFailureAction::IGNORE_FAILURE,  //failureAction
                    5,                                      //maximumReadFailures
                    5                                       //maximumWriteFailures
                };

                SyncServer_Core::DatabaseManagement::DALCache::DALCacheParameters dcParams
                (
                    10,     //maximumCommitTime
                    5,      //maximumCommitUpdates
                    0,      //minimumCommitUpdates
                    true,   //alwaysEvictObjects
                    false,  //alwaysClearObjectAge
                    10      //maximumCacheSize
                );

                SyncServer_Core::DatabaseManager * manager =
                        new SyncServer_Core::DatabaseManager(loggerParams, dqParams, dcParams, 5);

                DatabaseManagement_Interfaces::DALPtr statisticsTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_stat.log",
                        "./data_stat",
                        DatabaseObjectType::STATISTICS));

                DatabaseManagement_Interfaces::DALPtr systemTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_sys.log",
                        "./data_sys",
                        DatabaseObjectType::SYSTEM_SETTINGS));

                DatabaseManagement_Interfaces::DALPtr syncFilesTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_sync.log",
                        "./data_sync",
                        DatabaseObjectType::SYNC_FILE));

                DatabaseManagement_Interfaces::DALPtr devicesTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_dev.log",
                        "./data_dev",
                        DatabaseObjectType::DEVICE));

                DatabaseManagement_Interfaces::DALPtr schedulesTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_sched.log",
                        "./data_sched",
                        DatabaseObjectType::SCHEDULE));

                DatabaseManagement_Interfaces::DALPtr usersTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_user.log",
                        "./data_user",
                        DatabaseObjectType::USER));

                DatabaseManagement_Interfaces::DALPtr logsTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_logs.log",
                        "./data_logs",
                        DatabaseObjectType::LOG));

                DatabaseManagement_Interfaces::DALPtr sessionsTableDAL(
                    new DatabaseManagement_DALs::DebugDAL(
                        "./log_sess.log",
                        "./data_sess",
                        DatabaseObjectType::SESSION));

                statisticsTableDAL->clearDatabase();
                systemTableDAL->clearDatabase();
                syncFilesTableDAL->clearDatabase();
                devicesTableDAL->clearDatabase();
                schedulesTableDAL->clearDatabase();
                usersTableDAL->clearDatabase();
                logsTableDAL->clearDatabase();
                sessionsTableDAL->clearDatabase();
                
                bool enableCache = false;
                manager->addDAL(statisticsTableDAL, enableCache);
                manager->addDAL(systemTableDAL, enableCache);
                manager->addDAL(syncFilesTableDAL, enableCache);
                manager->addDAL(devicesTableDAL, enableCache);
                manager->addDAL(schedulesTableDAL, enableCache);
                manager->addDAL(usersTableDAL, enableCache);
                manager->addDAL(logsTableDAL, enableCache);
                manager->addDAL(sessionsTableDAL, enableCache);

                return manager;
            }

            static SyncServer_Core::InstructionDispatcher * createInstructionDispatcher(
                std::vector<InstructionManagement_Types::InstructionSetType> expectedSets,
                Utilities::FileLoggerPtr logger = Utilities::FileLoggerPtr())
            {
                expectedSets.push_back(InstructionManagement_Types::InstructionSetType::TEST);

                SyncServer_Core::InstructionDispatcher * dispatcher =
                        new SyncServer_Core::InstructionDispatcher(
                            SyncServer_Core::InstructionDispatcher::InstructionDispatcherParameters{expectedSets}, logger);


                TestInstructionSource testSource(true);
                TestInstructionTarget testTarget;
                dispatcher->registerInstructionSource(testSource);
                dispatcher->registerInstructionTarget<InstructionManagement_Types::TestInstructionType>(testTarget);

                return dispatcher;
            }
            
            static SyncServer_Core::InstructionDispatcher * createInstructionDispatcher(Utilities::FileLoggerPtr logger = Utilities::FileLoggerPtr())
            {
                return createInstructionDispatcher(std::vector<InstructionManagement_Types::InstructionSetType>(), logger);
            }

            static SyncServer_Core::SecurityManager *
            createSecurityManager(SyncServer_Core::InstructionDispatcher * dispatcher, SyncServer_Core::DatabaseManager * dbManager, Utilities::FileLoggerPtr logger)
            {
                SyncServer_Core::SecurityManager::PasswordHashingParameters hashingParams
                (
                    32,                             //userPasswordSaltSize
                    48,                             //devicePasswordSaltSize
                    HashAlgorithmType::SHA3_512,    //userPasswordHashAlgorithm
                    HashAlgorithmType::SHA_512     //devicePasswordHashAlgorithm
                );        

                KeyGenerator::DerivedKeysParameters derivedKeyParams
                {
                    PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
                    10000,  //derivedKeyIterations
                    32,     //derivedKeySize
                    16,     //derivedKeyMinSaltSize
                    16,     //derivedKeyDefaultSaltSize

                };

                KeyGenerator::SymmetricKeysParameters symmetricKeyParams
                {
                    SymmetricCipherType::AES,                   //defaultSymmetricCipher
                    AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
                    12, //defaultIVSize
                    32, //minSymmetricKeySize
                    32  //defaultSymmetricKeySize
                };

                KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams
                {
                    1024,                                   //minRSAKeySize
                    2048,                                   //defaultRSAKeySize
                    EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
                    AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
                };

                SyncServer_Core::SecurityManager::KeyGeneratorParameters keyGenParams
                {
                    derivedKeyParams,   //derivedKeyParams
                    symmetricKeyParams, //symKeyParams
                    asymmetricKeyParams //asymKeyParams
                };

                SyncServer_Core::SecurityManager::FailedAuthenticationDelayParameters failedAuthParams
                {
                    3,  //delayBase
                    SecurityManagement_Types::DelayEscalationType::CONSTANT, //escalationType
                    3,  //maxAttempts
                    2   //ignoredAttempts
                };

                SyncServer_Core::SecurityManager::SecurityManagerParameters managerParams
                {
                    4,                                                  //threadPoolSize
                    *dbManager,                                         //databaseManager
                    *dispatcher,                                        //instructionDispatcher
                    5,                                                  //maxUserDataEntries
                    3,                                                  //maxDeviceDataEntries
                    SecurityManagement_Types::CacheEvictionType::LRU,   //userEviction
                    SecurityManagement_Types::CacheEvictionType::MRU,   //deviceEviction
                    16,                                                 //authorizationTokenSignatureSize
                    24,                                                 //authenticationTokenSignatureSize
                    5,                                                  //authenticationTokenValidityDuration
                    std::vector<const NameRule *>(),                    //userNameRules
                    std::vector<const NameRule *>(),                    //deviceNameRules
                    std::vector<const PasswordRule *>(),                //userPasswordRules
                    std::vector<const PasswordRule *>(),                //devicePasswordRules
                    hashingParams,                                      //currentPasswordHashingConfiguration
                    SyncServer_Core::SecurityManager::PasswordHashingParameters(), //previousPasswordHashingConfiguration
                    keyGenParams,                                       //keyGeneratorConfig
                    failedAuthParams,                                   //userDelayConfig
                    failedAuthParams                                    //deviceDelayConfig
                };

                SyncServer_Core::SecurityManager * manager = new SecurityManager(managerParams, logger);

                SecurityManagement_Rules::MinPasswordLength * rule_1 = new SecurityManagement_Rules::MinPasswordLength(3);
                SecurityManagement_Rules::MinPasswordLength * rule_2 = new SecurityManagement_Rules::MinPasswordLength(3);
                SecurityManagement_Rules::MinNameLength * rule_3 = new SecurityManagement_Rules::MinNameLength(3);
                manager->addUserPasswordRule(rule_1);
                manager->addDevicePasswordRule(rule_2);
                manager->addDeviceNameRule(rule_3);

                return manager;
            }

            static SyncServer_Core::SessionManager *
            createSessionManager(
                SyncServer_Core::DatabaseManager * dbManager,
                SyncServer_Core::SecurityManager * secManager,
                Utilities::FileLoggerPtr logger)
            {
                SyncServer_Core::SessionManager::SessionManagerParameters params
                {
                    2,              //threadPoolSize
                    *dbManager,     //databaseManager
                    *secManager,    //securityManager
                    3,              //maxSessionsPerUser
                    3,              //maxSessionsPerDevice
                    SessionManagement_Types::SessionDataCommitType::ON_UPDATE,  //dataCommit
                    2,              //inactiveSessionExpirationTime
                    2               //unauthenticatedSessionExpirationTime
                };
                
                SyncServer_Core::SessionManager * manager = new SyncServer_Core::SessionManager(params, logger);
                
                secManager->registerSecurableComponent(*manager);
                
                return manager;
            }
            
            struct NetworkManagerData
            {
                ConnectionManagerID initManagerID = INVALID_CONNECTION_MANAGER_ID;
                ConnectionManagerID cmdManagerID = INVALID_CONNECTION_MANAGER_ID;
                ConnectionManagerID dataManagerID = INVALID_CONNECTION_MANAGER_ID;
                SyncServer_Core::NetworkManager * manager = nullptr;
            };
            
            static NetworkManagerData
            createNetworkManager(
                SyncServer_Core::InstructionDispatcher * dispatcher,
                SyncServer_Core::DatabaseManager * dbManager,
                SyncServer_Core::SecurityManager * secManager,
                SyncServer_Core::SessionManager * sessManager,
                SecurityManagement_Crypto::LocalAuthenticationDataStore * authStore,
                NetworkManagement_Types::DeviceIPSettings ipSettings,
                KeyGenerator & keyGenerator,
                Utilities::FileLoggerPtr logger)
            {
                DeviceID masterID = boost::uuids::random_generator()();
                
                ECDHCryptoDataContainerPtr ecCrypto = keyGenerator.getECDHCryptoData();
                std::string ecPublicKey;
                ecCrypto->getPublicKeyForStorage(ecPublicKey);
                
                NetworkManagement_Handlers::InitialConnectionsHandler::InitialConnectionsHandlerParameters initConnectionsParams
                {
                    *secManager,
                    16,
                    KeyExchangeType::EC_DH,
                    24,
                    10,
                    masterID,
                    ecPublicKey,
                    ipSettings
                };

                NetworkManagement_Handlers::CommandConnectionsHandler::CommandConnectionsHandlerParameters cmdConnectionsParams
                {
                    *secManager,
                    *sessManager,
                    masterID,
                    AsymmetricCryptoHandlerPtr(),
                    ecCrypto,
                    16,
                    KeyExchangeType::EC_DH
                };

                NetworkManagement_Handlers::DataConnectionsHandler::DataConnectionsHandlerParameters dataConnectionsParams
                {
                    masterID,
                    16,
                    256,
                    1
                };

                SyncServer_Core::NetworkManager::NetworkManagerParameters managerParams
                {
                    2,        //unsigned int networkThreadPoolSize;
                    2,        //unsigned int instructionsThreadPoolSize;

                    *dbManager,     //DatabaseManager & databaseManager;
                    *secManager,    //SecurityManager & securityManager;
                    *sessManager,   //SessionManager & sessionManager;
                    *authStore,     //LocalAuthenticationDataStore & authenticationStore;

                    initConnectionsParams,  //InitialConnectionsHandler::InitialConnectionsHandlerParameters initConnectionsParams;
                    cmdConnectionsParams,   //CommandConnectionsHandler::CommandConnectionsHandlerParameters commandConnectionsParams;
                    dataConnectionsParams,  //DataConnectionsHandler::DataConnectionsHandlerParameters dataConnectionsParams;

                    10,        //Seconds commandConnectionSetupTimeout;
                    15,        //Seconds dataConnectionSetupTimeout;
                    20,        //Seconds initConnectionSetupTimeout;
                    30,        //Seconds commandConnectionInactivityTimeout;
                    40,        //Seconds dataConnectionInactivityTimeout;
                    45,        //Seconds pendingConnectionDataDiscardTimeout;
                    60,        //Seconds expectedDataConnectionTimeout;
                    60         //Seconds expectedInitConnectionTimeout;
                };
                
                SyncServer_Core::NetworkManager * manager = new SyncServer_Core::NetworkManager(managerParams, logger);
                
                ConnectionManager::ConnectionManagerParameters commandManager
                {
                    ConnectionType::COMMAND,    //ConnectionType managerType;
                    PeerType::SERVER,           //PeerType localPeerType;
                    ipSettings.commandAddress,  //IPAddress listeningAddress;
                    ipSettings.commandPort,     //IPPort listeningPort;
                    0,                          //unsigned int maxActiveConnections;
                    2,                          //unsigned int initialThreadPoolSize;
                    0,                          //OperationTimeoutLength connectionRequestTimeout;
                    512                         //BufferSize defaultReadBufferSize;
                };

                ConnectionManager::ConnectionManagerParameters dataManager
                {
                    ConnectionType::DATA,       //ConnectionType managerType;
                    PeerType::SERVER,           //PeerType localPeerType;
                    ipSettings.dataAddress,     //IPAddress listeningAddress;
                    ipSettings.dataPort,        //IPPort listeningPort;
                    0,                          //unsigned int maxActiveConnections;
                    2,                          //unsigned int initialThreadPoolSize;
                    0,                          //OperationTimeoutLength connectionRequestTimeout;
                    512                         //BufferSize defaultReadBufferSize;
                };

                ConnectionManager::ConnectionManagerParameters initManager
                {
                    ConnectionType::INIT,       //ConnectionType managerType;
                    PeerType::SERVER,           //PeerType localPeerType;
                    ipSettings.initAddress,     //IPAddress listeningAddress;
                    ipSettings.initPort,        //IPPort listeningPort;
                    0,                          //unsigned int maxActiveConnections;
                    2,                          //unsigned int initialThreadPoolSize;
                    0,                          //OperationTimeoutLength connectionRequestTimeout;
                    512                         //BufferSize defaultReadBufferSize;
                };
                
                auto cmdManagerID = manager->startConnectionManager(commandManager);
                auto dataManagerID = manager->startConnectionManager(dataManager);
                auto initManagerID = manager->startConnectionManager(initManager);
                
                secManager->registerSecurableComponent(*manager);
                dispatcher->registerInstructionTarget<NetworkManagerConnectionLifeCycleInstructionType>(*manager);
                dispatcher->registerInstructionSource(*manager);
                
                NetworkManagerData result;
                result.cmdManagerID = cmdManagerID;
                result.dataManagerID = dataManagerID;
                result.initManagerID = initManagerID;
                result.manager = manager;
                
                return result;
            }

        private:
            Fixtures() {}
    };
}

#endif	/* FIXTURES_H */
