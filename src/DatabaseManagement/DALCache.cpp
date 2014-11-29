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

#include "DALCache.h"

SyncServer_Core::DatabaseManagement::DALCache::DALCache(DALPtr childDAL, Utilities::FileLogger& parentLogger, DALCacheParameters parameters)
{
    dal = childDAL;
    logger = &parentLogger;
    
    //TODO - move to initializer list
    alwaysEvict = parameters.alwaysEvictObjects;
    clearObjectAge = parameters.alwaysClearObjectAge;
    cacheSize = parameters.maximumCacheSize;
    maxCommitTime = parameters.maximumCommitTime;
    maxCommitUpdates = parameters.maximumCommitUpdates;
    minCommitUpdates = parameters.minimumCommitUpdates;
    
    cacheEnabled = true;
    cacheType = dal->getType();
    
    onSuccessConnection = dal->onSuccessEventAttach(boost::bind(&DatabaseManagement::DALCache::onSuccessHandler, this, _1, _2, _3));
    onFailureConnection = dal->onFailureEventAttach(boost::bind(&DatabaseManagement::DALCache::onFailureHandler, this, _1, _2, _3));
    
    requestsThreadObject = new boost::thread(&DatabaseManagement::DALCache::requestsThread, this);
    cacheThreadObject = new boost::thread(&DatabaseManagement::DALCache::cacheThread, this);
}

SyncServer_Core::DatabaseManagement::DALCache::~DALCache()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseManager::DALCache / " + Tools::toString(cacheType) + " (~) > Destruction initiated.");
    stopCache = true;
    forceCommit = true;
    
    cacheThreadLockCondition.notify_all();
    requestsThreadLockCondition.notify_all();
    
    cacheThreadObject->join();
    requestsThreadObject->join();
    
    delete cacheThreadObject;
    delete requestsThreadObject;
    
    onSuccessConnection.disconnect();
    onFailureConnection.disconnect();
    
    {
        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
        
        cache.clear();
        objectAgeTable.clear();
        uncommittedObjects.clear();
        pendingCommitRequests.clear();
    }
    
    {
        boost::lock_guard<boost::mutex> requestsLock(requestsThreadMutex);
        
        for(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*> currentRequestData : requestsData)
            delete currentRequestData.second;
        
        requestsData.clear();
        pendingDALRequests.clear();
        std::queue<DatabaseRequestID>().swap(pendingCacheRequests);
    }
    
    dal->disconnect();
    dal = nullptr;
    logger = nullptr;
}

bool SyncServer_Core::DatabaseManagement::DALCache::getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue)
{
    if(stopCache)
        return false;
    
    addRequest(requestID, RequestType::SELECT, constraintType, constraintValue);
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + "(Get Object) > Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Get Object) > Notification to main thread sent.");

    return true;
}

bool SyncServer_Core::DatabaseManagement::DALCache::putObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    if(stopCache)
        return false;
    
    bool result = false;
    if(isObjectInCache(inputData->getContainerID()))
    {
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Insert Object) > [" + Tools::toString(requestID) + "]: Object with ID <" + Tools::toString(inputData->getContainerID()) + "> already exists.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, inputData, 0);
    }
    else
    {
        addRequest(requestID, RequestType::INSERT, inputData, 0);
        result = true;
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Insert Object) > Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Insert Object) > Notification to main thread sent.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALCache::updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    if(stopCache)
        return false;
    
    bool result = false;
    if(isObjectInCache(inputData->getContainerID()))
    {
        addRequest(requestID, RequestType::UPDATE, inputData, 0);
        result = true;
    }
    else
    {
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Update Object) > [" + Tools::toString(requestID) + "]: Object with ID <" + Tools::toString(inputData->getContainerID()) + "> not found in cache.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, inputData, 0);
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(cacheType) + " (Update Object) > Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(cacheType) + " (Update Object) > Notification to main thread sent.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALCache::removeObject(DatabaseRequestID requestID, DBObjectID id)
{
    if(stopCache)
        return false;
    
    bool result = false;
    if(isObjectInCache(id))
    {
        addRequest(requestID, RequestType::REMOVE, id, cacheType);
        result = true;
    }
    else
    {
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Remove Object) > [" + Tools::toString(requestID) + "]: Object with ID <" + Tools::toString(id) + "> not found in cache.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, id, cacheType);
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(cacheType) + " (Remove Object) > Sending notification to requests thread.");
    requestsThreadLockCondition.notify_all();
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(cacheType) + " (Remove Object) > Notification to requests thread sent.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALCache::commitCache()
{
    if(stopCache)
        return false;
    
    if(!commitDisabled)
    {
        forceCommit = true;
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Commit Cache) > Sending notification to requests thread.");
        cacheThreadLockCondition.notify_all();
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Commit Cache) > Notification to requests thread sent.");
        
        return true;
    }
    else
    {
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Commit Cache) > Commit disabled.");
        return false;
    }
}

