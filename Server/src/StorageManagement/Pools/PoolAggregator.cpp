/* 
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

#include "PoolAggregator.h"

StorageManagement_Pools::PoolAggregator::PoolAggregator
(PoolAggregatorInitParameters parameters, Utilities::FileLogger * debugLogger)
: debugLogger(debugLogger), threadPool(parameters.threadPoolSize, debugLogger),
  completeRetrieve(parameters.completeRetrieve), completeDiscard(parameters.completeDiscard),
  completePendingStore(parameters.completePendingStore), eraseOnDiscard(parameters.eraseOnDiscard),
  cancelActionsOnShutdown(parameters.cancelActionsOnShutdown),
  maxNonStreamableData(parameters.maxNonStreamableData)
{
    lastEntityID = INVALID_STORED_DATA_ID;
    totalFreeSpace = 0;
    totalUsableSpace = 0;
    maxFreeSpace = 0;
    maxUsableSpace = 0;
    
    lastPoolID = INVALID_POOL_ID;
    aggregatorID = ++lastPoolID;
    pools.insert({aggregatorID, nullptr});
    links.insert({aggregatorID, std::deque<LinkParameters>()});
    
    if(parameters.streamingPool != nullptr)
    {
        if(parameters.streamingPool->areOutputStreamsSupported())
        {
            streamingPoolID = ++lastPoolID;
            pools.insert({streamingPoolID, parameters.streamingPool});
            links.insert({streamingPoolID, std::deque<LinkParameters>()});
            
            totalFreeSpace += parameters.streamingPool->getFreeSpace();
            maxFreeSpace += parameters.streamingPool->getPoolSize();
        }
        else
        {
            throw std::invalid_argument("PoolAggregator::() > Aggregator initialization "
                    "failed; the supplied streaming pool ["
                    + Convert::toString(parameters.streamingPool->getPoolUUID())
                    + "] does not support output streams.");
        }
    }
    else
        streamingPoolID = INVALID_POOL_ID;
    
    pendingActionsProcessingEnabled = false;
    state = PoolState::OPEN;
    mode = parameters.mode;
    bytesRead = 0;
    bytesWritten = 0;
    uuid = boost::uuids::random_generator()();
}

StorageManagement_Pools::PoolAggregator::PoolAggregator
(const PoolAggregatorLoadParameters parameters, Utilities::FileLogger * debugLogger)
: debugLogger(debugLogger), threadPool(parameters.threadPoolSize),
  completeRetrieve(parameters.completeRetrieve), completeDiscard(parameters.completeDiscard),
  completePendingStore(parameters.completePendingStore), eraseOnDiscard(parameters.eraseOnDiscard),
  cancelActionsOnShutdown(parameters.cancelActionsOnShutdown),
  maxNonStreamableData(parameters.maxNonStreamableData), lastEntityID(parameters.lastEntityID)
{
    totalFreeSpace = 0;
    totalUsableSpace = 0;
    maxFreeSpace = 0;
    maxUsableSpace = 0;

    lastPoolID = INVALID_POOL_ID;
    aggregatorID = ++lastPoolID;
    streamingPoolID = INVALID_POOL_ID;
    pools.insert({aggregatorID, nullptr});
    links.insert({aggregatorID, std::deque<LinkParameters>()});
    
    pendingActionsProcessingEnabled = false;
    state = PoolState::OPEN;
    mode = parameters.mode;
    bytesRead = parameters.bytesRead;
    bytesWritten = parameters.bytesWritten;
    uuid = parameters.uuid;

    //builds the pools map
    for(auto currentPoolData : parameters.pools)
    {
        if(currentPoolData.second == nullptr
           || currentPoolData.second->getPoolUUID() == INVALID_POOL_UUID
           || currentPoolData.first != currentPoolData.second->getPoolUUID())
        {
            throw std::invalid_argument("PoolAggregator::() > Aggregator initialization failed; the supplied pool ["
                    + Convert::toString(currentPoolData.second->getPoolUUID()) + "] is not valid.");
        }
        
        StoredDataID newPoolID = ++lastPoolID;
        pools.insert({newPoolID, currentPoolData.second});
        links.insert({newPoolID, std::deque<LinkParameters>()});
        
        if(currentPoolData.first == parameters.streamingPoolUUID)
        {
            if(currentPoolData.second->areOutputStreamsSupported())
                streamingPoolID = newPoolID;
            else
            {
                throw std::invalid_argument("PoolAggregator::() > Aggregator "
                        "initialization failed; the supplied streaming pool ["
                        + Convert::toString(parameters.streamingPoolUUID)
                        + "] does not support output streams.");
            }
        }
        
        totalFreeSpace += currentPoolData.second->getFreeSpace();
        maxFreeSpace += currentPoolData.second->getPoolSize();
    }

    //builds the links map
    for(auto currentPoolLinkData : parameters.links)
    {
        if(currentPoolLinkData.first != uuid
           && parameters.pools.find(currentPoolLinkData.first) == parameters.pools.end())
        {
            throw std::invalid_argument("PoolAggregator::() > Aggregator initialization "
                    "failed; the supplied link data is for a source pool ["
                    + Convert::toString(currentPoolLinkData.first)
                    + "] that is not present.");
        }
        
        PoolID sourcePool = getPoolID(currentPoolLinkData.first);
        std::deque<LinkParameters> & sourcePoolLinkData = links.at(sourcePool);

        for(const PersistentLinkParameters & currentLinkData : currentPoolLinkData.second)
        {
            if(currentLinkData.targetPool != INVALID_POOL_UUID
               && parameters.pools.find(currentLinkData.targetPool) == parameters.pools.end())
            {
                throw std::invalid_argument("PoolAggregator::() > Aggregator "
                        "initialization failed; the supplied link ["
                        + Convert::toString(currentLinkData.action)
                        + "] is for a target pool ["
                        + Convert::toString(currentLinkData.targetPool)
                        + "] that is not present.");
            }

            PoolID targetPool = getPoolID(currentLinkData.targetPool);

            //checks if the source and target pools already have a defined link
            for(LinkParameters & existingLink : sourcePoolLinkData)
            {
                if(existingLink.targetPool == targetPool)
                {
                    throw std::invalid_argument("PoolAggregator::() > Aggregator "
                            "initialization failed; there is a link ["
                            + Convert::toString(existingLink.action)
                            + "] already defined for the specified source ["
                            + Convert::toString(currentPoolLinkData.first)
                            + "] and target pools ["
                            + Convert::toString(currentLinkData.targetPool)
                            + "].");
                }
            }

            sourcePoolLinkData.push_back({targetPool,
                                          currentLinkData.action,
                                          currentLinkData.condition,
                                          currentLinkData.conditionValue});
        }
    }

    recalculateUsableSpace();
}

StorageManagement_Pools::PoolAggregator::~PoolAggregator()
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);
    
    state = PoolState::CLOSED;
    
    if(pendingStorageActions.size() > 0)
    {
        logDebugMessage("(~) > There are <" 
                + Convert::toString(pendingStorageActions.size())
                + "> storage actions still pending.");
        
        pendingStorageActions.clear();
        pendingStorageActionsCount.clear();
    }

    idMap.clear();
    pools.clear();
    links.clear();
    
    if(cancelActionsOnShutdown)
        threadPool.stopThreadPool();

    debugLogger = nullptr;
}

//<editor-fold defaultstate="collapsed" desc="DataPool Functions">
ByteVectorPtr StorageManagement_Pools::PoolAggregator::retrieveData(StoredDataID id)
{
    if(id == INVALID_STORED_DATA_ID)
    {
        throw std::invalid_argument("PoolAggregator::retrieveData() > Failed to retrieve data; "
                "the specified entity ID is not valid.");
    }
    
    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::retrieveData() > Failed to retrieve data; "
                "the aggregator is not in an open state.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto requestedEntityData = idMap.find(id);
    if(requestedEntityData != idMap.end())
    {
        //goes through each pool that has the entity and attempts to retrieve the data
        for(InternalEntityID & currentEntityIDData : requestedEntityData->second)
        {
            try
            {
                DataPool * sourcePool = pools.at(currentEntityIDData.poolID);
                DataSize dataSize = sourcePool->getEntitySize(id);
                if(maxNonStreamableData != 0 && dataSize > maxNonStreamableData)
                {
                    throw std::runtime_error("PoolAggregator::storeData() > Failed to retrieve data; "
                            "too much non-streamable data requested: ["
                            + Convert::toString(dataSize) + "] bytes.");
                }
                
                ByteVectorPtr result = sourcePool->retrieveData(currentEntityIDData.entityID);
                bytesRead += dataSize;
                return result;
            }
            catch(const std::exception & ex)
            {
                if(!completeRetrieve)
                    throw;

                logDebugMessage("(retrieveData) > Exception encountered during data retrieval: ["
                        + std::string(ex.what()) + "].");
            }
        }

        throw std::runtime_error("PoolAggregator::retrieveData() > Failed to retrieve "
                "the requested data; no pools were able to satisfy the request.");
    }
    else
    {
        throw std::runtime_error("PoolAggregator::retrieveData() > Failed to retrieve "
                "the requested data; ID [" + Convert::toString(id) + "] not found.");
    }
}

StoredDataID StorageManagement_Pools::PoolAggregator::storeData(const ByteVectorPtr data)
{
    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::storeData() > Failed to store data; "
                "the aggregator is not in an open state.");
    }

    if(mode != PoolMode::READ_WRITE)
    {
        throw std::runtime_error("PoolAggregator::storeData() > Failed to store data; "
                "the aggregator is not in read/write mode.");
    }
    
    if(data->size() > 0)
    {
        boost::lock_guard<boost::mutex> dataLock(dataMutex);

        std::deque<PlainLinkData> storageSequence = unwindPoolChain(aggregatorID, data->size());
        if(storageSequence.size() == 0)
        {
            throw std::runtime_error("PoolAggregator::storeData() > Failed to store data; "
                    "no valid storage sequence was found.");
        }

        StoredDataID newEntityID = ++lastEntityID;
        
        //creates a pointer to the specific function overload required here
        void (StorageManagement_Pools::PoolAggregator::*processStoreActionSequencePtr)
              (const ByteVectorPtr, const std::deque<PlainLinkData>, StoredDataID)
                 = &StorageManagement_Pools::PoolAggregator::processStoreActionSequence;
        
        threadPool.assignTask(boost::bind(processStoreActionSequencePtr, this,
                                          data, storageSequence, newEntityID));

        return newEntityID;
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::storeData() > Failed to store data; "
                "no data supplied.");
    }
}

void StorageManagement_Pools::PoolAggregator::discardData(StoredDataID id, bool erase)
{
    if(id == INVALID_STORED_DATA_ID)
    {
        throw std::invalid_argument("PoolAggregator::discardData() > Failed to discard data; "
                "the specified entity ID is not valid.");
    }
    
    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::discardData() > Failed to discard data; "
                "the aggregator is not in an open state.");
    }

    if(mode != PoolMode::READ_WRITE)
    {
        throw std::runtime_error("PoolAggregator::discardData() > Failed to discard data; "
                "the aggregator is not in read/write mode.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto requestedEntityData = idMap.find(id);
    if(requestedEntityData != idMap.end())
    {
        DataSize discardedSpace = 0;

        //goes through each pool that has the entity and attempts to discard the data
        for(InternalEntityID & currentEntityIDData : requestedEntityData->second)
        {
            try
            {
                DataPool * currentPool = pools.at(currentEntityIDData.poolID);
                DataSize prevPoolFreeSpace = currentPool->getFreeSpace();
                currentPool->discardData(currentEntityIDData.entityID, erase);
                discardedSpace += (currentPool->getFreeSpace() - prevPoolFreeSpace);
            }
            catch(const std::exception & ex)
            {
                if(!completeDiscard)
                    throw;

                logDebugMessage("(discardData) > Exception encountered during data discard: ["
                        + std::string(ex.what()) + "].");
            }
        }

        //checks for and discards any pending actions for the affected entity
        auto pendingActions = pendingStorageActionsCount.find(id);
        if(pendingActions != pendingStorageActionsCount.end())
        {
            unsigned int pendingActionsCount = pendingActions->second;
            pendingStorageActionsCount.erase(pendingActions);

            std::deque<PendingStorageAction> remainingActions;

            auto currentAction = pendingStorageActions.begin();
            while(pendingActionsCount > 0 || currentAction != pendingStorageActions.end())
            {
                if(currentAction->entityID != id)
                    remainingActions.push_back(*currentAction);
                else
                    --pendingActionsCount;

                ++currentAction;
            }

            pendingStorageActions.swap(remainingActions);
        }

        totalFreeSpace += discardedSpace;
        idMap.erase(requestedEntityData);
        recalculateUsableSpace();
    }
    else
    {
        throw std::runtime_error("PoolAggregator::discardData() > Failed to discard "
                "the requested data; ID [" + Convert::toString(id) + "] not found.");
    }
}

void StorageManagement_Pools::PoolAggregator::clearPool()
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    pendingStorageActions.clear();
    pendingStorageActionsCount.clear();

    totalFreeSpace = maxFreeSpace;
    totalUsableSpace = maxUsableSpace;

    for(auto currentPoolPair : pools)
        currentPoolPair.second->clearPool();

    idMap.clear();
}

bool StorageManagement_Pools::PoolAggregator::canStoreData(DataSize size) const
{
    try
    {
        boost::lock_guard<boost::mutex> dataLock(dataMutex);
        return (unwindPoolChain(aggregatorID, size).size() > 0);
    }
    catch(const std::exception & ex)
    {
        logDebugMessage("(canStoreData) > Exception encountered: ["
                + std::string(ex.what()) + "].");
        
        return false;
    }
}

DataSize StorageManagement_Pools::PoolAggregator::getEntitySize(StoredDataID id) const
{
    if(state != PoolState::OPEN)
        return 0;

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto requestedEntityData = idMap.find(id);
    if(requestedEntityData != idMap.end())
    {
        //goes through each pool that has the entity and attempts to retrieve the size
        for(const InternalEntityID & currentEntityIDData : requestedEntityData->second)
        {
            try
            {
                DataSize result = pools.at(currentEntityIDData.poolID)->getEntitySize(currentEntityIDData.entityID);
                return result;
            }
            catch(const std::exception & ex)
            {
                if(!completeRetrieve)
                    return 0;

                logDebugMessage("(getEntitySize) > Exception encountered during "
                        "entity size retrieval: [" + std::string(ex.what()) + "].");
            }
        }
    }

    return 0;
}

PoolInputStreamPtr StorageManagement_Pools::PoolAggregator::getInputStream(StoredDataID id)
{
    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::getInputStream() > Failed to retrieve "
                "the requested input stream; the aggregator is not in an open state.");
    }
    
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto requestedEntityData = idMap.find(id);
    if(requestedEntityData != idMap.end())
    {
        //goes through each pool that has the entity and attempts to retrieve the stream
        for(InternalEntityID & currentEntityIDData : requestedEntityData->second)
        {
            try
            {
                DataPool * currentPool = pools.at(currentEntityIDData.poolID);

                if(currentPool->areInputStreamsSupported())
                {
                    bytesRead += currentPool->getEntitySize(currentEntityIDData.entityID);
                    return currentPool->getInputStream(currentEntityIDData.entityID);
                }
            }
            catch(const std::exception & ex)
            {
                if(!completeRetrieve)
                    throw;

                logDebugMessage("(getInputStream) > Exception encountered during "
                        "input stream retrieval: [" + std::string(ex.what()) + "].");
            }
        }

        throw std::runtime_error("PoolAggregator::getInputStream() > Failed to retrieve "
                "the requested input stream; no pools were able to satisfy the request.");
    }
    else
    {
        throw std::runtime_error("PoolAggregator::getInputStream() > Failed to retrieve "
                "the requested input stream; ID [" + Convert::toString(id) + "] not found.");
    }
}

PoolOutputStreamPtr StorageManagement_Pools::PoolAggregator::getOutputStream(DataSize dataSize)
{
    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::getOutputStream() > Failed to retrieve "
                "output stream; the aggregator is not in an open state.");
    }

    if(mode != PoolMode::READ_WRITE)
    {
        throw std::runtime_error("PoolAggregator::getOutputStream() > Failed to retrieve "
                "output stream; the aggregator is not in read/write mode.");
    }

    if(dataSize > 0)
    {
        boost::lock_guard<boost::mutex> dataLock(dataMutex);

        if(streamingPoolID == INVALID_POOL_ID)
        {
            throw std::logic_error("PoolAggregator::getOutputStream() > Failed to retrieve "
                    "output stream; no streaming pool is available for incoming data.");
        }

        DataPool * streamingPool = pools.at(streamingPoolID);

        if(!streamingPool->canStoreData(dataSize))
        {
            throw std::runtime_error("PoolAggregator::getOutputStream() > Failed to retrieve "
                    "output stream; the streaming pool cannot store ["
                    + Convert::toString(dataSize) + "] bytes of data.");
        }

        StoredDataID newEntityID = ++lastEntityID;

        PoolOutputStreamPtr stream = streamingPool->getOutputStream(dataSize);
        std::deque<InternalEntityID> dataIDs{{streamingPoolID, stream->getDataID()}};
        idMap.insert({newEntityID, dataIDs});
        stream->resetDataID(newEntityID);
        totalFreeSpace -= (dataSize + streamingPool->getEntityManagementStorageOverhead());
        return stream;
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::getOutputStream() > Failed to retrieve "
                "output stream; no data supplied.");
    }
}

void StorageManagement_Pools::PoolAggregator::releaseStreamedData(StoredDataID streamedEntityID)
{
    if(streamedEntityID == INVALID_STORED_DATA_ID)
    {
        throw std::invalid_argument("PoolAggregator::releaseStreamedData() > Failed to release "
                "streamed data; the specified entity ID is not valid.");
    }

    if(state != PoolState::OPEN)
    {
        throw std::runtime_error("PoolAggregator::releaseStreamedData() > Failed to release "
                "streamed data; the aggregator is not in an open state.");
    }

    if(mode != PoolMode::READ_WRITE)
    {
        throw std::runtime_error("PoolAggregator::releaseStreamedData() > Failed to release "
                "streamed data; the aggregator is not in read/write mode.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto requestedEntityData = idMap.find(streamedEntityID);
    if(requestedEntityData != idMap.end())
    {
        if(requestedEntityData->second.size() != 1
           || requestedEntityData->second[0].poolID != streamingPoolID)
        {
            throw std::runtime_error("PoolAggregator::releaseStreamedData() > Failed to release "
                    "streamed data; unexpected ID data for entity ["
                    + Convert::toString(streamedEntityID) + "] encountered.");
        }

        //retrieves the usual sequence for storing data
        DataSize entitySize = pools.at(streamingPoolID)->getEntitySize(requestedEntityData->second[0].entityID);
        std::deque<PlainLinkData> storageSequence = unwindPoolChain(aggregatorID, entitySize);

        if(storageSequence.size() == 0)
        {
            throw std::runtime_error("PoolAggregator::releaseStreamedData() > Failed to release "
                    "streamed data; no valid storage sequence was found.");
        }

        Seconds longestDelayTime = 0;
        //goes through each action and updates the source, where necessary
        for(PlainLinkData & currentLinkData : storageSequence)
        {
            if(currentLinkData.source == aggregatorID)
            {
                //the streaming pool replaces the aggregator as the source
                currentLinkData.source = streamingPoolID;

                if(currentLinkData.delayTime > longestDelayTime)
                    longestDelayTime = currentLinkData.delayTime;
            }
        }

        //ensures that the data will be removed from the streaming pool
         //only after all other actions have been performed
        storageSequence.push_back({SimpleLinkActionType::REMOVE,
                                   streamingPoolID,
                                   INVALID_POOL_ID,
                                   longestDelayTime});

        //creates a pointer to the specific function overload required here
        void (StorageManagement_Pools::PoolAggregator::*processStoreActionSequencePtr)
             (DataSize, const std::deque<PlainLinkData>, StoredDataID)
                = &StorageManagement_Pools::PoolAggregator::processStoreActionSequence;

        threadPool.assignTask(boost::bind(processStoreActionSequencePtr, this,
                                          entitySize, storageSequence, streamedEntityID));
    }
    else
    {
        throw std::runtime_error("PoolAggregator::releaseStreamedData() > Failed to release "
                "streamed data; ID [" + Convert::toString(streamedEntityID) + "] not found.");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Pool & Link Management">
PoolID StorageManagement_Pools::PoolAggregator::addPool(DataPool * pool)
{
    if(pool == nullptr)
    {
        throw std::invalid_argument("PoolAggregator::addPool() > Pool addition failed; "
                "the specified pool is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    for(auto currentPool : pools)
    {
        if(currentPool.second == pool)
        {
            throw std::invalid_argument("PoolAggregator::addPool() > Pool addition failed; "
                    "the specified pool [" + Convert::toString(pool->getPoolUUID())
                    + "] has already been added as ["
                    + Convert::toString(currentPool.first) + "].");
        }
    }

    StoredDataID newPoolID = ++lastPoolID;
    pools.insert({newPoolID, pool});
    links.insert({newPoolID, std::deque<LinkParameters>()});

    totalFreeSpace += pool->getFreeSpace();
    maxFreeSpace += pool->getPoolSize();

    return newPoolID;
}

void StorageManagement_Pools::PoolAggregator::removePool(PoolID pool)
{
    if(pool == INVALID_POOL_ID)
    {
        throw std::invalid_argument("PoolAggregator::removePool() > Pool removal failed; "
                "the specified pool ID is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto poolData = pools.find(pool);
    if(poolData != pools.end())
    {
        //deletes all links that have the pool as a source
        links.at(pool).clear();

        //deletes all links that have the pool as a target
        for(std::pair<PoolID, std::deque<LinkParameters>> currentLinksData : links)
        {
            std::deque<LinkParameters> swapLinks;
            for(LinkParameters & currentLink : currentLinksData.second)
            {
                if(currentLink.targetPool != pool)
                    swapLinks.push_back(currentLink);
            }

            if(swapLinks.size() != currentLinksData.second.size())
            {
                links.at(currentLinksData.first).swap(swapLinks);
            }
        }

        recalculateUsableSpace();
        
        removeIDsForPool(pool);

        totalFreeSpace -= poolData->second->getFreeSpace();
        maxFreeSpace -= poolData->second->getPoolSize();

        pools.erase(poolData);
        links.erase(pool);
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::removePool() > Pool removal failed; "
                "the specified pool [" + Convert::toString(pool) + "] was not found.");
    }
}

void StorageManagement_Pools::PoolAggregator::addPoolLink
(PoolID sourcePool, const LinkParameters params)
{
    if(sourcePool == INVALID_POOL_ID)
    {
        throw std::invalid_argument("PoolAggregator::addPoolLink() > Pool link addition failed; "
                "the specified source pool ID is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    if(pools.find(sourcePool) != pools.end())
    {
        //checks if the source and target pools already have a defined link
        std::deque<LinkParameters> & sourcePoolLinkData = links.at(sourcePool);
        for(LinkParameters & currentLink : sourcePoolLinkData)
        {
            if(currentLink.targetPool == params.targetPool)
            {
                throw std::invalid_argument("PoolAggregator::addPoolLink() > Pool link addition failed; "
                        "there is a link [" + Convert::toString(currentLink.action)
                        + "] already defined for the specified source ["
                        + Convert::toString(sourcePool) + "] and target ["
                        + Convert::toString(params.targetPool) + "] pools.");
            }
        }

        sourcePoolLinkData.push_back(params);

        recalculateUsableSpace();
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::addPoolLink() > Pool link addition failed; "
                "the specified source pool [" + Convert::toString(sourcePool)
                + "] was not found.");
    }
}

void StorageManagement_Pools::PoolAggregator::removePoolLink(PoolID sourcePool, PoolID targetPool)
{
    if(sourcePool == INVALID_POOL_ID)
    {
        throw std::invalid_argument("PoolAggregator::removePoolLink() > Pool link removal failed; "
                "the specified source pool ID is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    if(pools.find(sourcePool) != pools.end())
    {
        std::deque<LinkParameters> & sourcePoolLinkData = links.at(sourcePool);

        bool linkRemoved = false;
        for(auto currentLink = sourcePoolLinkData.begin(); currentLink != sourcePoolLinkData.end(); currentLink++)
        {
            if(currentLink->targetPool == targetPool)
            {
                sourcePoolLinkData.erase(currentLink);
                linkRemoved = true;
                break;
            }
        }

        if(!linkRemoved)
        {
            throw std::invalid_argument("PoolAggregator::removePoolLink() > Pool link removal failed; "
                    "the specified target pool [" + Convert::toString(targetPool)
                    + "] was not found.");
        }

        recalculateUsableSpace();
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::removePoolLink() > Pool link removal failed; "
                "the specified source pool [" + Convert::toString(sourcePool)
                + "] was not found.");
    }
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Import/Export Functions">
StorageManagement_Pools::PoolAggregator::PoolAggregatorLoadParameters
StorageManagement_Pools::PoolAggregator::exportConfiguration() const
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    PoolAggregatorLoadParameters result;

    result.threadPoolSize           = threadPool.getPoolSize();
    result.completeRetrieve         = completeRetrieve;
    result.completeDiscard          = completeDiscard;
    result.completePendingStore     = completePendingStore;
    result.eraseOnDiscard           = eraseOnDiscard;
    result.cancelActionsOnShutdown  = cancelActionsOnShutdown;
    result.maxNonStreamableData     = maxNonStreamableData;
    result.lastEntityID             = lastEntityID;
    result.mode                     = mode;
    result.bytesRead                = bytesRead;
    result.bytesWritten             = bytesWritten;
    result.uuid                     = uuid;
    result.streamingPoolUUID        = (streamingPoolID != INVALID_POOL_ID) 
                                       ? pools.at(streamingPoolID)->getPoolUUID()
                                       : INVALID_POOL_UUID;

    for(auto currentPool : pools)
    {
        if(currentPool.first == aggregatorID)
            continue;
        
        result.pools.insert({currentPool.second->getPoolUUID(), currentPool.second});
    }

    for(auto currentLink : links)
    {
        std::deque<PersistentLinkParameters> currentLinkParametersList;

        for(const LinkParameters & currentLinkParameter : currentLink.second)
        {
            PoolUUID targetPoolUUID = (currentLinkParameter.targetPool != INVALID_POOL_ID)
                                        ? pools.at(currentLinkParameter.targetPool)->getPoolUUID()
                                        : INVALID_POOL_UUID;

            currentLinkParametersList.push_back({targetPoolUUID,
                                                 currentLinkParameter.action,
                                                 currentLinkParameter.condition,
                                                 currentLinkParameter.conditionValue});
        }

        if(currentLink.first == aggregatorID)
            result.links.insert({uuid, currentLinkParametersList});
        else
            result.links.insert({pools.at(currentLink.first)->getPoolUUID(), currentLinkParametersList});
    }

    return result;
}

boost::unordered_map<PoolUUID, std::deque<StorageManagement_Pools::PoolAggregator::EntityIDData>>
StorageManagement_Pools::PoolAggregator::exportIDData() const
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    boost::unordered_map<PoolUUID, std::deque<EntityIDData>> result;

    for(auto currentEntityData : idMap)
    {
        for(const InternalEntityID & currentEntityID : currentEntityData.second)
        {
            result[pools.at(currentEntityID.poolID)->getPoolUUID()].push_back({currentEntityData.first, 
                                                                               currentEntityID.entityID});
        }
    }

    return result;
}

std::deque<StorageManagement_Pools::PoolAggregator::EntityIDData>
StorageManagement_Pools::PoolAggregator::exportIDDataForPool(PoolID pool) const
{
    if(pool == INVALID_POOL_ID)
    {
        throw std::invalid_argument("PoolAggregator::exportIDDataForPool(ID) > Pool ID data export failed; "
                "the specified pool ID is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto poolData = pools.find(pool);
    if(poolData != pools.end())
    {
        std::deque<EntityIDData> result;

        if(poolData->second->getStoredEntitiesNumber() > 0)
        {
            for(auto currentEntityData : idMap)
            {
                for(const InternalEntityID & currentEntityID : currentEntityData.second)
                {
                    if(currentEntityID.poolID == pool)
                        result.push_back({currentEntityData.first, currentEntityID.entityID});
                }
            }
        }

        return result;
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::exportIDDataForPool(ID) > Pool ID data export failed; "
                "the specified pool [" + Convert::toString(pool) + "] was not found.");
    }
}

std::deque<StorageManagement_Pools::PoolAggregator::EntityIDData>
StorageManagement_Pools::PoolAggregator::exportIDDataForPool(PoolUUID pool) const
{
    if(pool == INVALID_POOL_UUID)
    {
        throw std::invalid_argument("PoolAggregator::exportIDDataForPool(UUID) > Pool ID data export failed; "
                "the specified pool ID is not valid.");
    }

    PoolID poolID = INVALID_POOL_ID;
    {
        boost::lock_guard<boost::mutex> dataLock(dataMutex);
        poolID = getPoolID(pool);
    }

    return exportIDDataForPool(poolID);
}

std::deque<StorageManagement_Pools::PoolAggregator::PoolEntityIDData>
StorageManagement_Pools::PoolAggregator::exportIDDataForEntity(StoredDataID entity) const
{
    if(entity == INVALID_STORED_DATA_ID)
    {
        throw std::invalid_argument("PoolAggregator::exportIDDataForEntity() > Entity ID data export failed; "
                "the specified entity ID is not valid.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto entityData = idMap.find(entity);
    if(entityData != idMap.end())
    {
        std::deque<PoolEntityIDData> result;

        for(const InternalEntityID & currentID : entityData->second)
            result.push_back({pools.at(currentID.poolID)->getPoolUUID(), currentID.entityID});

        return result;
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::exportIDDataForEntity() > Entity ID data export failed; "
                "the specified entity [" + Convert::toString(entity) + "] was not found.");
    }
}

void StorageManagement_Pools::PoolAggregator::importIDData
(const boost::unordered_map<PoolUUID, std::deque<EntityIDData>> & idData, bool verify)
{
    if(idData.size() == 0)
    {
        throw std::invalid_argument("PoolAggregator::importIDData() > ID data import failed; "
                "no data supplied.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    for(auto currentPoolData : idData)
    {
        PoolID pool = getPoolID(currentPoolData.first);

        if(verify)
        {
            if(pool == INVALID_POOL_ID)
            {
                throw std::invalid_argument("PoolAggregator::importIDData() > ID data import failed; "
                        "an invalid pool ID was found.");
            }

            if(pools.find(pool) == pools.end())
            {
                throw std::invalid_argument("PoolAggregator::importIDData() > ID data import failed; "
                        "the specified pool [" + Convert::toString(pool) + "] was not found.");
            }

            if(currentPoolData.second.size() == 0)
            {
                throw std::invalid_argument("PoolAggregator::importIDData() > ID data import failed; "
                        "no data supplied for pool [" + Convert::toString(pool) + "].");
            }
        }

        for(const EntityIDData & currentEntityData : currentPoolData.second)
        {
            if(verify)
            {
                if(currentEntityData.aggregatorEntityID == INVALID_STORED_DATA_ID
                   || currentEntityData.poolEntityID == INVALID_STORED_DATA_ID)
                {
                    throw std::invalid_argument("PoolAggregator::importIDData() > "
                            "ID data import failed; the supplied data ["
                            + Convert::toString(currentEntityData.aggregatorEntityID) 
                            + "/" + Convert::toString(currentEntityData.poolEntityID)
                            + "] is not valid.");
                }

                if(pools.at(pool)->getEntitySize(currentEntityData.poolEntityID) <= 0)
                {
                    throw std::invalid_argument("PoolAggregator::importIDData() > ID data import failed;"
                            "the specified pool [" + Convert::toString(pool)
                            + "] does not have the specified entity ["
                            + Convert::toString(currentEntityData.poolEntityID) + "].");
                }
            }

            idMap[currentEntityData.aggregatorEntityID].push_back({pool, currentEntityData.poolEntityID});
        }
    }
}

void StorageManagement_Pools::PoolAggregator::importIDDataForPool
(PoolID pool, const std::deque<EntityIDData> & idData, bool verify)
{
    if(pool == INVALID_POOL_ID)
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForPool(ID) > "
                "Pool ID data import failed; the specified pool ID is not valid.");
    }

    if(idData.size() == 0)
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForPool(ID) > "
                "Pool ID data import failed; no data supplied.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    if(verify && pools.find(pool) == pools.end())
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForPool(ID) > "
                "Pool ID data import failed; the specified pool ["
                + Convert::toString(pool) + "] was not found.");
    }

    for(const EntityIDData & currentIDData : idData)
    {
        if(verify)
        {
            if(currentIDData.aggregatorEntityID == INVALID_STORED_DATA_ID 
               || currentIDData.poolEntityID == INVALID_STORED_DATA_ID)
            {
                throw std::invalid_argument("PoolAggregator::importIDData(ID) > "
                        "ID data import failed; the supplied data ["
                        + Convert::toString(currentIDData.aggregatorEntityID) + "/"
                        + Convert::toString(currentIDData.poolEntityID)
                        + "] is not valid.");
            }

            if(pools.at(pool)->getEntitySize(currentIDData.poolEntityID) <= 0)
            {
                throw std::invalid_argument("PoolAggregator::importIDData(ID) > "
                        "ID data import failed; the specified pool ["
                        + Convert::toString(pool)
                        + "] does not have the specified entity ["
                        + Convert::toString(currentIDData.poolEntityID)
                        + "].");
            }
        }

        idMap[currentIDData.aggregatorEntityID].push_back({pool, currentIDData.poolEntityID});
    }
}

void StorageManagement_Pools::PoolAggregator::importIDDataForPool
(PoolUUID pool, const std::deque<EntityIDData> & idData, bool verify)
{
    if(pool == INVALID_POOL_UUID)
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForPool(UUID) > "
                "Pool ID data import failed; the specified pool ID is not valid.");
    }

    PoolID poolID = INVALID_POOL_ID;
    {
        boost::lock_guard<boost::mutex> dataLock(dataMutex);
        poolID = getPoolID(pool);
    }

    importIDDataForPool(poolID, idData, verify);
}

void StorageManagement_Pools::PoolAggregator::importIDDataForEntity
(StoredDataID entity, const std::deque<PoolEntityIDData> & idData, bool verify)
{
    if(entity == INVALID_STORED_DATA_ID)
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForEntity() > "
                "Entity ID data import failed; the specified entity ID is not valid.");
    }

    if(idData.size() == 0)
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForEntity() > "
                "Entity ID data import failed; no data supplied.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    auto entityData = idMap.find(entity);
    if(entityData == idMap.end())
    {
        for(const PoolEntityIDData & currentIDData : idData)
        {
            PoolID pool = getPoolID(currentIDData.pool);

            if(verify)
            {
                if(currentIDData.entity == INVALID_STORED_DATA_ID || pool == INVALID_POOL_ID)
                {
                    throw std::invalid_argument("PoolAggregator::importIDDataForEntity() > "
                        "Entity ID data import failed; the supplied data ["
                        + Convert::toString(currentIDData.entity) + "/"
                        + Convert::toString(pool) + "] is not valid.");
                }

                if(pools.at(pool)->getEntitySize(currentIDData.entity) <= 0)
                {
                    throw std::invalid_argument("PoolAggregator::importIDDataForEntity() > "
                        "Entity ID data import failed; the specified pool ["
                        + Convert::toString(pool) + "] does not have the specified entity ["
                        + Convert::toString(currentIDData.entity) + "].");
                }
            }

            //adds a new entry in the table (if needed) & pushes the current ID data
            idMap[entity].push_back({pool, currentIDData.entity});
        }
    }
    else
    {
        throw std::invalid_argument("PoolAggregator::importIDDataForEntity() > "
                "Entity ID data import failed; the specified entity ["
                + Convert::toString(entity) + "] is already present.");
    }
}

std::deque<StorageManagement_Pools::PoolAggregator::PendingActionData>
StorageManagement_Pools::PoolAggregator::exportPendingActions(bool discardActions)
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    std::deque<PendingActionData> result;

    for(const PendingStorageAction & currentAction : pendingStorageActions)
    {
        PoolUUID sourceUUID = pools.at(currentAction.linkData.source)->getPoolUUID();
        PoolUUID targetUUID = (currentAction.linkData.target != INVALID_POOL_ID)
                                ? pools.at(currentAction.linkData.target)->getPoolUUID()
                                : INVALID_POOL_UUID;

        result.push_back({currentAction.entityID,
                          currentAction.linkData.action,
                          sourceUUID,
                          targetUUID,
                          currentAction.processingTime
                         });
    }

    if(discardActions)
    {
        pendingStorageActionsCount.clear();
        pendingStorageActions.clear();
    }

    return result;
}

void StorageManagement_Pools::PoolAggregator::importPendingActions
(const std::deque<PendingActionData> & pendingActions)
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    if(pendingStorageActions.size() > 0)
    {
        throw std::runtime_error("PoolAggregator::importPendingActions() > "
                "Pending actions data import failed; pending actions are already "
                "present in the aggregator.");
    }

    Timestamp currentTime = boost::posix_time::second_clock::universal_time();
    Timestamp nextDelayTime = boost::posix_time::second_clock::universal_time();

    for(const PendingActionData & currentAction : pendingActions)
    {
        ++pendingStorageActionsCount[currentAction.aggregatorEntityID];

        PlainLinkData linkData{currentAction.action,
                               getPoolID(currentAction.source),
                               getPoolID(currentAction.target), 0};
        
        pendingStorageActions.push_back({currentAction.aggregatorEntityID,
                                         linkData,
                                         currentAction.processingTime
                                        });

        if(nextDelayTime > currentAction.processingTime
           && currentAction.processingTime > currentTime)
        {
            nextDelayTime = currentAction.processingTime;
        }
    }

    pendingActionsProcessingEnabled = true;
    threadPool.assignTimedTask(
            boost::bind(&StorageManagement_Pools::PoolAggregator::processPendingActions, this),
            nextDelayTime);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Helper Functions">
void StorageManagement_Pools::PoolAggregator::removeIDsForPool(PoolID pool)
{
    if(pools.at(pool)->getStoredEntitiesNumber() > 0)
    {
        for(auto currentEntityData : idMap)
        {
            std::deque<InternalEntityID> remainingData;
            for(const InternalEntityID & currentEntityID : currentEntityData.second)
            {
                if(currentEntityID.poolID != pool)
                    remainingData.push_back(currentEntityID);
            }

            currentEntityData.second.swap(remainingData);
        }
    }
}

const StorageManagement_Pools::PoolAggregator::LinkParameters *
StorageManagement_Pools::PoolAggregator::selectDistributedPool
(const std::vector<LinkParameters> & poolLinks, DataSize dataSize) const
{
    DataSize leastUsedSpaceAmount = MAX_DATA_SIZE;
    const LinkParameters * leastUsedSpacePoolParams = nullptr;

    //searches for the pool with the least amount of used space that can fit the incoming data
    for(const LinkParameters & currentPoolParams : poolLinks)
    {
        DataPool * currentPool = pools.at(currentPoolParams.targetPool);
        DataSize currentPoolUsedSpace = currentPool->getPoolSize() - currentPool->getFreeSpace();

        if(currentPool->canStoreData(dataSize) && currentPoolUsedSpace < leastUsedSpaceAmount)
        {
            leastUsedSpaceAmount = currentPoolUsedSpace;
            leastUsedSpacePoolParams = &currentPoolParams;
        }
    }

    return leastUsedSpacePoolParams;
}

void StorageManagement_Pools::PoolAggregator::processStoreActionSequence
(const ByteVectorPtr data, const std::deque<PlainLinkData> storageSequence, StoredDataID entityID)
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    Timestamp nextDelayTime = INVALID_DATE_TIME;

    std::deque<InternalEntityID> dataIDs;
    for(const PlainLinkData & currentLink : storageSequence)
    {
        if(currentLink.delayTime == 0)
        {//the current link action is to be processed now
            switch(currentLink.action)
            {
                case SimpleLinkActionType::COPY:
                {
                    DataPool * targetPool = pools.at(currentLink.target);
                    StoredDataID localEntityID = targetPool->storeData(data);
                    dataIDs.push_back({currentLink.target, localEntityID});
                    totalFreeSpace -= (data->size() + targetPool->getEntityManagementStorageOverhead());
                } break;

                case SimpleLinkActionType::REMOVE:
                {
                    bool removed = false;
                    for(auto currentIDData = dataIDs.begin(); currentIDData < dataIDs.end(); currentIDData++)
                    {
                        if(currentIDData->poolID == currentLink.source)
                        {
                            DataPool * sourcePool = pools.at(currentLink.source);
                            sourcePool->discardData(currentIDData->entityID, eraseOnDiscard);
                            dataIDs.erase(currentIDData);
                            removed = true;
                            totalFreeSpace += (data->size() + sourcePool->getEntityManagementStorageOverhead());
                            break;
                        }
                    }

                    if(!removed)
                    {
                        cleanupPartialStore(dataIDs, data->size());
                        throw std::runtime_error("PoolAggregator::processStoreActionSequence(ByteVectorPtr) > "
                                "Failed to store data; remove action for ["
                                + Convert::toString(currentLink.source) + "] failed.");
                    }
                } break;

                default:
                {
                    cleanupPartialStore(dataIDs, data->size());
                    throw std::runtime_error("PoolAggregator::processStoreActionSequence(ByteVectorPtr) > "
                            "Failed to store data; unexpected plain link data action encountered.");
                }
            }
        }
        else
        {//the current link action is to be delayed
            if(currentLink.source == aggregatorID)
            {
                cleanupPartialStore(dataIDs, data->size());
                throw std::runtime_error("PoolAggregator::processStoreActionSequence() > "
                        "Failed to store data; the aggregator cannot be the source for a "
                        "delayed operation.");
            }

            Timestamp delayTimestamp = (boost::posix_time::second_clock::universal_time() 
                                        + boost::posix_time::seconds(currentLink.delayTime));
            
            pendingStorageActions.push_back({entityID, currentLink, delayTimestamp});
            
            //increments the number of actions remaining for the entity
            //(and inserts a new element, if it does not exist)
            ++pendingStorageActionsCount[entityID];

            if(nextDelayTime == INVALID_DATE_TIME || nextDelayTime > delayTimestamp)
                nextDelayTime = delayTimestamp;
        }
    }

    bytesWritten += data->size();

    //adds the IDs for he current entity
    idMap.insert({entityID, dataIDs});

    if(nextDelayTime != INVALID_DATE_TIME)
    {
        pendingActionsProcessingEnabled = true;
        threadPool.assignTimedTask(
                boost::bind(&StorageManagement_Pools::PoolAggregator::processPendingActions, this),
                nextDelayTime);
    }
}

void StorageManagement_Pools::PoolAggregator::processStoreActionSequence
(DataSize dataSize, const std::deque<PlainLinkData> storageSequence, StoredDataID entityID)
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);
    Timestamp nextDelayTime = INVALID_DATE_TIME;

    auto currentEntityIDs = idMap.find(entityID);
    if(currentEntityIDs == idMap.end())
    {
        throw std::runtime_error("PoolAggregator::processStoreActionSequence(DataSize) > "
                "Failed to process action sequence; entity ID ["
                + Convert::toString(entityID) + "] not found.");
    }

    //retrieves the list of IDs for the current entity
    std::deque<InternalEntityID> & dataIDs = currentEntityIDs->second;

    for(const PlainLinkData & currentLink : storageSequence)
    {
        if(currentLink.delayTime == 0)
        {//the current link action is to be processed now
            switch(currentLink.action)
            {
                case SimpleLinkActionType::COPY:
                {
                    //retrieves the ID of the current entity associated with the current source pool
                    StoredDataID sourcePoolEntityID = INVALID_STORED_DATA_ID;
                    for(auto currentIDData = dataIDs.begin(); currentIDData < dataIDs.end(); currentIDData++)
                    {
                        if(currentIDData->poolID == currentLink.source)
                        {
                            sourcePoolEntityID = currentIDData->entityID;
                            break;
                        }
                    }

                    if(sourcePoolEntityID != INVALID_STORED_DATA_ID)
                    {
                        DataPool * sourcePool = pools.at(currentLink.source);
                        DataPool * targetPool = pools.at(currentLink.target);
                        StoredDataID targetPoolEntityID = INVALID_STORED_DATA_ID;
                        DataSize dataSize = sourcePool->getEntitySize(sourcePoolEntityID);
                        
                        if(sourcePool->areInputStreamsSupported() && targetPool->areOutputStreamsSupported())
                        {
                            //streams the data from the source to the target pool
                            PoolInputStreamPtr sourceStream = sourcePool->getInputStream(sourcePoolEntityID);
                            PoolOutputStreamPtr targetStream = targetPool->getOutputStream(dataSize);
                            targetPoolEntityID = targetStream->getDataID();
                            
                            targetStream << sourceStream;
                        }
                        else
                        {
                            if(maxNonStreamableData != 0 && dataSize > maxNonStreamableData)
                            {
                                throw std::runtime_error("PoolAggregator::processStoreActionSequence() > "
                                        "Failed to process action sequence; copy action for source ["
                                        + Convert::toString(currentLink.source) + "] and target ["
                                        + Convert::toString(currentLink.target) 
                                        + "] failed; too much non-streamable data requested: ["
                                        + Convert::toString(dataSize) + "] bytes..");
                            }
                            
                            //copies the data from the source to the target pool
                            ByteVectorPtr data = sourcePool->retrieveData(sourcePoolEntityID);
                            targetPoolEntityID = targetPool->storeData(data);
                        }
                        
                        if(targetPoolEntityID == INVALID_STORED_DATA_ID)
                        {
                            if(!completePendingStore)
                            {
                                throw std::runtime_error("PoolAggregator::processStoreActionSequence(DataSize) > "
                                        "Failed to process action sequence; copy action for source ["
                                        + Convert::toString(currentLink.source) + "] and target ["
                                        + Convert::toString(currentLink.target)
                                        + "] failed; target pool unable to store data.");
                            }
                            
                            logDebugMessage("(processStoreActionSequence) > Failed to process action sequence; "
                                    "copy action for source [" + Convert::toString(currentLink.source) 
                                    + "] and target [" + Convert::toString(currentLink.target)
                                    + "] failed; target pool unable to store data.");
                        }
                        
                        dataIDs.push_back({currentLink.target, targetPoolEntityID});
                        totalFreeSpace -= (dataSize + targetPool->getEntityManagementStorageOverhead());
                    }
                    else
                    {
                        if(!completePendingStore)
                        {
                            throw std::runtime_error("PoolAggregator::processStoreActionSequence(DataSize) > "
                                    "Failed to process action sequence; copy action for source ["
                                    + Convert::toString(currentLink.source) + "] and target ["
                                    + Convert::toString(currentLink.target)
                                    + "] failed; unable to find source entity ID..");
                        }

                        logDebugMessage("(processStoreActionSequence) > Failed to process action sequence; "
                                    "copy action for source ["
                                    + Convert::toString(currentLink.source) + "] and target ["
                                    + Convert::toString(currentLink.target)
                                    + "] failed; unable to find source entity ID..");
                    }
                } break;

                case SimpleLinkActionType::REMOVE:
                {
                    bool removed = false;
                    for(auto currentIDData = dataIDs.begin(); currentIDData < dataIDs.end(); currentIDData++)
                    {
                        if(currentIDData->poolID == currentLink.source)
                        {
                            //removes the data from the source pool
                            DataPool * sourcePool = pools.at(currentLink.source);
                            DataSize size = sourcePool->getEntitySize(currentIDData->entityID);
                            sourcePool->discardData(currentIDData->entityID, eraseOnDiscard);
                            dataIDs.erase(currentIDData);
                            removed = true;
                            totalFreeSpace += (size + sourcePool->getEntityManagementStorageOverhead());
                            break;
                        }
                    }

                    if(!removed)
                    {
                        if(!completePendingStore)
                        {
                            throw std::runtime_error("PoolAggregator::processStoreActionSequence() > "
                                    "Failed to process action sequence; remove action failed for ["
                                    + Convert::toString(currentLink.source) + "].");
                        }

                        logDebugMessage("(processStoreActionSequence) > Failed to process action sequence; "
                                    "remove action failed for ["
                                    + Convert::toString(currentLink.source) + "].");
                    }
                } break;

                default:
                {
                    if(!completePendingStore)
                        throw std::runtime_error("PoolAggregator::processStoreActionSequence() > "
                                "Failed to process action sequence; unexpected plain link "
                                "data action encountered.");

                    logDebugMessage("(processStoreActionSequence) > Failed to process action sequence; "
                            "unexpected plain link data action encountered.");
                }
            }
        }
        else
        {//the current link action is to be delayed
            Timestamp delayTimestamp = (boost::posix_time::second_clock::universal_time()
                                        + boost::posix_time::seconds(currentLink.delayTime));
            pendingStorageActions.push_back({entityID, currentLink, delayTimestamp});
            
            //increments the number of actions remaining for the entity
            //(and inserts a new element, if it does not exist)
            ++pendingStorageActionsCount[entityID]; 

            if(nextDelayTime == INVALID_DATE_TIME || nextDelayTime > delayTimestamp)
                nextDelayTime = delayTimestamp;
        }
    }

    bytesWritten += dataSize;

    if(nextDelayTime != INVALID_DATE_TIME && !pendingActionsProcessingEnabled)
    {
        pendingActionsProcessingEnabled = true;
        threadPool.assignTimedTask(
                boost::bind(&StorageManagement_Pools::PoolAggregator::processPendingActions, this),
                nextDelayTime);
    }

    recalculateUsableSpace();
}

void StorageManagement_Pools::PoolAggregator::processPendingActions()
{
    if(state != PoolState::OPEN)
    {
        throw std::invalid_argument("PoolAggregator::processPendingActions() > "
                "Processing pending actions failed; the aggregator is not in an open state.");
    }

    if(mode != PoolMode::READ_WRITE)
    {
        throw std::runtime_error("PoolAggregator::processPendingActions() > "
                "Failed to store data; the aggregator is not in read/write mode.");
    }

    boost::lock_guard<boost::mutex> dataLock(dataMutex);

    std::deque<PendingStorageAction> remainingActions;
    Timestamp nextDelayTime = INVALID_DATE_TIME;

    for(auto currentActionData = pendingStorageActions.begin();
        currentActionData != pendingStorageActions.end();
        currentActionData++)
    {
        if(currentActionData->processingTime <= boost::posix_time::second_clock::universal_time())
        {//the current action is to be executed now
            auto currentEntityIDs = idMap.find(currentActionData->entityID);
            if(currentEntityIDs == idMap.end())
                continue; //skips the current entity; it was discarded and no further processing is needed

            //retrieves the list of IDs for the current entity
            std::deque<InternalEntityID> & dataIDs = currentEntityIDs->second;

            switch(currentActionData->linkData.action)
            {
                case SimpleLinkActionType::COPY:
                {
                    //retrieves the ID of the current entity associated with the current source pool
                    StoredDataID sourcePoolEntityID = INVALID_STORED_DATA_ID;
                    for(auto currentIDData = dataIDs.begin(); currentIDData < dataIDs.end(); currentIDData++)
                    {
                        if(currentIDData->poolID == currentActionData->linkData.source)
                        {
                            sourcePoolEntityID = currentIDData->entityID;
                            break;
                        }
                    }

                    if(sourcePoolEntityID != INVALID_STORED_DATA_ID)
                    {
                        DataPool * sourcePool = pools.at(currentActionData->linkData.source);
                        DataPool * targetPool = pools.at(currentActionData->linkData.target);
                        StoredDataID targetPoolEntityID = INVALID_STORED_DATA_ID;
                        DataSize dataSize = sourcePool->getEntitySize(sourcePoolEntityID);
                        
                        if(sourcePool->areInputStreamsSupported() && targetPool->areOutputStreamsSupported())
                        {
                            //streams the data from the source to the target pool
                            PoolInputStreamPtr sourceStream = sourcePool->getInputStream(sourcePoolEntityID);
                            PoolOutputStreamPtr targetStream = targetPool->getOutputStream(dataSize);
                            targetPoolEntityID = targetStream->getDataID();
                            
                            targetStream << sourceStream;
                        }
                        else
                        {
                            if(maxNonStreamableData != 0 && dataSize > maxNonStreamableData)
                            {
                                throw std::runtime_error("PoolAggregator::processPendingActions() > "
                                        "Failed to process pending actions; copy action for source ["
                                        + Convert::toString(currentActionData->linkData.source)
                                        + "] and target ["
                                        + Convert::toString(currentActionData->linkData.target)
                                        + "] failed; too much non-streamable data requested: ["
                                        + Convert::toString(dataSize) + "] bytes..");
                            }
                            
                            //copies the data from the source to the target pool
                            ByteVectorPtr data = sourcePool->retrieveData(sourcePoolEntityID);
                            targetPoolEntityID = targetPool->storeData(data);
                        }
                        
                        if(targetPoolEntityID == INVALID_STORED_DATA_ID)
                        {
                            if(!completePendingStore)
                            {
                                throw std::runtime_error("PoolAggregator::processPendingActions() > "
                                        "Failed to process pending actions; copy action for source ["
                                        + Convert::toString(currentActionData->linkData.source)
                                        + "] and target ["
                                        + Convert::toString(currentActionData->linkData.target)
                                        + "] failed; target unable to store data.");
                            }
                            
                            logDebugMessage("(processPendingActions) > Failed to process pending actions; "
                                    "copy action for source ["
                                    + Convert::toString(currentActionData->linkData.source)
                                    + "] and target ["
                                    + Convert::toString(currentActionData->linkData.target)
                                    + "] failed; target pool unable to store data.");
                        }
                        
                        dataIDs.push_back({currentActionData->linkData.target, targetPoolEntityID});
                        totalFreeSpace -= (dataSize + targetPool->getEntityManagementStorageOverhead());
                    }
                    else
                    {
                        if(!completePendingStore)
                        {
                            throw std::runtime_error("PoolAggregator::processPendingActions() > "
                                    "Failed to process pending actions; copy action for source ["
                                    + Convert::toString(currentActionData->linkData.source)
                                    + "] and target ["
                                    + Convert::toString(currentActionData->linkData.target)
                                    + "] failed; unable to find source entity ID.");
                        }

                        logDebugMessage("(processPendingActions) > Failed to process pending actions; "
                                    "copy action for source ["
                                    + Convert::toString(currentActionData->linkData.source)
                                    + "] and target ["
                                    + Convert::toString(currentActionData->linkData.target)
                                    + "] failed; unable to find source entity ID.");
                    }
                } break;

                case SimpleLinkActionType::REMOVE:
                {
                    bool removed = false;
                    for(auto currentIDData = dataIDs.begin(); currentIDData < dataIDs.end(); currentIDData++)
                    {
                        if(currentIDData->poolID == currentActionData->linkData.source)
                        {
                            //removes the data from the source pool
                            DataPool * sourcePool = pools.at(currentActionData->linkData.source);
                            DataSize size = sourcePool->getEntitySize(currentIDData->entityID);
                            sourcePool->discardData(currentIDData->entityID, eraseOnDiscard);
                            dataIDs.erase(currentIDData);
                            removed = true;
                            totalFreeSpace += (size + sourcePool->getEntityManagementStorageOverhead());
                            break;
                        }
                    }

                    if(!removed)
                    {
                        if(!completePendingStore)
                        {
                            throw std::runtime_error("PoolAggregator::processPendingActions() > "
                                    "Failed to process pending actions; remove action failed for ["
                                    + Convert::toString(currentActionData->linkData.source) + "].");
                        }
                        
                        logDebugMessage("(processPendingActions) > Failed to process pending actions; "
                                    "remove action failed for ["
                                    + Convert::toString(currentActionData->linkData.source) + "].");
                    }
                } break;

                default:
                {
                    if(!completePendingStore)
                    {
                        throw std::runtime_error("PoolAggregator::processPendingActions() > "
                                "Failed to process pending actions; unexpected plain link "
                                "data action encountered.");
                    }

                    logDebugMessage("(processPendingActions) > Failed to process pending actions; "
                            "unexpected plain link data action encountered.");
                }
            }

            //decreases the number of pending actions for the current entity
            unsigned int & pendingActionsCount = 
                        pendingStorageActionsCount[currentActionData->entityID];
            if(pendingActionsCount == 1)
                pendingStorageActionsCount.erase(currentActionData->entityID);
            else
                --pendingActionsCount;
        }
        else
        {//the current action is to be executed at some future time
            remainingActions.push_back(*currentActionData);

            if(nextDelayTime == INVALID_DATE_TIME
               || nextDelayTime > currentActionData->processingTime)
            {
                nextDelayTime = currentActionData->processingTime;
            }
        }
    }

    pendingStorageActions.swap(remainingActions);

    if(pendingStorageActions.size() > 0)
    {
        threadPool.assignTimedTask(
                boost::bind(&StorageManagement_Pools::PoolAggregator::processPendingActions, this),
                nextDelayTime);
    }
    else
        pendingActionsProcessingEnabled = false;

    recalculateUsableSpace();
}

StorageManagement_Pools::PoolAggregator::AggregatorUsableSpace
StorageManagement_Pools::PoolAggregator::getUsableSpaceForPoolChain
(PoolID pool, std::vector<PoolID> * processedPools) const
{
    AggregatorUsableSpace result{INVALID_DATA_SIZE, INVALID_DATA_SIZE};
    
    //checks if the processed pools list needs to be created
    bool processedPoolsDefined = false;
    if(processedPools == nullptr)
    {
        processedPoolsDefined = true;
        processedPools = new std::vector<PoolID>();
    }

    if(pool == INVALID_POOL_ID || links.find(pool) == links.end())
    {
        if(processedPoolsDefined)
            delete processedPools;
        
        throw std::invalid_argument("PoolAggregator::getUsableSpaceForPoolChain() > "
                "The specified pool ID [" + Convert::toString(pool)
                + "] is not valid.");
    }

    processedPools->push_back(pool); //marks the current pool as processed

    //goes through all links at the current level (for the current pool)
    const std::deque<LinkParameters> & requestedLink = links.at(pool);
    for(const LinkParameters & currentLinkParameters : requestedLink)
    {
        //checks if the current target pool has already been processed
        if(std::find(processedPools->begin(), processedPools->end(), currentLinkParameters.targetPool)
                != processedPools->end())
        {
            continue;
        }

        switch(currentLinkParameters.action)
        {
            case LinkActionType::COPY:
            {
                if(pool == aggregatorID)
                {//only copy operations from the aggregator to a pool increase the usable space
                    result.max += pools.at(currentLinkParameters.targetPool)->getPoolSize();
                    result.total += pools.at(currentLinkParameters.targetPool)->getFreeSpace();
                }
            } break;

            case LinkActionType::DISCARD: /* no increase in usable space; end of chain */ continue;

            case LinkActionType::DISTRIBUTE:
            {
                result.max += pools.at(currentLinkParameters.targetPool)->getPoolSize();
                result.total += pools.at(currentLinkParameters.targetPool)->getFreeSpace();
            } break;

            case LinkActionType::MOVE:
            {
                result.max += pools.at(currentLinkParameters.targetPool)->getPoolSize();
                result.total += pools.at(currentLinkParameters.targetPool)->getFreeSpace();
            } break;

            case LinkActionType::SKIP:
            {
                if(currentLinkParameters.condition != LinkActionConditionType::NONE
                   && currentLinkParameters.condition != LinkActionConditionType::TIMED
                   && pool == aggregatorID)
                {
                    //only skip operations from the aggregator to a pool increase the usable space
                    //and only if they have set conditions
                    //(skip operations behave as copy operations, if the conditions are not met)
                    result.max += pools.at(currentLinkParameters.targetPool)->getPoolSize();
                    result.total += pools.at(currentLinkParameters.targetPool)->getFreeSpace();
                }
            } break;
            
            default:
            {
                if(processedPoolsDefined)
                    delete processedPools;
                
                throw std::invalid_argument("PoolAggregator::getUsableSpaceForPoolChain() > "
                        "Usable space calculation failed; unexpected link action encountered.");
            }
        }

        //retrieves the usable space for the current target pool
        AggregatorUsableSpace currentLinkResult = 
                getUsableSpaceForPoolChain(currentLinkParameters.targetPool, processedPools);
        result.max += currentLinkResult.max;
        result.total += currentLinkResult.total;
    }

    if(processedPoolsDefined)
        delete processedPools;

    return result;
}

