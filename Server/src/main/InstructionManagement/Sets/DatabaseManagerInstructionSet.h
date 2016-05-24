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

#ifndef DATABASEMANAGERINSTRUCTIONSET_H
#define	DATABASEMANAGERINSTRUCTIONSET_H

#include <string>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "../../Utilities/Strings/Instructions.h"
#include "InstructionSet.h"
#include "../../Common/Types.h"
#include "../../DatabaseManagement/Types/Types.h"
#include "../../DatabaseManagement/DALCache.h"
#include "../../DatabaseManagement/DALQueue.h"
#include "../../DatabaseManagement/Interfaces/DatabaseAbstractionLayer.h"

#include "../../DatabaseManagement/Containers/DeviceDataContainer.h"
#include "../../DatabaseManagement/Containers/SyncDataContainer.h"
#include "../../DatabaseManagement/Containers/ScheduleDataContainer.h"
#include "../../DatabaseManagement/Containers/UserDataContainer.h"
#include "../../DatabaseManagement/Containers/LogDataContainer.h"
#include "../../DatabaseManagement/Containers/StatisticDataContainer.h"
#include "../../DatabaseManagement/Containers/SessionDataContainer.h"

using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::DatabaseManagerInstructionType;

using SyncServer_Core::DatabaseManagement::DALCache;
using SyncServer_Core::DatabaseManagement::DALQueue;

using Common_Types::Timestamp;
using Common_Types::TransferredFilesAmount;
using Common_Types::TransferredDataAmount;
using DatabaseManagement_Interfaces::DALPtr;
using Common_Types::LogID;
using Common_Types::UserID;
using Common_Types::SyncID;
using Common_Types::DeviceID;
using Common_Types::SessionID;
using Common_Types::ScheduleID;
using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::FunctionCallTimeoutPeriod;
using DatabaseManagement_Types::DatabaseSelectConstraints;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;

using DatabaseManagement_Containers::LogDataContainerPtr;
using DatabaseManagement_Containers::SyncDataContainerPtr;
using DatabaseManagement_Containers::UserDataContainerPtr;
using DatabaseManagement_Containers::DeviceDataContainerPtr;
using DatabaseManagement_Containers::ScheduleDataContainerPtr;
using DatabaseManagement_Containers::StatisticDataContainerPtr;
using DatabaseManagement_Containers::SessionDataContainerPtr;

namespace InstructionManagement_Sets
{
    namespace DatabaseManagerInstructions
    {
        //<editor-fold defaultstate="collapsed" desc="CORE Instructions">
        struct GetQueuesList : public Instruction<DatabaseManagerInstructionType>
        {
            GetQueuesList() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_QUEUES_LIST) {}
            bool isValid() override { return true; }
        };
        
