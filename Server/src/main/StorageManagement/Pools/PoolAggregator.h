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

#ifndef POOLAGGREGATOR_H
#define	POOLAGGREGATOR_H

#include <string>
#include <vector>
#include <deque>

#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include "../../Common/Types.h"
#include "../Types/Types.h"
#include "../Interfaces/DataPool.h"
#include "Streams/PoolStreams.h"

#include "../../Utilities/Strings/Common.h"
#include "../../Utilities/Strings/Storage.h"
#include "../../Utilities/FileLogger.h"
#include "../../Utilities/ThreadPool.h"

using Common_Types::Seconds;
using Common_Types::Timestamp;
using Common_Types::ByteVectorPtr;
using Common_Types::DataPoolSize;
using StorageManagement_Types::DataPoolType;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::EntitiesCountType;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::PoolMode;
using StorageManagement_Types::DataSize;
        
using StorageManagement_Types::PoolID;
using StorageManagement_Types::PoolUUID;
using StorageManagement_Types::LinkActionType;
using StorageManagement_Types::SimpleLinkActionType;
using StorageManagement_Types::LinkActionConditionType;
using StorageManagement_Types::LinkActionConditionValue;

using StorageManagement_Pools::PoolInputStreamPtr;
using StorageManagement_Pools::PoolOutputStreamPtr;

using Common_Types::INVALID_DATE_TIME;
using StorageManagement_Types::INVALID_POOL_ID;
using StorageManagement_Types::INVALID_POOL_UUID;
using StorageManagement_Types::INVALID_STORED_DATA_ID;
using StorageManagement_Types::INVALID_DATA_SIZE;
using StorageManagement_Types::MAX_DATA_SIZE;

using StorageManagement_Interfaces::DataPool;

namespace Convert = Utilities::Strings;

namespace StorageManagement_Pools
{
    /**
     * Class for aggregating multiple data pools into one storage entity.
     * 
     * Concurrent External Operations on <code>DataPool</code>s handled by an aggregator:
     * - Store -> OK; new data will NOT be registered in the aggregator;
     * - Retrieve -> OK;
     * - Discard -> Partially OK; discarding data NOT handled by the aggregator is OK;
     * - Clear -> NOT OK; doing so will invalidate the aggregator's state
     */
    class PoolAggregator : public StorageManagement_Interfaces::DataPool
    {
        public:
            /** Parameters structure holding pool link configuration. */
            struct LinkParameters
            {
                /** Target pool associated with the link (if any) */
                PoolID targetPool;
                /** Action to be performed */
                LinkActionType action;
                /** Action condition */
                LinkActionConditionType condition;
                /** Action condition value (if any) */
                LinkActionConditionValue conditionValue;
                
                bool operator==(const LinkParameters& other) const;
            };
            
            /** Parameters structure holding pool link configuration for persistent storage. */
            struct PersistentLinkParameters
            {
                /** Target pool associated with the link (if any) */
                PoolUUID targetPool;
                /** Action to be performed */
                LinkActionType action;
                /** Action condition */
                LinkActionConditionType condition;
                /** Action condition value (if any) */
                LinkActionConditionValue conditionValue;
                
                bool operator==(const PersistentLinkParameters& other) const;
            };
            
            /** Data structure holding entity ID data. */
            struct EntityIDData
            {
                /** Entity ID, as assigned by the aggregator */
                StoredDataID aggregatorEntityID;
                /** Entity ID, as assigned by the pool */
                StoredDataID poolEntityID;
                
                bool operator==(const EntityIDData& other) const;
            };
            
            /** Data structure holding local (to specific storage pool) entity ID data. */
            struct PoolEntityIDData
            {
                /** Pool for which the entity ID is valid */
                PoolUUID pool;
                /** Entity ID */
                StoredDataID entity;
                
                bool operator==(const PoolEntityIDData& other) const;
            };
            