bool SyncServer_Core::DatabaseManagement::DALCache::rollbackCache()
{
    if(stopCache)
        return false;
    
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Rollback Cache) > Entering cache critical section.");
        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Rollback Cache) > Cache critical section entered.");
        
        if(uncommittedObjects.size() > 0)
        {
            for(std::pair<DBObjectID, RequestType> currentObject : uncommittedObjects)
            {
                if(cache[currentObject.first].use_count() == 2)
                {
                    cache.erase(currentObject.first);
                    if(clearObjectAge)
                        objectAgeTable.erase(currentObject.first);
                }
                else
                    logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Rollback Cache) > Failed to evict object from cache during rollback; object is in use <" + Tools::toString(currentObject.first) + ">.");
            }
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Rollback Cache) > Exiting cache critical section.");
    }
    
    return true;
}

bool SyncServer_Core::DatabaseManagement::DALCache::disableCommit()
{
    if(stopCache)
        return false;
    
    commitDisabled = true;
    return true;
}

bool SyncServer_Core::DatabaseManagement::DALCache::enableCommit()
{
    if(stopCache)
        return false;
    
    if(commitDisabled)
    {
        commitDisabled = false;
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Enable Commit) > Sending notification to requests thread.");
        cacheThreadLockCondition.notify_all();
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Enable Commit) > Notification to requests thread sent.");
        
        return true;
    }
    
    return false;
}

bool SyncServer_Core::DatabaseManagement::DALCache::setParameters(DALCacheParameters parameters)
{
    if(stopCache)
        return false;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Set Parameters) > Entering critical section.");
    boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Set Parameters) > Critical section entered.");

    alwaysEvict = parameters.alwaysEvictObjects;
    clearObjectAge = parameters.alwaysClearObjectAge;
    cacheSize = parameters.maximumCacheSize;
    maxCommitTime = parameters.maximumCommitTime;
    maxCommitUpdates = parameters.maximumCommitUpdates;
    minCommitUpdates = parameters.minimumCommitUpdates;

    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Set Parameters) > Exiting critical section.");
    return true;
}

SyncServer_Core::DatabaseManagement::DALCache::DALCacheParameters SyncServer_Core::DatabaseManagement::DALCache::getParameters()
{
    if(stopCache)
        return DALCacheParameters();
    
    return DALCacheParameters(maxCommitTime, maxCommitUpdates, minCommitUpdates, alwaysEvict, clearObjectAge, cacheSize);
}

SyncServer_Core::DatabaseManagement::DALCache::DALCacheInformation SyncServer_Core::DatabaseManagement::DALCache::getCacheInformation()
{
    return {dal->getDatabaseInfo(), nullptr, cache.size(), objectAgeTable.size(), uncommittedObjects.size(),
            globalCacheAge, forceCommit, commitDisabled, pendingCommitRequests.size(), currentCommitRequest,
            pendingCacheRequests.size(), pendingDALRequests.size(), dalID, alwaysEvict, clearObjectAge, cacheEnabled,
            cacheSize, cacheType, maxCommitTime, maxCommitUpdates, minCommitUpdates, cacheHits, cacheMisses,
            stopCache, cacheThreadRunning, requestsThreadRunning};
}

