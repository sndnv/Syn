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

#include "DatabaseManager.h"

namespace InstructionResults = InstructionManagement_Sets::DatabaseManagerInstructions::Results;

SyncServer_Core::DatabaseManager::DatabaseManager(Utilities::FileLoggerParameters loggerParameters, DALQueue::DALQueueParameters defaultQueueParams,
                                                  DALCache::DALCacheParameters defaultCacheParams, FunctionCallTimeoutPeriod functionTimeout)
                                                  : defaultCacheParameters(defaultCacheParams), functionCallTimeout(functionTimeout)
{
    logger = new Utilities::FileLogger(loggerParameters);
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager () > Creating function objects.");
    
    internal_Statistics = new DatabaseManager::Functions_Statistics(*this);
    internal_System     = new DatabaseManager::Functions_System(*this);
    internal_SyncFiles  = new DatabaseManager::Functions_SyncFiles(*this);
    internal_Devices    = new DatabaseManager::Functions_Devices(*this);
    internal_Schedules  = new DatabaseManager::Functions_Schedules(*this);
    internal_Users      = new DatabaseManager::Functions_Users(*this);
    internal_Logs       = new DatabaseManager::Functions_Logs(*this);
    internal_Sessions   = new DatabaseManager::Functions_Sessions(*this);
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager () > Creating queues.");
    
    statisticsTableDALs = new DALQueue(DatabaseObjectType::STATISTICS,  *logger, defaultQueueParams);
    systemTableDALs     = new DALQueue(DatabaseObjectType::SYSTEM_SETTINGS, *logger, defaultQueueParams);
    syncFilesTableDALs  = new DALQueue(DatabaseObjectType::SYNC_FILE, *logger, defaultQueueParams);
    devicesTableDALs    = new DALQueue(DatabaseObjectType::DEVICE, *logger, defaultQueueParams);
    schedulesTableDALs  = new DALQueue(DatabaseObjectType::SCHEDULE, *logger, defaultQueueParams);
    usersTableDALs      = new DALQueue(DatabaseObjectType::USER, *logger, defaultQueueParams);
    logsTableDALs       = new DALQueue(DatabaseObjectType::LOG, *logger, defaultQueueParams);
    sessionsTableDALs   = new DALQueue(DatabaseObjectType::SESSION, *logger, defaultQueueParams);
    
}

SyncServer_Core::DatabaseManager::~DatabaseManager()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager (~) > Destruction initiated.");
    
    delete statisticsTableDALs;
    delete systemTableDALs;
    delete syncFilesTableDALs;
    delete devicesTableDALs;
    delete schedulesTableDALs;
    delete usersTableDALs;
    delete logsTableDALs;
    delete sessionsTableDALs;
    
    delete internal_Statistics;
    delete internal_System;
    delete internal_SyncFiles;
    delete internal_Devices;
    delete internal_Schedules;
    delete internal_Users;
    delete internal_Logs;
    delete internal_Sessions;
    
    delete logger;
}

bool SyncServer_Core::DatabaseManager::addDAL(DALPtr dal, bool enableCache)
{
    DALPtr newDAL;
    
    boost::lock_guard<boost::mutex> configLock(configMutex);
    
    if(enableCache)
        newDAL = DALPtr(new SyncServer_Core::DatabaseManagement::DALCache(dal, *logger, defaultCacheParameters));
    else
        newDAL = dal;
    
    switch(dal->getType())
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::DEVICE: return devicesTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::USER: return usersTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::LOG: return logsTableDALs->addDAL(newDAL); break;
        case DatabaseObjectType::SESSION: return sessionsTableDALs->addDAL(newDAL); break;
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Add DAL) > Failed to add DAL; unexpected type found <" + Convert::toString(newDAL->getType()) + ">.");
            return false;
        } break;
    }
}

bool SyncServer_Core::DatabaseManager::addDAL(DALPtr dal, DALCache::DALCacheParameters cacheParams)
{
    DALPtr newDAL(new SyncServer_Core::DatabaseManagement::DALCache(dal, *logger, cacheParams));
    
    switch(dal->getType())
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->addDAL(newDAL);
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->addDAL(newDAL);
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->addDAL(newDAL);
        case DatabaseObjectType::DEVICE: return devicesTableDALs->addDAL(newDAL);
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->addDAL(newDAL);
        case DatabaseObjectType::USER: return usersTableDALs->addDAL(newDAL);
        case DatabaseObjectType::LOG: return logsTableDALs->addDAL(newDAL);
        case DatabaseObjectType::SESSION: return sessionsTableDALs->addDAL(newDAL);
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Add DAL) > Failed to add DAL; unexpected type found <" + Convert::toString(newDAL->getType()) + ">.");
            return false;
        }
    }
}

bool SyncServer_Core::DatabaseManager::removeDAL(const DALPtr dal)
{
    switch(dal->getType())
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->removeDAL(dal);
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->removeDAL(dal);
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->removeDAL(dal);
        case DatabaseObjectType::DEVICE: return devicesTableDALs->removeDAL(dal);
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->removeDAL(dal);
        case DatabaseObjectType::USER: return usersTableDALs->removeDAL(dal);
        case DatabaseObjectType::LOG: return logsTableDALs->removeDAL(dal);
        case DatabaseObjectType::SESSION: return sessionsTableDALs->removeDAL(dal);
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Remove DAL) > Failed to remove DAL; unexpected type found <" + Convert::toString(dal->getType()) + ">.");
            return false;
        }
    }
}

bool SyncServer_Core::DatabaseManager::setQueueParameters(DatabaseObjectType queueType, DALQueue::DALQueueParameters parameters)
{
    bool result = false;
    
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: result = statisticsTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::SYSTEM_SETTINGS: result = systemTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::SYNC_FILE: result = syncFilesTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::DEVICE: result = devicesTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::SCHEDULE: result = schedulesTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::USER: result = usersTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::LOG: result = logsTableDALs->setParameters(parameters); break;
        case DatabaseObjectType::SESSION: result = sessionsTableDALs->setParameters(parameters); break;
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Set Queue Parameters) > Failed to set DALQueue parameters; unexpected type found <" + Convert::toString(queueType) + ">.");
        } break;
    }
    
    return result;
}

SyncServer_Core::DatabaseManagement::DALQueue::DALQueueParameters SyncServer_Core::DatabaseManager::getQueueParameters(DatabaseObjectType queueType)
{
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->getParameters();
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->getParameters();
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->getParameters();
        case DatabaseObjectType::DEVICE: return devicesTableDALs->getParameters();
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->getParameters();
        case DatabaseObjectType::USER: return usersTableDALs->getParameters();
        case DatabaseObjectType::LOG: return logsTableDALs->getParameters();
        case DatabaseObjectType::SESSION: return sessionsTableDALs->getParameters();
        
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Get Queue Parameters) > Failed to get DALQueue information; unexpected type found <" + Convert::toString(queueType) + ">.");
            return DALQueue::DALQueueParameters();
        }
    }
}

bool SyncServer_Core::DatabaseManager::setCacheParameters(DatabaseObjectType cacheType, DatabaseAbstractionLayerID cacheID, DALCache::DALCacheParameters parameters)
{
    bool result = false;
    
    switch(cacheType)
    {
        case DatabaseObjectType::STATISTICS: result = statisticsTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::SYSTEM_SETTINGS: result = systemTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::SYNC_FILE: result = syncFilesTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::DEVICE: result = devicesTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::SCHEDULE: result = schedulesTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::USER: result = usersTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::LOG: result = logsTableDALs->setCacheParameters(cacheID, parameters); break;
        case DatabaseObjectType::SESSION: result = sessionsTableDALs->setCacheParameters(cacheID, parameters); break;
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Set Cache Parameters) > Failed to set DALCache parameters; unexpected type found <" + Convert::toString(cacheType) + ">.");
        } break;
    }
    
    return result;
}

