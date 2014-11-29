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

SyncServer_Core::DatabaseManagement::DALQueue::DALQueue(DatabaseObjectType type, Utilities::FileLogger& parentLogger, DALQueueParameters parameters)
{
    queueType = type;
    dbMode = parameters.dbMode;
    failureAction = parameters.failureAction;
    maxConsecutiveReadFailures = parameters.maximumReadFailures;
    maxConsecutiveWriteFailures = parameters.maximumWriteFailures;
    stopQueue = false;
    logger = &parentLogger;
    mainThread = new boost::thread(&DatabaseManagement::DALQueue::mainQueueThread, this);
}

SyncServer_Core::DatabaseManagement::DALQueue::~DALQueue()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (~) > Destruction initiated.");
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
    
    logger = nullptr;
}

bool SyncServer_Core::DatabaseManagement::DALQueue::addDAL(DALPtr dal)
{
    if(stopQueue)
        return false;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add DAL) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add DAL) > Acquired data lock.");
    
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
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add DAL) > Sending notification to main thread.");
            threadLockCondition.notify_all();
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add DAL) > Notification to main thread sent.");
        }
        
        return true;
    }
    else
    {
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Add DAL) > The requested DatabaseAbstractionLayer is already in the DALs table.");
        return false;
    }
}

bool SyncServer_Core::DatabaseManagement::DALQueue::removeDAL(const DALPtr dal)
{
    if(stopQueue)
        return false;
    
    bool result = false;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Remove DAL) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Remove DAL) > Acquired data lock.");
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
        logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Remove DAL) > The requested DatabaseAbstractionLayer was not found in the DALs table.");
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Remove DAL) > Data lock released.");
    return result;
}

bool SyncServer_Core::DatabaseManagement::DALQueue::setParameters(DALQueueParameters parameters)
{
    if(stopQueue)
        return false;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Parameters) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Parameters) > Acquired data lock.");
    
    dbMode = parameters.dbMode;
    failureAction = parameters.failureAction;
    maxConsecutiveReadFailures = parameters.maximumReadFailures;
    maxConsecutiveWriteFailures = parameters.maximumWriteFailures;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Parameters) > Data lock released.");
    
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
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Cache Parameters) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Cache Parameters) > Acquired data lock.");
    
    if(dals.find(cacheID) != dals.end())
    {
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(dals[cacheID]->get<0>());
        
        if(cache)
            result = cache->setParameters(parameters);
        else
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Cache Parameters) > Operation failed; the requested ID does not refer to a DALCache object <" + Tools::toString(cacheID) + ">.");
    }
    else
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Cache Parameters) > Operation failed; the requested cache ID was not found <" + Tools::toString(cacheID) + ">.");
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Set Cache Parameters) > Data lock released.");
    
    return result;
}

DALCache::DALCacheParameters SyncServer_Core::DatabaseManagement::DALQueue::getCacheParameters(DatabaseAbstractionLayerID cacheID)
{
    DALCache::DALCacheParameters result;
    
    if(stopQueue)
        return result;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Cache Parameters) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Cache Parameters) > Acquired data lock.");
    
    if(dals.find(cacheID) != dals.end())
    {
        shared_ptr<DatabaseManagement::DALCache> cache = boost::dynamic_pointer_cast<DatabaseManagement::DALCache>(dals[cacheID]->get<0>());
        
        if(cache)
            result = cache->getParameters();
        else
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Cache Parameters) > Operation failed; the requested ID does not refer to a DALCache object <" + Tools::toString(cacheID) + ">.");
    }
    else
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Cache Parameters) > Operation failed; the requested cache ID was not found <" + Tools::toString(cacheID) + ">.");
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Cache Parameters) > Data lock released.");
    
    return result;
}

SyncServer_Core::DatabaseManagement::DALQueue::DALQueueInformation SyncServer_Core::DatabaseManagement::DALQueue::getQueueInformation() const
{
    return {totalReadFailures, totalWriteFailures, totalReadRequests, totalWriteRequests, queueType, dbMode, failureAction, dalIDs.size(),
            maxConsecutiveReadFailures, maxConsecutiveWriteFailures, stopQueue, threadRunning, newRequests.size(), pendingRequests.size()};
}

