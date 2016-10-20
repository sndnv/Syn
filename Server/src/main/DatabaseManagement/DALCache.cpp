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

SyncServer_Core::DatabaseManagement::DALCache::DALCache(DALPtr childDAL, Utilities::FileLoggerPtr parentLogger, DALCacheParameters parameters)
: dal(childDAL), alwaysEvict(parameters.alwaysEvictObjects), clearObjectAge(parameters.alwaysClearObjectAge),
  cacheEnabled(true), cacheSize(parameters.maximumCacheSize), cacheType(dal->getType()),
  maxCommitTime(parameters.maximumCommitTime), maxCommitUpdates(parameters.maximumCommitUpdates),
  minCommitUpdates(parameters.minimumCommitUpdates), debugLogger(parentLogger)
{
    onSuccessConnection = dal->onSuccessEventAttach(boost::bind(&DatabaseManagement::DALCache::onSuccessHandler, this, _1, _2, _3));
    onFailureConnection = dal->onFailureEventAttach(boost::bind(&DatabaseManagement::DALCache::onFailureHandler, this, _1, _2, _3));
    
    requestsThreadObject = new boost::thread(&DatabaseManagement::DALCache::requestsThread, this);
    cacheThreadObject = new boost::thread(&DatabaseManagement::DALCache::cacheThread, this);
}

SyncServer_Core::DatabaseManagement::DALCache::~DALCache()
{
    logMessage(LogSeverity::Debug, "(~) Destruction initiated.");
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
}

bool SyncServer_Core::DatabaseManagement::DALCache::getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue)
{
    if(stopCache)
        return false;
    
    addRequest(requestID, RequestType::SELECT, constraintType, constraintValue);
    
    logMessage(LogSeverity::Debug, "(Get Object) Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logMessage(LogSeverity::Debug, "(Get Object) Notification to main thread sent.");

    return true;
}

bool SyncServer_Core::DatabaseManagement::DALCache::putObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    if(stopCache)
        return false;
    
    bool result = false;
    if(isObjectInCache(inputData->getContainerID()))
    {
        logMessage(LogSeverity::Error, "(putObject) [" + Convert::toString(requestID) + "]: Object with ID <" + Convert::toString(inputData->getContainerID()) + "already exists.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, inputData, 0);
    }
    else
    {
        addRequest(requestID, RequestType::INSERT, inputData, 0);
        result = true;
    }
    
    logMessage(LogSeverity::Debug, "(putObject) Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logMessage(LogSeverity::Debug, "(putObject) Notification to main thread sent.");
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
        logMessage(LogSeverity::Error, "(updateObject) [" + Convert::toString(requestID) + "]: Object with ID <" + Convert::toString(inputData->getContainerID()) + "not found in cache.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, inputData, 0);
    }
    
    logMessage(LogSeverity::Debug, "(updateObject) Sending notification to main thread.");
    requestsThreadLockCondition.notify_all();
    logMessage(LogSeverity::Debug, "(updateObject) Notification to main thread sent.");
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
        logMessage(LogSeverity::Error, "(removeObject) [" + Convert::toString(requestID) + "]: Object with ID <" + Convert::toString(id) + "not found in cache.");
        addRequest(requestID, RequestType::SEND_FAILURE_EVENT, id, cacheType);
    }
    
    logMessage(LogSeverity::Debug, "(removeObject) Sending notification to requests thread.");
    requestsThreadLockCondition.notify_all();
    logMessage(LogSeverity::Debug, "(removeObject) Notification to requests thread sent.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALCache::commitCache()
{
    if(stopCache)
        return false;
    
    if(!commitDisabled)
    {
        forceCommit = true;
        
        logMessage(LogSeverity::Debug, "(commitCache) Sending notification to requests thread.");
        cacheThreadLockCondition.notify_all();
        logMessage(LogSeverity::Debug, "(commitCache) Notification to requests thread sent.");
        
        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(commitCache) Commit disabled.");
        return false;
    }
}

bool SyncServer_Core::DatabaseManagement::DALCache::rollbackCache()
{
    if(stopCache)
        return false;
    
    {
        logMessage(LogSeverity::Debug, "(rollbackCache) Entering cache critical section.");
        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
        logMessage(LogSeverity::Debug, "(rollbackCache) Cache critical section entered.");
        
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
                    logMessage(LogSeverity::Error, "(rollbackCache) Failed to evict object from cache during rollback; object is in use <" + Convert::toString(currentObject.first) + ">.");
            }
        }
        
        logMessage(LogSeverity::Debug, "(rollbackCache) Exiting cache critical section.");
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
        
        logMessage(LogSeverity::Debug, "(enableCommit) Sending notification to requests thread.");
        cacheThreadLockCondition.notify_all();
        logMessage(LogSeverity::Debug, "(enableCommit) Notification to requests thread sent.");
        
        return true;
    }
    
    return false;
}

bool SyncServer_Core::DatabaseManagement::DALCache::setParameters(DALCacheParameters parameters)
{
    if(stopCache)
        return false;
    
    logMessage(LogSeverity::Debug, "(setParameters) Entering critical section.");
    boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
    logMessage(LogSeverity::Debug, "(setParameters) Critical section entered.");

    alwaysEvict = parameters.alwaysEvictObjects;
    clearObjectAge = parameters.alwaysClearObjectAge;
    cacheSize = parameters.maximumCacheSize;
    maxCommitTime = parameters.maximumCommitTime;
    maxCommitUpdates = parameters.maximumCommitUpdates;
    minCommitUpdates = parameters.minimumCommitUpdates;

    logMessage(LogSeverity::Debug, "(setParameters) Exiting critical section.");
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

//WARN -not thread safe
//WARN -commit objects before eviction
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
        
        logMessage(LogSeverity::Debug, "(evictObjects) Evicted <" + Convert::toString(evictedObjectsNum) + "object(s).");
        result = true;
    }
    else if(lruObject != nullptr)
    {
        cache.erase(*lruObject);
        if(clearObjectAge)
            objectAgeTable.erase(*lruObject);
        
        logMessage(LogSeverity::Debug, "(evictObjects) Evicted LRU object.");
        result = true;
    }
    else
        logMessage(LogSeverity::Debug, "(evictObjects) Failed to evict objects; no objects eligible found.");
    
    return result;
}

