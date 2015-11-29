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

#ifndef DALQUEUE_H
#define	DALQUEUE_H

#include <atomic>
#include <deque>
#include <vector>
#include <queue>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "Types/Types.h"
#include "../Utilities/Tools.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Database.h"
#include "../Utilities/FileLogger.h"
#include "Containers/DataContainer.h"
#include "Interfaces/DatabaseAbstractionLayer.h"

#include "DALCache.h"

namespace Convert = Utilities::Strings;

using boost::shared_ptr;
using boost::tuples::tuple;
using boost::unordered::unordered_map;

using std::deque;
using std::queue;
using std::vector;
using DatabaseManagement_Interfaces::DALPtr;

using Common_Types::DBObjectID;
using DatabaseManagement_Types::DatabaseManagerOperationMode;
using DatabaseManagement_Types::DatabaseFailureAction;
using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;
using DatabaseManagement_Containers::DataContainerPtr;

using SyncServer_Core::DatabaseManagement::DALCache;

namespace SyncServer_Core
{
    namespace DatabaseManagement
    {
        /**
         * Class for managing DALs and database request routing.
         */
        class DALQueue
        {
            public:
                /** Parameters structure for holding <code>DALQueue</code> configuration data. */
                struct DALQueueParameters
                {
                    /** Queue operation mode. */
                    DatabaseManagerOperationMode dbMode;
                    /** Failure action. */
                    DatabaseFailureAction failureAction;
                    /** Maximum number of allowed consecutive read failures. */
                    unsigned int maximumReadFailures;
                    /** Maximum number of allowed consecutive write failures. */
                    unsigned int maximumWriteFailures;
                };
                
                /** Information structure for holding <code>DALQueue</code> data. */
                struct DALQueueInformation
                {
                    DALQueueInformation() {}
                    DALQueueInformation(unsigned int readFailures, unsigned int writeFailures, unsigned long readRequests,
                    unsigned long writeRequests, DatabaseObjectType type, DatabaseManagerOperationMode mode, DatabaseFailureAction failureAction,
                    unsigned int dalsNumber, unsigned int maxReadFailures, unsigned int maxWriteFailures, bool stop, bool running,
                    unsigned long newRequestsNumber, unsigned long pendingRequestsNumber)
                    : totalReadFailures(readFailures), totalWriteFailures(writeFailures), totalReadRequests(readRequests),
                      totalWriteRequests(writeRequests), queueType(type), dbMode(mode), failureAction(failureAction),
                      dals(dalsNumber), maxConsecutiveReadFailures(maxReadFailures), maxConsecutiveWriteFailures(maxWriteFailures),
                      stopQueue(stop), threadRunning(running), newRequests(newRequestsNumber), pendingRequests(pendingRequestsNumber)
                    {}
                    
                    const unsigned int totalReadFailures = 0;
                    const unsigned int totalWriteFailures = 0;
                    const unsigned long totalReadRequests = 0;
                    const unsigned long totalWriteRequests = 0;
                    const DatabaseObjectType queueType = DatabaseObjectType::INVALID;
                    const DatabaseManagerOperationMode dbMode = DatabaseManagerOperationMode::INVALID;
                    const DatabaseFailureAction failureAction = DatabaseFailureAction::INVALID;
                    const unsigned int dals = 0;
                    const unsigned int maxConsecutiveReadFailures = 0; 
                    const unsigned int maxConsecutiveWriteFailures = 0; 
                    const bool stopQueue = true;
                    const bool threadRunning = false;
                    const unsigned long newRequests = 0;
                    const unsigned long pendingRequests = 0;
                };
                
                /** Information structure for holding <code>DatabaseAbstractionLayer</code> data. */
                struct DALInformation
                {
                    DALInformation() {}
                    DALInformation(DatabaseAbstractionLayerID id, unsigned int readFailures, unsigned int writeFailures,
                    bool cache, DatabaseObjectType type, const DatabaseInformationContainer * info, DatabaseSettingsContainer * settings)
                    : dalID(id), readFailures(readFailures), writeFailures(writeFailures), isCache(cache), dalType(type),
                      infoData(info), settingsData(settings)
                    {}
                    
                    DatabaseAbstractionLayerID dalID = 0;
                    unsigned int readFailures = 0;
                    unsigned int writeFailures = 0;
                    bool isCache = false;
                    DatabaseObjectType dalType = DatabaseObjectType::INVALID;
                    const DatabaseInformationContainer * infoData = nullptr;
                    const DatabaseSettingsContainer * settingsData = nullptr;
                };
                
                /**
                 * Initialises the queue.
                 * 
                 * @param type the queue type
                 * @param parentLogger the file logger of the parent DatabaseManager
                 * @param parameters the queue configuration parameters
                 */
                DALQueue(DatabaseObjectType type, Utilities::FileLogger& parentLogger, DALQueueParameters parameters);

