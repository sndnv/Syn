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

#ifndef SYNCDATACONTAINER_H
#define	SYNCDATACONTAINER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"

using Common_Types::SyncID;
using Common_Types::DeviceID;
using Common_Types::UserID;
using Common_Types::SessionID;
using Common_Types::Timestamp;
using Common_Types::INVALID_DATE_TIME;
using Common_Types::INVALID_SESSION_ID;
using DatabaseManagement_Types::ConflictResolutionRule_Directory;
using DatabaseManagement_Types::ConflictResolutionRule_File;
using DatabaseManagement_Types::SyncFailureAction;
using DatabaseManagement_Types::SyncResult;

namespace DatabaseManagement_Containers
{
    class SyncDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            SyncDataContainer(std::string syncName, std::string syncDescription, std::string sourceFullPath, std::string destinationFullPath, 
                              DeviceID sourceDeviceID, DeviceID destinationDeviceID, bool isSyncOneWay, bool isSyncOneTime, ConflictResolutionRule_Directory directoryConflictRule, 
                              ConflictResolutionRule_File fileConflicteRule, bool isEncryptionEnabled, bool isCompressionEnabled, UserID ownerID, std::string destPermissions, 
                              bool isOfflineSyncEnabled, bool isDifferentialSyncEnabled, unsigned int numberOfSyncRetries, SyncFailureAction failAction, Timestamp lastAttemptTime = INVALID_DATE_TIME, 
                              SyncResult lastSyncResult = SyncResult::NONE, SessionID lastSessionID = INVALID_SESSION_ID, SyncID syncID = boost::uuids::random_generator()())
                    : DataContainer(syncID, DatabaseObjectType::SYNC_FILE), name(syncName), description(syncDescription), sourcePath(sourceFullPath), destinationPath(destinationFullPath),
                      sourceDevice(sourceDeviceID), destinationDevice(destinationDeviceID), isOneWay(isSyncOneWay), isOneTime(isSyncOneTime), dirRule(directoryConflictRule),
                      fileRule(fileConflicteRule), encryptionEnabled(isEncryptionEnabled), compressionEnabled(isCompressionEnabled), owner(ownerID), destinationPermissions(destPermissions),
                      offlineSyncEnabled(isOfflineSyncEnabled), differentialSyncEnabled(isDifferentialSyncEnabled), syncRetries(numberOfSyncRetries), failureAction(failAction),
                      timestampLastAttempt(lastAttemptTime), lastResult(lastSyncResult), lastSession(lastSessionID)
            {}
            
            SyncDataContainer() = delete;                                       //No default constructor
            SyncDataContainer(const SyncDataContainer&) = default;              //Default copy constructor
            ~SyncDataContainer() = default;                                     //Default destructor
            SyncDataContainer& operator=(const SyncDataContainer&) = default;   //Default assignment operator
            
            SyncID getSyncID()                                                      const { return containerID; }
            std::string getSyncName()                                               const { return name; }
            std::string getSyncDescription()                                        const { return description; }
            std::string getSourcePath()                                             const { return sourcePath; }
            std::string getDestinationPath()                                        const { return destinationPath; }
            DeviceID getSourceDevice()                                              const { return sourceDevice; }
            DeviceID getDestinationDevice()                                         const { return destinationDevice; }
            bool isSyncOneWay()                                                     const { return isOneWay; }
            bool isSyncOneTime()                                                    const { return isOneTime; }
            ConflictResolutionRule_Directory getDirectoryConflictResolutionRule()   const { return dirRule; }
            ConflictResolutionRule_File getFileConflictResolutionRule()             const { return fileRule; }
            bool isEncryptionEnabled()                                              const { return encryptionEnabled; }
            bool isCompressionEnabled()                                             const { return compressionEnabled; }
            UserID getOwnerID()                                                     const { return owner; }
            std::string getDestinationPermissions()                                 const { return destinationPermissions; }
            bool isOfflineSyncEnabled()                                             const { return offlineSyncEnabled; }
            bool isDifferentialSyncEnabled()                                        const { return differentialSyncEnabled; }
            unsigned int getNumberOfSyncRetries()                                   const { return syncRetries; }
            SyncFailureAction getFailureAction()                                    const { return failureAction; }
            Timestamp getLastAttemptTimestamp()                                     const { return timestampLastAttempt; }
            SyncResult getLastResult()                                              const { return lastResult; }
            SessionID getLastSessionID()                                            const { return lastSession; }
            
            void setSyncName(std::string syncName)                      { name = syncName; modified = true; }
            void setSyncDescription(std::string syncDescription)        { description = syncDescription; modified = true; }
            void setSourcePath(std::string source)                      { sourcePath = source; modified = true; }
            void setDestinationPath(std::string destination)            { destinationPath = destination; modified = true; }
            void setSourceDevice(DeviceID source)                       { sourceDevice = source; modified = true; }
            void setDestinationDevice(DeviceID destination)             { destinationDevice = destination; modified = true; }
            void setSyncEncryption(bool enabled)                        { encryptionEnabled = enabled; modified = true; }
            void setSyncCompression(bool enabled)                       { compressionEnabled = enabled; modified = true; }
            void setDestinationPermissions(std::string permissions)     { destinationPermissions = permissions; modified = true; }
            void setOfflineSyncState(bool enabled)                      { offlineSyncEnabled = enabled; modified = true; }
            void setDifferentialSyncState(bool enabled)                 { differentialSyncEnabled = enabled; modified = true; }
            void setLastSyncState(SyncResult result, SessionID session) { lastResult = result; lastSession = session; modified = true; }
            
            /**
             * Sets the sync conflict resolution rules.
             * 
             * @param directoryConflictRule directory resolution rule
             * @param fileConflictRule file resolution rule
             */
            void setConflictRules(ConflictResolutionRule_Directory directoryConflictRule, ConflictResolutionRule_File fileConflictRule)
            {
                dirRule = directoryConflictRule;
                fileRule = fileConflictRule;
                modified = true;
            }
            
            /**
             * Redefines the sync behaviour.
             * 
             * @param oneWay true, if the synchronisation is one-way only
             * @param oneTime true, if the synchronisation is one-time only
             * @param failAction the action in case of failure
             * @param numberOfSyncRetries the number of retries in case of failure
             */
            void setSyncBehaviour(bool oneWay, bool oneTime, SyncFailureAction failAction, unsigned int numberOfSyncRetries) 
            {
                isOneWay = oneWay;
                isOneTime = oneTime;
                failureAction = failAction;
                syncRetries = numberOfSyncRetries;
                modified = true;
            }
            
        private:
            std::string name;
            std::string description;
            std::string sourcePath;
            std::string destinationPath;
            DeviceID sourceDevice;
            DeviceID destinationDevice;
            bool isOneWay;
            bool isOneTime;
            ConflictResolutionRule_Directory dirRule;
            ConflictResolutionRule_File fileRule;
            bool encryptionEnabled;
            bool compressionEnabled;
            UserID owner;
            std::string destinationPermissions;
            bool offlineSyncEnabled;
            bool differentialSyncEnabled;
            unsigned int syncRetries;
            SyncFailureAction failureAction;
            Timestamp timestampLastAttempt;
            SyncResult lastResult;
            SessionID lastSession;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::SyncDataContainer> SyncDataContainerPtr;
}

#endif	/* SYNCDATACONTAINER_H */

