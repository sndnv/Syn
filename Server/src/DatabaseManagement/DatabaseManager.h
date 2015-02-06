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

#ifndef DATABASEMANAGER_H
#define	DATABASEMANAGER_H

#include <atomic>
#include <vector>
#include <string>
#include <boost/any.hpp>
#include "Types/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Database.h"
#include "../Utilities/FileLogger.h"
#include "Containers/DataContainer.h"
#include "Containers/LogDataContainer.h"
#include "Containers/SyncDataContainer.h"
#include "Containers/UserDataContainer.h"
#include "Containers/VectorDataContainer.h"
#include "Containers/SystemDataContainer.h"
#include "Containers/DeviceDataContainer.h"
#include "Containers/SessionDataContainer.h"
#include "Containers/ScheduleDataContainer.h"
#include "Containers/StatisticDataContainer.h"
#include "Interfaces/DatabaseAbstractionLayer.h"

#include "DALCache.h"
#include "DALQueue.h"

#include "../InstructionManagement/Types/Types.h"
#include "../InstructionManagement/Sets/InstructionSet.h"
#include "../InstructionManagement/Interfaces/InstructionTarget.h"
#include "../InstructionManagement/Sets/DatabaseManagerInstructionSet.h"

namespace Convert = Utilities::Strings;

using std::string;
using std::vector;
using DatabaseManagement_Interfaces::DALPtr;

using Common_Types::IPPort;
using Common_Types::IPAddress;
using Common_Types::Timestamp;
using Common_Types::TransferredDataAmount;
using Common_Types::TransferredFilesAmount;
using Common_Types::DataPoolSize;
using Common_Types::DataPoolPath;
using Common_Types::DataPoolRetention;
using Common_Types::UserAccessLevel;
using Common_Types::DBObjectID;
using Common_Types::LogID;
using Common_Types::SessionID;
using Common_Types::SyncID;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::ScheduleID;
using Common_Types::SessionType;
using DatabaseManagement_Types::DatabaseManagerOperationMode;
using DatabaseManagement_Types::LogSeverity;
using DatabaseManagement_Types::DataTransferType;
using DatabaseManagement_Types::ScheduleIntervalType;
using DatabaseManagement_Types::ConflictResolutionRule_Directory;
using DatabaseManagement_Types::ConflictResolutionRule_File;
using DatabaseManagement_Types::SyncFailureAction;
using DatabaseManagement_Types::SyncResult;
using DatabaseManagement_Types::DatabaseSelectConstraints;
using DatabaseManagement_Types::SystemParameterType;
using DatabaseManagement_Types::DatabaseFailureAction;
using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;
using DatabaseManagement_Types::FunctionCallTimeoutPeriod;

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

using SyncServer_Core::DatabaseManagement::DALCache;
using SyncServer_Core::DatabaseManagement::DALQueue;

using InstructionManagement_Sets::InstructionPtr;
using InstructionManagement_Types::DatabaseManagerInstructionType;

//TODO - remove
using Utilities::FileLogSeverity;

//TODO LIST
// -> move from locking & waiting in Functions_* calls to futures & promises
// -> STAT & SYS function containers - do checking of boost::any parameters (avoid assert fails & exception further on)
// -> Verify correct & race-free destructors for all objects
// -> Add getters for stats from all DALQueues & the DB manager (such as # of failures, # of requests, etc) (?))
namespace SyncServer_Core
{
    /**
     * Class for managing database access and activities.
     */
    class DatabaseManager : public InstructionManagement_Interfaces::InstructionTarget<InstructionManagement_Types::DatabaseManagerInstructionType>
    {
        public:
            class Functions_Statistics;
            class Functions_System;
            class Functions_SyncFiles;
            class Functions_Devices;
            class Functions_Schedules;
            class Functions_Users;
            class Functions_Logs;
            class Functions_Sessions;
            