//WARN -> not thread safe
//WARN -> commit objects before eviction
bool SyncServer_Core::DatabaseManagement::DALCache::evictObjects()
{
    bool result = false;
    
    DBObjectID * lruObject = nullptr;
    unsigned long lruObjectAge = 0;
    queue<DBObjectID> objectsToEvict;
    
    for(std::pair<DBObjectID, DataContainerPtr> currentObject : cache)
    {
        //considers only containers that are not in use (only the cache references them)
        //use_count == 2 (one for the cache itself, one for the current reference)
        if(currentObject.second.use_count() == 2 && !currentObject.second->isModified())
        {
            unsigned long currentObjectAge = objectAgeTable[currentObject.first];
            
            if(currentObjectAge < globalCacheAge)
            {
                objectsToEvict.push(currentObject.first);
            }
            else if(currentObjectAge < lruObjectAge)
            {
                lruObject = &currentObject.first;
                lruObjectAge = currentObjectAge;
            }
        }
    }
    
    if(objectsToEvict.size() > 0)
    {
        unsigned long evictedObjectsNum = 0;
        
        while(objectsToEvict.size() > 0)
        {
            cache.erase(objectsToEvict.front());
            if(clearObjectAge)
                objectAgeTable.erase(objectsToEvict.front());
            objectsToEvict.pop();
            evictedObjectsNum++;
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Evict Objects) > Evicted <" + Tools::toString(evictedObjectsNum) + "> object(s).");
        result = true;
    }
    else if(lruObject != nullptr)
    {
        cache.erase(*lruObject);
        if(clearObjectAge)
            objectAgeTable.erase(*lruObject);
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Evict Objects) > Evicted LRU object.");
        result = true;
    }
    else
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Evict Objects) > Failed to evict objects; no objects eligible found.");
    
    return result;
}

void SyncServer_Core::DatabaseManagement::DALCache::cacheThread()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Started.");
    cacheThreadRunning = true;
    
    while(!stopCache || forceCommit)
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Acquiring cache lock.");
        boost::unique_lock<boost::mutex> cacheLock(cacheThreadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Cache lock acquired.");
        
        if(!cacheEnabled)
        {
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Cache disabled; thread will sleep until enabled.");
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Waiting on cache lock.");
            cacheThreadLockCondition.wait(cacheLock);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Cache lock re-acquired after wait.");
        }
        else
        {
            if(uncommittedObjects.size() >= minCommitUpdates || forceCommit)
            {
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Working on <"+Tools::toString(uncommittedObjects.size())+"> uncommitted objects.");
                unordered_map<DBObjectID, RequestType> failedCommits;
                
                for(std::pair<DBObjectID, RequestType> currentObject : uncommittedObjects)
                {
                    bool commitSuccessful = false;
                    currentCommitRequest++;
                    
                    {
                        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
                        pendingCommitRequests.insert(std::pair<DatabaseRequestID, DBObjectID>(currentCommitRequest, currentObject.first));
                    }
                    
                    switch(currentObject.second)
                    {
                        case RequestType::INSERT: commitSuccessful = dal->putObject(currentCommitRequest, cache[currentObject.first]); break;
                        case RequestType::UPDATE: commitSuccessful = dal->updateObject(currentCommitRequest, cache[currentObject.first]);  break;
                        case RequestType::REMOVE:
                        {
                            commitSuccessful = dal->removeObject(currentCommitRequest, currentObject.first);
                            
                            if(commitSuccessful)
                            {
                                cache.erase(currentObject.first);
                                
                                if(clearObjectAge)
                                    objectAgeTable.erase(currentObject.first);
                            }
                        } break;
                        
                        default: logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Invalid request type found during cache commit."); break;
                    }
                    
                    if(!commitSuccessful)
                    {
                        failedCommits.insert(currentObject);
                        
                        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
                        pendingCommitRequests.erase(currentCommitRequest);
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Failed to commit object <" + Tools::toString(currentObject.first) + ">.");
                    }
                }

                uncommittedObjects.clear();
                if(failedCommits.size() > 0)
                    uncommittedObjects.swap(failedCommits);
                
                globalCacheAge++;
                forceCommit = false;
            }
            else
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Skipping commit (not enough objects).");
            
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > EV ["+Tools::toString(cacheSize)+"] + ["+Tools::toString(cache.size())+"].");
            if(cacheSize > 0 && cache.size() >= cacheSize)
            {
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Eviction begin.");
                evictObjects();
            }
            else
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > No eviction.");
            
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Waiting on cache lock.");
            boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(maxCommitTime);
            while(!stopCache && cacheThreadLockCondition.timed_wait(cacheLock, nextWakeup) && uncommittedObjects.size() < maxCommitUpdates && !forceCommit)
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Exited wait without timer expiration.");
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Cache lock re-acquired after wait.");
        }
    }
    
    cacheThreadRunning = false;
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Cache Thread) > Stopped.");
    
    return;
}

