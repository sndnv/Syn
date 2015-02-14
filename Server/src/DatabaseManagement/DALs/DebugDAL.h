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

#ifndef DEBUGDAL_H
#define	DEBUGDAL_H

#include <deque>
#include <string>
#include <atomic>
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/tokenizer.hpp>
#include <queue>
#include "../../Utilities/Tools.h"
#include "../../Utilities/Strings/Common.h"
#include "../../Utilities/Strings/Database.h"
#include "../../Utilities/FileLogger.h"
#include "../Interfaces/DatabaseAbstractionLayer.h"
#include "../Types/Types.h"
#include "../Interfaces/DatabaseInformationContainer.h"
#include "../Interfaces/DatabaseSettingsContainer.h"
#include "../Containers/DataContainer.h"
#include "../Containers/DeviceDataContainer.h"
#include "../Containers/StatisticDataContainer.h"
#include "../Containers/SystemDataContainer.h"
#include "../Containers/UserDataContainer.h"
#include "../Containers/LogDataContainer.h"
#include "../Containers/DeviceDataContainer.h"
#include "../Containers/SessionDataContainer.h"
#include "../Containers/ScheduleDataContainer.h"
#include "../Containers/SyncDataContainer.h"
#include "../Containers/VectorDataContainer.h"
#include "../../SecurityManagement/Rules/AuthorizationRules.h"
#include "../../InstructionManagement/Types/Types.h"
#include "boost/uuid/random_generator.hpp"

namespace Convert = Utilities::Strings;

using DatabaseManagement_Interfaces::DatabaseInformationContainer;
using DatabaseManagement_Interfaces::DatabaseSettingsContainer;

using Common_Types::TransferredDataAmount;
using Common_Types::TransferredFilesAmount;
using Common_Types::IPAddress;
using Common_Types::IPPort;
using Common_Types::DataPoolSize;
using Common_Types::DataPoolPath;
using Common_Types::DataPoolRetention;
using Common_Types::DBObjectID;
using Common_Types::LogID;
using Common_Types::SessionID;
using Common_Types::SyncID;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::ScheduleID;
using DatabaseManagement_Types::DatabaseIDType;
using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;
using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::ObjectCacheAge;

using DatabaseManagement_Containers::DataContainerPtr;
using DatabaseManagement_Containers::StatisticDataContainerPtr;
using DatabaseManagement_Containers::SystemDataContainerPtr;
using DatabaseManagement_Containers::UserDataContainerPtr;
using DatabaseManagement_Containers::LogDataContainerPtr;
using DatabaseManagement_Containers::DeviceDataContainerPtr;
using DatabaseManagement_Containers::SessionDataContainerPtr;
using DatabaseManagement_Containers::ScheduleDataContainerPtr;
using DatabaseManagement_Containers::SyncDataContainerPtr;
using DatabaseManagement_Containers::VectorDataContainerPtr;

using DatabaseManagement_Containers::StatisticDataContainer;
using DatabaseManagement_Containers::SystemDataContainer;
using DatabaseManagement_Containers::UserDataContainer;
using DatabaseManagement_Containers::LogDataContainer;
using DatabaseManagement_Containers::DeviceDataContainer;
using DatabaseManagement_Containers::SessionDataContainer;
using DatabaseManagement_Containers::ScheduleDataContainer;
using DatabaseManagement_Containers::SyncDataContainer;
using DatabaseManagement_Containers::VectorDataContainer;

using SecurityManagement_Rules::UserAuthorizationRule;
using SecurityManagement_Types::PasswordData;

using boost::tuples::tuple;
using boost::unordered_map;

using std::getline;

using Utilities::FileLogger;
using Utilities::FileLogSeverity;


namespace DatabaseManagement_DALs
{
    class DebugDAL : public DatabaseManagement_Interfaces::DatabaseAbstractionLayer
    {
        public:
            enum class RequestType { INSERT, UPDATE, REMOVE, SELECT };
            
            class DebugDALSettingsContainer;
            class DebugDALInformationContainer;
            class Stringifier;
            
            DebugDAL(std::string logPath, std::string dataPath, DatabaseObjectType dbType);
            ~DebugDAL();
            
