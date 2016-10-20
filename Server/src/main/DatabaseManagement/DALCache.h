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

#ifndef DALCACHE_H
#define	DALCACHE_H

#include <atomic>
#include <queue>
#include <tuple>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include "Types/Types.h"
#include "../Utilities/Tools.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Database.h"
#include "../Utilities/FileLogger.h"
#include "Containers/DataContainer.h"
#include "Containers/VectorDataContainer.h"
#include "Interfaces/DatabaseAbstractionLayer.h"

namespace Tools = Utilities::Tools;
namespace Convert = Utilities::Strings;

using boost::tuples::tuple;
using boost::unordered::unordered_map;

using std::queue;

using Common_Types::DBObjectID;
using DatabaseManagement_Interfaces::DALPtr;
using DatabaseManagement_Interfaces::DatabaseSettingsContainer;
using DatabaseManagement_Interfaces::DatabaseInformationContainer;

using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;
using DatabaseManagement_Types::ObjectCacheAge;

using DatabaseManagement_Containers::DataContainerPtr;
using DatabaseManagement_Containers::VectorDataContainerPtr;

namespace SyncServer_Core
{
    namespace DatabaseManagement
    {
        /**
         * DAL class for managing database object caching.
         */
        class DALCache : public DatabaseManagement_Interfaces::DatabaseAbstractionLayer
        {
            //TODO
            /*
             * NOTE: -> when cache age reaches its size limit (ULONG_LIMIT), restart from 0 and reset
             *          all object ages (CACHE AGE - OBJECT AGE) || (OBJECT AGE - CACHE AGE)
             *       -> check for object age overflow
             */

            public:
                /** Parameters structure for holding <code>DALCache</code> configuration data. */
                struct DALCacheParameters
                {
                    DALCacheParameters() {}
                    DALCacheParameters(unsigned long maxCommitTime, unsigned int maxCommitUpdates, unsigned int minCommitUpdates,
                    bool alwaysEvict, bool clearObjectAge, unsigned int cacheSize)
                    : maximumCommitTime(maxCommitTime), maximumCommitUpdates(maxCommitUpdates), minimumCommitUpdates(minCommitUpdates),
                      alwaysEvictObjects(alwaysEvict), alwaysClearObjectAge(clearObjectAge), maximumCacheSize(cacheSize)
                    {}
                    
                    /** Maximum time between commits. */
                    unsigned long maximumCommitTime = 0;
                    /** Maximum number of changes between commits. */
                    unsigned int maximumCommitUpdates = 0;
                    /** Minimum number of changes between commits. */
                    unsigned int minimumCommitUpdates = 0;
                    /** Denotes whether to always remove eligible objects, even if the cache is not full. */
                    bool alwaysEvictObjects = false;
                    /** Denotes whether to remove the age data for an object once it's removed from the cache. */
                    bool alwaysClearObjectAge = false;
                    /**
                     * Denotes the maximum number of objects the cache should try to maintain at any one time.\n
                     * 
                     * Note: Depending on operational conditions, the actual number of entries in the cache may
                     * be higher (this is not a hard limit).
                     */
                    unsigned int maximumCacheSize = 0;
                };
                
                /** Information structure for holding <code>DALCache</code> data. */
                struct DALCacheInformation
                {
                    DALCacheInformation() {}
                    DALCacheInformation(const DatabaseInformationContainer * info, const DatabaseSettingsContainer * settings, unsigned long currentCacheSize,
                    unsigned long ageTableSize, unsigned long uncommittedObjects, ObjectCacheAge cacheAge, bool forceCommit, bool commitDisabled,
                    unsigned long pendingCommits, DatabaseRequestID currentCommit, unsigned long pendingCacheRequests, unsigned long pendingDALRequests,
                    DatabaseAbstractionLayerID id, bool alwaysEvict, bool clearAge, bool enabled, unsigned int size, DatabaseObjectType type,
                    unsigned long maxCommitTime, unsigned int maxUpdates, unsigned int minUpdates, unsigned long hits, unsigned long misses,
                    bool stop, bool cacheThreadRunning, bool requestsThreadRunning)
                    : childInfoData(info), childSettingsData(settings), currentCacheSize(currentCacheSize), objectAgeTableSize(ageTableSize),
                      uncommittedObjects(uncommittedObjects), globalCacheAge(cacheAge), forceCommit(forceCommit), commitDisabled(commitDisabled),
                      pendingCommitRequests(pendingCommits), currentCommitRequest(currentCommit), pendingCacheRequests(pendingCacheRequests),
                      pendingDALRequests(pendingDALRequests), dalID(id), alwaysEvict(alwaysEvict), clearObjectAge(clearAge), cacheEnabled(enabled),
                      cacheSize(size), cacheType(type), maxCommitTime(maxCommitTime), maxCommitUpdates(maxUpdates), minCommitUpdates(minUpdates),
                      cacheHits(hits), cacheMisses(misses), stopCache(stop), cacheThreadRunning(cacheThreadRunning), requestsThreadRunning(requestsThreadRunning)
                    {}
                    