        struct GetCachesList : public Instruction<DatabaseManagerInstructionType>
        {
            GetCachesList() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_CACHES_LIST) {}
            bool isValid() override { return true; }
        };
        
        struct GetDALsList : public Instruction<DatabaseManagerInstructionType>
        {
            GetDALsList() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_DALS_LIST) {}
            bool isValid() override { return true; }
        };
        
        struct SetDefaultDALCacheParameters : public Instruction<DatabaseManagerInstructionType>
        {
            SetDefaultDALCacheParameters(DALCache::DALCacheParameters input)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS),
              parameters(input)
            {}
            
            bool isValid() override { return true; }
            DALCache::DALCacheParameters parameters;
        };
        
        struct GetDefaultDALCacheParameters : public Instruction<DatabaseManagerInstructionType>
        {
            GetDefaultDALCacheParameters() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS) {}
            bool isValid() override { return true; }
        };
        
        struct SetCacheParameters : public Instruction<DatabaseManagerInstructionType>
        {
            SetCacheParameters(DatabaseObjectType type, DatabaseAbstractionLayerID id, DALCache::DALCacheParameters params)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::SET_CACHE_PARAMS),
              queueType(type), cacheID(id), parameters(params)
            {}
            bool isValid() override { return true; }
            DatabaseObjectType queueType;
            DatabaseAbstractionLayerID cacheID;
            DALCache::DALCacheParameters parameters;
        };
        
        struct GetCacheParameters : public Instruction<DatabaseManagerInstructionType>
        {
            GetCacheParameters(DatabaseObjectType type, DatabaseAbstractionLayerID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_CACHE_PARAMS),
              queueType(type), cacheID(id)
            {}
            bool isValid() override { return true; }
            DatabaseObjectType queueType;
            DatabaseAbstractionLayerID cacheID;
        };
        
        struct SetQueueParameters : public Instruction<DatabaseManagerInstructionType>
        {
            SetQueueParameters(DatabaseObjectType type, DALQueue::DALQueueParameters params)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::SET_QUEUE_PARAMS),
              queueType(type), parameters(params)
            {}
            bool isValid() override { return true; }
            DatabaseObjectType queueType;
            DALQueue::DALQueueParameters parameters;
        };
        
        struct GetQueueParameters : public Instruction<DatabaseManagerInstructionType>
        {
            GetQueueParameters(DatabaseObjectType type)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_QUEUE_PARAMS),
              queueType(type)
            {}
            bool isValid() override { return true; }
            DatabaseObjectType queueType;
        };
        
        struct SetFunctionTimeout : public Instruction<DatabaseManagerInstructionType>
        {
            SetFunctionTimeout(FunctionCallTimeoutPeriod timeoutPeriod)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT),
              timeout(timeoutPeriod)
            {}
            bool isValid() override { return true; }
            FunctionCallTimeoutPeriod timeout;
        };
        
        struct GetFunctionTimeout : public Instruction<DatabaseManagerInstructionType>
        {
            GetFunctionTimeout() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT) {}
            bool isValid() override { return true; }
        };
        
        struct AddDAL : public Instruction<DatabaseManagerInstructionType>
        {
            AddDAL(DALPtr dal, bool cache)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_DAL),
              newDAL(dal), enableCache(cache)
            {}
            bool isValid() override { return true; }
            DALPtr newDAL;
            bool enableCache;
        };
        
        struct AddDALWithCacheParameters : public Instruction<DatabaseManagerInstructionType>
        {
            AddDALWithCacheParameters(DALPtr dal, DALCache::DALCacheParameters params)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS),
              newDAL(dal), parameters(params)
            {}
            bool isValid() override { return true; }
            DALPtr newDAL;
            DALCache::DALCacheParameters parameters;
        };
        
        struct RemoveDAL : public Instruction<DatabaseManagerInstructionType>
        {
            RemoveDAL(DALPtr dal)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::REMOVE_DAL),
              DALToRemove(dal)
            {}
            bool isValid() override { return true; }
            DALPtr DALToRemove;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_STATISTICS Instructions">
        struct GetSystemInstallTimestamp : public Instruction<DatabaseManagerInstructionType>
        {
            GetSystemInstallTimestamp() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP) {}
            bool isValid() override { return true; }
        };
        
        struct GetSystemStartTimestamp : public Instruction<DatabaseManagerInstructionType>
        {
            GetSystemStartTimestamp() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP) {}
            bool isValid() override { return true; }
        };
        
        struct GetTotalTransferredData : public Instruction<DatabaseManagerInstructionType>
        {
            GetTotalTransferredData() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA) {}
            bool isValid() override { return true; }
        };
        
        struct GetTotalTransferredFiles : public Instruction<DatabaseManagerInstructionType>
        {
            GetTotalTransferredFiles() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES) {}
            bool isValid() override { return true; }
        };
        
        struct GetTotalFailedTransfers : public Instruction<DatabaseManagerInstructionType>
        {
            GetTotalFailedTransfers() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS) {}
            bool isValid() override { return true; }
        };
        
        struct GetTotalRetriedTransfers : public Instruction<DatabaseManagerInstructionType>
        {
            GetTotalRetriedTransfers() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS) {}
            bool isValid() override { return true; }
        };
        
        struct GetAllStats : public Instruction<DatabaseManagerInstructionType>
        {
            GetAllStats() : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_ALL_STATS) {}
            bool isValid() override { return true; }
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYSTEM Instructions">
        //TODO
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYNC_FILES Instructions">
        struct AddSync : public Instruction<DatabaseManagerInstructionType>
        {
            AddSync(SyncDataContainerPtr sync)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_SYNC),
              syncData(sync)
            {}
            bool isValid() override { return true; }
            SyncDataContainerPtr syncData;
        };
        
        struct RemoveSync : public Instruction<DatabaseManagerInstructionType>
        {
            RemoveSync(SyncID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::REMOVE_SYNC),
              syncID(id)
            {}
            bool isValid() override { return true; }
            SyncID syncID;
        };
        
        struct UpdateSync : public Instruction<DatabaseManagerInstructionType>
        {
            UpdateSync(SyncDataContainerPtr sync)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::UPDATE_SYNC),
              syncData(sync)
            {}
            bool isValid() override { return true; }
            SyncDataContainerPtr syncData;
        };
        
        struct GetSyncsByConstraint: public Instruction<DatabaseManagerInstructionType>
        {
            GetSyncsByConstraint(DatabaseSelectConstraints::SYNC type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::SYNC constraintType;
            boost::any constraintValue;
        };
        
        struct GetSync: public Instruction<DatabaseManagerInstructionType>
        {
            GetSync(SyncID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SYNC),
              syncID(id)
            {}
            bool isValid() override { return true; }
            SyncID syncID;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_DEVICES Instructions">
        struct AddDevice : public Instruction<DatabaseManagerInstructionType>
        {
            AddDevice(DeviceDataContainerPtr device)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_DEVICE),
              deviceData(device)
            {}
            bool isValid() override { return true; }
            DeviceDataContainerPtr deviceData;
        };
        
        struct UpdateDevice : public Instruction<DatabaseManagerInstructionType>
        {
            UpdateDevice(DeviceDataContainerPtr device)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::UPDATE_DEVICE),
              deviceData(device)
            {}
            bool isValid() override { return true; }
            DeviceDataContainerPtr deviceData;
        };
        
        struct RemoveDevice : public Instruction<DatabaseManagerInstructionType>
        {
            RemoveDevice(DeviceID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::REMOVE_DEVICE),
              deviceID(id)
            {}
            bool isValid() override { return true; }
            DeviceID deviceID;
        };
        
        struct GetDevicesByConstraint : public Instruction<DatabaseManagerInstructionType>
        {
            GetDevicesByConstraint(DatabaseSelectConstraints::DEVICES type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::DEVICES constraintType;
            boost::any constraintValue;
        };
        
        struct GetDevice : public Instruction<DatabaseManagerInstructionType>
        {
            GetDevice(DeviceID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_DEVICE),
              deviceID(id)
            {}
            bool isValid() override { return true; }
            DeviceID deviceID;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SCHEDULES Instructions">
        struct AddSchedule : public Instruction<DatabaseManagerInstructionType>
        {
            AddSchedule(ScheduleDataContainerPtr schedule)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_SCHEDULE),
              scheduleData(schedule)
            {}
            bool isValid() override { return true; }
            ScheduleDataContainerPtr scheduleData;
        };
        
        struct UpdateSchedule : public Instruction<DatabaseManagerInstructionType>
        {
            UpdateSchedule(ScheduleDataContainerPtr schedule)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::UPDATE_SCHEDULE),
              scheduleData(schedule)
            {}
            bool isValid() override { return true; }
            ScheduleDataContainerPtr scheduleData;
        };
        
        struct RemoveSchedule : public Instruction<DatabaseManagerInstructionType>
        {
            RemoveSchedule(ScheduleID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::REMOVE_SCHEDULE),
              scheduleID(id)
            {}
            bool isValid() override { return true; }
            ScheduleID scheduleID;
        };
        
        struct GetSchedulesByConstraint : public Instruction<DatabaseManagerInstructionType>
        {
            GetSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::SCHEDULES constraintType;
            boost::any constraintValue;
        };
        
        struct GetSchedule : public Instruction<DatabaseManagerInstructionType>
        {
            GetSchedule(ScheduleID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SCHEDULE),
              scheduleID(id)
            {}
            bool isValid() override { return true; }
            ScheduleID scheduleID;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_USERS Instructions">
        struct AddUser : public Instruction<DatabaseManagerInstructionType>
        {
            AddUser(UserDataContainerPtr user)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_USER),
              userData(user)
            {}
            bool isValid() override { return true; }
            UserDataContainerPtr userData;
        };
        
        struct UpdateUser : public Instruction<DatabaseManagerInstructionType>
        {
            UpdateUser(UserDataContainerPtr user)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::UPDATE_USER),
              userData(user)
            {}
            bool isValid() override { return true; }
            UserDataContainerPtr userData;
        };
        
        struct RemoveUser : public Instruction<DatabaseManagerInstructionType>
        {
            RemoveUser(UserID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::REMOVE_USER),
              userID(id)
            {}
            bool isValid() override { return true; }
            UserID userID;
        };
        
        struct GetUsersByConstraint : public Instruction<DatabaseManagerInstructionType>
        {
            GetUsersByConstraint(DatabaseSelectConstraints::USERS type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::USERS constraintType;
            boost::any constraintValue;
        };
        
        struct GetUser : public Instruction<DatabaseManagerInstructionType>
        {
            GetUser(UserID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_USER),
              userID(id), idSet(true)
            {}
            
            GetUser(std::string name)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_USER),
              username(name), idSet(false)
            {}
            
            bool isValid() override { return true; }
            UserID userID;
            std::string username;
            bool idSet;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_LOGS Instructions">
        struct AddLog : public Instruction<DatabaseManagerInstructionType>
        {
            AddLog(LogDataContainerPtr log)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_LOG),
              logData(log)
            {}
            bool isValid() override { return true; }
            LogDataContainerPtr logData;
        };
        
        struct AddLogAsync : public Instruction<DatabaseManagerInstructionType>
        {
            AddLogAsync(LogDataContainerPtr log)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::ADD_LOG_ASYNC),
              logData(log)
            {}
            bool isValid() override { return true; }
            LogDataContainerPtr logData;
        };
        
        struct GetLogsByConstraint : public Instruction<DatabaseManagerInstructionType>
        {
            GetLogsByConstraint(DatabaseSelectConstraints::LOGS type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::LOGS constraintType;
            boost::any constraintValue;
        };
        
        struct GetLog : public Instruction<DatabaseManagerInstructionType>
        {
            GetLog(LogID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_LOG),
              logID(id)
            {}
            bool isValid() override { return true; }
            LogID logID;
        };
        //</editor-fold>
        
        //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SESSIONS Instructions">
        struct GetSessionsByConstraint : public Instruction<DatabaseManagerInstructionType>
        {
            GetSessionsByConstraint(DatabaseSelectConstraints::SESSIONS type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::SESSIONS constraintType;
            boost::any constraintValue;
        };
        
        struct GetSession : public Instruction<DatabaseManagerInstructionType>
        {
            GetSession(SessionID id)
            : Instruction(InstructionSetType::DATABASE_MANAGER, DatabaseManagerInstructionType::GET_SESSION),
              sessionID(id)
            {}
            bool isValid() override { return true; }
            SessionID sessionID;
        };
        //</editor-fold>
        
        namespace Results
        {
            //<editor-fold defaultstate="collapsed" desc="CORE Results">
            struct GetQueuesList : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetQueuesList(std::vector<DALQueue::DALQueueInformation> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_QUEUES_LIST), result(input) {}
                
                std::vector<DALQueue::DALQueueInformation> result;
            };

            struct GetCachesList : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetCachesList(std::vector<DALCache::DALCacheInformation> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_CACHES_LIST), result(input) {}
                
                std::vector<DALCache::DALCacheInformation> result;
            };

            struct GetDALsList : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetDALsList(std::vector<DALQueue::DALInformation> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_DALS_LIST), result(input) {}
                
                std::vector<DALQueue::DALInformation> result;
            };

            struct SetDefaultDALCacheParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                SetDefaultDALCacheParameters(bool input)
                : InstructionResult(DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS), result(input) {}
                
                bool result;
            };
            
            struct GetDefaultDALCacheParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetDefaultDALCacheParameters(DALCache::DALCacheParameters input)
                : InstructionResult(DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS), result(input) {}
                
                DALCache::DALCacheParameters result;
            };
            
            struct SetCacheParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                SetCacheParameters(bool input)
                : InstructionResult(DatabaseManagerInstructionType::SET_CACHE_PARAMS), result(input) {}
                
                bool result;
            };

            struct GetCacheParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetCacheParameters(DALCache::DALCacheParameters input)
                : InstructionResult(DatabaseManagerInstructionType::GET_CACHE_PARAMS), result(input) {}
                
                DALCache::DALCacheParameters result;
            };

            struct SetQueueParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                SetQueueParameters(bool input)
                : InstructionResult(DatabaseManagerInstructionType::SET_QUEUE_PARAMS), result(input) {}
                
                bool result;
            };

            struct GetQueueParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetQueueParameters(DALQueue::DALQueueParameters input)
                : InstructionResult(DatabaseManagerInstructionType::GET_QUEUE_PARAMS), result(input) {}
                
                DALQueue::DALQueueParameters result;
            };

            struct SetFunctionTimeout : public InstructionResult<DatabaseManagerInstructionType>
            {
                SetFunctionTimeout(bool input)
                : InstructionResult(DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT), result(input) {}
                
                bool result;
            };

            struct GetFunctionTimeout : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetFunctionTimeout(FunctionCallTimeoutPeriod input)
                : InstructionResult(DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT), result(input) {}
                
                FunctionCallTimeoutPeriod result;
            };

            struct AddDAL : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddDAL(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_DAL), result(input) {}
                
                bool result;
            };

            struct AddDALWithCacheParameters : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddDALWithCacheParameters(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS), result(input) {}
                
                bool result;
            };

            struct RemoveDAL : public InstructionResult<DatabaseManagerInstructionType>
            {
                RemoveDAL(bool input)
                : InstructionResult(DatabaseManagerInstructionType::REMOVE_DAL), result(input) {}
                
                bool result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_STATISTICS Results">
            struct GetSystemInstallTimestamp : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSystemInstallTimestamp(Timestamp input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP), result(input) {}
                
                Timestamp result;
            };

            struct GetSystemStartTimestamp : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSystemStartTimestamp(Timestamp input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP), result(input) {}
                
                Timestamp result;
            };

            struct GetTotalTransferredData : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetTotalTransferredData(TransferredDataAmount input)
                : InstructionResult(DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA), result(input) {}
                
                TransferredDataAmount result;
            };

            struct GetTotalTransferredFiles : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetTotalTransferredFiles(TransferredFilesAmount input)
                : InstructionResult(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES), result(input) {}
                
                TransferredFilesAmount result;
            };

            struct GetTotalFailedTransfers : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetTotalFailedTransfers(TransferredFilesAmount input)
                : InstructionResult(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS), result(input) {}
                
                TransferredFilesAmount result;
            };

            struct GetTotalRetriedTransfers : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetTotalRetriedTransfers(TransferredFilesAmount input)
                : InstructionResult(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS), result(input) {}
                
                TransferredFilesAmount result;
            };

            struct GetAllStats : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetAllStats(vector<StatisticDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_ALL_STATS), result(input) {}
                
                std::vector<StatisticDataContainerPtr> result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYSTEM Results">
            //TODO
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYNC_FILES Results">
            struct AddSync : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddSync(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_SYNC), result(input) {}
                
                bool result;
            };
            
            struct UpdateSync : public InstructionResult<DatabaseManagerInstructionType>
            {
                UpdateSync(bool input)
                : InstructionResult(DatabaseManagerInstructionType::UPDATE_SYNC), result(input) {}
                
                bool result;
            };
            
            struct RemoveSync : public InstructionResult<DatabaseManagerInstructionType>
            {
                RemoveSync(bool input)
                : InstructionResult(DatabaseManagerInstructionType::REMOVE_SYNC), result(input) {}
                
                bool result;
            };
            
            struct GetSyncsByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSyncsByConstraint(std::vector<SyncDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT), result(input) {}
                
                std::vector<SyncDataContainerPtr> result;
            };
            
            struct GetSync : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSync(SyncDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SYNC), result(input) {}
                
                SyncDataContainerPtr result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_DEVICES Results">
            struct AddDevice : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddDevice(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct UpdateDevice : public InstructionResult<DatabaseManagerInstructionType>
            {
                UpdateDevice(bool input)
                : InstructionResult(DatabaseManagerInstructionType::UPDATE_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct RemoveDevice : public InstructionResult<DatabaseManagerInstructionType>
            {
                RemoveDevice(bool input)
                : InstructionResult(DatabaseManagerInstructionType::REMOVE_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct GetDevicesByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetDevicesByConstraint(std::vector<DeviceDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT), result(input) {}
                
                std::vector<DeviceDataContainerPtr> result;
            };
            
            struct GetDevice : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetDevice(DeviceDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_DEVICE), result(input) {}
                
                DeviceDataContainerPtr result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SCHEDULES Results">
            struct AddSchedule : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddSchedule(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_SCHEDULE), result(input) {}
                
                bool result;
            };
            
            struct UpdateSchedule : public InstructionResult<DatabaseManagerInstructionType>
            {
                UpdateSchedule(bool input)
                : InstructionResult(DatabaseManagerInstructionType::UPDATE_SCHEDULE), result(input) {}
                
                bool result;
            };
            
            struct RemoveSchedule : public InstructionResult<DatabaseManagerInstructionType>
            {
                RemoveSchedule(bool input)
                : InstructionResult(DatabaseManagerInstructionType::REMOVE_SCHEDULE), result(input) {}
                
                bool result;
            };
            
            struct GetSchedulesByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSchedulesByConstraint(std::vector<ScheduleDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT), result(input) {}
                
                std::vector<ScheduleDataContainerPtr> result;
            };
            
            struct GetSchedule : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSchedule(ScheduleDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SCHEDULE), result(input) {}
                
                ScheduleDataContainerPtr result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_USERS Results">
            struct AddUser : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddUser(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_USER), result(input) {}
                
                bool result;
            };
            
            struct UpdateUser : public InstructionResult<DatabaseManagerInstructionType>
            {
                UpdateUser(bool input)
                : InstructionResult(DatabaseManagerInstructionType::UPDATE_USER), result(input) {}
                
                bool result;
            };
            
            struct RemoveUser : public InstructionResult<DatabaseManagerInstructionType>
            {
                RemoveUser(bool input)
                : InstructionResult(DatabaseManagerInstructionType::REMOVE_USER), result(input) {}
                
                bool result;
            };
            
            struct GetUsersByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetUsersByConstraint(std::vector<UserDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT), result(input) {}
                
                std::vector<UserDataContainerPtr> result;
            };
            
            struct GetUser : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetUser(UserDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_USER), result(input) {}
                
                UserDataContainerPtr result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_LOGS Results">
            struct AddLog : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddLog(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_LOG), result(input) {}
                
                bool result;
            };
            
            struct AddLogAsync : public InstructionResult<DatabaseManagerInstructionType>
            {
                AddLogAsync(bool input)
                : InstructionResult(DatabaseManagerInstructionType::ADD_LOG_ASYNC), result(input) {}
                
                bool result;
            };
            
            struct GetLogsByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetLogsByConstraint(std::vector<LogDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT), result(input) {}
                
                std::vector<LogDataContainerPtr> result;
            };
            
            struct GetLog : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetLog(LogDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_LOG), result(input) {}
                
                LogDataContainerPtr result;
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SESSIONS Results">
            struct GetSessionsByConstraint : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSessionsByConstraint(std::vector<SessionDataContainerPtr> input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT), result(input) {}
                
                std::vector<SessionDataContainerPtr> result;
            };
            
            struct GetSession : public InstructionResult<DatabaseManagerInstructionType>
            {
                GetSession(SessionDataContainerPtr input)
                : InstructionResult(DatabaseManagerInstructionType::GET_SESSION), result(input) {}
                
                SessionDataContainerPtr result;
            };
            //</editor-fold>
        }
    }
}

#endif	/* DATABASEMANAGERINSTRUCTIONSET_H */