DALCache::DALCacheParameters SyncServer_Core::DatabaseManager::getCacheParameters(DatabaseObjectType queueType, DatabaseAbstractionLayerID cacheID)
{
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::DEVICE: return devicesTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::USER: return usersTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::LOG: return logsTableDALs->getCacheParameters(cacheID);
        case DatabaseObjectType::SESSION: return sessionsTableDALs->getCacheParameters(cacheID);
        
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Get Cache Parameters) > Failed to get caches information; unexpected type found <" + Convert::toString(queueType) + ">.");
            return DALCache::DALCacheParameters();
        }
    }
}

void SyncServer_Core::DatabaseManager::setDefaultCacheParameters(DALCache::DALCacheParameters parameters)
{
    boost::lock_guard<boost::mutex> configLock(configMutex);
    defaultCacheParameters = parameters;
}

void SyncServer_Core::DatabaseManager::setFunctionCallTimeout(FunctionCallTimeoutPeriod timeout)
{
    boost::lock_guard<boost::mutex> configLock(configMutex);
    functionCallTimeout = timeout;
}

DALQueue::DALQueueInformation SyncServer_Core::DatabaseManager::getQueueInformation(DatabaseObjectType queueType)
{
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->getQueueInformation();
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->getQueueInformation();
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->getQueueInformation();
        case DatabaseObjectType::DEVICE: return devicesTableDALs->getQueueInformation();
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->getQueueInformation();
        case DatabaseObjectType::USER: return usersTableDALs->getQueueInformation();
        case DatabaseObjectType::LOG: return logsTableDALs->getQueueInformation();
        case DatabaseObjectType::SESSION: return sessionsTableDALs->getQueueInformation();
        
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Get Queue Information) > Failed to get queue information; unexpected type found <" + Convert::toString(queueType) + ">.");
            return DALQueue::DALQueueInformation();
        }
    }
}

std::vector<DALCache::DALCacheInformation> SyncServer_Core::DatabaseManager::getCachesInformation(DatabaseObjectType queueType)
{
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->getCachesInformation();
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->getCachesInformation();
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->getCachesInformation();
        case DatabaseObjectType::DEVICE: return devicesTableDALs->getCachesInformation();
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->getCachesInformation();
        case DatabaseObjectType::USER: return usersTableDALs->getCachesInformation();
        case DatabaseObjectType::LOG: return logsTableDALs->getCachesInformation();
        case DatabaseObjectType::SESSION: return sessionsTableDALs->getCachesInformation();
        
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Get Caches Information) > Failed to get caches information; unexpected type found <" + Convert::toString(queueType) + ">.");
            return std::vector<DALCache::DALCacheInformation>();
        }
    }
}

std::vector<DALQueue::DALInformation> SyncServer_Core::DatabaseManager::getDALsInformation(DatabaseObjectType queueType)
{
    switch(queueType)
    {
        case DatabaseObjectType::STATISTICS: return statisticsTableDALs->getDALsInformation();
        case DatabaseObjectType::SYSTEM_SETTINGS: return systemTableDALs->getDALsInformation();
        case DatabaseObjectType::SYNC_FILE: return syncFilesTableDALs->getDALsInformation();
        case DatabaseObjectType::DEVICE: return devicesTableDALs->getDALsInformation();
        case DatabaseObjectType::SCHEDULE: return schedulesTableDALs->getDALsInformation();
        case DatabaseObjectType::USER: return usersTableDALs->getDALsInformation();
        case DatabaseObjectType::LOG: return logsTableDALs->getDALsInformation();
        case DatabaseObjectType::SESSION: return sessionsTableDALs->getDALsInformation();
        
        default:
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DatabaseManager (Get DALs Information) > Failed to get DALs information; unexpected type found <" + Convert::toString(queueType) + ">.");
            return std::vector<DALQueue::DALInformation>();
        }
    }
}

//<editor-fold defaultstate="collapsed" desc="Functions_Statistics">
SyncServer_Core::DatabaseManager::Functions_Statistics::Functions_Statistics(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Statistics::~Functions_Statistics()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::updateStatistic(StatisticType type, boost::any value)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    bool successful = false; //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateStatistic/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateStatistic/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateStatistic/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateStatistic/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateStatistic/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateStatistic/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateStatistic/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateStatistic/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateStatistic/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateStatistic/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->statisticsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->statisticsTableDALs->onFailureEventAttach(onFailureHandler);
    
    StatisticDataContainerPtr data(new DatabaseManagement_Containers::StatisticDataContainer(type, value));
    requestID = parentManager->statisticsTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateStatistic/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateStatistic/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::setSystemInstallTimestamp()
{
    return updateStatistic(StatisticType::INSTALL_TIMESTAMP, boost::posix_time::second_clock::universal_time());
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::setSystenStartTimestamp()
{
    return updateStatistic(StatisticType::START_TIMESTAMP, boost::posix_time::second_clock::universal_time());
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::incrementTotalTransferredData(TransferredDataAmount amount)
{
    return updateStatistic(StatisticType::TOTAL_TRANSFERRED_DATA, amount);
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::incrementTotalNumberOfTransferredFiles(TransferredFilesAmount amount)
{
    return updateStatistic(StatisticType::TOTAL_TRANSFERRED_FILES, amount);
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::incrementTotalNumberOfFailedTransfers(TransferredFilesAmount amount)
{
    return updateStatistic(StatisticType::TOTAL_FAILED_TRANSFERS, amount);
}

bool SyncServer_Core::DatabaseManager::Functions_Statistics::incrementTotalNumberOfRetriedTransfers(TransferredFilesAmount amount)
{
    return updateStatistic(StatisticType::TOTAL_RETRIED_TRANSFERS, amount);
}

StatisticDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Statistics::getStatistic(StatisticType type)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    StatisticDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getStatistic/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getStatistic/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getStatistic/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getStatistic/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::StatisticDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getStatistic/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getStatistic/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getStatistic/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getStatistic/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getStatistic/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getStatistic/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->statisticsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->statisticsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->statisticsTableDALs->addSelectRequest(DatabaseSelectConstraints::STATISTCS::LIMIT_BY_TYPE, type);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getStatistic/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getStatistic/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<StatisticDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Statistics::getAllStatistics()
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllStatistics/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllStatistics/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllStatistics/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllStatistics/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllStatistics/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllStatistics/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllStatistics/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllStatistics/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllStatistics/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllStatistics/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->statisticsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->statisticsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->statisticsTableDALs->addSelectRequest(DatabaseSelectConstraints::STATISTCS::GET_ALL, 0);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getAllStatistics/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<StatisticDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::StatisticDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getAllStatistics/END> ["+Convert::toString(requestID)+"]");
    return result;
}

Timestamp SyncServer_Core::DatabaseManager::Functions_Statistics::getSystemInstallTimestamp()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::INSTALL_TIMESTAMP);
    
    if(container || container->getStatisticType() != StatisticType::INSTALL_TIMESTAMP)
        return boost::any_cast<Timestamp>(container->getStatisticValue());
    else
        return boost::posix_time::ptime(boost::posix_time::not_a_date_time);
}

Timestamp SyncServer_Core::DatabaseManager::Functions_Statistics::getSystemStartTimestamp()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::START_TIMESTAMP);
    
    if(container || container->getStatisticType() != StatisticType::START_TIMESTAMP)
        return boost::any_cast<Timestamp>(container->getStatisticValue());
    else
        return boost::posix_time::ptime(boost::posix_time::not_a_date_time);
}