            /** Data structure holding pending action data. */
            struct PendingActionData
            {
                /** Entity ID, as assigned by the aggregator */
                StoredDataID aggregatorEntityID;
                /** Action type */
                SimpleLinkActionType action;
                /** Action source pool UUID */
                PoolUUID source;
                /** Action target pool UUID */
                PoolUUID target;
                /** Timestamp for the processing of the action */
                Timestamp processingTime;
                
                bool operator==(const PendingActionData& other) const;
            };
            
            /**
             * Parameters structure holding <code>PoolAggregator</code>
             * configuration for new aggregator initialization.
             */
            struct PoolAggregatorInitParameters
            {
                /**
                 * Number of threads to create in the internal thread pool
                 */
                unsigned long threadPoolSize;
                
                /**
                 * Denotes whether a retrieve operation will stop on the first
                 * failure or ignore as many failures as possible
                 */
                bool completeRetrieve;
                
                /**
                 * Denotes whether a discard operation will stop on the first
                 * failure or ignore as many failures as possible
                 */
                bool completeDiscard;
                
                /**
                 * Denotes whether a pending store operation will stop on the
                 * first failure or ignore as many failures as possible
                 */
                bool completePendingStore;
                
                /**
                 * Denotes whether a discard operation should also erase the data.
                 * 
                 * Note: Does not affect the <code>erase</code> parameter of the 
                 * <code>discardData(StoredDataID, bool)</code> function.
                 */
                bool eraseOnDiscard;
                
                /**
                 * Denotes whether all actions in the thread pool are to be
                 * cancelled on object destruction (set to <code>true</code>)
                 * or whether the aggregator will wait until all pending
                 * actions are complete (set to <code>false</code>).
                 */
                bool cancelActionsOnShutdown;
                
                /**
                 * Maximum size of data handled by the aggregator that cannot be 
                 * streamed (in bytes; 0 = no maximum).
                 * 
                 * Any data beyond this size, that the aggregator cannot
                 * stream, is to be rejected.
                 */
                DataSize maxNonStreamableData;
                
                /**
                 * Mode in which the aggregator will operate
                 */
                PoolMode mode;
                
                /**
                 * Pointer to a data pool that will be used for temporary
                 * storage of incoming stream data
                 */
                DataPool * streamingPool;
            };
            
            /**
             * Parameters structure holding <code>PoolAggregator</code>
             * configuration for existing aggregator loading.
             */
            struct PoolAggregatorLoadParameters
            {
                /**
                 * Number of threads to create in the internal thread pool
                 */
                unsigned long threadPoolSize;
                
                /**
                 * Denotes whether a retrieve operation will stop on the first
                 * failure or ignore as many failures as possible
                 */
                bool completeRetrieve;
                
                /**
                 * Denotes whether a discard operation will stop on the first
                 * failure or ignore as many failures as possible
                 */
                bool completeDiscard;
                
                /**
                 * Denotes whether a pending store operation will stop on the
                 * first failure or ignore as many failures as possible
                 */
                bool completePendingStore;
                
                /**
                 * Denotes whether a discard operation should also erase the data.
                 * 
                 * Note: Does not affect the <code>erase</code> parameter of the 
                 * <code>discardData(StoredDataID, bool)</code> function.
                 */
                bool eraseOnDiscard;
                
                /**
                 * Denotes whether all actions in the thread pool are to be
                 * cancelled on object destruction (set to <code>true</code>)
                 * or whether the aggregator will wait until all pending actions
                 * are complete (set to <code>false</code>).
                 */
                bool cancelActionsOnShutdown;
                
                /**
                 * Maximum size of data handled by the aggregator that cannot be 
                 * streamed (in bytes; 0 = no maximum).
                 * 
                 * Any data beyond this size, that the aggregator cannot
                 * stream, is to be rejected.
                 */
                DataSize maxNonStreamableData;
                