std::vector<SyncServer_Core::DatabaseManagement::DALCache::DALCacheInformation> SyncServer_Core::DatabaseManagement::DALQueue::getCachesInformation() const
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Caches Information) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get Caches Information) > Acquired data lock.");
    
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
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get DALs Information) > Acquiring data lock.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Get DALs Information) > Acquired data lock.");
    
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
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add Request To Queue) > Entering critical section.");
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add Request To Queue) > Critical section entered.");
    newRequests.push(nextRequestID);
    requestsData.insert(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*>(nextRequestID, data));
    nextRequestID++;
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add Request To Queue) > Sending notification to main thread.");
    threadLockCondition.notify_all();
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Add Request To Queue) > Notification to main thread sent.");

    return (nextRequestID - 1);
}

void SyncServer_Core::DatabaseManagement::DALQueue::mainQueueThread()
{
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Started.");
    threadRunning = true;
    
    while(!stopQueue)
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Acquiring data lock.");
        boost::unique_lock<boost::mutex> dataLock(threadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Data lock acquired.");
        
        if(dals.size() == 0)
        {
            logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > No DALs found; thread will sleep until a DAL is added.");
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Waiting on data lock.");
            threadLockCondition.wait(dataLock);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Data lock re-acquired after wait.");
        }
        else if(newRequests.size() == 0)
        {
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Waiting on data lock.");
            threadLockCondition.wait(dataLock);
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Data lock re-acquired after wait.");
        }
        else
        {
            if(newMode != DatabaseManagerOperationMode::INVALID)
                dbMode = newMode;
            
            unsigned int numberOfNewRequests = newRequests.size();
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Starting work on <" + Tools::toString(numberOfNewRequests) + "> new requests.");
            for(unsigned int i = 0; i < numberOfNewRequests; i++)
            {
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Working with request <" + Tools::toString(i) + ">.");
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
                            logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Unexpected DB operation mode encountered on SELECT request.");
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
                            logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Unexpected DB operation mode encountered on INSERT request.");
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
                            logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Unexpected DB operation mode encountered on UPDATE request.");
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
                            logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Unexpected DB operation mode encountered on DELETE request.");
                        }
                    } break;
                    
                    default:
                    {
                        logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Unexpected request type encountered for new request.");
                    } break;
                }
                
                pendingRequests.insert(std::pair<DatabaseRequestID, vector<DatabaseAbstractionLayerID>>(currentRequest, pendingDALs));
                newRequests.pop();
                
                logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Done with request <" + Tools::toString(i) + ">.");
            }
            
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Work on new requests finished.");
        }
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Data lock released.");
    }
    
    threadRunning = false;
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (Main Thread) > Stopped.");
    return;
}

void SyncServer_Core::DatabaseManagement::DALQueue::onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id)
{
    if(stopQueue)
        return;
    
    unsigned int readFailures = 0;
    unsigned int writeFailures = 0;
    
    {//ensures the locks are released as soon as they are not needed
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Entering critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> dataLock(threadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        
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
            logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Read/write failure detected during request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
            
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
                    logger->logMessage(Utilities::FileLogSeverity::Warning, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > INITIATE_RECONNECT failure action not implemented; request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
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
                    logger->logMessage(Utilities::FileLogSeverity::Error, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Unexpected DB failure action encountered.");
                } break;
            }
        }
        
        std::remove(pendingRequests[requestID].begin(), pendingRequests[requestID].end(), dalID);
        if(pendingRequests[requestID].size() == 0)
            pendingRequests.erase(requestID);

        delete requestsData[requestID];
        requestsData.erase(requestID);
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Exiting critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Sending signal for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    onFailure(requestID, id);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Failure Handler) > Signal sent for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
}

void SyncServer_Core::DatabaseManagement::DALQueue::onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr data)
{
    if(stopQueue)
        return;
    
    {
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Success Handler) > Entering critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        boost::lock_guard<boost::mutex> dataLock(threadMutex);
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Success Handler) > Critical section entered for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
        
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
        
        std::remove(pendingRequests[requestID].begin(), pendingRequests[requestID].end(), dalID);
        if(pendingRequests[requestID].size() == 0)
            pendingRequests.erase(requestID);

        delete requestsData[requestID];
        requestsData.erase(requestID);
        
        logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Success Handler) > Exiting critical section for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    }
    
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Success Handler) > Sending signal for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
    onSuccess(requestID, data);
    logger->logMessage(Utilities::FileLogSeverity::Debug, "DALQueue / " + Tools::toString(queueType) + " (On Success Handler) > Signal sent for request/DAL <" + Tools::toString(requestID) + "/" + Tools::toString(dalID) + ">.");
}