            /**
             * Initialises the Database Manager.
             * 
             * Note: DALs need to added after initialisation.
             * 
             * @param loggerParameters file logger configuration parameters
             * @param defaultQueueParams default DAL queue configuration parameters
             * @param defaultCacheParams default DAL cache configuration parameters
             * @param functionTimeout function call timeout (in seconds)
             */
            DatabaseManager(Utilities::FileLoggerParameters loggerParameters, DALQueue::DALQueueParameters defaultQueueParams,
                            DALCache::DALCacheParameters defaultCacheParams, FunctionCallTimeoutPeriod functionTimeout);
            
            /**
             * Database Manager destructor.
             * 
             * Stops all threads and clears all data structures.
             */
            ~DatabaseManager();
            
            DatabaseManager() = delete;                                     //No default constructor
            DatabaseManager(const DatabaseManager&) = delete;               //Copying not allowed (pass/access only by reference/pointer)
            DatabaseManager& operator=(const DatabaseManager&) = delete;    //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Adds a new DAL.
             * 
             * @param dal the DAL to be added
             * @param enableCache denotes whether caching is to be enabled for the new DAL
             * 
             * @return <code>true</code>, if the operation was successful
             */
            bool addDAL(DALPtr dal, bool enableCache = false);
            
            /**
             * Adds a new DAL with caching enabled.
             * 
             * @param dal the DAL to be added
             * @param cacheParams the cache configuration parameters
             * 
             * @return <code>true</code>, if the operation was successful
             */
            bool addDAL(DALPtr dal, DALCache::DALCacheParameters cacheParams);
            
            /**
             * Removes a DAL.
             * 
             * @param dal the DAL to be removed
             * 
             * @return <code>true</code>, if the operation was successful
             */
            bool removeDAL(const DALPtr dal);
            
            /**
             * Sets new queue configuration parameters.
             * 
             * @param queueType the type of the affected queue
             * @param parameters the new queue configuration
             * 
             * @return <code>true</code>, if the operation was successful
             */
            bool setQueueParameters(DatabaseObjectType queueType, DALQueue::DALQueueParameters parameters);
            
            /**
             * Retrieves the parameters for the specified queue.
             * 
             * @param queueType the type of the queue
             * 
             * @return the requested data
             */
            DALQueue::DALQueueParameters getQueueParameters(DatabaseObjectType queueType);
            
            /**
             * Sets new cache configuration parameters.
             * 
             * @param cacheType the type of the affected cache
             * @param cacheID the ID of the affected cache
             * @param parameters the new cache configuration
             * 
             * @return <code>true</code>, if the operation was successful
             */
            bool setCacheParameters(DatabaseObjectType cacheType, DatabaseAbstractionLayerID cacheID, DALCache::DALCacheParameters parameters);
            
            /**
             * Retrieves the parameters for the specified cache.
             * 
             * @param queueType the queue type which holds the cache
             * @param cacheID the ID of the cache
             * 
             * @return the requested parameters
             */
            DALCache::DALCacheParameters getCacheParameters(DatabaseObjectType queueType, DatabaseAbstractionLayerID cacheID);
            
            /**
             * Sets the default cache configuration parameters.
             * 
             * @param parameters the new default parameters
             */
            void setDefaultCacheParameters(DALCache::DALCacheParameters parameters);
            
            /**
             * Retrieves the default cache configuration parameters.
             * 
             * @return the parameters
             */
            DALCache::DALCacheParameters getDefaultCacheParameters() const { return defaultCacheParameters; }
            
            /**
             * Sets the function call timeout.
             * 
             * Each call to the function objects will wait for the number of seconds specified (at most) before returning.
             * 
             * @param timeoutPeriod the timeout, in seconds
             */
            void setFunctionCallTimeout(FunctionCallTimeoutPeriod timeout);
            
            /**
             * Retrieves the timeout period for external function calls.
             * 
             * @return the timeout, in seconds
             */
            FunctionCallTimeoutPeriod getFunctionCallTimeout() const { return functionCallTimeout; }
            
            /**
             * Retrieves the information associated with the specified queue.
             * 
             * @param queueType the queue for which to gather data
             * @return the requested data
             */
            DALQueue::DALQueueInformation getQueueInformation(DatabaseObjectType queueType);
            