                    const DatabaseInformationContainer * childInfoData = nullptr;
                    const DatabaseSettingsContainer * childSettingsData = nullptr;
                    unsigned long currentCacheSize = 0;
                    unsigned long objectAgeTableSize = 0;
                    unsigned long uncommittedObjects = 0;
                    ObjectCacheAge globalCacheAge = 0;
                    bool forceCommit = false;
                    bool commitDisabled = false;
                    unsigned long pendingCommitRequests = 0;
                    DatabaseRequestID currentCommitRequest = DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID;
                    unsigned long pendingCacheRequests = 0;
                    unsigned long pendingDALRequests = 0;
                    DatabaseAbstractionLayerID dalID = DatabaseManagement_Types::INVALID_DAL_ID;
                    bool alwaysEvict = false;
                    bool clearObjectAge = false;
                    bool cacheEnabled = false;
                    unsigned int cacheSize = 0;
                    DatabaseObjectType cacheType = DatabaseObjectType::INVALID;
                    unsigned long maxCommitTime = 0;
                    unsigned int maxCommitUpdates = 0;
                    unsigned int minCommitUpdates = 0;
                    unsigned long cacheHits = 0;
                    unsigned long cacheMisses = 0;
                    bool stopCache = true;
                    bool cacheThreadRunning = false;
                    bool requestsThreadRunning = false;
                };
                
                /**
                 * Initialises the cache.
                 * 
                 * @param childDAL the child DAL that will process all DB requests
                 * @param parentLogger the file logger of the parent DatabaseManager
                 * @param parameters the cache configuration parameters
                 */
                DALCache(DALPtr childDAL, Utilities::FileLoggerPtr parentLogger, DALCacheParameters parameters);

                /**
                 * Cache destructor.
                 * 
                 * Stops all threads and clears all data structures.
                 */
                ~DALCache() override;

                DALCache() = delete;                            //No default constructor
                DALCache(const DALCache&) = delete;             //Copying not allowed (pass/access only by reference/pointer)
                DALCache& operator=(const DALCache&) = delete;  //Copying not allowed (pass/access only by reference/pointer)

                bool getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue) override;
                bool putObject(DatabaseRequestID requestID, const DataContainerPtr inputData) override;
                bool updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData) override;
                bool removeObject(DatabaseRequestID requestID, DBObjectID id) override;

                bool changeDatabaseSettings(const DatabaseSettingsContainer settings) override  { return dal->changeDatabaseSettings(settings); }
                bool buildDatabase() override                                                   { return dal->buildDatabase(); }
                bool rebuildDatabase() override                                                 { return dal->rebuildDatabase(); }
                bool clearDatabase() override                                                   { return dal->clearDatabase(); }
                bool connect() override                                                         { return dal->connect(); }
                bool disconnect() override                                                      { return commitCache(); }
                const DatabaseInformationContainer* getDatabaseInfo() const override            { return dal->getDatabaseInfo(); }
                void setID(DatabaseAbstractionLayerID id) override                              { dalID = id; dal->setID(dalID); }
                DatabaseAbstractionLayerID getID() const override                               { return dalID; }
                DatabaseObjectType getType() const override                                     { return dal->getType(); }

