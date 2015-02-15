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
using Common_Types::DeviceID;
using Common_Types::SessionID;
using Common_Types::UserID;
using Common_Types::SyncID;
using Common_Types::TransferredDataAmount;
using Common_Types::INVALID_TRANSFERRED_DATA_AMOUNT;
using Common_Types::SessionType;

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
                    : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::SESSION),
                      timestampOpen(boost::posix_time::second_clock::universal_time()), 
                      timestampClose(INVALID_DATE_TIME),
                      timestampLastActivity(boost::posix_time::second_clock::universal_time()),
                      type(sessionType), device(deviceID), user(userID), isPersistent(persistent),
                      isActive(true), dataSent(0), dataReceived(0), commandsSent(0), commandsReceived(0)
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
             * @param sentData the amount of data sent
             * @param receivedData the amount of data received
             * @param sentCommands the number of commands sent
             * @param receivedCommands the number of commands received
             */
            SessionDataContainer(SessionID sessionID, Timestamp openTime, Timestamp closeTime,
                                Timestamp lastActivityTime, SessionType sessionType, DeviceID deviceID, 
                                UserID userID, bool persistent, bool active, TransferredDataAmount sentData,
                                TransferredDataAmount receivedData, unsigned long sentCommands,
                                unsigned long receivedCommands)
            : DataContainer(sessionID, DatabaseObjectType::SESSION), timestampOpen(openTime),
              timestampClose(closeTime), timestampLastActivity(lastActivityTime), type(sessionType),
              device(deviceID), user(userID), isPersistent(persistent), isActive(active), dataSent(sentData),
              dataReceived(receivedData), commandsSent(sentCommands), commandsReceived(receivedCommands)
            {}

            SessionDataContainer() = delete;                                          //No default constructor
            SessionDataContainer(const SessionDataContainer&) = default;              //Default copy constructor
            ~SessionDataContainer() = default;                                        //Default destructor
            SessionDataContainer& operator=(const SessionDataContainer&) = default;   //Default assignment operator
            
            SessionID getSessionID()                const { return containerID; }
            Timestamp getOpenTimestamp()            const { return timestampOpen; }
            Timestamp getCloseTimestamp()           const { return timestampClose; }
            Timestamp getLastActivityTimestamp()    const { return timestampLastActivity; }
            SessionType getSessionType()            const { return type; }
            DeviceID getDevice()                    const { return device; }
            UserID getUser()                        const { return user; }
            bool isSessionPersistent()              const { return isPersistent; }
            bool isSessionActive()                  const { return isActive; }
            TransferredDataAmount getDataSent()     const { return dataSent; }
            TransferredDataAmount getDataReceived() const { return dataReceived; }
            unsigned long getCommandsSent()         const { return commandsSent; }
            unsigned long getCommandsReceived()     const { return commandsReceived; }
            
            void closeSession()
            {
                timestampClose = boost::posix_time::second_clock::universal_time();
                isActive = false;
                modified = true;
            }
            
            void addDataSent(TransferredDataAmount amount)
            {
                timestampLastActivity = boost::posix_time::second_clock::universal_time();
                dataSent += amount;
                modified = true;
            }
            
            void addDataReceived(TransferredDataAmount amount)
            {
                timestampLastActivity = boost::posix_time::second_clock::universal_time();
                dataReceived += amount; modified = true;
            }
            
            void addCommandsSent(unsigned long amount)
            {
                timestampLastActivity = boost::posix_time::second_clock::universal_time();
                commandsSent += amount;
                modified = true;
            }
            
            void addCommandsReceived(unsigned long amount)
            {
                timestampLastActivity = boost::posix_time::second_clock::universal_time();
                commandsReceived += amount;
                modified = true;
            }
            
        private:
            Timestamp timestampOpen;
            Timestamp timestampClose;
            Timestamp timestampLastActivity;
            SessionType type;
            DeviceID device;
            UserID user;
            bool isPersistent;
            bool isActive;
            TransferredDataAmount dataSent;
            TransferredDataAmount dataReceived;
            unsigned long commandsSent;
            unsigned long commandsReceived;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::SessionDataContainer> SessionDataContainerPtr;
}

#endif	/* SESSIONDATACONTAINER_H */