                /**
                 * Queue destructor.
                 * 
                 * Stops all threads and clears all data structures.
                 */
                ~DALQueue();

                DALQueue() = delete;                            //No default constructor
                DALQueue(const DALQueue&) = delete;             //Copying not allowed (pass/access only by reference/pointer)
                DALQueue& operator=(const DALQueue&) = delete;  //Copying not allowed (pass/access only by reference/pointer)

                /**
                 * Adds a new SELECT request to the queue.
                 * 
                 * @param constraintType type of constraint; it must always be selected from the enum subclasses of DatabaseManagement_Types::DatabaseSelectConstraints
                 * @param constraintParameter parameter associated with the constraint (if any)
                 * @return the ID assigned to the new request
                 */
                DatabaseRequestID addSelectRequest(const boost::any constraintType, const boost::any constraintParameter)
                {
                    return addRequestToQueue(RequestType::SELECT, constraintType, constraintParameter);
                }

                /**
                 * Adds a new INSERT request to the queue.
                 * 
                 * @param data the container to be inserted
                 * @return the ID assigned to the new request
                 */
                DatabaseRequestID addInsertRequest(const DataContainerPtr data)
                {
                    return addRequestToQueue(RequestType::INSERT, data);
                }

                /**
                 * Adds a new UPDATE request to the queue.
                 * 
                 * @param data the container to be updated
                 * @return the ID assigned to the new request
                 */
                DatabaseRequestID addUpdateRequest(const DataContainerPtr data)
                {
                    return addRequestToQueue(RequestType::UPDATE, data);
                }

                /**
                 * Adds a new DELETE request to the queue.
                 * 
                 * @param id the ID of the object to be removed
                 * @return the ID assigned to the new request
                 */
                DatabaseRequestID addDeleteRequest(const DBObjectID id)
                {
                    return addRequestToQueue(RequestType::REMOVE, id);
                }

                /**
                 * Attaches the specified event handler to the "onFailure" event of the queue.
                 * 
                 * @param function the handler to be attached
                 * @return the associated connection object
                 */
                boost::signals2::connection onFailureEventAttach(std::function<void(DatabaseRequestID, DBObjectID)> function)
                {
                    return onFailure.connect(function);
                }

                /**
                 * Attaches the specified event handler to the "onSuccess" event of the queue.
                 * 
                 * @param function the handler to be attached
                 * @return the associated connection object
                 */
                boost::signals2::connection onSuccessEventAttach(std::function<void(DatabaseRequestID, DataContainerPtr)> function)
                {
                    return onSuccess.connect(function);
                }

                /**
                 * Adds a new DAL to the queue.
                 * 
                 * @param dal the DAL to be added
                 * 
                 * @return <code>true</code>, if the operation was successful
                 */
                bool addDAL(DALPtr dal);

                /**
                 * Removes a DAL from the queue.
                 * 
                 * @param dal the DAL to be removed
                 * 
                 * @return <code>true</code>, if the operation was successful
                 */
                bool removeDAL(const DALPtr dal);

                /**
                 * Sets new queue configuration parameters.
                 * 
                 * @param parameters the new queue configuration parameters
                 * 
                 * @return <code>true</code>, if the operation was successful
                 */
                bool setParameters(DALQueueParameters parameters);

                /**
                 * Retrieves the parameters of the queue.
                 * 
                 * @return the requested data
                 */
                DALQueueParameters getParameters();
                
                /**
                 * Sets new cache configuration parameters.
                 * 
                 * @param cacheID the ID of the affected cache
                 * @param parameters the new cache configuration parameters
                 * 
                 * @return <code>true</code>, if the operation was successful
                 */
                bool setCacheParameters(DatabaseAbstractionLayerID cacheID, DALCache::DALCacheParameters parameters);
                
                /**
                 * Retrieves the parameters for the specified cache.
                 * 
                 * @param cacheID the ID of the cache
                 * @return the requested data
                 */
                DALCache::DALCacheParameters getCacheParameters(DatabaseAbstractionLayerID cacheID);