                /**
                 * Sends a request for performing a data commit.
                 * 
                 * All standard commit parameters will be ignored.
                 * 
                 * @return true, if the request to perform a commit was successfully enqueued
                 */
                bool commitCache();       //writes all modified containers to the DAL

                /**
                 * Drops all modified containers from the cache.
                 * 
                 * @return true, if the rollback was successful
                 */
                bool rollbackCache();     //drops all changes made to the containers

                /**
                 * Disables the normal commit process until explicitly enabled.
                 * 
                 * @return true, if successful
                 */
                bool disableCommit();

                /**
                 * Enables the normal commit process, after being explicitly disabled.
                 * 
                 * @return true, if successful
                 */
                bool enableCommit();

                /**
                 * Sets new cache configuration parameters.
                 * 
                 * @param parameters the new cache configuration parameters
                 * 
                 * @return <code>true</code>, if the operation was successful
                 */
                bool setParameters(DALCacheParameters parameters);
                
                /**
                 * Retrieves the configuration data for the cache.
                 * 
                 * @return the requested data
                 */
                DALCacheParameters getParameters();
                
                /**
                 * Retrieves general information for the cache.
                 * 
                 * @return the requested information
                 */
                DALCacheInformation getCacheInformation();

            private:
                enum class RequestType { SELECT, INSERT, UPDATE, REMOVE, SEND_SUCCESS_EVENT, SEND_FAILURE_EVENT, CACHE_OBJECT };

                //Cache management
                DALPtr dal;                                                 //next DAL
                unordered_map<DBObjectID, DataContainerPtr> cache;          //object cache
                unordered_map<DBObjectID, ObjectCacheAge> objectAgeTable;   //object ages
                unordered_map<DBObjectID, RequestType> uncommittedObjects;  //modified objects that have not been sent to DB (object -> modification type)
                ObjectCacheAge globalCacheAge = 0;                          //global cache age
                std::atomic<bool> forceCommit {false};                      //denotes whether a manual commit was requested
                std::atomic<bool> commitDisabled {false};                   //denotes whether the commit process is disabled
                unordered_map<DatabaseRequestID, DBObjectID> pendingCommitRequests; //list of pending cache commit requests
                DatabaseRequestID currentCommitRequest = DatabaseManagement_Types::INVALID_DATABASE_REQUEST_ID;

                //Requests management
                queue<DatabaseRequestID> pendingCacheRequests;              //pending SELECT/INSERT/REMOVE/UPDATE requests
                unordered_map<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*> requestsData; //data associated with requests
                unordered_map<DatabaseRequestID, bool> pendingDALRequests;  //pending requests sent to child DAL

                //Configuration
                DatabaseAbstractionLayerID dalID = DatabaseManagement_Types::INVALID_DAL_ID;
                bool alwaysEvict;                                   //always remove eligible objects, even if the cache is not full
                bool clearObjectAge;                                //remove the age data for an object once it's removed from the cache
                bool cacheEnabled;                                  //denotes the state of the cache
                unsigned int cacheSize;                             //elements; 0=no limit
                DatabaseObjectType cacheType;                       //type of stored containers (defines the behaviour of the cache)
                unsigned long maxCommitTime;                        //the maximum amount of time to wait before committing
                unsigned int maxCommitUpdates;                      //the maximum number of updates, before a commit is triggered; 0=no automatic commit
                unsigned int minCommitUpdates;                      //the minimum number of updates required for a commit;
                boost::signals2::connection onSuccessConnection;    //Events connection (success)
                boost::signals2::connection onFailureConnection;    //Events connection (failure)

                //Stats
                unsigned long cacheHits = 0;                        //total cache hits (objects were found in the cache)
                unsigned long cacheMisses = 0;                      //total cache misses (objects were not found in the cache)