            /**
             * Retrieves the information associated with all caches under the specified queue.
             * 
             * @param queueType the queue for which to gather cache data
             * @return the requested data
             */
            std::vector<DALCache::DALCacheInformation> getCachesInformation(DatabaseObjectType queueType);
            
            /**
             * Retrieves the information associated with all DALs under the specified queue.
             * 
             * @param queueType the queue for which to gather DAL data
             * @return the requested data
             */
            std::vector<DALQueue::DALInformation> getDALsInformation(DatabaseObjectType queueType);

            /** Retrieves a reference to the STATISTICS DB access functions object.\n\n@return the statistics functions object */
            Functions_Statistics  &  Statistics() { return *internal_Statistics; }
            /** Retrieves a reference to the SYSTEM DB access functions object.\n\n@return the system functions object */
            Functions_System      &  System()     { return *internal_System; }
            /** Retrieves a reference to the SYNC_FILES DB access functions object.\n\n@return the sync files functions object */
            Functions_SyncFiles   &  SyncFiles()  { return *internal_SyncFiles; }
            /** Retrieves a reference to the DEVICES DB access functions object.\n\n@return the devices functions object */
            Functions_Devices     &  Devices()    { return *internal_Devices; }
            /** Retrieves a reference to the SCHEDULES DB access functions object.\n\n@return the schedules functions object */
            Functions_Schedules   &  Schedules()  { return *internal_Schedules; }
            /** Retrieves a reference to the USERS DB access functions object.\n\n@return the users functions object */
            Functions_Users       &  Users()      { return *internal_Users; }
            /** Retrieves a reference to the LOGS DB access functions object.\n\n@return the logs functions object */
            Functions_Logs        &  Logs()       { return *internal_Logs; }
            /** Retrieves a reference to the SESSIONS DB access functions object.\n\n@return the sessions functions object */
            Functions_Sessions    &  Sessions()   { return *internal_Sessions; }
        
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<DatabaseManagerInstructionType> set) const;
            InstructionManagement_Types::InstructionSetType getType() const { return InstructionManagement_Types::InstructionSetType::DATABASE_MANAGER; };
            
        private:
            //File Logger
            Utilities::FileLogger * logger;

            //Database Manager Settings
            DALCache::DALCacheParameters defaultCacheParameters;  //default cache configuration
            FunctionCallTimeoutPeriod functionCallTimeout;        //function call timeout (in seconds))
            boost::mutex configMutex;   //mutex for synchronising access to the above configuration

            //DALs
            DALQueue * statisticsTableDALs;
            DALQueue * systemTableDALs;
            DALQueue * syncFilesTableDALs;
            DALQueue * devicesTableDALs;
            DALQueue * schedulesTableDALs;
            DALQueue * usersTableDALs;
            DALQueue * logsTableDALs;
            DALQueue * sessionsTableDALs;

            //Internal functions pointers
            Functions_Statistics  *  internal_Statistics;
            Functions_System      *  internal_System;
            Functions_SyncFiles   *  internal_SyncFiles;
            Functions_Devices     *  internal_Devices;
            Functions_Schedules   *  internal_Schedules;
            Functions_Users       *  internal_Users;
            Functions_Logs        *  internal_Logs;
            Functions_Sessions    *  internal_Sessions;
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Statistics
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Statistics(DatabaseManager & parent);
            ~Functions_Statistics();

        public:
            bool setSystemInstallTimestamp();   //now
            bool setSystenStartTimestamp();     //now
            bool incrementTotalTransferredData(TransferredDataAmount amount);           //in MBs
            bool incrementTotalNumberOfTransferredFiles(TransferredFilesAmount amount);  //
            bool incrementTotalNumberOfFailedTransfers(TransferredFilesAmount amount);   //
            bool incrementTotalNumberOfRetriedTransfers(TransferredFilesAmount amount);  //

            bool updateStatistic(StatisticType type, boost::any value);
            StatisticDataContainerPtr getStatistic(StatisticType type);
            vector<StatisticDataContainerPtr> getAllStatistics();
            Timestamp getSystemInstallTimestamp();
            Timestamp getSystemStartTimestamp();
            TransferredDataAmount getTotalTransferredData();
            TransferredFilesAmount getTotalNumberOfTransferredFiles();
            TransferredFilesAmount getTotalNumberOfFailedTransfers();
            TransferredFilesAmount getTotalNumberOfRetriedTransfers();
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_System
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_System(DatabaseManager & parent);
            ~Functions_System();

