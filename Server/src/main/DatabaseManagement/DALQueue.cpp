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

#include "DALQueue.h"
#include <boost/shared_ptr.hpp>

SyncServer_Core::DatabaseManagement::DALQueue::DALQueue(DatabaseObjectType type, Utilities::FileLoggerPtr parentLogger, DALQueueParameters parameters)
: queueType(type), dbMode(parameters.dbMode), failureAction(parameters.failureAction),
  maxConsecutiveReadFailures(parameters.maximumReadFailures), maxConsecutiveWriteFailures(parameters.maximumWriteFailures), debugLogger(parentLogger)
{
    stopQueue = false;
    mainThread = new boost::thread(&DatabaseManagement::DALQueue::mainQueueThread, this);
}

SyncServer_Core::DatabaseManagement::DALQueue::~DALQueue()
{
    logMessage(LogSeverity::Debug, "(~) Destruction initiated.");
    stopQueue = true;
    threadLockCondition.notify_all();
    
    mainThread->join();
    delete mainThread;
    
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    
    for(std::pair<unsigned int, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> currentDAL : dals)
    {
        currentDAL.second->get<0>()->disconnect(); //disconnects the DAL
        currentDAL.second->get<3>().disconnect(); //disconnects the onSuccess signal connection
        currentDAL.second->get<4>().disconnect(); //disconnects the onFailure signal connection
        delete currentDAL.second;
    }
    
    for(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*> currentRequesData : requestsData)
        delete currentRequesData.second;
    
    dalIDs.clear();
    dals.clear();
    std::queue<DatabaseRequestID>().swap(newRequests);
    pendingRequests.clear();
    requestsData.clear();
}

bool SyncServer_Core::DatabaseManagement::DALQueue::addDAL(DALPtr dal)
{
    if(stopQueue)
        return false;
    
    logMessage(LogSeverity::Debug, "(addDAL) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(addDAL) Acquired data lock.");
    
    if(dal->getID() == DatabaseManagement_Types::INVALID_DAL_ID || dals.find(dal->getID()) == dals.end())
    {
        boost::signals2::connection onFailreConnection, onSuccessConnection;
        onSuccessConnection = dal->onSuccessEventAttach(boost::bind(&DatabaseManagement::DALQueue::onSuccessHandler, this, _1, _2, _3));
        onFailreConnection = dal->onFailureEventAttach(boost::bind(&DatabaseManagement::DALQueue::onFailureHandler, this, _1, _2, _3));
        
        tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection> * newDAL = 
            new tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>(dal, 0, 0, onSuccessConnection, onFailreConnection);
        dalIDs.push_back(nextDALID);
        dals.insert(std::pair<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*>(nextDALID, newDAL));
        dal->setID(nextDALID);
        dal->connect();
        nextDALID++;
        
        if(dalIDs.size() == 1)
        {
            logMessage(LogSeverity::Debug, "(addDAL) Sending notification to main thread.");
            threadLockCondition.notify_all();
            logMessage(LogSeverity::Debug, "(addDAL) Notification to main thread sent.");
        }
        
        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(addDAL) The requested DatabaseAbstractionLayer is already in the DALs table.");
        return false;
    }
}