std::deque<StorageManagement_Pools::PoolAggregator::PlainLinkData>
StorageManagement_Pools::PoolAggregator::unwindPoolChain
(PoolID pool, DataSize dataSize, std::vector<PoolID> * processedPools) const
{
    std::deque<PlainLinkData> result;

    //checks if the processed pools list needs to be created
    bool processedPoolsDefined = false;
    if(processedPools == nullptr)
    {
        processedPoolsDefined = true;
        processedPools = new std::vector<PoolID>();
    }

    if(pool == INVALID_POOL_ID || pools.find(pool) == pools.end())
    {
        if(processedPoolsDefined)
            delete processedPools;
        
        throw std::invalid_argument("PoolAggregator::unwindPoolChain() > "
                "The specified pool ID [" + Convert::toString(pool)
                + "] is not valid.");
    }

    processedPools->push_back(pool); //marks the current pool as processed

    //list of target pools marked for distribution (at the current chain level)
    std::vector<LinkParameters> distributionPoolsParameters;

    //goes through all links at the current level (for the current pool)
    const std::deque<LinkParameters> & requestedLink = links.at(pool);
    for(const LinkParameters & currentLinkParameters : requestedLink)
    {
        //checks if the current target pool has already been processed
        if(std::find(processedPools->begin(), processedPools->end(), currentLinkParameters.targetPool) 
                != processedPools->end())
        {
            continue; //skips the link (avoids infinite circular chains & duplicate data storage)
        }

        //checks if the conditions for the current pool & link allow data to be stored
        bool actionRequired = isActionRequired(pool, currentLinkParameters, dataSize);
        if((actionRequired && currentLinkParameters.action != LinkActionType::SKIP)
            || (!actionRequired && currentLinkParameters.action == LinkActionType::SKIP))
        {
            Seconds delayTime = (currentLinkParameters.condition == LinkActionConditionType::TIMED) 
                                 ? currentLinkParameters.conditionValue
                                 : 0;

            switch(currentLinkParameters.action)
            {
                case LinkActionType::SKIP:
                case LinkActionType::COPY:
                {
                    if(!pools.at(currentLinkParameters.targetPool)->canStoreData(dataSize))
                    {
                        if(processedPoolsDefined)
                            delete processedPools;
                        
                        throw std::runtime_error("PoolAggregator::unwindPoolChain() > The target pool <" 
                                                 + Convert::toString(currentLinkParameters.targetPool)
                                                 + "> does not have enough free space.");
                    }

                    result.push_back({SimpleLinkActionType::COPY,
                                      pool,
                                      currentLinkParameters.targetPool,
                                      delayTime});
                } break;

                case LinkActionType::DISCARD:
                {
                    if(pool != aggregatorID)
                        result.push_back({SimpleLinkActionType::REMOVE, pool, INVALID_POOL_ID, delayTime});
                    continue;
                }

                case LinkActionType::DISTRIBUTE:
                {
                    distributionPoolsParameters.push_back(currentLinkParameters);
                    continue;
                } break;

                case LinkActionType::MOVE:
                {
                    if(!pools.at(currentLinkParameters.targetPool)->canStoreData(dataSize))
                    {
                        if(processedPoolsDefined)
                            delete processedPools;
                        
                        throw std::runtime_error("PoolAggregator::unwindPoolChain() > The target pool <" 
                                                 + Convert::toString(currentLinkParameters.targetPool)
                                                 + "> does not have enough free space.");
                    }

                    result.push_back({SimpleLinkActionType::COPY,
                                      pool,
                                      currentLinkParameters.targetPool,
                                      delayTime});
                    
                    if(pool != aggregatorID)
                        result.push_back({SimpleLinkActionType::REMOVE, pool, INVALID_POOL_ID, delayTime});
                } break;

                default:
                {
                    if(processedPoolsDefined)
                        delete processedPools;
                    
                    throw std::invalid_argument("PoolAggregator::unwindPoolChain() > "
                            "Unexpected link action type encountered.");
                } break;
            }

            //retrieves the action sequence for the current target pool
            std::deque<PlainLinkData> currentLinkResult = 
                    unwindPoolChain(currentLinkParameters.targetPool, dataSize, processedPools);
            
            //merges the target pool sequence with the current one
            //and updates the delay times accordingly
            for(PlainLinkData & currentLinkData : currentLinkResult)
            {
                currentLinkData.delayTime += delayTime;
                result.push_back(currentLinkData);
            }
        }
    }

    //processes the distribution pools
    if(distributionPoolsParameters.size() > 0)
    {
        const LinkParameters * targetPoolParameters = 
                selectDistributedPool(distributionPoolsParameters, dataSize);

        if(targetPoolParameters == nullptr)
        {
            if(processedPoolsDefined)
                delete processedPools;
            
            throw std::runtime_error("PoolAggregator::unwindPoolChain() > "
                    "Data distribution failed; No suitable target pool found.");
        }

        //retrieves the action sequence for the pool selected as the target
        Seconds delayTime = (targetPoolParameters->condition == LinkActionConditionType::TIMED)
                             ? targetPoolParameters->conditionValue
                             : 0;
        
        std::deque<PlainLinkData> distributedLinkResult = 
                unwindPoolChain(targetPoolParameters->targetPool, dataSize, processedPools);
        
        //merges the target pool sequence with the current one
        //and updates the delay times accordingly
        for(PlainLinkData & currentLinkData : distributedLinkResult)
        {
            currentLinkData.delayTime += delayTime;
            result.push_back(currentLinkData);
        }

        //adds the selected target pool at the front of the sequence
        //(distribution always happens first)
        result.push_front({SimpleLinkActionType::COPY,
                           pool,
                           targetPoolParameters->targetPool,
                           delayTime});
    }

    if(processedPoolsDefined)
        delete processedPools;

    return result;
}