void SyncServer_Core::DatabaseManagement::DALCache::cacheThread()
{
    logMessage(LogSeverity::Debug, "(cacheThread) Started.");
    cacheThreadRunning = true;
    
    while(!stopCache || forceCommit)
    {
        logMessage(LogSeverity::Debug, "(cacheThread) Acquiring cache lock.");
        boost::unique_lock<boost::mutex> cacheLock(cacheThreadMutex);
        logMessage(LogSeverity::Debug, "(cacheThread) Cache lock acquired.");
        
        if(!cacheEnabled)
        {
            logMessage(LogSeverity::Debug, "(cacheThread) Cache disabled; thread will sleep until enabled.");
            logMessage(LogSeverity::Debug, "(cacheThread) Waiting on cache lock.");
            cacheThreadLockCondition.wait(cacheLock);
            logMessage(LogSeverity::Debug, "(cacheThread) Cache lock re-acquired after wait.");
        }
        else
        {
            if(uncommittedObjects.size() >= minCommitUpdates || forceCommit)
            {
                logMessage(LogSeverity::Debug, "(cacheThread) Working on <"+Convert::toString(uncommittedObjects.size())+"uncommitted objects.");
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
                        
                        default: logMessage(LogSeverity::Debug, "(cacheThread) Invalid request type found during cache commit."); break;
                    }
                    
                    if(!commitSuccessful)
                    {
                        failedCommits.insert(currentObject);
                        
                        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
                        pendingCommitRequests.erase(currentCommitRequest);
                        logMessage(LogSeverity::Debug, "(cacheThread) Failed to commit object <" + Convert::toString(currentObject.first) + ">.");
                    }
                }

                uncommittedObjects.clear();
                if(failedCommits.size() > 0)
                    uncommittedObjects.swap(failedCommits);
                
                globalCacheAge++;
                forceCommit = false;
            }
            else
                logMessage(LogSeverity::Debug, "(cacheThread) Skipping commit (not enough objects).");
            
            logMessage(LogSeverity::Debug, "(cacheThread) EV ["+Convert::toString(cacheSize)+"] + ["+Convert::toString(cache.size())+"].");
            if(cacheSize > 0 && cache.size() >= cacheSize)
            {
                logMessage(LogSeverity::Debug, "(cacheThread) Eviction begin.");
                evictObjects();
            }
            else
                logMessage(LogSeverity::Debug, "(cacheThread) No eviction.");
            
            logMessage(LogSeverity::Debug, "(cacheThread) Waiting on cache lock.");
            boost::system_time nextWakeup = boost::get_system_time() + boost::posix_time::seconds(maxCommitTime);
            while(!stopCache && cacheThreadLockCondition.timed_wait(cacheLock, nextWakeup) && uncommittedObjects.size() < maxCommitUpdates && !forceCommit)
                logMessage(LogSeverity::Debug, "(cacheThread) Exited wait without timer expiration.");
            logMessage(LogSeverity::Debug, "(cacheThread) Cache lock re-acquired after wait.");
        }
    }
    
    cacheThreadRunning = false;
    logMessage(LogSeverity::Debug, "(cacheThread) Stopped.");
    
    return;
}