TransferredDataAmount SyncServer_Core::DatabaseManager::Functions_Statistics::getTotalTransferredData()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::TOTAL_TRANSFERRED_DATA);
    
    if(container || container->getStatisticType() != StatisticType::TOTAL_TRANSFERRED_DATA)
        return boost::any_cast<TransferredDataAmount>(container->getStatisticValue());
    else
        return Common_Types::INVALID_TRANSFERRED_DATA_AMOUNT;
}

TransferredFilesAmount SyncServer_Core::DatabaseManager::Functions_Statistics::getTotalNumberOfTransferredFiles()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::TOTAL_TRANSFERRED_FILES);
    
    if(container || container->getStatisticType() != StatisticType::TOTAL_TRANSFERRED_FILES)
        return boost::any_cast<TransferredFilesAmount>(container->getStatisticValue());
    else
        return Common_Types::INVALID_TRANSFERRED_FILES_AMOUNT;
}

TransferredFilesAmount SyncServer_Core::DatabaseManager::Functions_Statistics::getTotalNumberOfFailedTransfers()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::TOTAL_FAILED_TRANSFERS);
    
    if(container || container->getStatisticType() != StatisticType::TOTAL_FAILED_TRANSFERS)
        return boost::any_cast<TransferredFilesAmount>(container->getStatisticValue());
    else
        return Common_Types::INVALID_TRANSFERRED_FILES_AMOUNT;
}

TransferredFilesAmount SyncServer_Core::DatabaseManager::Functions_Statistics::getTotalNumberOfRetriedTransfers()
{
    StatisticDataContainerPtr container = getStatistic(StatisticType::TOTAL_RETRIED_TRANSFERS);
    
    if(container || container->getStatisticType() != StatisticType::TOTAL_RETRIED_TRANSFERS)
        return boost::any_cast<TransferredFilesAmount>(container->getStatisticValue());
    else
        return Common_Types::INVALID_TRANSFERRED_FILES_AMOUNT;
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_System">
SyncServer_Core::DatabaseManager::Functions_System::Functions_System(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_System::~Functions_System()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_System::setSystemParameter(SystemParameterType type, boost::any value)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <setSystemParameter/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <setSystemParameter/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <setSystemParameter/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <setSystemParameter/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <setSystemParameter/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <setSystemParameter/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <setSystemParameter/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <setSystemParameter/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <setSystemParameter/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <setSystemParameter/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->systemTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->systemTableDALs->onFailureEventAttach(onFailureHandler);
    
    SystemDataContainerPtr data(new DatabaseManagement_Containers::SystemDataContainer(type, value));
    requestID = parentManager->systemTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <setSystemParameter/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <setSystemParameter/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDataIPAddress(IPAddress address)
{
    return setSystemParameter(SystemParameterType::DATA_IP_ADDRESS, address);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDataPort(IPPort port)
{
    return setSystemParameter(SystemParameterType::DATA_IP_PORT, port);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setCommandIPAddress(IPAddress address)
{
    return setSystemParameter(SystemParameterType::COMMAND_IP_ADDRESS, address);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setCommandPort(IPPort port)
{
    return setSystemParameter(SystemParameterType::COMMAND_IP_PORT, port);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setForceCommandEncryption(bool value)
{
    return setSystemParameter(SystemParameterType::FORCE_COMMAND_ENCRYPTION, value);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setForceDataEncryption(bool value)
{
    return setSystemParameter(SystemParameterType::FORCE_DATA_ENCRYPTION, value);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setForceDataCompression(bool value)
{
    return setSystemParameter(SystemParameterType::FORCE_DATA_COMPRESSION, value);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setPendingDataPoolSize(DataPoolSize size)
{
    return setSystemParameter(SystemParameterType::PENDING_DATA_POOL_SIZE, size);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setPendingDataPoolPath(DataPoolPath path)
{
    return setSystemParameter(SystemParameterType::PENDING_DATA_POOL_PATH, path);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setPendingDataPoolRetention(DataPoolRetention length)
{
    return setSystemParameter(SystemParameterType::PENDING_DATA_RETENTION, length);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setInMemoryDataPoolSize(DataPoolSize size)
{
    return setSystemParameter(SystemParameterType::IN_MEMORY_POOL_SIZE, size);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setInMemoryDataPoolRetention(DataPoolRetention length)
{
    return setSystemParameter(SystemParameterType::IN_MEMORY_POOL_RETENTION, length);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setCommandResendRetries(unsigned int retries)
{
    return setSystemParameter(SystemParameterType::COMMAND_RETRIES_MAX, retries);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDataResendRetries(unsigned int retries)
{
    return setSystemParameter(SystemParameterType::DATA_RETRIES_MAX, retries);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setSessionTimeout(unsigned long length)
{
    return setSystemParameter(SystemParameterType::SESSION_TIMEOUT, length);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setSessionKeepAliveState(bool state)
{
    return setSystemParameter(SystemParameterType::SESSION_KEEP_ALIVE, state);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setMinimizeMemoryUsageState(bool state)
{
    return setSystemParameter(SystemParameterType::MINIMIZE_MEMORY_USAGE, state);
}

bool SyncServer_Core::DatabaseManager::Functions_System::addSupportedProtocol(string protocol)
{
    return setSystemParameter(SystemParameterType::SUPPORTED_PROTOCOLS, protocol);
}

bool SyncServer_Core::DatabaseManager::Functions_System::removeSupportedProtocol(string protocol)
{
    return setSystemParameter(SystemParameterType::SUPPORTED_PROTOCOLS, protocol);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDBImmediateLogFlushState(bool state)
{
    return setSystemParameter(SystemParameterType::DB_IMMEDIATE_FLUSH, state);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDBCacheFlushInterval(unsigned long length)//0=on shutdown
{
    return setSystemParameter(SystemParameterType::DB_CACHE_FLUSH_INTERVAL, length);
}

bool SyncServer_Core::DatabaseManager::Functions_System::setDBOperationMode(DatabaseManagerOperationMode mode)
{
    return setSystemParameter(SystemParameterType::DB_OPERATION_MODE, mode);
}

SystemDataContainerPtr SyncServer_Core::DatabaseManager::Functions_System::getSystemParameter(SystemParameterType type)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    SystemDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSystemParameter/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSystemParameter/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSystemParameter/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSystemParameter/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::SystemDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSystemParameter/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSystemParameter/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSystemParameter/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSystemParameter/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSystemParameter/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSystemParameter/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->systemTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->systemTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->systemTableDALs->addSelectRequest(DatabaseSelectConstraints::SYSTEM::LIMIT_BY_TYPE, type);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSystemParameter/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSystemParameter/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<SystemDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_System::getAllSystemparameters()
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllSystemparameters/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllSystemparameters/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllSystemparameters/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllSystemparameters/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getAllSystemparameters/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllSystemparameters/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllSystemparameters/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllSystemparameters/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllSystemparameters/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getAllSystemparameters/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->systemTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->systemTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->systemTableDALs->addSelectRequest(DatabaseSelectConstraints::SYSTEM::GET_ALL, 0);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getAllSystemparameters/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<SystemDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::SystemDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getAllSystemparameters/END> ["+Convert::toString(requestID)+"]");
    return result;
}

IPAddress SyncServer_Core::DatabaseManager::Functions_System::getDataIPAddress()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DATA_IP_ADDRESS);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DATA_IP_ADDRESS)
        return boost::any_cast<IPAddress>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_IP_ADDRESS;
}

IPPort SyncServer_Core::DatabaseManager::Functions_System::getDataPort()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DATA_IP_PORT);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DATA_IP_PORT)
        return boost::any_cast<IPPort>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_IP_PORT;
}

IPAddress SyncServer_Core::DatabaseManager::Functions_System::getCommandIPAddress()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::COMMAND_IP_ADDRESS);
    
    if(container || container->getSystemParameterType() != SystemParameterType::COMMAND_IP_ADDRESS)
        return boost::any_cast<IPAddress>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_IP_ADDRESS;
}

IPPort SyncServer_Core::DatabaseManager::Functions_System::getCommandPort()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::COMMAND_IP_PORT);
    
    if(container || container->getSystemParameterType() != SystemParameterType::COMMAND_IP_PORT)
        return boost::any_cast<IPPort>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_IP_PORT;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getForceCommandEncryption()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::FORCE_COMMAND_ENCRYPTION);
    
    if(container || container->getSystemParameterType() != SystemParameterType::FORCE_COMMAND_ENCRYPTION)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getForceDataEncryption()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::FORCE_DATA_ENCRYPTION);
    
    if(container || container->getSystemParameterType() != SystemParameterType::FORCE_DATA_ENCRYPTION)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getForceDataCompression()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::FORCE_DATA_COMPRESSION);
    
    if(container || container->getSystemParameterType() != SystemParameterType::FORCE_DATA_COMPRESSION)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

DataPoolSize SyncServer_Core::DatabaseManager::Functions_System::getPendingDataPoolSize()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::PENDING_DATA_POOL_SIZE);
    
    if(container || container->getSystemParameterType() != SystemParameterType::PENDING_DATA_POOL_SIZE)
        return boost::any_cast<DataPoolSize>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_DATA_POOL_SIZE;
}

DataPoolPath SyncServer_Core::DatabaseManager::Functions_System::getPendingDataPoolPath()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::PENDING_DATA_POOL_PATH);
    
    if(container || container->getSystemParameterType() != SystemParameterType::PENDING_DATA_POOL_PATH)
        return boost::any_cast<DataPoolPath>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_DATA_POOL_PATH;
}

DataPoolRetention SyncServer_Core::DatabaseManager::Functions_System::getPendingDataPoolRetention()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::PENDING_DATA_RETENTION);
    
    if(container || container->getSystemParameterType() != SystemParameterType::PENDING_DATA_RETENTION)
        return boost::any_cast<DataPoolRetention>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_DATA_POOL_RETENTION;
}

DataPoolSize SyncServer_Core::DatabaseManager::Functions_System::getInMemoryDataPoolSize()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::IN_MEMORY_POOL_SIZE);
    
    if(container || container->getSystemParameterType() != SystemParameterType::IN_MEMORY_POOL_SIZE)
        return boost::any_cast<DataPoolSize>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_DATA_POOL_SIZE;
}

