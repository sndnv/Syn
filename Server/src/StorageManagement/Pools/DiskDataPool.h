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

#ifndef DISKDATAPOOL_H
#define	DISKDATAPOOL_H

#include <map>
#include <string>
#include <vector>
#include <deque>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/system/error_code.hpp>
#include <boost/lexical_cast.hpp>

#include "../../Common/Types.h"
#include "../Types/Types.h"
#include "../Interfaces/DataPool.h"
#include "Streams/DiskPoolStreams.h"

#include "../../Utilities/Tools.h"
#include "../../Utilities/FileLogger.h"

using Common_Types::Byte;
using Common_Types::ByteVector;
using Common_Types::ByteVectorPtr;
using Common_Types::DataPoolSize;
using Common_Types::DataPoolPath;
using StorageManagement_Types::DataPoolType;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::EntitiesCountType;
using StorageManagement_Types::DiskDataAddress;
using StorageManagement_Types::DataSize;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::PoolMode;
using StorageManagement_Types::PoolUUID;

using StorageManagement_Types::INVALID_DATA_SIZE;
using StorageManagement_Types::INVALID_STORED_DATA_ID;
using StorageManagement_Types::INVALID_DISK_DATA_ADDRESS;

using StorageManagement_Pools::PoolInputStreamPtr;
using StorageManagement_Pools::PoolOutputStreamPtr;
using StorageManagement_Pools::DiskPoolInputStream;
using StorageManagement_Pools::DiskPoolOutputStream;

namespace StorageManagement_Pools
{
    /**
     * Class for managing disk data pools.
     * 
     * Note: Over time the disk pool will fragment. It is up to higher management functions to
     * determine how and when to deal with fragmentation.
     */
    class DiskDataPool : public StorageManagement_Interfaces::DataPool
    {
        public:
            /** Disk pool file signature */
            const std::string FILE_SIGNATURE = "DDP";
            /** Disk pool version */
            const char CURRENT_VERSION = '1';
            /** Pool UUID size (in bytes) */
            const DataSize UUID_BYTE_LENGTH = 36;
            
            /** Parameters structure holding <code>DiskDataPool</code> configuration for new pool initialization. */
            struct DiskDataPoolInitParameters
            {
                /** Full path to the disk pool file */
                DataPoolPath poolFilePath;
                /** Required size of the disk pool (in bytes) */
                DataPoolSize poolSize;
                /** Denotes whether already written data should be erased if a store operation fails (see <code>discardData()<code/>)  */
                bool eraseDataOnFailure;
            };
            
            /** Parameters structure holding <code>DiskDataPool</code> configuration for existing pool loading. */
            struct DiskDataPoolLoadParameters
            {
                /** Full path to the disk pool file */
                DataPoolPath poolFilePath;
                /** Mode in which the pool will operate */
                PoolMode mode;
                /** Denotes whether already written data should be erased if a store operation fails (see <code>discardData()<code/>)  */
                bool eraseDataOnFailure;
                /** Amount of data read from the pool during previous runs (in bytes) */
                DataSize bytesRead;
                /** Amount of data written to the pool during previous runs (in bytes) */
                DataSize bytesWritten;
            };
            
            /** Data structure holding an in-memory representation of the disk pool header. */
            struct PoolHeader
            {
                /** Size of the pool header, when converted into bytes */
                static const std::size_t BYTE_LENGTH = sizeof(DiskDataAddress);
                
                /** Starting address of the pool's footer. */
                DiskDataAddress footer;
                
                /**
                 * Converts the header into bytes.
                 * 
                 * @return the byte representation of the header
                 * 
                 * @throw invalid_argument if there is a mismatch between expected and actual byte size of the header
                 */
                ByteVector toBytes();
                
                /**
                 * Attempts to convert the supplied data into a valid pool header.
                 * 
                 * @param data the data to be converted
                 * @return the requested pool header
                 * 
                 * @throw invalid_argument if the supplied data does not have the expected size
                 */
                static PoolHeader fromBytes(const ByteVector & data);
            };
            