void SyncServer_Core::DatabaseManagement::DALCache::requestsThread()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Started.");
    requestsThreadRunning = true;
    
    while(!stopCache)
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Acquiring requests lock.");
        boost::unique_lock<boost::mutex> requestsLock(requestsThreadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Requests lock acquired.");
        
        if(pendingCacheRequests.size() == 0)
        {
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Waiting on requests lock.");
            requestsThreadLockCondition.wait(requestsLock);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Requests lock re-acquired after wait.");
        }
        else
        {
            DatabaseRequestID currentRequest = pendingCacheRequests.front();
            tuple<RequestType, boost::any, boost::any> * currentRequestData = requestsData[currentRequest];
            
            switch(currentRequestData->get<0>())
            {
                case RequestType::SELECT:
                {
                    DBObjectID objectID = Tools::getIDFromConstraint(cacheType, currentRequestData->get<1>(), currentRequestData->get<2>());
                    
                    if(objectID != DatabaseManagement_Types::INVALID_OBJECT_ID)
                    {
                        bool inCache = false;
                        bool pendingRemoval = false;
                        
                        {
                            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / SELECT) > Acquiring cache lock.");
                            boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / SELECT) > Cache lock acquired.");

                            if((cache.find(objectID) != cache.end()))
                            {
                                inCache = true;
                                objectAgeTable[objectID]++;
                                
                                if(uncommittedObjects.find(objectID) != uncommittedObjects.end())
                                    pendingRemoval = (uncommittedObjects[objectID] == RequestType::REMOVE);
                            }

                            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / SELECT) > Cache lock released.");
                        }
                        
                        if(inCache && !pendingRemoval)
                        {
                            cacheHits++;
                            onSuccess(dalID, currentRequest, cache[objectID]);
                        }
                        else if(!inCache)
                        {
                            cacheMisses++;
                            pendingDALRequests.insert(std::pair<DatabaseRequestID, bool>(currentRequest, true));
                            dal->getObject(currentRequest, currentRequestData->get<1>(), currentRequestData->get<2>());
                        }
                        else
                        {
                            cacheHits++;
                            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / SELECT) > Requested object found in cache but is pending removal.");
                            onFailure(dalID, currentRequest, objectID);
                        }
                    }
                    else
                    {
                        cacheMisses++;
                        pendingDALRequests.insert(std::pair<DatabaseRequestID, bool>(currentRequest, true));
                        dal->getObject(currentRequest, currentRequestData->get<1>(), currentRequestData->get<2>());
                    }
                } break;
                
                case RequestType::INSERT:
                case RequestType::UPDATE:
                {
                    DataContainerPtr containerData = boost::any_cast<DataContainerPtr>(currentRequestData->get<1>());
                    bool successful = false;
                    
                    {
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / I;U) > Acquiring cache lock.");
                        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / I;U) > Cache lock acquired.");
                        
                        if(uncommittedObjects.find(containerData->getContainerID()) != uncommittedObjects.end())
                        {
                            if(uncommittedObjects.at(containerData->getContainerID()) == RequestType::REMOVE)
                                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / I;U) > Request failed; removal request is already pending <" + containerData->toString() + ">.");
                            else //INSERT or UPDATE already pending; the current request can be discarded
                                successful = true;
                        }
                        else
                        {
                            if(currentRequestData->get<0>() == RequestType::INSERT)
                            {
                                cache.insert(std::pair<DBObjectID, DataContainerPtr>(containerData->getContainerID(), containerData));
                                if(objectAgeTable.find(containerData->getContainerID()) == objectAgeTable.end())
                                    objectAgeTable.insert(std::pair<DBObjectID, ObjectCacheAge>(containerData->getContainerID(), globalCacheAge));
                                else if(objectAgeTable[containerData->getContainerID()] < globalCacheAge)
                                    objectAgeTable[containerData->getContainerID()] = globalCacheAge;
                            }
                            else //UPDATE
                                objectAgeTable[containerData->getContainerID()]++;
                            
                            uncommittedObjects.insert(std::pair<DBObjectID, RequestType>(containerData->getContainerID(), currentRequestData->get<0>()));
                            if(uncommittedObjects.size() >= maxCommitUpdates)
                                cacheThreadLockCondition.notify_all();
                            
                            successful = true;
                        }
                        
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / I;U) > Cache lock released.");
                    }
                    
                    if(successful)
                        onSuccess(dalID, currentRequest, containerData);
                    else
                        onFailure(dalID, currentRequest, containerData->getContainerID());
                } break;
                
                case RequestType::REMOVE:
                {
                    DBObjectID objectID = boost::any_cast<DBObjectID>(currentRequestData->get<1>());
                    bool successful = false;
                    DataContainerPtr container;
                    
                    {
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / REMOVE) > Acquiring cache lock.");
                        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / REMOVE) > Cache lock acquired.");
                        
                        container = cache.at(objectID);

                        if(uncommittedObjects.find(objectID) != uncommittedObjects.end())
                        {
                            RequestType existingRequest = uncommittedObjects.at(objectID);

                            if(existingRequest == RequestType::REMOVE)
                                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / REMOVE) > Object removal failed; removal request already pending <" + container->toString() + ">.");
                            else if(existingRequest == RequestType::INSERT)
                            {
                                uncommittedObjects.erase(objectID);
                                cache.erase(objectID);
                                if(clearObjectAge)
                                    objectAgeTable.erase(objectID);
                                successful = true;
                            }
                            else //UPDATE
                            {
                                uncommittedObjects[objectID] = RequestType::REMOVE;
                                successful = true;
                            }
                        }
                        else
                        {
                            uncommittedObjects.insert(std::pair<DBObjectID, RequestType>(objectID, RequestType::REMOVE));
                            
                            if(uncommittedObjects.size() >= maxCommitUpdates)
                                cacheThreadLockCondition.notify_all();
                            
                            successful = true;
                        }

                        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / REMOVE) > Cache lock released.");
                    }
                    
                    if(successful)
                        onSuccess(dalID, currentRequest, container);
                    else
                        onFailure(dalID, currentRequest, objectID);
                } break;
                
                case RequestType::CACHE_OBJECT:
                {
                    DataContainerPtr containerData = boost::any_cast<DataContainerPtr>(currentRequestData->get<1>());
                    
                    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / CACHE_OBJECT) > Acquiring cache lock.");
                    boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / CACHE_OBJECT) > Cache lock acquired.");
                    
                    std::vector<DataContainerPtr> containersToCache;
                    
                    if(containerData->getDataType() == DatabaseObjectType::VECTOR)
                    {
                         VectorDataContainerPtr vectorContainer = boost::dynamic_pointer_cast<DatabaseManagement_Containers::VectorDataContainer>(containerData);
                         containersToCache = vectorContainer->getContainers();
                    }
                    else
                        containersToCache.push_back(containerData);
                    
                    for(DataContainerPtr currentContainer : containersToCache)
                    {
                        cache.insert(std::pair<DBObjectID, DataContainerPtr>(currentContainer->getContainerID(), currentContainer));
                        if(objectAgeTable.find(currentContainer->getContainerID()) == objectAgeTable.end())
                            objectAgeTable.insert(std::pair<DBObjectID, ObjectCacheAge>(currentContainer->getContainerID(), globalCacheAge));
                        else if(objectAgeTable[currentContainer->getContainerID()] < globalCacheAge)
                            objectAgeTable[currentContainer->getContainerID()] = globalCacheAge;
                    }
                    
                    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread / CACHE_OBJECT) > Cache lock released.");
                } break;
                
                case RequestType::SEND_FAILURE_EVENT:
                {
                    onFailure(dalID, currentRequest, boost::any_cast<DBObjectID>(currentRequestData->get<1>()));
                } break;
                
                case RequestType::SEND_SUCCESS_EVENT:
                {
                    onSuccess(dalID, currentRequest, boost::any_cast<DataContainerPtr>(currentRequestData->get<1>()));
                } break;
                
                default:
                {
                    logger->logMessage(Utilities::FileLogSeverity::Error, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Unexpected request type encountered.");
                    onFailure(dalID, currentRequest, DBObjectID());
                } break;
            }
            
            pendingCacheRequests.pop();
            requestsData.erase(currentRequest);
            delete currentRequestData;
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Data lock released.");
    }
    
    requestsThreadRunning = false;
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (Requests Thread) > Stopped.");
    return;
}