DataPoolRetention SyncServer_Core::DatabaseManager::Functions_System::getInMemoryDataPoolRetention()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::IN_MEMORY_POOL_RETENTION);
    
    if(container || container->getSystemParameterType() != SystemParameterType::IN_MEMORY_POOL_RETENTION)
        return boost::any_cast<DataPoolRetention>(container->getSystemParameterValue());
    else
        return Common_Types::INVALID_DATA_POOL_RETENTION;
}

string SyncServer_Core::DatabaseManager::Functions_System::getSupportedProtocols()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::SUPPORTED_PROTOCOLS);
    
    if(container || container->getSystemParameterType() != SystemParameterType::SUPPORTED_PROTOCOLS)
        return boost::any_cast<string>(container->getSystemParameterValue());
    else
        return "";
}

unsigned int SyncServer_Core::DatabaseManager::Functions_System::getCommandResendRetries()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::COMMAND_RETRIES_MAX);
    
    if(container || container->getSystemParameterType() != SystemParameterType::COMMAND_RETRIES_MAX)
        return boost::any_cast<unsigned int>(container->getSystemParameterValue());
    else
        return 0;
}

unsigned int SyncServer_Core::DatabaseManager::Functions_System::getDataResendRetries()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DATA_RETRIES_MAX);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DATA_RETRIES_MAX)
        return boost::any_cast<unsigned int>(container->getSystemParameterValue());
    else
        return 0;
}

unsigned long SyncServer_Core::DatabaseManager::Functions_System::getSessionTimeout()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::SESSION_TIMEOUT);
    
    if(container || container->getSystemParameterType() != SystemParameterType::SESSION_TIMEOUT)
        return boost::any_cast<unsigned long>(container->getSystemParameterValue());
    else
        return 0;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getSessionKeepAliveState()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::SESSION_KEEP_ALIVE);
    
    if(container || container->getSystemParameterType() != SystemParameterType::SESSION_KEEP_ALIVE)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getMinimizeMemoryUsageState()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::MINIMIZE_MEMORY_USAGE);
    
    if(container || container->getSystemParameterType() != SystemParameterType::MINIMIZE_MEMORY_USAGE)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

bool SyncServer_Core::DatabaseManager::Functions_System::getDBImmediateLogFlushState()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DB_IMMEDIATE_FLUSH);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DB_IMMEDIATE_FLUSH)
        return boost::any_cast<bool>(container->getSystemParameterValue());
    else
        return false;
}

unsigned long SyncServer_Core::DatabaseManager::Functions_System::getDBCacheFlushInterval()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DB_CACHE_FLUSH_INTERVAL);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DB_CACHE_FLUSH_INTERVAL)
        return boost::any_cast<unsigned long>(container->getSystemParameterValue());
    else
        return 0;
}

DatabaseManagerOperationMode SyncServer_Core::DatabaseManager::Functions_System::getDBOperationMode()
{
    SystemDataContainerPtr container = getSystemParameter(SystemParameterType::DB_OPERATION_MODE);
    
    if(container || container->getSystemParameterType() != SystemParameterType::DB_OPERATION_MODE)
        return boost::any_cast<DatabaseManagerOperationMode>(container->getSystemParameterValue());
    else
        return DatabaseManagerOperationMode::INVALID;
}

//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_SyncFiles">
SyncServer_Core::DatabaseManager::Functions_SyncFiles::Functions_SyncFiles(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_SyncFiles::~Functions_SyncFiles()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_SyncFiles::addSync(const SyncDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->syncFilesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->syncFilesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->syncFilesTableDALs->addInsertRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSync/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSync/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_SyncFiles::updateSync(const SyncDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->syncFilesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->syncFilesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->syncFilesTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSync/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSync/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_SyncFiles::removeSync(SyncID sync)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->syncFilesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->syncFilesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->syncFilesTableDALs->addDeleteRequest(DBObjectID(sync));
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeSync/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeSync/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

SyncDataContainerPtr SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSync(SyncID sync)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    SyncDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::SyncDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSync/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSync/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSync/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSync/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSync/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->syncFilesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->syncFilesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->syncFilesTableDALs->addSelectRequest(DatabaseSelectConstraints::SYNC::LIMIT_BY_ID, sync);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSync/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSync/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByConstraint(DatabaseSelectConstraints::SYNC constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSyncsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSyncsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSyncsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSyncsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSyncsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSyncsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSyncsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSyncsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSyncsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSyncsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->syncFilesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->syncFilesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->syncFilesTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSyncsByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<SyncDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
        {
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::SyncDataContainer>(currentContainer));
        }
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSyncsByConstraint/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncs()
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::GET_ALL, 0);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByOwner(UserID owner)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_OWNER, owner);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByDevice(DeviceID device)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_DEVICE, device);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByPath(string path)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_PATH, path);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByEncryption(bool enabled)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_ENCRYPTION, enabled);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByCompression(bool enabled)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_COMPRESSION, enabled);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByOfflineSynchronisation(bool enabled)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_OFFLINE_SYNC, enabled);
}