            DebugDAL() = delete;                            //No default constructor
            DebugDAL(const DebugDAL&) = delete;             //Copy not allowed (pass/access only by reference/pointer)
            DebugDAL& operator=(const DebugDAL&) = delete;  //Copy not allowed (pass/access only by reference/pointer)

            bool getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue);
            bool putObject(DatabaseRequestID requestID, const DataContainerPtr inputData);
            bool updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData);
            bool removeObject(DatabaseRequestID requestID, DBObjectID id);
            
            bool changeDatabaseSettings(const DatabaseSettingsContainer settings);
            bool buildDatabase();
            bool rebuildDatabase();
            bool clearDatabase();
            bool connect();
            bool disconnect();
            const DatabaseInformationContainer* getDatabaseInfo() const;
            DatabaseObjectType getType() const;
            void setID(DatabaseAbstractionLayerID id);
            DatabaseAbstractionLayerID getID() const;
            
        private:
            //Configuration
            mutable FileLogger logger;
            std::string dataFilePath;
            DatabaseObjectType dalType;
            DatabaseAbstractionLayerID dalID = DatabaseManagement_Types::INVALID_DAL_ID;
            DebugDALInformationContainer * info;
            
            //File management
            unsigned long nextIntID;
            unordered_map<DBObjectID, std::string> data;
            
            //Requests management
            std::queue<DatabaseRequestID> pendingRequests;
            unordered_map<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*> requestsData;
            
            //Thread management
            std::atomic<bool> isConnected {false};
            std::atomic<bool> mainThreadRunnig {false};
            std::atomic<bool> stopDebugger {false};
            boost::thread * mainThreadObject;
            boost::mutex mainThreadMutex;
            boost::condition_variable mainThreadLockCondition;
            
            void addRequest(DatabaseRequestID requestID, RequestType type, boost::any requestParameter, boost::any additionalParameter)
            {
                if(!isConnected)
                {
                    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Failed to add request; DAL is not connected to DB.");
                    return;
                }
                
                tuple<RequestType, boost::any, boost::any> * data = new tuple<RequestType, boost::any, boost::any>(type, requestParameter, additionalParameter);
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Entering critical section.");
                boost::unique_lock<boost::mutex> requestsLock(mainThreadMutex);
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Critical section entered.");

                pendingRequests.push(requestID);
                requestsData.insert(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*>(requestID, data));

                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Sending notification to requests thread.");
                mainThreadLockCondition.notify_all();
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Notification to requests thread sent.");

                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL (Add Request) > Exiting critical section.");
            }
            
            void loadDataFile();
            void saveDataFile();
            
            void mainThread();
    };
    
    class DebugDAL::DebugDALSettingsContainer : public DatabaseSettingsContainer
    {
        public:
            std::string toString() { return "NO SETTINGS DEFINED"; }
    };
    
    class DebugDAL::DebugDALInformationContainer : public DatabaseInformationContainer
    {
        public:
            ~DebugDALInformationContainer() {}

            std::string toString() { return "NO INFORMATION DEFINED"; }

            virtual std::string getDatabaseName() { return "DEBUG FILE DB"; }
            virtual long getDatabaseSize() { return 0; }
    };
    
    //<editor-fold defaultstate="collapsed" desc="Stringifier">
    class DebugDAL::Stringifier
    {
        public:
            //<editor-fold defaultstate="collapsed" desc="To-Container Methods">
            static DeviceDataContainerPtr toDevice(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                UserID ownerID = boost::lexical_cast<UserID>(*currentToken);
                currentToken++;
                IPAddress address = *currentToken;
                currentToken++;
                IPPort port = boost::lexical_cast<IPPort>(*currentToken);
                currentToken++;
                DataTransferType xferType = Convert::toDataTransferType(*currentToken);
                currentToken++;
                std::string providedID = *currentToken;
                currentToken++;
                std::string deviceName = *currentToken;
                currentToken++;
                PasswordData password = Convert::toSecByteBlock(*currentToken);
                currentToken++;
                std::string deviceInfo = *currentToken;
                currentToken++;
                bool locked = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                boost::posix_time::ptime timestampLastSuccessfulAuth = Convert::toTimestamp(*currentToken);
                currentToken++;
                boost::posix_time::ptime timestampLastFailedAuth = Convert::toTimestamp(*currentToken);
                currentToken++;
                unsigned int failedAttempts = boost::lexical_cast<unsigned int>(*currentToken);

                return DeviceDataContainerPtr(
                        new DeviceDataContainer(id, providedID, deviceName, password, ownerID, 
                                                address, port, xferType, deviceInfo, locked,
                                                timestampLastSuccessfulAuth, timestampLastFailedAuth,
                                                failedAttempts));
            }

            static LogDataContainerPtr toLog(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                LogSeverity sev = Convert::toLogSeverity(*currentToken);
                currentToken++;
                std::string source = *currentToken;
                currentToken++;
                boost::posix_time::ptime timestamp = Convert::toTimestamp(*currentToken);
                currentToken++;
                std::string message = *currentToken;

                return LogDataContainerPtr(new LogDataContainer(id, sev, source, timestamp, message));
            }

            static ScheduleDataContainerPtr toSchedule(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                bool isActive = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                boost::posix_time::ptime nextRun = Convert::toTimestamp(*currentToken);
                currentToken++;
                int repetitions = boost::lexical_cast<int>(*currentToken);
                currentToken++;
                ScheduleIntervalType type = Convert::toScheduleIntervalType(*currentToken);
                currentToken++;
                unsigned long length = boost::lexical_cast<unsigned long>(*currentToken);
                currentToken++;
                bool runIfMissed = ((*currentToken).compare("TRUE") == 0);
                bool deleteAfterCompl = ((*currentToken).compare("TRUE") == 0);

                return ScheduleDataContainerPtr(new ScheduleDataContainer(isActive, nextRun, repetitions, type, length, runIfMissed, deleteAfterCompl, id));
            }

            static SessionDataContainerPtr toSession(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                boost::posix_time::ptime openT = Convert::toTimestamp(*currentToken);
                currentToken++;
                boost::posix_time::ptime closeT = Convert::toTimestamp(*currentToken);
                currentToken++;
                boost::posix_time::ptime lastActT = Convert::toTimestamp(*currentToken);
                currentToken++;
                SessionType type = Convert::toSessionType(*currentToken);
                currentToken++;
                DeviceID device = boost::lexical_cast<DeviceID>(*currentToken);
                currentToken++;
                UserID user = boost::lexical_cast<UserID>(*currentToken);
                currentToken++;
                bool isPersistent = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                bool isActive = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                TransferredDataAmount dataSent = boost::lexical_cast<TransferredDataAmount>(*currentToken);
                currentToken++;
                TransferredDataAmount dataRec = boost::lexical_cast<TransferredDataAmount>(*currentToken);
                currentToken++;
                unsigned long cmdsSent = boost::lexical_cast<unsigned long>(*currentToken);
                currentToken++;
                unsigned long cmdsRec = boost::lexical_cast<unsigned long>(*currentToken);

                return SessionDataContainerPtr(new SessionDataContainer(id, openT, closeT, lastActT, type, device, user, isPersistent, isActive, dataSent, dataRec, cmdsSent, cmdsRec));
            }

            static StatisticDataContainerPtr toStat(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                StatisticType type = Convert::toStatisticType(*currentToken);
                currentToken++;
                std::string stringValue = *currentToken;

                boost::any actualValue;

                switch(type)
                {
                    case StatisticType::INSTALL_TIMESTAMP:          actualValue = Convert::toTimestamp(stringValue); break;
                    case StatisticType::START_TIMESTAMP:            actualValue = Convert::toTimestamp(stringValue); break;
                    case StatisticType::TOTAL_FAILED_TRANSFERS:     actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case StatisticType::TOTAL_RETRIED_TRANSFERS:    actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case StatisticType::TOTAL_TRANSFERRED_DATA:     actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case StatisticType::TOTAL_TRANSFERRED_FILES:    actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    default: actualValue = "UNDEFINED"; break;
                }

                return StatisticDataContainerPtr(new StatisticDataContainer(type, actualValue));
            }

            static SyncDataContainerPtr toSync(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                std::string syncName = *currentToken;
                currentToken++;
                std::string syncDescription = *currentToken;
                currentToken++;
                std::string sourcePath = *currentToken;
                currentToken++;
                std::string destinationPath = *currentToken;
                currentToken++;
                DeviceID sourceDev = boost::lexical_cast<DeviceID>(*currentToken);
                currentToken++;
                DeviceID destinationDev = boost::lexical_cast<DeviceID>(*currentToken);
                currentToken++;
                bool oneWay = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                bool oneTime = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                ConflictResolutionRule_Directory conflDir = Convert::toDirConflictResolutionRule(*currentToken);
                currentToken++;
                ConflictResolutionRule_File conflFile = Convert::toFileConflictResolutionRule(*currentToken);
                currentToken++;
                bool encrypt = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                bool compress = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                UserID user = boost::lexical_cast<UserID>(*currentToken);
                currentToken++;
                std::string destPerm = *currentToken;
                currentToken++;
                bool offline = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                bool diff = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                unsigned int retries = boost::lexical_cast<unsigned int>(*currentToken);
                currentToken++;
                SyncFailureAction failAct = Convert::toSyncFailureAction(*currentToken);
                currentToken++;
                boost::posix_time::ptime lastAttempt = Convert::toTimestamp(*currentToken);
                currentToken++;
                SyncResult result = Convert::toSyncResult(*currentToken);
                currentToken++;
                SessionID sessID = boost::lexical_cast<SessionID>(*currentToken);

                return SyncDataContainerPtr(new SyncDataContainer(syncName, syncDescription, sourcePath, destinationPath, sourceDev, destinationDev, oneWay, oneTime,
                                                                  conflDir, conflFile, encrypt, compress, user, destPerm, offline, diff, retries, failAct, lastAttempt, result, sessID, id));
            }

            static SystemDataContainerPtr toSystem(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                //SystemDataContainer(SystemParameterType type, boost::any value)
                SystemParameterType type = Convert::toSystemParameterType(*currentToken);
                currentToken++;
                std::string stringValue = *currentToken;

                boost::any actualValue;

                switch(type)
                {
                    //TODO - any_cast to proper types (DataPoolRetention instead of unsigned long)
                    case SystemParameterType::COMMAND_IP_ADDRESS:       actualValue = stringValue; break;
                    case SystemParameterType::COMMAND_IP_PORT:          actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::COMMAND_RETRIES_MAX:      actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::DATA_IP_ADDRESS:          actualValue = stringValue; break;
                    case SystemParameterType::DATA_IP_PORT:             actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::DATA_RETRIES_MAX:         actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::DB_CACHE_FLUSH_INTERVAL:  actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::DB_IMMEDIATE_FLUSH:       actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::DB_MAX_READ_RETRIES:      actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::DB_MAX_WRITE_RETRIES:     actualValue = boost::lexical_cast<unsigned int>(stringValue); break;
                    case SystemParameterType::DB_OPERATION_MODE:        actualValue = Convert::toDatabaseManagerOperationMode(stringValue); break;
                    case SystemParameterType::FORCE_COMMAND_ENCRYPTION: actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::FORCE_DATA_COMPRESSION:   actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::FORCE_DATA_ENCRYPTION:    actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::IN_MEMORY_POOL_RETENTION: actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::IN_MEMORY_POOL_SIZE:      actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::MINIMIZE_MEMORY_USAGE:    actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::PENDING_DATA_POOL_PATH:   actualValue = stringValue; break;
                    case SystemParameterType::PENDING_DATA_POOL_SIZE:   actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::PENDING_DATA_RETENTION:   actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::SESSION_KEEP_ALIVE:       actualValue = ((stringValue).compare("TRUE") == 0); break;
                    case SystemParameterType::SESSION_TIMEOUT:          actualValue = boost::lexical_cast<unsigned long>(stringValue); break;
                    case SystemParameterType::SUPPORTED_PROTOCOLS:      actualValue = stringValue; break;
                    default: actualValue = "UNDEFINED"; break;
                }

                return SystemDataContainerPtr(new SystemDataContainer(type, actualValue));
            }

            static UserDataContainerPtr toUser(std::string value, DBObjectID id)
            {
                boost::char_separator<char> separator(",");
                boost::tokenizer<boost::char_separator<char>> tokens(value, separator);

                auto currentToken = tokens.begin();
                currentToken++; //skips object ID
                currentToken++; //skips object type

                std::string username = *currentToken;
                currentToken++;
                PasswordData password = Convert::toSecByteBlock(*currentToken);
                currentToken++;
                UserAccessLevel level = Convert::toUserAccessLevel(*currentToken);
                currentToken++;
                bool pwReset = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                bool locked = ((*currentToken).compare("TRUE") == 0);
                currentToken++;
                boost::posix_time::ptime create = Convert::toTimestamp(*currentToken);
                currentToken++;
                boost::posix_time::ptime login = Convert::toTimestamp(*currentToken);
                currentToken++;
                boost::posix_time::ptime timestampLastFailedAuth = Convert::toTimestamp(*currentToken);
                currentToken++;
                unsigned int failedAttempts = boost::lexical_cast<unsigned int>(*currentToken);
                
                std::deque<UserAuthorizationRule> rules;
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DATABASE_MANAGER));
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::SESSION_MANAGER));
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::USER_MANAGER_ADMIN));
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::USER_MANAGER_SELF));
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_ADMIN));
                rules.push_back(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_USER));

                return UserDataContainerPtr(new UserDataContainer(id, username, password, level, pwReset, locked, create, login, timestampLastFailedAuth, failedAttempts, rules));
            }

            static DataContainerPtr toContainer(std::string value, DatabaseObjectType type, DBObjectID id)
            {
                DataContainerPtr result;

                switch(type)
                {
                    case DatabaseObjectType::DEVICE: result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::DataContainer>(toDevice(value, id)); break;
                    case DatabaseObjectType::LOG: result = toLog(value, id); break;
                    case DatabaseObjectType::SCHEDULE: result = toSchedule(value, id); break;
                    case DatabaseObjectType::SESSION: result = toSession(value, id); break;
                    case DatabaseObjectType::STATISTICS: result = toStat(value, id); break;
                    case DatabaseObjectType::SYNC_FILE: result = toSync(value, id); break;
                    case DatabaseObjectType::SYSTEM_SETTINGS: result = toSystem(value, id); break;
                    case DatabaseObjectType::USER: result = toUser(value, id); break;
                    default: ; break; //ignore
                }

                return result;
            }
            //</editor-fold>

            //<editor-fold defaultstate="collapsed" desc="To-String Methods">
            static std::string toString(DataContainerPtr container)
            {
                std::string childContainerString;

                switch(container->getDataType())
                {
                    case DatabaseObjectType::DEVICE: childContainerString = toString(boost::dynamic_pointer_cast<DeviceDataContainer>(container)); break;
                    case DatabaseObjectType::LOG: childContainerString = toString(boost::dynamic_pointer_cast<LogDataContainer>(container)); break;
                    case DatabaseObjectType::SCHEDULE: childContainerString = toString(boost::dynamic_pointer_cast<ScheduleDataContainer>(container)); break;
                    case DatabaseObjectType::SESSION: childContainerString = toString(boost::dynamic_pointer_cast<SessionDataContainer>(container)); break;
                    case DatabaseObjectType::STATISTICS: childContainerString = toString(boost::dynamic_pointer_cast<StatisticDataContainer>(container)); break;
                    case DatabaseObjectType::SYNC_FILE: childContainerString = toString(boost::dynamic_pointer_cast<SyncDataContainer>(container)); break;
                    case DatabaseObjectType::SYSTEM_SETTINGS: childContainerString = toString(boost::dynamic_pointer_cast<SystemDataContainer>(container)); break;
                    case DatabaseObjectType::USER: childContainerString = toString(boost::dynamic_pointer_cast<UserDataContainer>(container)); break;
                    default: childContainerString = "INVALID"; break;
                }

                return childContainerString;
            }

            static std::string toString(DeviceDataContainerPtr container)
            {
                return container->toString() + "," + Convert::toString(container->getDeviceOwner()) + "," + container->getDeviceAddress() + "," + Convert::toString(container->getDevicePort())
                       + "," + Convert::toString(container->getTransferType()) + "," + container->getDeviceProvidedID() + "," + container->getDeviceName()
                       + "," + Convert::toString(container->getPasswordData()) + "," + container->getDeviceInfo() + "," + Convert::toString(container->isDeviceLocked()) 
                       + "," + Convert::toString(container->getLastSuccessfulAuthenticationTimestamp())
                       + "," + Convert::toString(container->getLastFailedAuthenticationTimestamp())
                       + "," + Convert::toString(container->getFailedAuthenticationAttempts());
            }

            static std::string toString(LogDataContainerPtr container)
            {
                return container->toString() + "," + Convert::toString(container->getLogSeverity()) + "," + container->getLogSourceName() + ","
                       + Convert::toString(container->getLogTimestamp()) + "," + container->getLogMessage();
            }

            static std::string toString(ScheduleDataContainerPtr container)
            {
                return container->toString() + "," + Convert::toString(container->isScheduleActive()) + "," + Convert::toString(container->getNextRun()) + ","
                       + Convert::toString(container->getNumberOfRepetitions()) + "," + Convert::toString(container->getIntervalType()) + ","
                       + Convert::toString(container->getIntervalLength()) + "," + Convert::toString(container->runScheduleIfMissed()) + ","
                       + Convert::toString(container->deleteScheduleAfterCompletion());
            }

            static std::string toString(SessionDataContainerPtr container)
            {
                return container->toString() + "," + Convert::toString(container->getOpenTimestamp()) + "," + Convert::toString(container->getCloseTimestamp()) + ","
                       + Convert::toString(container->getLastActivityTimestamp()) + "," + Convert::toString(container->getSessionType()) + ","
                       + Convert::toString(container->getDevice()) + "," + Convert::toString(container->getUser()) + ","+ Convert::toString(container->isSessionPersistent()) + ","
                       + Convert::toString(container->isSessionActive()) + "," + Convert::toString(container->getDataSent()) + "," + Convert::toString(container->getDataReceived()) + ","
                       + Convert::toString(container->getCommandsSent()) + "," + Convert::toString(container->getCommandsReceived());
            }

            static std::string toString(StatisticDataContainerPtr container)
            {
                std::string value;

                switch(container->getStatisticType())
                {
                    case StatisticType::INSTALL_TIMESTAMP: value = Convert::toString(boost::any_cast<boost::posix_time::ptime>(container->getStatisticValue())); break;
                    case StatisticType::START_TIMESTAMP: value = Convert::toString(boost::any_cast<boost::posix_time::ptime>(container->getStatisticValue())); break;
                    case StatisticType::TOTAL_FAILED_TRANSFERS: value = Convert::toString(boost::any_cast<unsigned long>(container->getStatisticValue())); break;
                    case StatisticType::TOTAL_RETRIED_TRANSFERS: value = Convert::toString(boost::any_cast<unsigned long>(container->getStatisticValue())); break;
                    case StatisticType::TOTAL_TRANSFERRED_DATA: value = Convert::toString(boost::any_cast<unsigned long>(container->getStatisticValue())); break;
                    case StatisticType::TOTAL_TRANSFERRED_FILES: value = Convert::toString(boost::any_cast<unsigned long>(container->getStatisticValue())); break;
                    default: value = "UNDEFINED"; break;
                }

                return container->toString() + "," + Convert::toString(container->getStatisticType()) + "," + value;
            }

            static std::string toString(SyncDataContainerPtr container)
            {
                return container->toString() + "," + container->getSyncName() + "," + container->getSyncDescription() + "," + container->getSourcePath() + "," 
                       + container->getDestinationPath() + "," + Convert::toString(container->getSourceDevice()) + "," + Convert::toString(container->getDestinationDevice()) + ","
                       + Convert::toString(container->isSyncOneWay()) + "," + Convert::toString(container->isSyncOneTime()) + ","
                       + Convert::toString(container->getDirectoryConflictResolutionRule()) + "," + Convert::toString(container->getFileConflictResolutionRule()) + ","
                       + Convert::toString(container->isEncryptionEnabled()) + "," + Convert::toString(container->isCompressionEnabled()) + "," + Convert::toString(container->getOwnerID()) + ","
                       + container->getDestinationPermissions() + "," + Convert::toString(container->isOfflineSyncEnabled()) + "," + Convert::toString(container->isDifferentialSyncEnabled()) + ","
                       + Convert::toString(container->getNumberOfSyncRetries()) + "," + Convert::toString(container->getFailureAction()) + ","
                       + Convert::toString(container->getLastAttemptTimestamp()) + "," + Convert::toString(container->getLastResult()) + "," + Convert::toString(container->getLastSessionID());
            }

            static std::string toString(SystemDataContainerPtr container)
            {
                std::string value;

                switch(container->getSystemParameterType())
                {
                    //TODO - any_cast to proper types (DataPoolRetention instead of unsigned long)
                    case SystemParameterType::COMMAND_IP_ADDRESS: value = boost::any_cast<std::string>(container->getSystemParameterValue()); break;
                    case SystemParameterType::COMMAND_IP_PORT: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::COMMAND_RETRIES_MAX: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DATA_IP_ADDRESS: value = boost::any_cast<std::string>(container->getSystemParameterValue()); break;
                    case SystemParameterType::DATA_IP_PORT: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DATA_RETRIES_MAX: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DB_CACHE_FLUSH_INTERVAL: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DB_IMMEDIATE_FLUSH: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DB_MAX_READ_RETRIES: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DB_MAX_WRITE_RETRIES: value = Convert::toString(boost::any_cast<unsigned int>(container->getSystemParameterValue())); break;
                    case SystemParameterType::DB_OPERATION_MODE: value = Convert::toString(boost::any_cast<DatabaseManagerOperationMode>(container->getSystemParameterValue())); break;
                    case SystemParameterType::FORCE_COMMAND_ENCRYPTION: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::FORCE_DATA_COMPRESSION: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::FORCE_DATA_ENCRYPTION: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::IN_MEMORY_POOL_RETENTION: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::IN_MEMORY_POOL_SIZE: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::MINIMIZE_MEMORY_USAGE: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::PENDING_DATA_POOL_PATH: value = boost::any_cast<std::string>(container->getSystemParameterValue()); break;
                    case SystemParameterType::PENDING_DATA_POOL_SIZE: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::PENDING_DATA_RETENTION: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::SESSION_KEEP_ALIVE: value = Convert::toString(boost::any_cast<bool>(container->getSystemParameterValue())); break;
                    case SystemParameterType::SESSION_TIMEOUT: value = Convert::toString(boost::any_cast<unsigned long>(container->getSystemParameterValue())); break;
                    case SystemParameterType::SUPPORTED_PROTOCOLS: value = boost::any_cast<std::string>(container->getSystemParameterValue()); break;
                    default: value = "UNDEFINED"; break;
                }

                return container->toString() + "," + Convert::toString(container->getSystemParameterType()) + "," + value;
            }

            static std::string toString(UserDataContainerPtr container)
            {
                return container->toString() + "," + container->getUsername() + "," + Convert::toString(container->getPasswordData())
                        + "," + Convert::toString(container->getUserAccessLevel()) + "," + Convert::toString(container->getForcePasswordReset()) 
                        + "," + Convert::toString(container->isUserLocked()) + "," + Convert::toString(container->getCreationTimestamp()) 
                        + "," + Convert::toString(container->getLastSuccessfulAuthenticationTimestamp())
                        + "," + Convert::toString(container->getLastFailedAuthenticationTimestamp())
                        + "," + Convert::toString(container->getFailedAuthenticationAttempts());
            }

            static std::string toString(VectorDataContainerPtr container)
            {
                return container->toString() + "," + Convert::toString(container->getContainers().size());
            }
            //</editor-fold>

        private:
            Stringifier();
    };
    //</editor-fold>
}

#endif	/* DEBUGDAL_H */