bool SyncServer_Core::DatabaseManagement::DALQueue::removeDAL(const DALPtr dal)
{
    if(stopQueue)
        return false;
    
    bool result = false;
    
    logMessage(LogSeverity::Debug, "(removeDAL) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(removeDAL) Acquired data lock.");
    DatabaseAbstractionLayerID dalID = dal->getID();
    if(dals.find(dalID) != dals.end())
    {
        dalIDs.erase(std::remove(dalIDs.begin(), dalIDs.end(), dalID));
        auto dalData = dals[dalID];
        dalData->get<0>()->disconnect();//disconnects the DAL
        dalData->get<3>().disconnect(); //disconnects the onSuccess signal connection
        dalData->get<4>().disconnect(); //disconnects the onFailure signal connection
        delete dals[dalID];
        dals.erase(dalID);
        
        result = true;
    }
    else
        logMessage(LogSeverity::Error, "(removeDAL) The requested DatabaseAbstractionLayer was not found in the DALs table.");
    
    logMessage(LogSeverity::Debug, "(removeDAL) Data lock released.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALQueue::setParameters(DALQueueParameters parameters)
{
    if(stopQueue)
        return false;
    
    logMessage(LogSeverity::Debug, "(setParameters) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(setParameters) Acquired data lock.");
    
    dbMode = parameters.dbMode;
    failureAction = parameters.failureAction;
    maxConsecutiveReadFailures = parameters.maximumReadFailures;
    maxConsecutiveWriteFailures = parameters.maximumWriteFailures;
    
    logMessage(LogSeverity::Debug, "(setParameters) > Data lock released.");
    
    return true;
}

SyncServer_Core::DatabaseManagement::DALQueue::DALQueueParameters SyncServer_Core::DatabaseManagement::DALQueue::getParameters()
{
    return DALQueueParameters{dbMode, failureAction, maxConsecutiveReadFailures, maxConsecutiveWriteFailures};
}

bool SyncServer_Core::DatabaseManagement::DALQueue::setCacheParameters(DatabaseAbstractionLayerID cacheID, DALCache::DALCacheParameters parameters)
{
    if(stopQueue)
        return false;
    
    bool result = false;
    
    logMessage(LogSeverity::Debug, "(setCacheParameters) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(setCacheParameters) Acquired data lock.");
    
    if(dals.find(cacheID) != dals.end())
    {
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(dals[cacheID]->get<0>());
        
        if(cache)
            result = cache->setParameters(parameters);
        else
            logMessage(LogSeverity::Debug, "(setCacheParameters) Operation failed; the requested ID does not refer to a DALCache object <" + Convert::toString(cacheID) + ">.");
    }
    else
        logMessage(LogSeverity::Debug, "(setCacheParameters) Operation failed; the requested cache ID was not found <" + Convert::toString(cacheID) + ">.");
    
    logMessage(LogSeverity::Debug, "(setCacheParameterss) Data lock released.");
    
    return result;
}

DALCache::DALCacheParameters SyncServer_Core::DatabaseManagement::DALQueue::getCacheParameters(DatabaseAbstractionLayerID cacheID)
{
    DALCache::DALCacheParameters result;
    
    if(stopQueue)
        return result;
    
    logMessage(LogSeverity::Debug, "(getCacheParameters) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(getCacheParameters) Acquired data lock.");
    
    if(dals.find(cacheID) != dals.end())
    {
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(dals[cacheID]->get<0>());
        
        if(cache)
            result = cache->getParameters();
        else
            logMessage(LogSeverity::Debug, "(getCacheParameters) Operation failed; the requested ID does not refer to a DALCache object <" + Convert::toString(cacheID) + ">.");
    }
    else
        logMessage(LogSeverity::Debug, "(getCacheParameters) Operation failed; the requested cache ID was not found <" + Convert::toString(cacheID) + ">.");
    
    logMessage(LogSeverity::Debug, "(getCacheParameters) Data lock released.");
    
    return result;
}

SyncServer_Core::DatabaseManagement::DALQueue::DALQueueInformation SyncServer_Core::DatabaseManagement::DALQueue::getQueueInformation() const
{
    return {totalReadFailures, totalWriteFailures, totalReadRequests, totalWriteRequests, queueType, dbMode, failureAction, dalIDs.size(),
            maxConsecutiveReadFailures, maxConsecutiveWriteFailures, stopQueue, threadRunning, newRequests.size(), pendingRequests.size()};
}

std::vector<SyncServer_Core::DatabaseManagement::DALCache::DALCacheInformation> SyncServer_Core::DatabaseManagement::DALQueue::getCachesInformation() const
{
    logMessage(LogSeverity::Debug, "(getCachesInformation) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(getCachesInformation) Acquired data lock.");
    
    std::vector<SyncServer_Core::DatabaseManagement::DALCache::DALCacheInformation> result;
    
    for(DatabaseAbstractionLayerID currentID : dalIDs)
    {
        auto currentDAL = dals.at(currentID);
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(currentDAL->get<0>());
        
        if(cache)
            result.push_back(cache->getCacheInformation());
    }
    
    return result;
}

std::vector<SyncServer_Core::DatabaseManagement::DALQueue::DALInformation> SyncServer_Core::DatabaseManagement::DALQueue::getDALsInformation() const
{
    logMessage(LogSeverity::Debug, "(getDALsInformation) Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(getDALsInformation) Acquired data lock.");
    
    std::vector<SyncServer_Core::DatabaseManagement::DALQueue::DALInformation> result;
    
    for(DatabaseAbstractionLayerID currentID : dalIDs)
    {
        auto currentDAL = dals.at(currentID);
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(currentDAL->get<0>());
        result.push_back(DALInformation(currentID, currentDAL->get<1>(), currentDAL->get<2>(), (bool)cache, queueType, currentDAL->get<0>()->getDatabaseInfo(), nullptr));
    }
    
    return result;
}

DatabaseRequestID SyncServer_Core::DatabaseManagement::DALQueue::addRequestToQueue(RequestType type, boost::any requestParameter, boost::any additionalParameter)
{
    if(stopQueue)
        return DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID;
    
    tuple<RequestType, boost::any, boost::any> * data = new tuple<RequestType, boost::any, boost::any>(type, requestParameter, additionalParameter);
    logMessage(LogSeverity::Debug, "(addRequestToQueue) Entering critical section.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logMessage(LogSeverity::Debug, "(addRequestToQueue) Critical section entered.");
    newRequests.push(nextRequestID);
    requestsData.insert(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*>(nextRequestID, data));
    nextRequestID++;
    
    logMessage(LogSeverity::Debug, "(addRequestToQueue) Sending notification to main thread.");
    threadLockCondition.notify_all();
    logMessage(LogSeverity::Debug, "(addRequestToQueue) Notification to main thread sent.");

    return (nextRequestID - 1);
}

void SyncServer_Core::DatabaseManagement::DALQueue::mainQueueThread()
{
    logMessage(LogSeverity::Debug, "(mainQueueThread) Started.");
    threadRunning = true;
    
    while(!stopQueue)
    {
        logMessage(LogSeverity::Debug, "(mainQueueThread) Acquiring data lock.");
        boost::unique_lock<boost::mutex> dataLock(threadMutex);
        logMessage(LogSeverity::Debug, "(mainQueueThread) Data lock acquired.");
        
        if(dals.size() == 0)
        {
            logMessage(LogSeverity::Error, "(mainQueueThread) No DALs found; thread will sleep until a DAL is added.");
            logMessage(LogSeverity::Debug, "(mainQueueThread) Waiting on data lock.");
            threadLockCondition.wait(dataLock);
            logMessage(LogSeverity::Debug, "(mainQueueThread) Data lock re-acquired after wait.");
        }
        else if(newRequests.size() == 0)
        {
            logMessage(LogSeverity::Debug, "(mainQueueThread) Waiting on data lock.");
            threadLockCondition.wait(dataLock);
            logMessage(LogSeverity::Debug, "(mainQueueThread) Data lock re-acquired after wait.");
        }
        else
        {
            if(newMode != DatabaseManagerOperationMode::INVALID)
                dbMode = newMode;
            
            unsigned int numberOfNewRequests = newRequests.size();
            logMessage(LogSeverity::Debug, "(mainQueueThread) Starting work on <" + Convert::toString(numberOfNewRequests) + "> new requests.");
            for(unsigned int i = 0; i < numberOfNewRequests; i++)
            {
                logMessage(LogSeverity::Debug, "(mainQueueThread) Working with request <" + Convert::toString(i) + ">.");
                DatabaseRequestID currentRequest = newRequests.front();
                tuple<RequestType, boost::any, boost::any> * currentRequestData = requestsData[currentRequest];
                vector<DatabaseAbstractionLayerID> pendingDALs;
                
                switch(currentRequestData->get<0>())
                {
                    case RequestType::SELECT:
                    {
                        //sends to the first DAL in the queue only
                        if(dbMode == DatabaseManagerOperationMode::PRCW || dbMode == DatabaseManagerOperationMode::PRPW)
                        {
                            //get DAL tuple -> gets the DAL -> calls getObject on the DAL
                            dals[dalIDs.front()]->get<0>()->getObject(currentRequest, currentRequestData->get<1>(), currentRequestData->get<2>());
                            pendingDALs.push_back(dalIDs.front());
                        }
                        else if(dbMode == DatabaseManagerOperationMode::CRCW) //sends to all DALs in the queue, from first to last
                        {
                            for(std::pair<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> currentDAL : dals)
                            {
                                currentDAL.second->get<0>()->getObject(currentRequest, currentRequestData->get<1>(), currentRequestData->get<2>());
                                pendingDALs.push_back(currentDAL.first);
                            }
                        }
                        else
                        {
                            logMessage(LogSeverity::Error, "(mainQueueThread) Unexpected DB operation mode encountered on SELECT request.");
                        }
                    } break;
                    
                    case RequestType::INSERT:
                    {
                        if(dbMode == DatabaseManagerOperationMode::PRCW || dbMode == DatabaseManagerOperationMode::CRCW)
                        {
                            for(std::pair<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> currentDAL : dals)
                            {
                                currentDAL.second->get<0>()->putObject(currentRequest, boost::any_cast<DataContainerPtr>(currentRequestData->get<1>()));
                                pendingDALs.push_back(currentDAL.first);
                            }
                        }
                        else if(dbMode == DatabaseManagerOperationMode::PRPW)
                        {
                            dals[dalIDs.front()]->get<0>()->putObject(currentRequest, boost::any_cast<DataContainerPtr>(currentRequestData->get<1>()));
                            pendingDALs.push_back(dalIDs.front());
                        }
                        else
                        {
                            logMessage(LogSeverity::Error, "(mainQueueThread) Unexpected DB operation mode encountered on INSERT request.");
                        }
                    } break;
                        
                    case RequestType::UPDATE:
                    {
                        if(dbMode == DatabaseManagerOperationMode::PRCW || dbMode == DatabaseManagerOperationMode::CRCW)
                        {
                            for(std::pair<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> currentDAL : dals)
                            {
                                currentDAL.second->get<0>()->updateObject(currentRequest, boost::any_cast<DataContainerPtr>(currentRequestData->get<1>()));
                                pendingDALs.push_back(currentDAL.first);
                            }
                        }
                        else if(dbMode == DatabaseManagerOperationMode::PRPW)
                        {
                            dals[dalIDs.front()]->get<0>()->updateObject(currentRequest, boost::any_cast<DataContainerPtr>(currentRequestData->get<1>()));
                            pendingDALs.push_back(dalIDs.front());
                        }
                        else
                        {
                            logMessage(LogSeverity::Error, "(mainQueueThread) Unexpected DB operation mode encountered on UPDATE request.");
                        }
                    } break;
                    
                    case RequestType::REMOVE:
                    {
                        if(dbMode == DatabaseManagerOperationMode::PRCW || dbMode == DatabaseManagerOperationMode::CRCW)
                        {
                            for(std::pair<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> currentDAL : dals)
                            {
                                currentDAL.second->get<0>()->removeObject(currentRequest, boost::any_cast<DBObjectID>(currentRequestData->get<1>()));
                                pendingDALs.push_back(currentDAL.first);
                            }
                        }
                        else if(dbMode == DatabaseManagerOperationMode::PRPW)
                        {
                            dals[dalIDs.front()]->get<0>()->removeObject(currentRequest, boost::any_cast<DBObjectID>(currentRequestData->get<1>()));
                            pendingDALs.push_back(dalIDs.front());
                        }
                        else
                        {
                            logMessage(LogSeverity::Error, "(mainQueueThread) Unexpected DB operation mode encountered on DELETE request.");
                        }
                    } break;
                    
                    default:
                    {
                        logMessage(LogSeverity::Error, "(mainQueueThread) Unexpected request type encountered for new request.");
                    } break;
                }
                
                pendingRequests.insert(std::pair<DatabaseRequestID, vector<DatabaseAbstractionLayerID>>(currentRequest, pendingDALs));
                newRequests.pop();
                
                logMessage(LogSeverity::Debug, "(mainQueueThread) Done with request <" + Convert::toString(i) + ">.");
            }
            
            logMessage(LogSeverity::Debug, "(mainQueueThread) Work on new requests finished.");
        }
        
        logMessage(LogSeverity::Debug, "(mainQueueThread) Data lock released.");
    }
    
    threadRunning = false;
    logMessage(LogSeverity::Debug, "(mainQueueThread) Stopped.");
    return;
}

void SyncServer_Core::DatabaseManagement::DALQueue::onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id)
{
    if(stopQueue)
        return;
    
    unsigned int readFailures = 0;
    unsigned int writeFailures = 0;
    
    {//ensures the locks are released as soon as they are not needed
        logMessage(LogSeverity::Debug, "(onFailureHandler) Entering critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> dataLock(threadMutex);
        logMessage(LogSeverity::Debug, "(onFailureHandler) Critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        
        if(requestsData[requestID]->get<0>() == RequestType::SELECT)
        {
            totalReadRequests++;
            totalReadFailures++;
            readFailures = ++(dals[dalID]->get<1>());
        }
        else
        {
            totalWriteRequests++;
            totalWriteFailures++;
            writeFailures = ++(dals[dalID]->get<2>());
        }
        
        if((readFailures >= maxConsecutiveReadFailures || writeFailures >= maxConsecutiveWriteFailures) && failureAction != DatabaseFailureAction::IGNORE_FAILURE)
        {
            logMessage(LogSeverity::Debug, "(onFailureHandler) Read/write failure detected during request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
            
            switch(failureAction)
            {
                case DatabaseFailureAction::DROP_DAL:
                {
                    dalIDs.erase(std::remove(dalIDs.begin(), dalIDs.end(), dalID));
                    dals[dalID]->get<3>().disconnect();
                    dals[dalID]->get<4>().disconnect();
                    delete dals[dalID];
                    dals.erase(dalID);
                } break;

                case DatabaseFailureAction::DROP_IF_NOT_LAST:
                {
                    if(dals.size() > 1)
                    {
                        dalIDs.erase(std::remove(dalIDs.begin(), dalIDs.end(), dalID));
                        dals[dalID]->get<3>().disconnect();
                        dals[dalID]->get<4>().disconnect();
                        delete dals[dalID];
                        dals.erase(dalID);
                    }
                } break;

                case DatabaseFailureAction::INITIATE_RECONNECT:
                {
                    //TODO - implement
                    logMessage(LogSeverity::Warning, "(onFailureHandler) INITIATE_RECONNECT failure action not implemented; request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
                } break;

                case DatabaseFailureAction::PUSH_TO_BACK:
                {
                    if(dals.size() > 1)
                    {
                        dalIDs.erase(std::remove(dalIDs.begin(), dalIDs.end(), dalID));
                        dalIDs.push_back(dalID);
                    }
                } break;

                default:
                {
                    logMessage(LogSeverity::Error, "(onFailureHandler) Unexpected DB failure action encountered.");
                } break;
            }
        }
        
        pendingRequests[requestID].erase(std::remove(pendingRequests[requestID].begin(), pendingRequests[requestID].end(), dalID));
        if(pendingRequests[requestID].size() == 0)
            pendingRequests.erase(requestID);

        delete requestsData[requestID];
        requestsData.erase(requestID);
        
        logMessage(LogSeverity::Debug, "(onFailureHandler) Exiting critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    logMessage(LogSeverity::Debug, "(onFailureHandler) Sending signal for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    onFailure(requestID, id);
    logMessage(LogSeverity::Debug, "(onFailureHandler) Signal sent for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
}

void SyncServer_Core::DatabaseManagement::DALQueue::onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr data)
{
    if(stopQueue)
        return;
    
    {
        logMessage(LogSeverity::Debug, "(onFailureHandler) Entering critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> dataLock(threadMutex);
        logMessage(LogSeverity::Debug, "(onFailureHandler) Critical section entered for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
        
        if(requestsData[requestID]->get<0>() == RequestType::SELECT)
        {
            totalReadRequests++;
            dals[dalID]->get<1>() = 0;
        }
        else
        {
            totalWriteRequests++;
            dals[dalID]->get<2>() = 0;
        }
        
        pendingRequests[requestID].erase(std::remove(pendingRequests[requestID].begin(), pendingRequests[requestID].end(), dalID));
        if(pendingRequests[requestID].size() == 0)
            pendingRequests.erase(requestID);

        delete requestsData[requestID];
        requestsData.erase(requestID);
        
        logMessage(LogSeverity::Debug, "(onFailureHandler) Exiting critical section for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    }
    
    logMessage(LogSeverity::Debug, "(onFailureHandler) Sending signal for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
    onSuccess(requestID, data);
    logMessage(LogSeverity::Debug, "(onFailureHandler) Signal sent for request/DAL <" + Convert::toString(requestID) + "/" + Convert::toString(dalID) + ">.");
}