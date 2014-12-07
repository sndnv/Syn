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

#ifndef DATAPOOL_H
#define	DATAPOOL_H

#include "../Types/Types.h"
#include "../../Common/Types.h"

using Common_Types::ByteVectorPtr;
using Common_Types::DataPoolSize;
using StorageManagement_Types::DataPoolType;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::EntitiesCountType;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::PoolMode;
using StorageManagement_Types::DiskDataSize;

namespace StorageManagement_Interfaces
{
    /**
     * Interface for defining a data storage pool.
     */
    class DataPool
    {
        public:
            virtual ~DataPool() {}
            
            /**
             * Retrieves the data associated with the specified ID.
             * 
             * @param id the DI associated with the required data
             * @return the requested data
             * 
             * @throw implementation-specific exceptions
             */
            virtual ByteVectorPtr retrieveData(StoredDataID id) = 0;
            
            /**
             * Stores the supplied data in the pool.
             * 
             * @param data the data to be stored
             * @return the new ID associated with the data
             * 
             * @throw implementation-specific exceptions
             */
            virtual StoredDataID storeData(const ByteVectorPtr data) = 0;
            
            /**
             * Discards the data associated with the specified ID.
             * 
             * @param id the ID associated with the data to be discarded
             * @param erase denotes whether the data should be erased
             * 
             * @throw implementation-specific exceptions
             */
            virtual void discardData(StoredDataID id, bool erase) = 0;
            
            /**
             * Clears all information associated with the data in the pool.
             * 
             * @throw implementation-specific exceptions
             */
            virtual void clearPool() = 0;
            
            /** Retrieves the type of the pool.\n\n@return the pool type */
            virtual DataPoolType getPoolType() const = 0;
            /** Retrieves the total amount of free space available in the pool.\n\n@return the free space in the pool (in bytes) */
            virtual DiskDataSize getFreeSpace() const = 0;
            /** Retrieves the number of stored entities (pieces of data) in the pool.\n\n@return the number of entities */
            virtual EntitiesCountType getStoredEntitiesNumber() const = 0;
            
            /** Retrieves the size of the pool.\n\n@return the pool size */
            DataPoolSize getPoolSize() const { return size; }
            /** Retrieves the state of the pool.\n\n@return the pool state */
            PoolState getPoolState() const { return state; }
            /** Retrieves the mode of the pool.\n\n@return the pool mode */
            PoolMode getPoolMode() const { return mode; }
            /** Retrieves the amount of data read from the pool.\n\n@return the number of bytes read */
            DiskDataSize getBytesRead() const { return bytesRead; }
            /** Retrieves the amount of data written to the pool.\n\n@return the number of bytes written */
            DiskDataSize getBytesWritten() const { return bytesWritten; }
            
        protected:
            PoolState state;          //current pool state
            PoolMode mode;            //current pool mode
            DataPoolSize size;        //pool size (in bytes)
            DiskDataSize bytesRead;   //amount of data read from the pool (in bytes)
            DiskDataSize bytesWritten;//amount of data written to the pool (in bytes)
    };
}

#endif	/* DATAPOOL_H */