            /** Data structure for holding an in-memory representation of the disk pool footer. */
            struct PoolFooter
            {
                /** Size of the pool footer, when converted into bytes */
                static const std::size_t BYTE_LENGTH = sizeof(EntitiesCountType) + sizeof(DiskDataAddress) + sizeof(StoredDataID);
                
                /** Number of entities stored in the pool */
                EntitiesCountType entitiesNumber;
                /** Address of the first entity's header (if any) */
                DiskDataAddress firstHeader;
                /** ID of the last entity stored in the pool (for maintaining consistent ID generation) */
                StoredDataID lastDataID;
                
                /**
                 * Converts the footer into bytes.
                 * 
                 * @return the byte representation of the footer
                 * 
                 * @throw invalid_argument if there is a mismatch between expected and actual byte size of the footer
                 */
                ByteVector toBytes();
                
                /**
                 * Attempts to convert the supplied data into a valid pool footer.
                 * 
                 * @param datathe data to be converted
                 * @return the requested pool footer
                 * 
                 * @throw invalid_argument if the supplied data does not have the expected size
                 */
                static PoolFooter fromBytes(const ByteVector & data);
            };
            
            /** Data structure for holding an in-memory representation of a data entity header. */
            struct EntityHeader
            {
                /** Size of the entity header, when converted into bytes */
                static const std::size_t BYTE_LENGTH = sizeof(StoredDataID) + sizeof(DataSize) + sizeof(DiskDataAddress);
                
                /** ID associated with the stored data */
                StoredDataID id;
                /** Size of the stored data */
                DataSize size;
                /** Address of the next entity header (if any) */
                DiskDataAddress nextHeader;
                
                /**
                 * Converts the header into bytes.
                 * 
                 * @return the byte representation of the header 
                 * 
                 * @throw invalid_argument if there is a mismatch between expected and actual byte size of the header
                 */
                ByteVector toBytes();
                
                /**
                 * Attempts to convert the supplied data into a valid entity header.
                 * 
                 * @param datathe data to be converted
                 * @return the requested entity header
                 * 
                 * @throw invalid_argument if the supplied data does not have the expected size
                 */
                static EntityHeader fromBytes(const ByteVector & data);
            };
            
            /** Data structure for internal entity management. */
            struct EntityDescriptor
            {
                /** Address of the entity header */
                DiskDataAddress entityAddress;
                /** Entity header */
                EntityHeader rawHeader;
                /** Pointer to the previous entity descriptor in the chain (if any) */
                EntityDescriptor * previousEntity;
                /** Pointer to the next entity descriptor in the chain (if any) */
                EntityDescriptor * nextEntity;
                
                /**
                 * These are meant to ensure that no conflicting operations are performed
                 * on the entity between the retrieval of a stream and its use.
                 * 
                 * Example: (1) retrieve input stream for entity 1;
                 *          (2) discard entity 1;
                 *          (3) read from stream retrieved with (1);
                 * 
                 *          Operation (3) will read whatever currently is at the location
                 *          of entity 1, which might or might not be entity 1's data
                 */
                /** Denotes how many stream read operations are currently pending for the entity */
                unsigned int streamReadLocks; //if > 0, discard operations are not allowed
                /** Denotes whether the entity is currently locked, waiting for an output stream to write it */
                bool streamWriteLocked; //if 'true', retrieve operations are not allowed
            };
            
            /** Storage overhead for managing the disk pool (in bytes) */
            const DataSize OVERHEAD_POOL_MANAGEMENT = (FILE_SIGNATURE.size() + sizeof(CURRENT_VERSION) + UUID_BYTE_LENGTH
                                                       + PoolHeader::BYTE_LENGTH + PoolFooter::BYTE_LENGTH);
            /** Storage overhead for managing each entity (piece of data) stored (in bytes; per entity) */
            const DataSize OVERHEAD_ENTITY_MANAGEMENT = sizeof(EntityHeader);
            
            /**
             * Constructs a new disk data pool management object and creates
             * a new disk pool at the specified path, if it does not exists.
             * 
             * Note: The pool size must be above <code>OVERHEAD_POOL_MANAGEMENT</code>.
             * 
             * @param parameters the initialization configuration
             * 
             * @throw invalid_argument if the file already exists or if the pool size is too low
             * @throw runtime_error if the new pool file cannot be opened or if the file initialization fails
             */
            DiskDataPool(DiskDataPoolInitParameters parameters); 
           
