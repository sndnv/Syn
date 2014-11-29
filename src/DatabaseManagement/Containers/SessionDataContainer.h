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

#ifndef SESSIONDATACONTAINER_H
#define	SESSIONDATACONTAINER_H

#include <queue>
#include <string>
#include <vector>
#include <boost/uuid/random_generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/shared_ptr.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"

using Common_Types::Timestamp;
using Common_Types::INVALID_DATE_TIME;
using DatabaseManagement_Types::SessionType;
using DatabaseManagement_Types::DeviceID;
using DatabaseManagement_Types::SessionID;
using DatabaseManagement_Types::UserID;
using DatabaseManagement_Types::SyncID;
using Common_Types::TransferredDataAmount;
using Common_Types::INVALID_TRANSFERRED_DATA_AMOUNT;

namespace DatabaseManagement_Containers
{
    class SessionDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            /**
             * Creates a new session data container.
             * 
             * Note: Used when supplying data to the database.
             * 
             * @param sessionType session type
             * @param deviceID associated device ID
             * @param userID associated user ID
             * @param persistent true, if the session is to be flagged as persistent
             */
            SessionDataContainer(SessionType sessionType, DeviceID deviceID, UserID userID, bool persistent)
                    : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::SESSION), timestampOpen(INVALID_DATE_TIME), 
                      timestampClose(INVALID_DATE_TIME), timestampLastActivity(INVALID_DATE_TIME), type(sessionType), device(deviceID), user(userID), 
                      isPersistent(persistent), isActive(false), dataTransferred(INVALID_TRANSFERRED_DATA_AMOUNT), commandsSent(0)
            {}
                    
            /**
             * Creates a new session data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param sessionID session UUID
             * @param openTime session start timestamp
             * @param closeTime session end timestamp
             * @param lastActivityTime last activity timestamp
             * @param sessionType type
             * @param deviceID associated device ID
             * @param userID associated user ID
             * @param persistent true, if the session is flagged as persistent
             * @param active true, if the session is currently active
             * @param dataXferred amount of data transferred
             * @param numberOfCommandsSent number of commands sent
             */
            SessionDataContainer(SessionID sessionID, Timestamp openTime, Timestamp closeTime, Timestamp lastActivityTime, SessionType sessionType, DeviceID deviceID, 
                                 UserID userID, bool persistent, bool active, TransferredDataAmount dataXferred, unsigned long numberOfCommandsSent)
                    : DataContainer(sessionID, DatabaseObjectType::SESSION), timestampOpen(openTime), timestampClose(closeTime), timestampLastActivity(lastActivityTime),
                      type(sessionType), device(deviceID), user(userID), isPersistent(persistent), isActive(active), dataTransferred(dataXferred), commandsSent(numberOfCommandsSent)
            {}

            SessionDataContainer() = delete;                                          //No default constructor
            SessionDataContainer(const SessionDataContainer&) = default;              //Default copy constructor
            ~SessionDataContainer() = default;                                        //Default destructor
            SessionDataContainer& operator=(const SessionDataContainer&) = default;   //Default assignment operator
            
            SessionID getSessionID()                            const { return containerID; }
            Timestamp getOpenTimestamp()                        const { return timestampOpen; }
            Timestamp getCloseTimestamp()                       const { return timestampClose; }
            Timestamp getLastActivityTimestamp()                const { return timestampLastActivity; }
            SessionType getSessionType()                        const { return type; }
            DeviceID getDevice()                                const { return device; }
            UserID getUser()                                    const { return user; }
            bool isSessionPersistent()                          const { return isPersistent; }
            bool isSessionActive()                              const { return isActive; }
            TransferredDataAmount getDataTransferred()          const { return dataTransferred; }
            std::queue<SyncID> getPendingTransfers()            const { return pendingTransfers; }
            std::queue<SyncID> getFailedTransfers()             const { return failedTransfers; }
            std::queue<SyncID> getCompletedTransfers()          const { return completedTransfers; }
            unsigned long getNumberOfCommandsSent()             const { return commandsSent; }
            std::queue<std::string> getPendingCommands()        const { return pendingCommands; }
            std::queue<std::string> getFailedCommands()         const { return failedCommands; }
            std::queue<std::string> getCompletedCommands()      const { return completedCommands; }
            
            void closeSession()                                         { isActive = false; modified = true; }
            void incrementDataTrasnferred(TransferredDataAmount amount) { dataTransferred += amount; modified = true; }
            void incrementNumberOfCommandsSent()                        { commandsSent++; modified = true; }
            void incrementNumberOfCommandsSent(unsigned long amount)    { commandsSent += amount; modified = true; }
            void addPendingTransfer(SyncID id)                          { pendingTransfers.push(id); modified = true; }
            void addPendingTransfers(std::vector<SyncID> syncIDs)       { for(SyncID currentTransfer : syncIDs) pendingTransfers.push(currentTransfer); modified = true; }
            SyncID popNextPendingTransfer()                             { SyncID result = pendingTransfers.front(); pendingTransfers.pop(); modified = true; return result; }
            void addFailedTransfer(SyncID id)                           { failedTransfers.push(id); modified = true; }
            void addCompletedTransfer(SyncID id)                        { completedTransfers.push(id); modified = true; }
            void addPendingCommand(std::string command)                 { pendingCommands.push(command); modified = true; }
            void addPendingCommands(std::vector<std::string> commands)  { for(std::string currentCommand : commands) pendingCommands.push(currentCommand); modified = true; }
            std::string popNextPendingCommand()                         { std::string result = pendingCommands.front(); pendingCommands.pop(); modified = true; return result; }
            void addFailedCommand(std::string command)                  { failedCommands.push(command); modified = true; }
            void addCompletedCommand(std::string command)               { completedCommands.push(command); modified = true; }
            
        private:
            Timestamp timestampOpen;
            Timestamp timestampClose;
            Timestamp timestampLastActivity;
            SessionType type;
            DeviceID device;
            UserID user;
            bool isPersistent;
            bool isActive;
            TransferredDataAmount dataTransferred;
            std::queue<SyncID> pendingTransfers;
            std::queue<SyncID> failedTransfers;
            std::queue<SyncID> completedTransfers;
            unsigned long commandsSent;
            std::queue<std::string> pendingCommands;
            std::queue<std::string> failedCommands;
            std::queue<std::string> completedCommands;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::SessionDataContainer> SessionDataContainerPtr;
}

#endif	/* SESSIONDATACONTAINER_H */