void SyncServer_Core::DatabaseManagement::DALCache::requestsThread()
{
    logMessage(LogSeverity::Debug, "(requestsThread) Started.");
    requestsThreadRunning = true;
    
    while(!stopCache)
    {
        logMessage(LogSeverity::Debug, "(requestsThread) Acquiring requests lock.");
        boost::unique_lock<boost::mutex> requestsLock(requestsThreadMutex);
        logMessage(LogSeverity::Debug, "(requestsThread) Requests lock acquired.");
        
        if(pendingCacheRequests.size() == 0)
        {
            logMessage(LogSeverity::Debug, "(requestsThread) Waiting on requests lock.");
            requestsThreadLockCondition.wait(requestsLock);
            logMessage(LogSeverity::Debug, "(requestsThread) Requests lock re-acquired after wait.");
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
                    
                    if(objectID != Common_Types::INVALID_OBJECT_ID)
                    {
                        bool inCache = false;
                        bool pendingRemoval = false;
                        
                        {
                            logMessage(LogSeverity::Debug, "(requestsThread / SELECT) Acquiring cache lock.");
                            boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                            logMessage(LogSeverity::Debug, "(requestsThread / SELECT) Cache lock acquired.");

                            if((cache.find(objectID) != cache.end()))
                            {
                                inCache = true;
                                objectAgeTable[objectID]++;
                                
                                if(uncommittedObjects.find(objectID) != uncommittedObjects.end())
                                    pendingRemoval = (uncommittedObjects[objectID] == RequestType::REMOVE);
                            }

                            logMessage(LogSeverity::Debug, "(requestsThread / SELECT) Cache lock released.");
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
                            logMessage(LogSeverity::Debug, "(requestsThread / SELECT) Requested object found in cache but is pending removal.");
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
                        logMessage(LogSeverity::Debug, "(requestsThread / I;U) Acquiring cache lock.");
                        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                        logMessage(LogSeverity::Debug, "(requestsThread / I;U) Cache lock acquired.");
                        
                        if(uncommittedObjects.find(containerData->getContainerID()) != uncommittedObjects.end())
                        {
                            if(uncommittedObjects.at(containerData->getContainerID()) == RequestType::REMOVE)
                                logMessage(LogSeverity::Debug, "(requestsThread / I;U) Request failed; removal request is already pending <" + containerData->toString() + ">.");
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
                        
                        logMessage(LogSeverity::Debug, "(requestsThread / I;U) Cache lock released.");
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
                        logMessage(LogSeverity::Debug, "(requestsThread / REMOVE) Acquiring cache lock.");
                        boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                        logMessage(LogSeverity::Debug, "(requestsThread / REMOVE) Cache lock acquired.");
                        
                        container = cache.at(objectID);

                        if(uncommittedObjects.find(objectID) != uncommittedObjects.end())
                        {
                            RequestType existingRequest = uncommittedObjects.at(objectID);

                            if(existingRequest == RequestType::REMOVE)
                                logMessage(LogSeverity::Debug, "(requestsThread / REMOVE) Object removal failed; removal request already pending <" + container->toString() + ">.");
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

                        logMessage(LogSeverity::Debug, "(requestsThread / REMOVE) Cache lock released.");
                    }
                    
                    if(successful)
                        onSuccess(dalID, currentRequest, container);
                    else
                        onFailure(dalID, currentRequest, objectID);
                } break;
                
                case RequestType::CACHE_OBJECT:
                {
                    DataContainerPtr containerData = boost::any_cast<DataContainerPtr>(currentRequestData->get<1>());
                    
                    logMessage(LogSeverity::Debug, "(requestsThread / CACHE_OBJECT) Acquiring cache lock.");
                    boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                    logMessage(LogSeverity::Debug, "(requestsThread / CACHE_OBJECT) Cache lock acquired.");
                    
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
                    
                    logMessage(LogSeverity::Debug, "(requestsThread / CACHE_OBJECT) Cache lock released.");
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
                    logMessage(LogSeverity::Error, "(requestsThread) Unexpected request type encountered.");
                    onFailure(dalID, currentRequest, DBObjectID());
                } break;
            }
            
            pendingCacheRequests.pop();
            requestsData.erase(currentRequest);
            delete currentRequestData;
        }
        
        logMessage(LogSeverity::Debug, "(requestsThread) Data lock released.");
    }
    
    requestsThreadRunning = false;
    logMessage(LogSeverity::Debug, "(requestsThread) Stopped.");
    return;
}

void SyncServer_Core::DatabaseManagement::DALCache::onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id)
{
    if(stopCache)
        return;
    
    {
        logMessage(LogSeverity::Debug, "(onFailureHandler) Entering PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
        logMessage(LogSeverity::Debug, "(onFailureHandler) PENDING_COMMITS critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        
        auto commitRequest = pendingCommitRequests.find(requestID);
        if(commitRequest != pendingCommitRequests.end() && pendingCommitRequests[requestID] == id)
        {
            logMessage(LogSeverity::Debug, "(onFailureHandler) Object commit failed for <" + Convert::toString(id) + "/ <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
            pendingCommitRequests.erase(commitRequest);
            logMessage(LogSeverity::Debug, "(onFailureHandler) Exiting PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
            return;
        }
        
        logMessage(LogSeverity::Debug, "(onFailureHandler) Exiting PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    {//ensures the locks are released as soon as they are not needed
        logMessage(LogSeverity::Debug, "(onFailureHandler) Entering REQUESTS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> requestsLock(requestsThreadMutex);
        logMessage(LogSeverity::Debug, "(onFailureHandler) REQUESTS critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

        auto dalRequest = pendingDALRequests.find(requestID);

        if(dalRequest != pendingDALRequests.end())
            pendingDALRequests.erase(dalRequest);
        else
            logMessage(LogSeverity::Debug, "(onFailureHandler) Unexpected response received <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

        logMessage(LogSeverity::Debug, "(onFailureHandler) Exiting REQUESTS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    logMessage(LogSeverity::Debug, "(onFailureHandler) Sending signal for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    onFailure(dalID, requestID, id);
    logMessage(LogSeverity::Debug, "(onFailureHandler) Signal sent for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
}

void SyncServer_Core::DatabaseManagement::DALCache::onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr data)
{
    if(stopCache)
        return;
    
    {//ensures the locks are released as soon as they are not needed
        logMessage(LogSeverity::Debug, "(onSuccessHandler) Entering PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> pendingCommitRequestsLock(pendingCommitRequestsMutex);
        logMessage(LogSeverity::Debug, "(onSuccessHandler) PENDING_COMMITS critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

        auto commitRequest = pendingCommitRequests.find(requestID);
        if(commitRequest != pendingCommitRequests.end() && pendingCommitRequests[requestID] == data->getContainerID())
        {
            pendingCommitRequests.erase(commitRequest);
            logMessage(LogSeverity::Debug, "(onSuccessHandler) Exiting PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
            return;
        }
        
        logMessage(LogSeverity::Debug, "(onSuccessHandler) Exiting PENDING_COMMITS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    {//ensures the locks are released as soon as they are not needed
        logMessage(LogSeverity::Debug, "(onSuccessHandler) Entering REQUESTS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::unique_lock<boost::mutex> requestsLock(requestsThreadMutex);
        logMessage(LogSeverity::Debug, "(onSuccessHandler) REQUESTS critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

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
                logMessage(LogSeverity::Debug, "(onSuccessHandler) Unexpected container received for response <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

            pendingDALRequests.erase(dalRequest);
        }
        else
            logMessage(LogSeverity::Debug, "(onSuccessHandler) Unexpected response received <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");

        logMessage(LogSeverity::Debug, "(onSuccessHandler) Exiting REQUESTS critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    logMessage(LogSeverity::Debug, "(onSuccessHandler) Sending signal for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    onSuccess(dalID, requestID, data);
    logMessage(LogSeverity::Debug, "(onSuccessHandler) Signal sent for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
 }