        public:
            bool setSystemParameter(SystemParameterType type, boost::any value);
            bool setDataIPAddress(IPAddress address);
            bool setDataPort(IPPort port);
            bool setCommandIPAddress(IPAddress address);
            bool setCommandPort(IPPort port);
            bool setForceCommandEncryption(bool value);
            bool setForceDataEncryption(bool value);
            bool setForceDataCompression(bool value);
            bool setPendingDataPoolSize(DataPoolSize size);
            bool setPendingDataPoolPath(DataPoolPath path);
            bool setPendingDataPoolRetention(DataPoolRetention length);
            bool setInMemoryDataPoolSize(DataPoolSize size);
            bool setInMemoryDataPoolRetention(DataPoolRetention length);
            bool setCommandResendRetries(unsigned int retries);
            bool setDataResendRetries(unsigned int retries);
            bool setSessionTimeout(unsigned long length);
            bool setSessionKeepAliveState(bool state);
            bool setMinimizeMemoryUsageState(bool state);

            bool addSupportedProtocol(string protocol);
            bool removeSupportedProtocol(string protocol);

            bool setDBImmediateLogFlushState(bool state);
            bool setDBCacheFlushInterval(unsigned long length); //0=on shutdown
            bool setDBOperationMode(DatabaseManagerOperationMode mode);

            SystemDataContainerPtr getSystemParameter(SystemParameterType type);
            vector<SystemDataContainerPtr> getAllSystemparameters();
            IPAddress getDataIPAddress();
            IPPort getDataPort();
            IPAddress getCommandIPAddress();
            IPPort getCommandPort();
            bool getForceCommandEncryption();
            bool getForceDataEncryption();
            bool getForceDataCompression();
            DataPoolSize getPendingDataPoolSize();
            DataPoolPath getPendingDataPoolPath();
            DataPoolRetention getPendingDataPoolRetention();
            DataPoolSize getInMemoryDataPoolSize();
            DataPoolRetention getInMemoryDataPoolRetention();
            string getSupportedProtocols();
            unsigned int getCommandResendRetries();
            unsigned int getDataResendRetries();
            unsigned long getSessionTimeout();
            bool getSessionKeepAliveState();
            bool getMinimizeMemoryUsageState();

            bool getDBImmediateLogFlushState();
            unsigned long getDBCacheFlushInterval();
            DatabaseManagerOperationMode getDBOperationMode();
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_SyncFiles
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_SyncFiles(DatabaseManager & parent);
            ~Functions_SyncFiles();

        public:
            bool addSync(const SyncDataContainerPtr data);
            bool updateSync(const SyncDataContainerPtr data);
            bool removeSync(SyncID sync);

            SyncDataContainerPtr getSync(SyncID sync);
            vector<SyncDataContainerPtr> getSyncsByConstraint(DatabaseSelectConstraints::SYNC constraintType, boost::any constraintValue);
            vector<SyncDataContainerPtr> getSyncs();
            vector<SyncDataContainerPtr> getSyncsByOwner(UserID owner);
            vector<SyncDataContainerPtr> getSyncsByDevice(DeviceID device);
            vector<SyncDataContainerPtr> getSyncsByPath(string path);
            vector<SyncDataContainerPtr> getSyncsByEncryption(bool enabled);
            vector<SyncDataContainerPtr> getSyncsByCompression(bool enabled);
            vector<SyncDataContainerPtr> getSyncsByOfflineSynchronisation(bool enabled);
            vector<SyncDataContainerPtr> getSyncsByDifferentialSynchronisation(bool enabled);
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Devices
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Devices(DatabaseManager & parent);
            ~Functions_Devices();

        public:
            bool addDevice(const DeviceDataContainerPtr data);
            bool updateDevice(const DeviceDataContainerPtr data);
            bool removeDevice(DeviceID device);