                //Thread management
                Utilities::FileLoggerPtr debugLogger;                   //parent file logger
                boost::thread * requestsThreadObject;                   //main requests thread
                boost::thread * cacheThreadObject;                      //cache management thread
                boost::mutex cacheThreadMutex;                          //cache mutex
                boost::mutex requestsThreadMutex;                       //requests mutex
                boost::mutex pendingCommitRequestsMutex;                //pending commit requests mutex
                boost::condition_variable cacheThreadLockCondition;     //cache condition variable
                boost::condition_variable requestsThreadLockCondition;  //requests condition variable
                std::atomic<bool> stopCache {false};    //atomic variable for denoting the state of the cache
                std::atomic<bool> cacheThreadRunning;   //atomic variable for denoting the state of the cache thread
                std::atomic<bool> requestsThreadRunning;//atomic variable for denoting the state of the requests thread

                /**
                 * Evicts all cache objects that are eligible or the LRU object.
                 * 
                 * Note: This method is not thread-safe; the caller must ensure proper locking/synchronisation is in place.
                 * 
                 * @return true, if the eviction process completes successfully
                 */
                bool evictObjects();

                /**
                 * Main cache thread.
                 * 
                 * Deals with cache commit and object eviction.
                 */
                void cacheThread();

                /**
                 * Main requests thread.
                 * 
                 * Deals with managing incoming data requests.
                 */
                void requestsThread();

                /**
                 * Event handler for "onFailure" signals coming from the child DAL.
                 * 
                 * @param dalID the ID of the caller DAL
                 * @param requestID the associated request ID
                 * @param id the associated object ID (if any)
                 */
                void onFailureHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DBObjectID id);

                /**
                 * Event handler for "onSuccess" signals coming from the child DAL.
                 * 
                 * @param dalID the ID of the caller DAL
                 * @param requestID the associated request ID
                 * @param data the associated request data
                 */
                void onSuccessHandler(DatabaseAbstractionLayerID dalID, DatabaseRequestID requestID, DataContainerPtr data);

                /**
                 * Adds a new data request for the DAL.
                 * 
                 * Note: Thread-safe.
                 * 
                 * @param requestID the ID of the request
                 * @param type the type of the request
                 * @param requestParameter primary request parameter
                 * @param additionalParameter secondary request parameter (optional)
                 */
                void addRequest(DatabaseRequestID requestID, RequestType type, boost::any requestParameter, boost::any additionalParameter = 0)
                {
                    tuple<RequestType, boost::any, boost::any> * data = new tuple<RequestType, boost::any, boost::any>(type, requestParameter, additionalParameter);
                    logMessage(LogSeverity::Debug, "(addRequest) Entering critical section.");
                    boost::lock_guard<boost::mutex> requestsLock(requestsThreadMutex);
                    logMessage(LogSeverity::Debug, "(addRequest) Critical section entered.");

                    pendingCacheRequests.push(requestID);
                    requestsData.insert(std::pair<DatabaseRequestID, tuple<RequestType, boost::any, boost::any>*>(requestID, data));

                    logMessage(LogSeverity::Debug, "(addRequest) Sending notification to requests thread.");
                    requestsThreadLockCondition.notify_all();
                    logMessage(LogSeverity::Debug, "(addRequest) Notification to requests thread sent.");

                    logMessage(LogSeverity::Debug, "(addRequest) Exiting critical section.");
                }

                /**
                 * Checks whether the object with the specified ID is present in the cache.
                 * 
                 * Note: Thread-safe.
                 * 
                 * @param id the ID of the object to be checked
                 * @return true, if the specified object is in the cache
                 */
                bool isObjectInCache(const DBObjectID id)
                {
                    logMessage(LogSeverity::Debug, "(isObjectInCache) Entering critical section.");
                    boost::lock_guard<boost::mutex> cacheLock(cacheThreadMutex);
                    logMessage(LogSeverity::Debug, "(Is Object In Cache) Critical section entered.");
                    bool result = (cache.find(id) != cache.end());
                    logMessage(LogSeverity::Debug, "(Is Object In Cache) Exiting critical section.");
                    return result;
                }
                
                /**
                 * Logs the specified message, if the log handler is set.
                 * 
                 * @param severity the severity associated with the message/event
                 * @param message the message to be logged
                 */
                void logMessage(LogSeverity severity, const std::string & message) const
                {
                    //TODO - add DB logging

                    if(debugLogger)
                        debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "DALCache / " + Convert::toString(cacheType) + " > " + message);
                }
        };
    }
}

#endif	/* DALCACHE_H */