            /**
             * Constructs a new disk data pool management object and loads an existing disk.
             * 
             * @param parameters the loading configuration
             * 
             * @throw runtime error if the pool file cannot be opened, or if the file is not a valid disk pool,
             * or if the pool is corrupted
             */
            DiskDataPool(DiskDataPoolLoadParameters parameters);
            
            /**
             * Clears all data structures and closes the file stream.
             */
            ~DiskDataPool();
            
            DiskDataPool() = delete;                                    //No default constructor
            DiskDataPool(const DiskDataPool& orig) = delete;            //Copying not allowed (pass/access only by reference/pointer)
            DiskDataPool& operator=(const DiskDataPool& orig) = delete; //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Retrieves the data associated with the specified ID.
             * 
             * @param id the ID associated with the required data
             * @return the requested data
             * 
             * @throw runtime_error if the ID is not found or if the read operation fails or if the pool is not in the correct state
             */
            ByteVectorPtr retrieveData(StoredDataID id);
            
            /**
             * Stores the supplied data in the pool.
             * 
             * @param data the data to be stored
             * @return the new ID associated with the data
             * 
             * @throw runtime_error if the pool doesn't have enough space, or if there is no single fragment large enough to store the data,
             *                      or if the pool is not in the correct state/mode
             * @throw invalid_argument if an empty or invalid data container was supplied
             */
            StoredDataID storeData(const ByteVectorPtr data);
            
            /**
             * Discards the data associated with the specified ID.
             * 
             * Note 1: All information associated with the data will be removed, but
             * the data will remain in the disk pool file until it is partially/fully
             * overwritten by subsequent operations.
             * 
             * Note 2: Erasing is done by overwriting all data associated with the ID
             * with <code>0</code>s.
             * 
             * @param id the ID associated with the data to be discarded
             * @param erase denotes whether the data in the pool should be erased
             * 
             * @throw runtime_error if the specified ID was not found or if the pool is not in the correct state/mode
             */
            void discardData(StoredDataID id, bool erase = false)
            {
                boost::lock_guard<boost::mutex> fileLock(fileMutex);
                discardDataWithoutLock(id, erase);
            }
            
            /**
             * Clears all information associated with the data in the pool.
             * 
             * Note: The data is not erased from the pool file itself.
             * 
             * @throw runtime_error if the pool is not in the correct state/mode or if the pool file update operation fails
             */
            void clearPool();
            
            /**
             * Retrieves a stream for reading data from the pool.
             * 
             * @param dataID the ID associated with the data to be read
             * @return the requested stream
             * 
             * @throw runtime_error if the stream is not in an open state or if the specified ID could not be found
             */
            PoolInputStreamPtr getInputStream(StoredDataID dataID);
            
            /**
             * Retrieves a stream for writing data to the pool.
             * 
             * Note: The new ID associated with the data can be retrieved from the stream object.
             * 
             * @param dataSize the size of the data to be written
             * @return the requested stream
             * 
             * @throw runtime_error if the pool doesn't have enough space, or if there is no single fragment large enough to store the data,
             * or if the pool is not in the correct state/mode
             * @throw invalid_argument if an empty or invalid data container was supplied
             */
            PoolOutputStreamPtr getOutputStream(DataSize dataSize);
            
            DataPoolType getPoolType() const { return DataPoolType::LOCAL_DISK; }
            DataSize getFreeSpace() const { return totalFreeSpace; }
            EntitiesCountType getStoredEntitiesNumber() const { return entities.size(); }
            bool canStoreData(DataSize size) const { return (freeChunks.lower_bound(size) != freeChunks.end()); }
            DataSize getEntityManagementStorageOverhead() const { return OVERHEAD_ENTITY_MANAGEMENT;}
            DataSize getPoolManagementStorageOverhead() const { return OVERHEAD_POOL_MANAGEMENT; }
            DataSize getEntitySize(StoredDataID id) const;
            bool areInputStreamsSupported() const { return true; }
            bool areOutputStreamsSupported() const { return true; }
            