bool StorageManagement_Pools::PoolAggregator::isActionRequired
(PoolID sourcePool, const LinkParameters & params, DataSize dataSize) const
{
    switch(params.condition)
    {
        case LinkActionConditionType::NONE: return true;
        case LinkActionConditionType::TIMED: return true;

        case LinkActionConditionType::SOURCE_MIN_FULL:
        {
            DataPool * source = pools.at(sourcePool);
            unsigned int percentFull = ((source->getPoolSize() - source->getFreeSpace())*100ULL)/source->getPoolSize();
            return (percentFull >= params.conditionValue);
        }

        case LinkActionConditionType::SOURCE_MAX_FULL:
        {
            DataPool * source = pools.at(sourcePool);
            unsigned int percentFull = ((source->getPoolSize() - source->getFreeSpace())*100ULL)/source->getPoolSize();
            return (percentFull <= params.conditionValue);
        }

        case LinkActionConditionType::TARGET_MIN_FULL:
        {
            DataPool * target = pools.at(params.targetPool);
            unsigned int percentFull = ((target->getPoolSize() - target->getFreeSpace())*100ULL)/target->getPoolSize();
            return (percentFull >= params.conditionValue);
        }

        case LinkActionConditionType::TARGET_MAX_FULL:
        {
            DataPool * target = pools.at(params.targetPool);
            unsigned int percentFull = ((target->getPoolSize() - target->getFreeSpace())*100ULL)/target->getPoolSize();
            return (percentFull <= params.conditionValue);
        }

        case LinkActionConditionType::SOURCE_MIN_ENTITIES:
            return (pools.at(sourcePool)->getStoredEntitiesNumber() >= params.conditionValue);
            
        case LinkActionConditionType::SOURCE_MAX_ENTITIES:
            return (pools.at(sourcePool)->getStoredEntitiesNumber() <= params.conditionValue);
            
        case LinkActionConditionType::TARGET_MIN_ENTITIES:
            return (pools.at(params.targetPool)->getStoredEntitiesNumber() >= params.conditionValue);
            
        case LinkActionConditionType::TARGET_MAX_ENTITIES:
            return (pools.at(params.targetPool)->getStoredEntitiesNumber() <= params.conditionValue);

        case LinkActionConditionType::DATA_MIN_SIZE: return (dataSize >= params.conditionValue);
        case LinkActionConditionType::DATA_MAX_SIZE: return (dataSize <= params.conditionValue);
        
        default: 
        {
            throw std::invalid_argument("PoolAggregator::isActionRequired() >"
                " Unexpected link action condition encountered.");
        }
    }
}
//</editor-fold>