                /** Aggregator UUID */
                PoolUUID uuid;
                /** Mode in which the aggregator will operate */
                PoolMode mode;
                /** Amount of data read from the pool during previous runs (in bytes) */
                DataSize bytesRead;
                /** Amount of data written to the pool during previous runs (in bytes) */
                DataSize bytesWritten;
                /** Last entity ID assigned by the aggregator */
                StoredDataID lastEntityID;
                /** Streaming pool UUID (if any) */
                PoolUUID streamingPoolUUID;
                
                /** Link parameters for each data pool */
                boost::unordered_map<PoolUUID, std::deque<PersistentLinkParameters>> links;
                
                /**
                 * Data pools to be used by the aggregator.
                 * 
                 * Note: The data pool pointers are only for supplying the pools to the
                 * aggregator during initialization. <code>nullptr</code>s are stored
                 * when retrieving the parameters via <code>exportConfiguration()</code>.
                 */
                boost::unordered_map<PoolUUID, DataPool *> pools;
            };
            
            /**
             * Constructs a new empty aggregator object.
             * 
             * @param parameters the aggregator configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             * 
             * @throw invalid_argument if a streaming data pool is supplied but it does not
             * support output streams
             */
            PoolAggregator(const PoolAggregatorInitParameters & parameters, Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Constructs a new aggregator object from existing configuration data.
             * 
             * @param parameters the aggregator configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             * 
             * @throw invalid_argument if invalid configuration parameters are found or
             * if a streaming data pool is supplied but it does not support output streams
             */
            PoolAggregator(const PoolAggregatorLoadParameters & parameters, Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Clears all data structures and discards all pending operations.
             * 
             * Note: The state of any assigned storage pool remains unchanged.
             */
            ~PoolAggregator();
            
            PoolAggregator() = delete;                                      //No default constructor
            PoolAggregator(const PoolAggregator& orig) = delete;            //Copying not allowed (pass/access only by reference/pointer)
            PoolAggregator& operator=(const PoolAggregator& orig) = delete; //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Retrieves the data associated with the specified ID.
             * 
             * @param id the ID associated with the required data
             * @return the requested data
             * 
             * @throw runtime_error if the ID is not found, or if the read
             * operation fails, or if the aggregator is not in the correct
             * state, or if no data pools are able to satisfy the request
             */
            ByteVectorPtr retrieveData(StoredDataID id) override;
            
            /**
             * Begins the storage process in the aggregator for the supplied data.
             * 
             * Note: The operation is asynchronous and the actual storage may not
             * be performed immediately and the data may not be available for 
             * immediate retrieval.
             * 
             * @param data the data to be stored
             * @return the new ID associated with the data
             * 
             * @throw runtime_error if the aggregator doesn't have enough space, 
             * or if there is no single fragment large enough to store the data,
             * or if the pool is not in the correct state/mode, or if no valid
             * storage sequence is found
             * 
             * @throw invalid_argument if an empty or invalid data container
             * was supplied
             */
            StoredDataID storeData(const ByteVectorPtr data) override;
            
            /**
             * Discards the data associated with the specified ID.
             * 
             * Note: All pending actions for the specified entity will be discarded.
             * 
             * @param id the ID associated with the data to be discarded
             * @param erase denotes whether the data in the aggregator and
             * all affected data pools should be erased
             * 
             * @throw runtime_error if the specified ID was not found or if the
             * aggregator is not in the correct state/mode
             */
            void discardData(StoredDataID id, bool erase = false) override;
            
            /**
             * Clears all information associated with the data in the aggregator.
             * 
             * Note 1: All data in the associated pools is cleared.
             * 
             * Note 2: The structure of the aggregator remains unchanged; no pools or
             * links are removed.
             */
            void clearPool() override;
            
            /**
             * Retrieves the pool ID of the aggregator.
             * 
             * @return the requested ID
             */
            PoolID getAggregatorID() const { return aggregatorID; }
            
            DataPoolType getPoolType() const override { return DataPoolType::AGGREGATE; }
            DataSize getFreeSpace() const override { return totalUsableSpace; }
            EntitiesCountType getStoredEntitiesNumber() const override { return idMap.size(); }
            bool canStoreData(DataSize size) const override;
            DataSize getEntityManagementStorageOverhead() const override { return 0; }
            DataSize getPoolManagementStorageOverhead() const override { return 0; }
            DataSize getEntitySize(StoredDataID id) const override;
            bool areInputStreamsSupported() const override { return true; }
            bool areOutputStreamsSupported() const override { return (streamingPoolID != INVALID_POOL_ID); }
            
            /**
             * Retrieves a stream for reading data from the aggregator.
             * 
             * @param dataID the ID associated with the data to be read
             * @return the requested stream
             * 
             * @throw runtime_error if the ID is not found, or if the stream retrieval
             * operation fails, or if the aggregator is not in the correct state,
             * or if no data pools are able to satisfy the request
             */
            PoolInputStreamPtr getInputStream(StoredDataID id) override;
            
            /**
             * Retrieves a stream for writing data to the aggregator.
             * 
             * Note: <code>releaseStreamedData()</code> must be called after the
             * streaming completes, otherwise, the data will remain in the streaming
             * pool.
             * 
             * @param dataSize the size of the data to be written
             * @return the requested stream
             * 
             * @throw runtime_error if the aggregator is not in the correct mode/state 
             * or if the streaming pool is unable to store the data
             * 
             * @throw logic_error if a streaming pool is not available
             * 
             * @throw invalid_argument if the specified data size is 0
             */
            PoolOutputStreamPtr getOutputStream(DataSize dataSize) override;
            
            /**
             * Notifies the aggregator that streaming of the specified entity
             * is complete and further processing can begin
             * 
             * @param id the ID for which streaming is complete
             * 
             * @throw runtime_error if the aggregator is not in the correct state/mode, 
             * or if an invalid storage sequence is encountered, or if the specified
             * ID is not found, or if unexpected ID data is found for the entity
             * 
             * @throw invalid_argument if an invalid entity ID is specified
             */
            void releaseStreamedData(StoredDataID streamedEntityID);
            
            /**
             * Adds the specified data pool to the aggregator
             * 
             * Note: The caller remains responsible for the life-cycle management
             * of the supplied data pool (closing, deleting memory, etc).
             * 
             * @param pool the pool to be added
             * @return the ID associated with the pool
             * 
             * @throw invalid_argument if the specified pool is not valid or if
             * the pool is already associated with the aggregator
             */
            PoolID addPool(DataPool * pool);
            
            /**
             * Removes the specified data pool from the aggregator.
             * 
             * Note 1: The state of the pool remains unchanged (it is NOT closed,
             * cleared, modified).
             * 
             * Note 2: If the pool is to be reused, the ID data associated with it
             * must be extracted before removing it. See <code>exportIDData()</code>;
             * all IDs associated with the pool will be removed.
             * 
             * Note 1: Removing a pool also removes all links associated with it
             * (as a source and/or target).
             * 
             * @param pool the ID of the pool to be removed
             * 
             * @throw invalid_argument if the specified ID is not valid or if the
             * pool is not found
             */
            void removePool(PoolID pool);
            
            /**
             * Adds a new pool link with the supplied parameters for the specified
             * source data pool.
             * 
             * @param sourcePool the ID of the source pool
             * @param params the link parameters
             * 
             * @throw invalid_argument if the source pool ID is not valid, or if
             * the target pool set in the parameter already has link with the 
             * specified source pool, or if the source pool cannot be found
             */
            void addPoolLink(PoolID sourcePool, const LinkParameters & params);
            
            /**
             * Removes the link associated with the specified source and target pools.
             * 
             * @param sourcePool source pool ID
             * @param targetPool target pool ID
             * 
             * @throw invalid_argument if the source pool ID is not valid or if either
             *                         of the pools are not found
             */
            void removePoolLink(PoolID sourcePool, PoolID targetPool);
            
            /**
             * Exports the current aggregator configuration.
             * 
             * Note 1: Importing is done by creating a new aggregator with the
             * appropriate constructor.
             * 
             * Note 2: Only configuration is extracted, including pools and links.
             * Any entity IDs need to be extracted separately.
             * 
             * @return the exported configuration
             */
            PoolAggregatorLoadParameters exportConfiguration() const;
            
            /**
             * Exports all ID data.
             * 
             * Note: The result maps pool UUIDs to all entities in those pools.
             * 
             * @return the ID map
             */
            boost::unordered_map<PoolUUID, std::deque<EntityIDData>> exportIDData() const;
            
            /**
             * Exports the ID data associated with the specified data pool.
             * 
             * @param pool the pool for which to export data
             * @return the requested data
             * 
             * @throw invalid_argument if the specified pool ID is not valid or
             * if the pool was not found
             */
            std::deque<EntityIDData> exportIDDataForPool(PoolID pool) const;
            
            /**
             * Exports the ID data associated with the specified data pool.
             * 
             * @param pool the pool for which to export data
             * @return the requested data
             * 
             * @throw invalid_argument if the specified pool ID is not valid
             */
            std::deque<EntityIDData> exportIDDataForPool(PoolUUID pool) const;
            
            /**
             * Exports the ID data associated with the specified entity.
             * 
             * @param entity the entity for which to export data
             * @return the requested data
             * 
             * @throw invalid_argument if the entity ID is not valid or if the entity was not found
             */
            std::deque<PoolEntityIDData> exportIDDataForEntity(StoredDataID entity) const;
            
            /**
             * Imports the supplied ID data.
             * 
             * Note: The ID data must be in the same format as returned by <code>exportIDData()</code>.
             * 
             * @param idData data to be imported
             * @param verify denotes whether verification of the data will be done while importing
             * 
             * @throw invalid_argument if no or invalid/corrupted data is supplied
             */
            void importIDData(const boost::unordered_map<PoolUUID, std::deque<EntityIDData>> & idData, bool verify = true);
            
            /**
             * Imports the supplied ID data for the specified pool.
             * 
             * @param pool the pool for which to import the data
             * @param idData the data to be imported
             * @param verify denotes whether verification of the data will be done while importing
             * 
             * @throw invalid_argument if no or invalid/corrupted data is supplied
             */
            void importIDDataForPool(PoolID pool, const std::deque<EntityIDData> & idData, bool verify = true);
            
            /**
             * Imports the supplied ID data for the specified pool.
             * 
             * @param pool the pool for which to import the data
             * @param idData the data to be imported
             * @param verify denotes whether verification of the data will be done while importing
             * 
             * @throw invalid_argument if no or invalid/corrupted data is supplied
             */
            void importIDDataForPool(PoolUUID pool, const std::deque<EntityIDData> & idData, bool verify = true);
            
            /**
             * Imports the supplied ID data for the specified entity.
             * 
             * @param entity the entity for which to import the data
             * @param idData the data to be imported
             * @param verify denotes whether verification of the data will be done while importing
             * 
             * @throw invalid_argument if no or invalid/corrupted data is supplied
             */
            void importIDDataForEntity(StoredDataID entity, const std::deque<PoolEntityIDData> & idData, bool verify = true);
            
            /**
             * Exports all currently pending actions.
             * 
             * @param discardActions denotes whether the pending actions are to be discarded
             * @return a list of pending actions
             */
            std::deque<PendingActionData> exportPendingActions(bool discardActions);
            
            /**
             * Imports the supplied pending actions and schedules their processing.
             * 
             * Note: In order to avoid conflicts, importing is only allowed when 
             * no other pending actions are present.
             * 
             * @param pendingActions the pending actions data
             * 
             * @throw runtime_error if there are other pending actions in the aggregator
             */
            void importPendingActions(const std::deque<PendingActionData> & pendingActions);
            
            /**
             * Retrieves a map of pool UUIDs to pool IDs currently used by the aggregator.
             * 
             * @return the requested map
             */
            boost::unordered_map<PoolUUID, PoolID> getPoolIDsMap() const;
            
            /**
             * Retrieves the total amount of free space currently available in the aggregator.
             * 
             * This includes the storage from all data pools, regardless of their
             * link relationships. Use <code>getTotalUsableSpace()</code> for
             * getting the actual amount of space the aggregator has for storing data.
             * 
             * @return the total amount of free space (in bytes)
             */
            DataSize getTotalFreeSpace() const { return totalFreeSpace; }
            
            /**
             * Retrieves the total amount of usable space currently available in the aggregator.
             * 
             * This depends on the link relationships between the pools and may
             * be lower than the sum of the storage of all pools.
             * 
             * Example: Adding two data pools, both with 10GB capacity, but with
             * a COPY link, will increase the usable space by 10GB only.
             * A copy from the source is always made to the target, so the effective
             * size is that of the source.
             * 
             * Note: Some data pools have storage overhead for their pool and entity management.
             * 
             * @return the total amount of usable space (in bytes)
             */
            DataSize getTotalUsableSpace() const { return totalUsableSpace; }
            
            /**
             * Retrieves the maximum amount of free space the aggregator has.
             * 
             * This includes the storage from all data pools, regardless of their
             * link relationships. Use <code>getMaxUsableSpace()</code> for
             * getting the actual amount of space the aggregator is able to use for
             * storing data.
             * 
             * @return the maximum amount of free space (in bytes)
             */
            DataSize getMaxFreeSpace() const { return maxFreeSpace; }
            
            /**
             * Retrieves the maxim amount of usable space the aggregator has.
             * 
             * This depends on the link relationships between the pools and may
             * be lower than the sum of the storage of all pools.
             * 
             * Example: Adding two data pools, both with 10GB capacity, but with
             * a COPY link, will increase the usable space by 10GB only.
             * A copy from the source is always made to the target, so the effective
             * size is that of the source.
             * 
             * Note: Some data pools have storage overhead for their pool and entity management.
             * 
             * @return the maximum amount of usable space (in bytes)
             */
            DataSize getMaxUsableSpace() const { return maxUsableSpace; }
            
        private:
            /** Data structure holding simple link data. */
            struct PlainLinkData
            {
                /** Action to be performed */
                SimpleLinkActionType action;
                /** Source pool for the action */
                PoolID source;
                /** Target pool for the action (if any) */
                PoolID target;
                /** Action delay time (if any; in seconds) */
                Seconds delayTime;
            };
            
            /** Data structure holding local (to specific storage pool) entity IDs. */
            struct InternalEntityID
            {
                /** Pool for which the entity ID is valid */
                PoolID poolID;
                /** Entity ID */
                StoredDataID entityID;
            };
            
            /** Data structure holding maximum and current usable space. */
            struct AggregatorUsableSpace
            {
                /** Maximum amount of usable space the aggregator has (in bytes) */
                DataSize max;
                /** Current amount of usable space (in bytes) */
                DataSize total;
            };
            
            /** Data structure holding pending storage action data. */
            struct PendingStorageAction
            {
                /** ID associated with the entity for which the action is pending */
                StoredDataID entityID;
                /** Pending link data */
                PlainLinkData linkData;
                /** Time/date for action processing */
                Timestamp processingTime;
            };
            
            mutable Utilities::FileLoggerPtr debugLogger;   //logger for debugging
            Utilities::ThreadPool threadPool;               //threads for handling storage actions
            
            bool completeRetrieve;        //denotes whether a retrieve operation will attempts to get data from any pool or fail on the first exception
            bool completeDiscard;         //denotes whether a discard operation will attempt to delete data from all pools or fail on the first exception
            bool completePendingStore;    //denotes whether a pending store operation will attempt to finish all remaining actions or fail on the first exception
            bool eraseOnDiscard;          //denotes whether discard operations should erase the data
            bool cancelActionsOnShutdown; //denotes whether all tasks in the thread pool are to be stopped on object destruction
            DataSize maxNonStreamableData;//the maximum size of non-streamable data the aggregator can handle (in bytes; 0 = unlimited)
            
            PoolID lastPoolID;                //last pool ID to be assigned (the first ID is reserved for the aggregator)
            PoolID aggregatorID;              //the pool ID of the aggregator
            PoolID streamingPoolID;           //the ID of the pool that acts as a target for incoming streaming
            StoredDataID lastEntityID;        //ID of the last entity stored in the aggregator
            DataSize totalFreeSpace;          //total amount of free space in the aggregator (in bytes)
            DataSize totalUsableSpace;        //total amount of free space (based on the aggregator structure; in bytes)
            DataSize maxFreeSpace;            //maximum amount of free space (in bytes)
            DataSize & maxUsableSpace = size; //maximum amount of usable space (based on the aggregator structure; in bytes)
            
            mutable boost::mutex dataMutex;
            bool pendingActionsProcessingEnabled;
            boost::unordered_map<StoredDataID, unsigned int> pendingStorageActionsCount; //number of pending actions for each entity
            std::deque<PendingStorageAction> pendingStorageActions;                      //list of pending storage actions
            boost::unordered_map<StoredDataID, std::deque<InternalEntityID>> idMap;      //aggregator ID to pool IDs for each entity
            boost::unordered_map<PoolID, DataPool *> pools;                              //pools handled by the aggregator
            boost::unordered_map<PoolID, std::deque<LinkParameters>> links;              //links associated with the pools
            
            /**
             * Retrieves the ID of the pool with the specified UUID.
             * 
             * Note: NOT thread-safe.
             * 
             * @param the pool UUID
             * @return the pool ID assigned by the aggregator or <code>INVALID_POOL_ID</code>
             * if the operation fails
             */
            PoolID getPoolID(PoolUUID pool) const
            {
                if(pool == INVALID_POOL_UUID) return INVALID_POOL_ID;
                
                if(pool == uuid) return aggregatorID;
                
                for(auto currentPool : pools)
                    if(currentPool.second->getPoolUUID() == pool)
                        return currentPool.first;
                
                return INVALID_POOL_ID;
            }
            
            /**
             * Performs cleanup of a partially completed store operation by discarding
             * all data that was stored before the failure/invocation of the function.
             * 
             * Note: NOT thread-safe.
             * 
             * @param dataIDs a list of data IDs to be used for the cleanup
             * @param size the size of the data
             */
            void cleanupPartialStore(std::deque<InternalEntityID> & dataIDs, DataSize size)
            {
                for(InternalEntityID currentIDData : dataIDs)
                {
                    DataPool * pool = pools.at(currentIDData.poolID);
                    pool->discardData(currentIDData.entityID, eraseOnDiscard);
                    totalFreeSpace += (size + pool->getEntityManagementStorageOverhead());
                }
            }
            
            /**
             * Removes all IDs associated with the specified pool.
             * 
             * Note: NOT thread-safe.
             * 
             * @param pool the pool for which to remove IDs
             */
            void removeIDsForPool(PoolID pool);
            
            /**
             * Attempts to select a pool from the supplied list of distributed pools.
             * 
             * Note: Selection is based on the amount of data currently used in each pool.
             * 
             * @param poolLinks the link parameters of each target pool
             * @param dataSize the size of the data to be stored
             * @return a pointer to the selected pool parameters or <code>nullptr</code>
             * if no suitable pool is found
             */
            const LinkParameters * selectDistributedPool(const std::vector<LinkParameters> & poolLinks, DataSize dataSize) const;
            
            /**
             * Processes the specified storage sequence for the supplied data.
             * 
             * Note: Thread-safe.
             * 
             * @param data the data to be stored
             * @param storageSequence the storage sequence to be used
             * @param entityID the entity ID, as supplied by the aggregator
             * 
             * @throw runtime_error if the aggregator is set as the source of a
             * delayed operation, or if a remove action fails, or if an
             * unexpected action is encountered
             */
            void processStoreActionSequence(const ByteVectorPtr data, const std::deque<PlainLinkData> storageSequence, StoredDataID entityID);
            
            /**
             * Processes the specified storage sequence with data retrieved from
             * the source pools defined in the supplied storage sequence.
             * 
             * Note: Thread-safe.
             * 
             * @param dataSize the size of the data to processed
             * @param storageSequence the storage sequence to be used
             * @param entityID the entity ID, as supplied by the aggregator
             * 
             * @throw runtime_error if a copy/remove operation fails, or if the 
             * entity ID is not found, or if an unexpected action is encountered
             */
            void processStoreActionSequence(DataSize dataSize, const std::deque<PlainLinkData> storageSequence, StoredDataID entityID);
            
            /**
             * Processes all eligible pending actions.
             * 
             * Note: Thread-safe.
             * 
             * @throw invalid_argument if the aggregator is not in the correct
             * state/mode
             * 
             * @throw runtime_error if a copy/remove action fails or if an
             * unexpected link action is encountered
             */
            void processPendingActions();
            
            /**
             * Recalculates the current usable space and updates the aggregator's variables.
             * 
             * Note: NOT thread-safe.
             */
            void recalculateUsableSpace()
            {
                AggregatorUsableSpace usableSpace = getUsableSpaceForPoolChain(aggregatorID);
                maxUsableSpace = usableSpace.max;
                totalUsableSpace = usableSpace.total;
            }
            
            /**
             * Calculates the current usable space for the aggregator by traversing
             * all links fromt he specified pool until the end of the chain.
             * 
             * Note: NOT thread-safe.
             * 
             * @param pool the pool from which to start the traversal
             * @param processedPools a list of pools already processed (should not be supplied externally)
             * @return the current amount of usable space in the aggregator
             * 
             * @throw invalid_argument the specified pool ID is not valid or if
             * an unexpected link action is encountered
             */
            AggregatorUsableSpace getUsableSpaceForPoolChain(PoolID pool, std::vector<PoolID> * processedPools = nullptr) const;
            
            /**
             * Traverses all links from the specified pool until the end of the chain
             * and returns a sequence of actions to be performed when storing data.
             * 
             * Note: NOT thread-safe.
             * 
             * @param pool the start pool for the traversal
             * @param dataSize the size of the data to be stored
             * @param processedPools a list of pools already processed (should not be supplied externally)
             * @return a sequence of actions to be followed when storing data
             * 
             * @throw invalid_argument if an invalid pool ID is specified or if
             * an unexpected link action is encountered;
             * 
             * @throw runtime_error if a target pool does not have enough free
             * space or if no suitable data pool is found for a 'distribution' action
             */
            std::deque<PlainLinkData> unwindPoolChain(PoolID pool, DataSize dataSize, std::vector<PoolID> * processedPools = nullptr) const;
            
            /**
             * Checks whether an action is required for the specified pool, based on the
             * supplied link parameters.
             * 
             * Note: NOT thread-safe.
             * 
             * @param sourcePool the source pool
             * @param params the parameters to be used in the check
             * @param dataSize size of the data to be stored (in bytes)
             * @return <code>true></code> if an action is required to be taken
             * 
             * @throw invalid_argument if an unexpected link action condition is encountered
             */
            bool isActionRequired(PoolID sourcePool, const LinkParameters & params, DataSize dataSize) const;
    
            /**
             * Logs the specified message, if a debugging file logger is assigned to the dispatcher.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string & message) const
            {
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "PoolAggregator " + message);
            }
    };
}
#endif	/* POOLAGGREGATOR_H */