        private:
            //Pool state/configuration
            DataPoolPath poolPath;  //full path to the disk pool file
            bool eraseDataOnFailure;//denotes whether data is to be erased if a store operation fails

            PoolHeader header;      //pool header
            PoolFooter footer;      //pool footer (flushed on each operation that modifies it) 
            
            mutable boost::mutex fileMutex;         //mutex synchronizing access to the pool file
            boost::filesystem::fstream fileStream;  //input/output stream to the pool file
            
            //Entities
            StoredDataID lastEntityInChain; //the ID of the last entity in the pool
            boost::unordered_map<StoredDataID, EntityDescriptor> entities;  //entities table
            
            //Free Space
            DataSize totalFreeSpace; //total amount of free space for the pool (in bytes)
            std::map<DataSize, std::deque<DiskDataAddress>> freeChunks; //free space fragments, organised by available size
            std::map<DiskDataAddress, DataSize> freeSpace;              //free space fragments, organised by address
            
            /**
             * Writes the current file signature, version and header to the pool
             * file and flushes the stream buffer.
             * 
             * Warning: The method is not thread-safe.
             */
            void flushCompleteHeader()
            {
                fileStream.seekp(0);
                fileStream.write(FILE_SIGNATURE.c_str(), FILE_SIGNATURE.size());
                fileStream.write(&CURRENT_VERSION, sizeof(CURRENT_VERSION));
                fileStream.write(Utilities::Tools::toString(uuid).c_str(), UUID_BYTE_LENGTH);

                ByteVector rawHeader = header.toBytes();
                fileStream.write((const char*)&rawHeader[0], PoolHeader::BYTE_LENGTH);
                
                fileStream.flush();
            }
            
            /**
             * Writes the current footer to the pool file and flushes the stream buffer.
             * 
             * Warning: The method is not thread-safe.
             */
            void flushFooter()
            {
                fileStream.seekp(header.footer);
                ByteVector rawFooter = footer.toBytes();
                fileStream.write((const char*)&rawFooter[0], PoolFooter::BYTE_LENGTH);

                fileStream.flush();
            }
            
            /**
             * Attempts to allocate a new free space chunk suitable to hold 
             * the requested amount of data.
             * 
             * Note: Handles all structures related to free space management.
             * 
             * Warning: The method is not thread-safe.
             * 
             * @param entitySize the size of the chunk
             * @return the address of the new chunk or <code>INVALID_DISK_DATA_ADDRESS</code>,
             * if no chunk is found, big enough to hold the requested amount of data
             */
            DiskDataAddress allocateEntityChunk(DataSize entitySize);
            
            /**
             * Marks the specified amount of data, starting at the specified address, 
             * as free and available for reuse.
             * 
             * Note: Handles all structures related to free space management.
             * 
             * Warning: The method is not thread-safe.
             * 
             * @param entityAddress the starting address of the chunk to be freed
             * @param entitySize the size of the chunk to be freed
             */
            void freeEntityChunk(DiskDataAddress entityAddress, DataSize entitySize);
            
            /**
             * Uses the current value of <code>errno</code> to generate an error message.
             * 
             * @return the message associated with the <code>errno</code> value
             */
            std::string getErrnoMessage() const
            {
                std::string result = boost::system::error_code(errno, boost::system::system_category()).message();
                errno = 0;
                return result;
            }
            
            /**
             * Discards the data associated with the specified ID, without obtaining
             * a lock on the file (NOT thread-safe).
             * 
             * Note 1: All information associated with the data will be removed, but
             * the data will remain in the disk pool file until it is partially/fully
             * overwritten by subsequent operations.
             * 
             * Note 2: Erasing is done by overwriting all data associated with the ID
             * with <code>0</code>s.
             * 
             * @param id the ID associated with the data to be discarded
             * @param erase denotes whether the data in the pool should be erased
             * 
             * @throw runtime_error if the specified ID was not found or if the pool is not in the correct state/mode
             */
            void discardDataWithoutLock(StoredDataID id, bool erase);
    };
}
#endif	/* DISKDATAPOOL_H */