void SyncServer_Core::DatabaseManagement::DALCache::onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id)
{
    if(stopCache)
        return;
    
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Entering PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > PENDING_COMMITS critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        
        auto commitRequest = pendingCommitRequests.find(requestID);
        if(commitRequest != pendingCommitRequests.end() && pendingCommitRequests[requestID] == id)
        {
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Object commit failed for <" + Tools::toString(id) + "> / <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
            pendingCommitRequests.erase(commitRequest);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Exiting PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
            return;
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Exiting PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    {//ensures the locks are released as soon as they are not needed
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Entering REQUESTS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> requestsLock(requestsThreadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > REQUESTS critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

        auto dalRequest = pendingDALRequests.find(requestID);

        if(dalRequest != pendingDALRequests.end())
            pendingDALRequests.erase(dalRequest);
        else
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Unexpected response received <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Exiting REQUESTS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Sending signal for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    onFailure(dalID, requestID, id);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Failure Handler) > Signal sent for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
}

void SyncServer_Core::DatabaseManagement::DALCache::onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr data)
{
    if(stopCache)
        return;
    
    {//ensures the locks are released as soon as they are not needed
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Entering PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > PENDING_COMMITS critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

        auto commitRequest = pendingCommitRequests.find(requestID);
        if(commitRequest != pendingCommitRequests.end() && pendingCommitRequests[requestID] == data->getContainerID())
        {
            pendingCommitRequests.erase(commitRequest);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Exiting PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
            return;
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Exiting PENDING_COMMITS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    {//ensures the locks are released as soon as they are not needed
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Entering REQUESTS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::unique_lock<boost::mutex> requestsLock(requestsThreadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > REQUESTS critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

        auto dalRequest = pendingDALRequests.find(requestID);
        if(dalRequest != pendingDALRequests.end())
        {
            if(data->getDataType() != DatabaseObjectType::INVALID)
            {
                //TODO - this is expensive
                requestsLock.unlock();
                addRequest(requestID, RequestType::CACHE_OBJECT, data, 0);
                requestsLock.lock();
            }
            else
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Unexpected container received for response <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

            pendingDALRequests.erase(dalRequest);
        }
        else
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Unexpected response received <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");

        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Exiting REQUESTS critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Sending signal for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    onSuccess(dalID, requestID, data);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Tools::toString(cacheType) + " (On Success Handler) > Signal sent for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
 }