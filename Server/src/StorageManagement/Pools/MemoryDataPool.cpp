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

#include "MemoryDataPool.h"

StorageManagement_Pools::MemoryDataPool::MemoryDataPool(MemoryDataPoolParameters parameters)
{
    size = parameters.size;
    mode = parameters.mode;
    state = PoolState::OPEN;
    lastEntityID = 0;
    totalFreeSpace = parameters.size;
    bytesRead = 0;
    bytesWritten = 0;
}

StorageManagement_Pools::MemoryDataPool::~MemoryDataPool()
{
    boost::lock_guard<boost::mutex> entitiesLock(entitiesMutex);
    state = PoolState::CLOSED;

    entities.clear();
}

ByteVectorPtr StorageManagement_Pools::MemoryDataPool::retrieveData(StoredDataID id)
{
    boost::lock_guard<boost::mutex> entitiesLock(entitiesMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("MemoryDataPool::retrieveData() > Failed to retrieve data; the pool is not in an open state.");

    auto requestedEntity = entities.find(id);
    if(requestedEntity != entities.end())
    {
        bytesRead += requestedEntity->second->size();
        return requestedEntity->second;
    }
    else
        throw std::runtime_error("MemoryDataPool::retrieveData() > Failed to retrieve the requested data; id not found.");
}

StoredDataID StorageManagement_Pools::MemoryDataPool::storeData(const ByteVectorPtr data)
{
    boost::lock_guard<boost::mutex> entitiesLock(entitiesMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("MemoryDataPool::storeData() > Failed to store data; the pool is not in an open state.");

    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("MemoryDataPool::storeData() > Failed to store data; the pool is not in read/write mode.");

    if(data->size() > 0)
    {
        if(data->size() > totalFreeSpace)
            throw std::runtime_error("MemoryDataPool::storeData() > Failed to store data; the pool has insufficient free space.");

        StoredDataID newEntityID = ++lastEntityID;
        entities.insert({newEntityID, data});
        totalFreeSpace -= data->size();
        bytesWritten += data->size();
        return newEntityID;
    }
    else
        throw std::invalid_argument("MemoryDataPool::storeData() > Failed to store data; no data supplied.");
}

void StorageManagement_Pools::MemoryDataPool::discardData(StoredDataID id, bool)
{
    boost::lock_guard<boost::mutex> entitiesLock(entitiesMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("MemoryDataPool::discardData() > Failed to discard data; the pool is not in an open state.");

    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("MemoryDataPool::discardData() > Failed to discard data; the pool is not in read/write mode.");

    auto requestedEntity = entities.find(id);
    if(requestedEntity != entities.end())
    {
        totalFreeSpace += requestedEntity->second->size();
        entities.erase(requestedEntity);
    }
    else
        throw std::runtime_error("MemoryDataPool::discardData() > Failed to discard the requested data; id not found.");
}

void StorageManagement_Pools::MemoryDataPool::clearPool()
{
    boost::lock_guard<boost::mutex> entitiesLock(entitiesMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("MemoryDataPool::clearPool() > Failed to clear pool; the pool is not in an open state.");

    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("MemoryDataPool::clearPool() > Failed to clear pool; the pool is not in read/write mode.");

    entities.clear();
    totalFreeSpace = size;
}