vector<SyncDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_SyncFiles::getSyncsByDifferentialSynchronisation(bool enabled)
{
    return getSyncsByConstraint(DatabaseSelectConstraints::SYNC::LIMIT_BY_DIFFERENTIAL_SYNC, enabled);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_Devices">
SyncServer_Core::DatabaseManager::Functions_Devices::Functions_Devices(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Devices::~Functions_Devices()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Devices::addDevice(const DeviceDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->devicesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->devicesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->devicesTableDALs->addInsertRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addDevice/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addDevice/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Devices::updateDevice(const DeviceDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->devicesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->devicesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->devicesTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateDevice/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateDevice/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Devices::removeDevice(DeviceID device)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->devicesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->devicesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->devicesTableDALs->addDeleteRequest(DBObjectID(device));
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeDevice/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeDevice/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

DeviceDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Devices::getDevice(DeviceID device)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    DeviceDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::DeviceDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevice/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevice/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevice/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevice/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevice/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->devicesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->devicesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->devicesTableDALs->addSelectRequest(DatabaseSelectConstraints::DEVICES::LIMIT_BY_ID, device);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getDevice/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getDevice/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<DeviceDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Devices::getDevicesByConstraint(DatabaseSelectConstraints::DEVICES constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevicesByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevicesByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevicesByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevicesByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getDevicesByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevicesByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevicesByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevicesByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevicesByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getDevicesByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->devicesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->devicesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->devicesTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getDevicesByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<DeviceDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::DeviceDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getDevicesByConstraint/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<DeviceDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Devices::getDevices()
{
    return getDevicesByConstraint(DatabaseSelectConstraints::DEVICES::GET_ALL, 0);
}

vector<DeviceDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Devices::getDevicesByTransferType(DataTransferType xferType)
{
    return getDevicesByConstraint(DatabaseSelectConstraints::DEVICES::LIMIT_BY_TRANSFER_TYPE, xferType);
}

vector<DeviceDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Devices::getDevicesByOwner(UserID owner)
{
    return getDevicesByConstraint(DatabaseSelectConstraints::DEVICES::LIMIT_BY_OWNER, owner);
}

vector<DeviceDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Devices::getDevicesByIPAddress(IPAddress address)
{
    return getDevicesByConstraint(DatabaseSelectConstraints::DEVICES::LIMIT_BY_ADDRESS, address);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_Schedules">
SyncServer_Core::DatabaseManager::Functions_Schedules::Functions_Schedules(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Schedules::~Functions_Schedules()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Schedules::addSchedule(const ScheduleDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->schedulesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->schedulesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->schedulesTableDALs->addInsertRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSchedule/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSchedule/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Schedules::updateSchedule(const ScheduleDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->schedulesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->schedulesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->schedulesTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSchedule/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSchedule/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Schedules::removeSchedule(ScheduleID schedule)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->schedulesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->schedulesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->schedulesTableDALs->addDeleteRequest(DBObjectID(schedule));
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeSchedule/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeSchedule/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

ScheduleDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Schedules::getSchedule(ScheduleID schedule)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    ScheduleDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::ScheduleDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedule/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedule/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedule/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedule/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedule/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->schedulesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->schedulesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->schedulesTableDALs->addSelectRequest(DatabaseSelectConstraints::SCHEDULES::LIMIT_BY_ID, schedule);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSchedule/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSchedule/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<ScheduleDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Schedules::getSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedulesByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedulesByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedulesByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedulesByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSchedulesByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedulesByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedulesByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedulesByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedulesByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSchedulesByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->schedulesTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->schedulesTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->schedulesTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSchedulesByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<ScheduleDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::ScheduleDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSchedulesByConstraint/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<ScheduleDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Schedules::getSchedules()
{
    return getSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES::GET_ALL, 0);
}

vector<ScheduleDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Schedules::getSchedulesByState(bool active)
{
    return getSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES::LIMIT_BY_STATE, active);
}

vector<ScheduleDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Schedules::getSchedulesBySyncID(SyncID sync)
{
    return getSchedulesByConstraint(DatabaseSelectConstraints::SCHEDULES::LIMIT_BY_SYNC, sync);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_Users">
SyncServer_Core::DatabaseManager::Functions_Users::Functions_Users(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Users::~Functions_Users()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Users::addUser(const UserDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addUser/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addUser/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addUser/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addUser/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addUser/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addUser/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addUser/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addUser/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addUser/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addUser/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addInsertRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addUser/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addUser/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Users::updateUser(const UserDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateUser/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateUser/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateUser/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateUser/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateUser/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateUser/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateUser/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateUser/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateUser/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateUser/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateUser/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateUser/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Users::removeUser(UserID user)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    { 
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeUser_I/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeUser_I/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeUser_I/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeUser_I/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <removeUser_I/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeUser_I/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeUser_I/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeUser_I/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeUser_I/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <removeUser_I/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addDeleteRequest(DBObjectID(user));
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeUser_I/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <removeUser_I/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

UserDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Users::getUser(string username)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    UserDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_U/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_U/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_U/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_U/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::UserDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_U/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_U/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_U/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_U/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_U/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_U/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addSelectRequest(DatabaseSelectConstraints::USERS::LIMIT_BY_NAME, username);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUser_U/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUser_U/END> ["+Convert::toString(requestID)+"]");
    return result;
}

UserDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Users::getUser(UserID user)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    UserDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_I/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_I/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_I/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_I/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::UserDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUser_I/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_I/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_I/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_I/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_I/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUser_I/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addSelectRequest(DatabaseSelectConstraints::USERS::LIMIT_BY_ID, user);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUser_I/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUser_I/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<UserDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Users::getUsersByConstraint(DatabaseSelectConstraints::USERS constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUsersByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUsersByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUsersByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUsersByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getUsersByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUsersByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUsersByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUsersByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUsersByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getUsersByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->usersTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->usersTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->usersTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUsersByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<UserDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::UserDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getUsersByConstraint/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<UserDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Users::getUsers()
{
    return getUsersByConstraint(DatabaseSelectConstraints::USERS::GET_ALL, 0);
}

vector<UserDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Users::getUsersByAccessLevel(UserAccessLevel level)
{
    return getUsersByConstraint(DatabaseSelectConstraints::USERS::LIMIT_BY_ACCESS_LEVEL, level);
}

vector<UserDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Users::getUsersByLockedState(bool isUserLocked)
{
    return getUsersByConstraint(DatabaseSelectConstraints::USERS::LIMIT_BY_LOCKED_STATE, isUserLocked);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_Logs">
SyncServer_Core::DatabaseManager::Functions_Logs::Functions_Logs(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Logs::~Functions_Logs()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Logs::addLog(const LogDataContainerPtr log)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addLog/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addLog/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addLog/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addLog/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addLog/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addLog/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addLog/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addLog/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addLog/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addLog/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->logsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->logsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->logsTableDALs->addInsertRequest(log);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addLog/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addLog/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

DatabaseRequestID SyncServer_Core::DatabaseManager::Functions_Logs::addLogAsync(const LogDataContainerPtr log)
{
    return parentManager->logsTableDALs->addInsertRequest(boost::dynamic_pointer_cast<DatabaseManagement_Containers::DataContainer>(log));
}

LogDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Logs::getLog(LogID log)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    LogDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLog/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLog/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLog/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLog/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::LogDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLog/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLog/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLog/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLog/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLog/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLog/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->logsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->logsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->logsTableDALs->addSelectRequest(DatabaseSelectConstraints::LOGS::LIMIT_BY_ID, log);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getLog/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getLog/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<LogDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Logs::getLogsByConstraint(DatabaseSelectConstraints::LOGS constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLogsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLogsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLogsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getLogsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getLogsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    
    onSuccessConnection = parentManager->logsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->logsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->logsTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getLogsByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<LogDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::LogDataContainer>(currentContainer));
    }
    
    return result;
}

vector<LogDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Logs::getLogs()
{
    return getLogsByConstraint(DatabaseSelectConstraints::LOGS::GET_ALL, 0);
}

vector<LogDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Logs::getLogsBySeverity(LogSeverity severity)
{
    return getLogsByConstraint(DatabaseSelectConstraints::LOGS::LIMIT_BY_SEVERITY, severity);
}

vector<LogDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Logs::getLogsBySource(string source)
{
    return getLogsByConstraint(DatabaseSelectConstraints::LOGS::LIMIT_BY_SOURCE, source);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Functions_Sessions">
SyncServer_Core::DatabaseManager::Functions_Sessions::Functions_Sessions(DatabaseManager & parent)
{
    parentManager = &parent;
}

SyncServer_Core::DatabaseManager::Functions_Sessions::~Functions_Sessions()
{
    releaseLocks = true;
}

bool SyncServer_Core::DatabaseManager::Functions_Sessions::addSession(const SessionDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <addSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <addSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->sessionsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->sessionsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->sessionsTableDALs->addInsertRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSession/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <addSession/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

bool SyncServer_Core::DatabaseManager::Functions_Sessions::updateSession(const SessionDataContainerPtr data)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};    //
    bool successful = false;        //result from operation
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <updateSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            successful = false;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <updateSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->sessionsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->sessionsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->sessionsTableDALs->addUpdateRequest(data);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSession/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <updateSession/END> ["+Convert::toString(requestID)+"]");
    return successful;
}

SessionDataContainerPtr SyncServer_Core::DatabaseManager::Functions_Sessions::getSession(SessionID session)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    SessionDataContainerPtr result;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            result = boost::dynamic_pointer_cast<DatabaseManagement_Containers::SessionDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSession/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSession/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSession/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSession/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
        
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSession/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->sessionsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->sessionsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->sessionsTableDALs->addSelectRequest(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_ID, session);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSession/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSession/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS constraintType, boost::any constraintValue)
{
    std::atomic<DatabaseRequestID> requestID {DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID};
    boost::signals2::connection onFailreConnection, onSuccessConnection;
    boost::condition_variable resultCondition;
    boost::mutex resultMutex;
    std::atomic<bool> resultReceived {false};
    VectorDataContainerPtr containersWrapper;
    
    std::function<void(DatabaseRequestID, DataContainerPtr)> onSuccessHandler = [&](DatabaseRequestID id, DataContainerPtr data)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSessionsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSessionsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSessionsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSessionsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            containersWrapper = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(data);
            resultReceived = true;
            resultCondition.notify_all();
        }
            
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onSuccess <getSessionsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    std::function<void(DatabaseRequestID, DBObjectID)> onFailureHandler = [&](DatabaseRequestID id, DBObjectID)
    {
        boost::lock_guard<boost::mutex> resultLock(resultMutex);
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSessionsByConstraint/OUT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        while(requestID == DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID)
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSessionsByConstraint/SPINLOCK> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");;
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSessionsByConstraint/MID> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
        
        if(id == requestID)
        {
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSessionsByConstraint/IN> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
            
            resultReceived = true;
            resultCondition.notify_all();
        }
            
        parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> onFailure <getSessionsByConstraint/EXT> ["+Convert::toString(id)+"]|["+Convert::toString(requestID)+"]");
    };
    
    onSuccessConnection = parentManager->sessionsTableDALs->onSuccessEventAttach(onSuccessHandler);
    onFailreConnection = parentManager->sessionsTableDALs->onFailureEventAttach(onFailureHandler);
    
    requestID = parentManager->sessionsTableDALs->addSelectRequest(constraintType, constraintValue);
    
    {
        boost::unique_lock<boost::mutex> resultLock(resultMutex);
        boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(parentManager->functionCallTimeout);
        while(!resultReceived && !releaseLocks && resultCondition.timed_wait(resultLock, nextWakeup))
            parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSessionsByConstraint/TLOCK> ["+Convert::toString(requestID)+"]");
    }
    
    onSuccessConnection.disconnect();
    onFailreConnection.disconnect();
    
    vector<SessionDataContainerPtr> result;
    
    if(containersWrapper)
    {
        for(DataContainerPtr currentContainer : containersWrapper->getContainers())
            result.push_back(boost::dynamic_pointer_cast<DatabaseManagement_Containers::SessionDataContainer>(currentContainer));
    }
    
    parentManager->logger->logMessage(FileLogSeverity::Warning, ">>> <getSessionsByConstraint/END> ["+Convert::toString(requestID)+"]");
    return result;
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getSessions()
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::GET_ALL, 0);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getSessionsByType(SessionType type)
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_TYPE, type);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getSessionsByDevice(DeviceID device)
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_DEVICE, device);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getSessionsByUser(UserID user)
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_USER, user);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getActiveSessions()
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_STATE, true);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getInactiveSessions()
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_STATE, false);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getPersistentSessions()
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_PERSISTENCY, true);
}

vector<SessionDataContainerPtr> SyncServer_Core::DatabaseManager::Functions_Sessions::getTemporarySessions()
{
    return getSessionsByConstraint(DatabaseSelectConstraints::SESSIONS::LIMIT_BY_PERSISTENCY, false);
}
//</editor-fold>

bool SyncServer_Core::DatabaseManager::registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<DatabaseManagerInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::ADMIN);
        
        try
        {
            //<editor-fold defaultstate="collapsed" desc="CORE Instructions">
            auto getQueuesListHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                std::vector<DALQueue::DALQueueInformation> resultData;
                resultData.push_back(getQueueInformation(DatabaseObjectType::DEVICE));
                resultData.push_back(getQueueInformation(DatabaseObjectType::LOG));
                resultData.push_back(getQueueInformation(DatabaseObjectType::SCHEDULE));
                resultData.push_back(getQueueInformation(DatabaseObjectType::SESSION));
                resultData.push_back(getQueueInformation(DatabaseObjectType::STATISTICS));
                resultData.push_back(getQueueInformation(DatabaseObjectType::SYNC_FILE));
                resultData.push_back(getQueueInformation(DatabaseObjectType::SYSTEM_SETTINGS));
                resultData.push_back(getQueueInformation(DatabaseObjectType::USER));
                auto result = boost::shared_ptr<InstructionResults::GetQueuesList> (new InstructionResults::GetQueuesList{resultData});
                instruction->getPromise().set_value(result);
            };
            
            auto getCachesListHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                std::vector<DALCache::DALCacheInformation> resultData;
                std::vector<DALCache::DALCacheInformation> currentResult;
                currentResult = getCachesInformation(DatabaseObjectType::DEVICE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::LOG);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::SCHEDULE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::SESSION);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::STATISTICS);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::SYNC_FILE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::SYSTEM_SETTINGS);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getCachesInformation(DatabaseObjectType::USER);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                auto result = boost::shared_ptr<InstructionResults::GetCachesList> (new InstructionResults::GetCachesList{resultData});
                instruction->getPromise().set_value(result);
            };
            
            auto getDALsListHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                std::vector<DALQueue::DALInformation> resultData;
                std::vector<DALQueue::DALInformation> currentResult;
                currentResult = getDALsInformation(DatabaseObjectType::DEVICE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::LOG);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::SCHEDULE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::SESSION);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::STATISTICS);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::SYNC_FILE);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::SYSTEM_SETTINGS);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                currentResult = getDALsInformation(DatabaseObjectType::USER);
                resultData.insert(resultData.end(), currentResult.begin(), currentResult.end());
                auto result = boost::shared_ptr<InstructionResults::GetDALsList> (new InstructionResults::GetDALsList{resultData});
                instruction->getPromise().set_value(result);
            };
            
            auto setDefaultCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::SetDefaultDALCacheParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::SetDefaultDALCacheParameters>(instruction);
                
                if(actualInstruction)
                {
                    setDefaultCacheParameters(actualInstruction->parameters);
                    resultValue = true;
                }
                
                auto result = boost::shared_ptr<InstructionResults::SetDefaultDALCacheParameters> (new InstructionResults::SetDefaultDALCacheParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getDefaultCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetDefaultDALCacheParameters> (new InstructionResults::GetDefaultDALCacheParameters{getDefaultCacheParameters()});
                instruction->getPromise().set_value(result);
            };
            
            auto setCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::SetCacheParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::SetCacheParameters>(instruction);
                
                if(actualInstruction)
                    resultValue = setCacheParameters(actualInstruction->queueType, actualInstruction->cacheID, actualInstruction->parameters);
                
                auto result = boost::shared_ptr<InstructionResults::SetCacheParameters> (new InstructionResults::SetCacheParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                DALCache::DALCacheParameters resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetCacheParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetCacheParameters>(instruction);
                
                if(actualInstruction)
                    resultValue = getCacheParameters(actualInstruction->queueType, actualInstruction->cacheID);
                
                auto result = boost::shared_ptr<InstructionResults::GetCacheParameters> (new InstructionResults::GetCacheParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto setQueueParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::SetQueueParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::SetQueueParameters>(instruction);
                
                if(actualInstruction)
                    resultValue = setQueueParameters(actualInstruction->queueType, actualInstruction->parameters);
                
                auto result = boost::shared_ptr<InstructionResults::SetQueueParameters> (new InstructionResults::SetQueueParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getQueueParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                DALQueue::DALQueueParameters resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetQueueParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetQueueParameters>(instruction);
                
                if(actualInstruction)
                    resultValue = getQueueParameters(actualInstruction->queueType);
                
                auto result = boost::shared_ptr<InstructionResults::GetQueueParameters> (new InstructionResults::GetQueueParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto setFunctionTimeoutHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::SetFunctionTimeout> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::SetFunctionTimeout>(instruction);
                
                if(actualInstruction)
                {
                    setFunctionCallTimeout(actualInstruction->timeout);
                    resultValue = true;
                }
                
                auto result = boost::shared_ptr<InstructionResults::SetFunctionTimeout> (new InstructionResults::SetFunctionTimeout{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getFunctionTimeoutHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetFunctionTimeout> (new InstructionResults::GetFunctionTimeout{getFunctionCallTimeout()});
                instruction->getPromise().set_value(result);
            };
            
            auto addDALHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddDAL> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddDAL>(instruction);
                
                if(actualInstruction)
                    resultValue = addDAL(actualInstruction->newDAL, actualInstruction->enableCache);
                
                auto result = boost::shared_ptr<InstructionResults::AddDAL> (new InstructionResults::AddDAL{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto addDALWithCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddDALWithCacheParameters> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddDALWithCacheParameters>(instruction);
                
                if(actualInstruction)
                    resultValue = addDAL(actualInstruction->newDAL, actualInstruction->parameters);
                
                auto result = boost::shared_ptr<InstructionResults::AddDALWithCacheParameters> (new InstructionResults::AddDALWithCacheParameters{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto removeDALHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveDAL> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveDAL>(instruction);
                
                if(actualInstruction)
                    resultValue = removeDAL(actualInstruction->DALToRemove);
                
                auto result = boost::shared_ptr<InstructionResults::RemoveDAL> (new InstructionResults::RemoveDAL{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_STATISTICS Instructions">
            auto getSystemInstallTimestampHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetSystemInstallTimestamp> (new InstructionResults::GetSystemInstallTimestamp{Statistics().getSystemInstallTimestamp()});
                instruction->getPromise().set_value(result);
            };
            
            auto getSystemStartTimestampHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetSystemStartTimestamp> (new InstructionResults::GetSystemStartTimestamp{Statistics().getSystemStartTimestamp()});
                instruction->getPromise().set_value(result);
            };
            
            auto getTotalTransferredDataHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetTotalTransferredData> (new InstructionResults::GetTotalTransferredData{Statistics().getTotalTransferredData()});
                instruction->getPromise().set_value(result);
            };
            
            auto getTotalTransferredFilesHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetTotalTransferredFiles> (new InstructionResults::GetTotalTransferredFiles{Statistics().getTotalNumberOfTransferredFiles()});
                instruction->getPromise().set_value(result);
            };
            
            auto getTotalFailedTransfersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetTotalFailedTransfers> (new InstructionResults::GetTotalFailedTransfers{Statistics().getTotalNumberOfFailedTransfers()});
                instruction->getPromise().set_value(result);
            };
            
            auto getTotalRetriedTransfersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetTotalRetriedTransfers> (new InstructionResults::GetTotalRetriedTransfers{Statistics().getTotalNumberOfRetriedTransfers()});
                instruction->getPromise().set_value(result);
            };
            
            auto getAllStatsHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                auto result = boost::shared_ptr<InstructionResults::GetAllStats> (new InstructionResults::GetAllStats{Statistics().getAllStatistics()});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>

            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYSTEM Instructions">
            //TODO
            //</editor-fold>

            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SYNC_FILES Instructions">
            auto addSyncHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddSync> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddSync>(instruction);
                
                if(actualInstruction)
                    resultValue = SyncFiles().addSync(actualInstruction->syncData);
                
                auto result = boost::shared_ptr<InstructionResults::AddSync> (new InstructionResults::AddSync{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto removeSyncHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveSync> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveSync>(instruction);
                
                if(actualInstruction)
                    resultValue = SyncFiles().removeSync(actualInstruction->syncID);
                
                auto result = boost::shared_ptr<InstructionResults::RemoveSync> (new InstructionResults::RemoveSync{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto updateSyncHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateSync> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateSync>(instruction);
                
                if(actualInstruction)
                    resultValue = SyncFiles().updateSync(actualInstruction->syncData);
                
                auto result = boost::shared_ptr<InstructionResults::UpdateSync> (new InstructionResults::UpdateSync{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getSyncsByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<SyncDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSyncsByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSyncsByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = SyncFiles().getSyncsByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetSyncsByConstraint> (new InstructionResults::GetSyncsByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getSyncHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                SyncDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSync> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSync>(instruction);
                
                if(actualInstruction)
                    resultValue = SyncFiles().getSync(actualInstruction->syncID);
                
                auto result = boost::shared_ptr<InstructionResults::GetSync> (new InstructionResults::GetSync{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>
        
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_DEVICES Instructions">
            auto addDeviceHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddDevice> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddDevice>(instruction);
                
                if(actualInstruction)
                    resultValue = Devices().addDevice(actualInstruction->deviceData);
                
                auto result = boost::shared_ptr<InstructionResults::AddDevice> (new InstructionResults::AddDevice{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto removeDeviceHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveDevice> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveDevice>(instruction);
                
                if(actualInstruction)
                    resultValue = Devices().removeDevice(actualInstruction->deviceID);
                
                auto result = boost::shared_ptr<InstructionResults::RemoveDevice> (new InstructionResults::RemoveDevice{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto updateDeviceHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateDevice> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateDevice>(instruction);
                
                if(actualInstruction)
                    resultValue = Devices().updateDevice(actualInstruction->deviceData);
                
                auto result = boost::shared_ptr<InstructionResults::UpdateDevice> (new InstructionResults::UpdateDevice{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getDevicesByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<DeviceDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetDevicesByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetDevicesByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = Devices().getDevicesByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetDevicesByConstraint> (new InstructionResults::GetDevicesByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getDeviceHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                DeviceDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetDevice> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetDevice>(instruction);
                
                if(actualInstruction)
                    resultValue = Devices().getDevice(actualInstruction->deviceID);
                
                auto result = boost::shared_ptr<InstructionResults::GetDevice> (new InstructionResults::GetDevice{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>

            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SCHEDULES Instructions">
            auto addScheduleHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddSchedule> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddSchedule>(instruction);
                
                if(actualInstruction)
                    resultValue = Schedules().addSchedule(actualInstruction->scheduleData);
                
                auto result = boost::shared_ptr<InstructionResults::AddSchedule> (new InstructionResults::AddSchedule{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto removeScheduleHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveSchedule> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveSchedule>(instruction);
                
                if(actualInstruction)
                    resultValue = Schedules().removeSchedule(actualInstruction->scheduleID);
                
                auto result = boost::shared_ptr<InstructionResults::RemoveSchedule> (new InstructionResults::RemoveSchedule{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto updateScheduleHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateSchedule> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateSchedule>(instruction);
                
                if(actualInstruction)
                    resultValue = Schedules().updateSchedule(actualInstruction->scheduleData);
                
                auto result = boost::shared_ptr<InstructionResults::UpdateSchedule> (new InstructionResults::UpdateSchedule{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getSchedulesByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<ScheduleDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSchedulesByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSchedulesByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = Schedules().getSchedulesByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetSchedulesByConstraint> (new InstructionResults::GetSchedulesByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getScheduleHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                ScheduleDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSchedule> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSchedule>(instruction);
                
                if(actualInstruction)
                    resultValue = Schedules().getSchedule(actualInstruction->scheduleID);
                
                auto result = boost::shared_ptr<InstructionResults::GetSchedule> (new InstructionResults::GetSchedule{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>

            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_USERS Instructions">
            auto addUserHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddUser> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddUser>(instruction);
                
                if(actualInstruction)
                    resultValue = Users().addUser(actualInstruction->userData);
                
                auto result = boost::shared_ptr<InstructionResults::AddUser> (new InstructionResults::AddUser{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto removeUserHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveUser> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::RemoveUser>(instruction);
                
                if(actualInstruction)
                    resultValue = Users().removeUser(actualInstruction->userID);
                
                auto result = boost::shared_ptr<InstructionResults::RemoveUser> (new InstructionResults::RemoveUser{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto updateUserHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateUser> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::UpdateUser>(instruction);
                
                if(actualInstruction)
                    resultValue = Users().updateUser(actualInstruction->userData);
                
                auto result = boost::shared_ptr<InstructionResults::UpdateUser> (new InstructionResults::UpdateUser{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getUsersByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<UserDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetUsersByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetUsersByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = Users().getUsersByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetUsersByConstraint> (new InstructionResults::GetUsersByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getUserHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                UserDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetUser> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetUser>(instruction);
                
                if(actualInstruction)
                    resultValue = (actualInstruction->idSet) ? Users().getUser(actualInstruction->userID) : Users().getUser(actualInstruction->username);
                
                auto result = boost::shared_ptr<InstructionResults::GetUser> (new InstructionResults::GetUser{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>
        
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_LOGS Instructions">
            auto addLogHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddLog> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddLog>(instruction);
                
                if(actualInstruction)
                    resultValue = Logs().addLog(actualInstruction->logData);
                
                auto result = boost::shared_ptr<InstructionResults::AddLog> (new InstructionResults::AddLog{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto addLogAsyncHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                bool resultValue = false;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::AddLogAsync> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::AddLogAsync>(instruction);
                
                if(actualInstruction)
                    resultValue = Logs().addLogAsync(actualInstruction->logData);
                
                auto result = boost::shared_ptr<InstructionResults::AddLogAsync> (new InstructionResults::AddLogAsync{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getLogsByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<LogDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetLogsByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetLogsByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = Logs().getLogsByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetLogsByConstraint> (new InstructionResults::GetLogsByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getLogHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                LogDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetLog> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetLog>(instruction);
                
                if(actualInstruction)
                    resultValue = Logs().getLog(actualInstruction->logID);
                
                auto result = boost::shared_ptr<InstructionResults::GetLog> (new InstructionResults::GetLog{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>
        
            //<editor-fold defaultstate="collapsed" desc="FUNCTIONS_SESSIONS Instructions">
            auto getSessionsByConstraintHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                vector<SessionDataContainerPtr> resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSessionsByConstraint> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSessionsByConstraint>(instruction);
                
                if(actualInstruction)
                    resultValue = Sessions().getSessionsByConstraint(actualInstruction->constraintType, actualInstruction->constraintValue);
                
                auto result = boost::shared_ptr<InstructionResults::GetSessionsByConstraint> (new InstructionResults::GetSessionsByConstraint{resultValue});
                instruction->getPromise().set_value(result);
            };
            
            auto getSessionHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)
            {
                SessionDataContainerPtr resultValue;
                boost::shared_ptr<InstructionManagement_Sets::DatabaseManagerInstructions::GetSession> actualInstruction =
                        boost::dynamic_pointer_cast<InstructionManagement_Sets::DatabaseManagerInstructions::GetSession>(instruction);
                
                if(actualInstruction)
                    resultValue = Sessions().getSession(actualInstruction->sessionID);
                
                auto result = boost::shared_ptr<InstructionResults::GetSession> (new InstructionResults::GetSession{resultValue});
                instruction->getPromise().set_value(result);
            };
            //</editor-fold>
            
            //Core Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_QUEUES_LIST, getQueuesListHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_CACHES_LIST, getCachesListHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_DALS_LIST, getDALsListHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS, getDefaultCacheParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS, setDefaultCacheParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::SET_CACHE_PARAMS, setCacheParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_CACHE_PARAMS, getCacheParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::SET_QUEUE_PARAMS, setQueueParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_QUEUE_PARAMS, getQueueParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT, setFunctionTimeoutHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT, getFunctionTimeoutHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_DAL, addDALHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS, addDALWithCacheParametersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::REMOVE_DAL, removeDALHandler);

            //FUNCTIONS_STATISTICS Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP, getSystemInstallTimestampHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP, getSystemStartTimestampHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA, getTotalTransferredDataHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES, getTotalTransferredFilesHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS, getTotalFailedTransfersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS, getTotalRetriedTransfersHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_ALL_STATS, getAllStatsHandler);
            
            //FUNCTIONS_SYSTEM Instructions
            //TODO
            
            //FUNCTIONS_SYNC Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_SYNC, addSyncHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::REMOVE_SYNC, removeSyncHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::UPDATE_SYNC, updateSyncHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT, getSyncsByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SYNC, getSyncHandler);
            
            //FUNCTIONS_DEVICES Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_DEVICE, addDeviceHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::REMOVE_DEVICE, removeDeviceHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::UPDATE_DEVICE, updateDeviceHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT, getDevicesByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_DEVICE, getDeviceHandler);
            
            //FUNCTIONS_SCHEDULES Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_SCHEDULE, addScheduleHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::REMOVE_SCHEDULE, removeScheduleHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::UPDATE_SCHEDULE, updateScheduleHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT, getSchedulesByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SCHEDULE, getScheduleHandler);
            
            //FUNCTIONS_USERS Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_USER, addUserHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::REMOVE_USER, removeUserHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::UPDATE_USER, updateUserHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT, getUsersByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_USER, getUserHandler);
            
            //FUNCTIONS_LOGS Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_LOG, addLogHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::ADD_LOG_ASYNC, addLogAsyncHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT, getLogsByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_LOG, getLogHandler);
            
            //FUNCTIONS_SESSIONS Instructions
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT, getSessionsByConstraintHandler);
            set->bindInstructionHandler(DatabaseManagerInstructionType::GET_SESSION, getSessionHandler);
            
            return true;
        }
        catch(std::invalid_argument & ex)
        {
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager (registerInstructionSet) > Exception encountered: <" + std::string(ex.what()) + ">");
            return false;
        }
    }
    else
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager (registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}