            DeviceDataContainerPtr getDevice(DeviceID device);
            vector<DeviceDataContainerPtr> getDevicesByConstraint(DatabaseSelectConstraints::DEVICES constraintType, boost::any constraintValue);
            vector<DeviceDataContainerPtr> getDevices();
            vector<DeviceDataContainerPtr> getDevicesByTransferType(DataTransferType xferType);
            vector<DeviceDataContainerPtr> getDevicesByOwner(UserID owner);
            vector<DeviceDataContainerPtr> getDevicesByIPAddress(IPAddress address);
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Schedules
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Schedules(DatabaseManager & parent);
            ~Functions_Schedules();

        public:
            bool addSchedule(const ScheduleDataContainerPtr data);
            bool updateSchedule(const ScheduleDataContainerPtr data);
            bool removeSchedule(ScheduleID schedule);

            ScheduleDataContainerPtr getSchedule(ScheduleID schedule);
            vector<ScheduleDataContainerPtr> getSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES constraintType, boost::any constraintValue);
            vector<ScheduleDataContainerPtr> getSchedules();
            vector<ScheduleDataContainerPtr> getSchedulesByState(bool active);
            vector<ScheduleDataContainerPtr> getSchedulesBySyncID(SyncID sync);
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Users
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Users(DatabaseManager & parent);
            ~Functions_Users();

        public:
            bool addUser(const UserDataContainerPtr data);
            bool updateUser(const UserDataContainerPtr data);
            bool removeUser(UserID user);

            UserDataContainerPtr getUser(string username);
            UserDataContainerPtr getUser(UserID user);
            vector<UserDataContainerPtr> getUsersByConstraint(DatabaseSelectConstraints::USERS constraintType, boost::any constraintValue);
            vector<UserDataContainerPtr> getUsers();
            vector<UserDataContainerPtr> getUsersByAccessLevel(UserAccessLevel level);
            vector<UserDataContainerPtr> getUsersByLockedState(bool isUserLocked);
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Logs
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Logs(DatabaseManager & parent);
            ~Functions_Logs();

        public:
            
            /**
             * Adds a new event log to the database.
             * 
             * @param log the event data
             * @return true, if the operation was successful
             */
            bool addLog(const LogDataContainerPtr log);
            
            /**
             * Adds a new event log to the database, without waiting for a response.
             * 
             * @param log the event data
             * @return the ID associated with the request
             */
            DatabaseRequestID addLogAsync(const LogDataContainerPtr log);

            LogDataContainerPtr getLog(LogID log);
            vector<LogDataContainerPtr> getLogs();
            vector<LogDataContainerPtr> getLogsByConstraint(DatabaseSelectConstraints::LOGS constraintType, boost::any constraintValue);
            vector<LogDataContainerPtr> getLogsBySeverity(LogSeverity severity);
            vector<LogDataContainerPtr> getLogsBySource(string source);
    };

    /** Container class for database access functions. */
    class DatabaseManager::Functions_Sessions
    {
        friend class DatabaseManager;

        private:
            DatabaseManager * parentManager;
            std::atomic<bool> releaseLocks {false};

            Functions_Sessions(DatabaseManager & parent);
            ~Functions_Sessions();

        public:
            bool addSession(const SessionDataContainerPtr session);
            bool updateSession(const SessionDataContainerPtr session);

            SessionDataContainerPtr getSession(SessionID session);
            vector<SessionDataContainerPtr> getSessions();
            vector<SessionDataContainerPtr> getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS constraintType, boost::any constraintValue);
            vector<SessionDataContainerPtr> getSessionsByType(SessionType type);
            vector<SessionDataContainerPtr> getSessionsByDevice(DeviceID device);
            vector<SessionDataContainerPtr> getSessionsByUser(UserID user);
            vector<SessionDataContainerPtr> getActiveSessions();
            vector<SessionDataContainerPtr> getInactiveSessions();
            vector<SessionDataContainerPtr> getPersistentSessions();
            vector<SessionDataContainerPtr> getTemporarySessions();
    };
}
#endif	/* DATABASEMANAGER_H */