                /** Retrieves the number of DALs currently in the queue.\n\n@return the number of DALs */
                unsigned int getNumberOfDALs()              const { return dals.size(); }
                /** Retrieves the total number of READ failures.\n\n@return the number of read failures */
                unsigned int getTotalReadFailures()         const { return totalReadFailures; }
                /** Retrieves the total number of WRITE failures.\n\n@return the number of write failures */
                unsigned int getTotalWriteFailures()        const { return totalWriteFailures; }
                /** Retrieves the total number of READ requests.\n\n@return the number of read requests */
                unsigned long getTotalReadRequests()        const { return totalReadRequests; }
                /** Retrieves the total number of WRITE requests.\n\n@return the number of write requests */
                unsigned long getTotalWriteRequests()       const { return totalWriteRequests; }
                /** Retrieves the number of currently new requests.\n\n@return the number of new requests */
                unsigned long getNumberOfNewRequests()      const { return newRequests.size(); }
                /** Retrieves the number of currently pending requests.\n\n@return the number of pending requests */
                unsigned long getNumberOfPendingRequests()  const { return pendingRequests.size(); }
                /** Retrieves general information for the queue.\n\n@return the requested information */
                DALQueueInformation getQueueInformation() const;
                /** Retrieves general information for all caches in the queue.\n\n@return the requested information */
                std::vector<DALCache::DALCacheInformation> getCachesInformation() const;
                /** Retrieves general information for all DALs in the queue.\n\n@return the requested information */
                std::vector<DALInformation> getDALsInformation() const;

            private:
                enum class RequestType { SELECT, INSERT, UPDATE, REMOVE };

                //Statistics
                unsigned int totalReadFailures;     //number of read failures for the queue
                unsigned int totalWriteFailures;    //number of write failures for the queue
                unsigned long totalReadRequests;    //number of read requests for the queue
                unsigned long totalWriteRequests;   //number of write requests for the queue

                //DALs management
                DatabaseObjectType queueType;                   //the type of the objects the queue will handle
                std::atomic<DatabaseManagerOperationMode> newMode {DatabaseManagerOperationMode::INVALID};//new DB operation mode
                DatabaseManagerOperationMode dbMode;            //database operation mode
                DatabaseFailureAction failureAction;            //denotes the queue behaviour in case of DAL failure (depending on # of failures for read/write)
                DatabaseAbstractionLayerID nextDALID = 0;       //ID that will be assigned to the next new DAL
                deque<DatabaseAbstractionLayerID> dalIDs;       //list of currently active DAL IDs
                //ID -> <DAL pointer, read fails, write fails, on success connection, on failure connection>
                unordered_map<DatabaseAbstractionLayerID, tuple<DALPtr, unsigned int, unsigned int, boost::signals2::connection, boost::signals2::connection>*> dals; 
                unsigned int maxConsecutiveReadFailures;        //maximum number of consecutive read failures before a DAL is considered as failed
                unsigned int maxConsecutiveWriteFailures;       //maximum number of consecutive write failures before a DAL is considered as failed

                //Thread management
                Utilities::FileLogger * logger;
                boost::thread * mainThread;                     //main thread object
                mutable boost::mutex threadMutex;               //main thread mutex (for synchronising access to all the data structures of the queue)
                boost::condition_variable threadLockCondition;  //condition variable for sleeping/notifying the main thread
                std::atomic<bool> stopQueue;                    //atomic variable for denoting the state of the queue
                std::atomic<bool> threadRunning;                //atomic variable for denoting the state of the main thread

                //Requests management
                unsigned long nextRequestID = 1;                //ID that will be assigned to the next new request
                queue<DatabaseRequestID> newRequests;           //list of requests waiting for processing by the queue
                //request id -> list of DALs working on that request
                unordered_map<DatabaseRequestID, vector<DatabaseAbstractionLayerID>> pendingRequests; //table of requests waiting for processing by the corresponding DAL(s)
                //request id -> <type, parameter 1, parameter 2>
                unordered_map<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*> requestsData; //table of data accompanying each request

                boost::signals2::signal<void (DatabaseRequestID, DBObjectID)> onFailure;
                boost::signals2::signal<void (DatabaseRequestID, DataContainerPtr)> onSuccess;

                /**
                 * Adds a new request to the queue.
                 * 
                 * Note: Thread-safe.
                 * 
                 * @param type the type of the request
                 * @param requestParameter primary request parameter
                 * @param additionalParameter secondary request parameter (optional)
                 */
                DatabaseRequestID addRequestToQueue(RequestType type, boost::any requestParameter, boost::any additionalParameter = 0);

                /**
                 * Main queue thread.
                 * 
                 * Deals with all DB requests for a specific logical unit (queue type).
                 */
                void mainQueueThread();

                /**
                 * Event handler for "onFailure" signals coming from the DALs in the queue.
                 * 
                 * @param dalID the ID of the DAL that fired the event
                 * @param requestID the associated request ID
                 * @param id the associated object ID (if any)
                 */
                void onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id);

                /**
                 * Event handler for "onSuccess" signals coming from the DALs in the queue.
                 * 
                 * @param dalID the ID of the DAL that fired the event
                 * @param requestID the associated request ID
                 * @param id the associated data
                 */
                void onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr id);
        };
    }
}

#endif	/* DALQUEUE_H */

