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

#ifndef MEMORYDATAPOOL_H
#define	MEMORYDATAPOOL_H

#include <stdexcept>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>

#include "../../Common/Types.h"
#include "../Types/Types.h"
#include "../Interfaces/DataPool.h"

using Common_Types::ByteVectorPtr;
using Common_Types::DataPoolSize;
using StorageManagement_Types::DataPoolType;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::EntitiesCountType;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::PoolMode;
using StorageManagement_Pools::PoolInputStreamPtr;
using StorageManagement_Pools::PoolOutputStreamPtr;

namespace StorageManagement_Pools
{
    /**
     * Class for managing memory data pools.
     */
    class MemoryDataPool : public StorageManagement_Interfaces::DataPool
    {
        public:
            /** Parameters structure holding <code>MemoryDataPool</code> configuration. */
            struct MemoryDataPoolParameters
            {
                /** Mode in which the pool will operate */
                PoolMode mode;
                /** Required size of the memory pool (in bytes) */
                DataPoolSize size;
            };
            
            /**
             * Constructs a new memory data pool with the supplied parameters.
             * 
             * @param parameters the pool configuration
             */
            explicit MemoryDataPool(MemoryDataPoolParameters parameters);
            
            /**
             * Clears all data structures.
             */
            ~MemoryDataPool();
            
            MemoryDataPool() = delete;                                      //No default constructor
            MemoryDataPool(const MemoryDataPool& orig) = delete;            //Copying not allowed (pass/access only by reference/pointer)
            MemoryDataPool& operator=(const MemoryDataPool& orig) = delete; //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Retrieves the data associated with the specified ID.
             * 
             * @param id the ID associated with the required data
             * @return the requested data
             * 
             * @throw runtime_error if the ID is not found or if the pool is not in the correct state
             */
            ByteVectorPtr retrieveData(StoredDataID id) override;
            
            /**
             * Stores the supplied data in the pool.
             * 
             * @param data the data to be stored
             * @return the new ID associated with the data
             * 
             * @throw runtime_error if the pool doesn't have enough space or if the pool is not in the correct state/mode
             * @throw invalid_argument if an empty or invalid data container was supplied
             */
            StoredDataID storeData(const ByteVectorPtr data) override;
            
            /**
             * Discards the data associated with the specified ID.
             * 
             * Note 1: All information associated with the data will be removed, but
             * the data will remain in memory as long as there are other references to it.
             * 
             * Note 2: Erasing is not performed on data in a memory data pool.
             * 
             * @param id the ID associated with the data to be discarded
             * @param erase NOT USED
             * 
             * @throw runtime_error if the specified ID was not found or if the pool is not in the correct state/mode
             */
            void discardData(StoredDataID id, bool erase = false) override;
            
            /**
             * Clears all information associated with the data in the pool.
             * 
             * Note: If there are any other references to the data stored in the
             * pool, the data will remain in memory.
             * 
             * @throw runtime_error if the pool is not in the correct state/mode
             */
            void clearPool() override;
            
            DataPoolType getPoolType() const override { return DataPoolType::LOCAL_MEMORY; }
            DataPoolSize getFreeSpace() const override { return totalFreeSpace; }
            EntitiesCountType getStoredEntitiesNumber() const override { return entities.size(); }
            bool canStoreData(DataSize size) const override { return (totalFreeSpace >= size); }
            DataSize getEntityManagementStorageOverhead() const override { return 0;}
            DataSize getPoolManagementStorageOverhead() const override { return 0; }
            DataSize getEntitySize(StoredDataID id) const override;
            bool areInputStreamsSupported() const override { return false; }
            bool areOutputStreamsSupported() const override { return false; }
            PoolInputStreamPtr getInputStream(StoredDataID dataID) override
            { throw std::logic_error("MemoryDataPool::getInputStream() > Input streams are not supported."); }
            PoolOutputStreamPtr getOutputStream(DataSize dataSize) override
            { throw std::logic_error("MemoryDataPool::getOutputStream() > Output streams are not supported."); }
            
        private:
            StoredDataID lastEntityID;          //ID of the last entity stored in the pool
            DataSize totalFreeSpace;            //total amount of free space for the pool (in bytes)
            
            mutable boost::mutex entitiesMutex; //mutex synchronizing access to the entities table
            boost::unordered_map<StoredDataID, ByteVectorPtr> entities;  //entities table
    };
}
#endif	/* MEMORYDATAPOOL_H */

