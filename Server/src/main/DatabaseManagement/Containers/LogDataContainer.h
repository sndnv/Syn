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

#ifndef LOGDATACONTAINER_H
#define	LOGDATACONTAINER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include <boost/uuid/random_generator.hpp>

using Common_Types::LogSeverity;
using Common_Types::LogID;
using Common_Types::Timestamp;

namespace DatabaseManagement_Containers
{
    class LogDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            /**
             * Creates a new log data container.
             * 
             * Note: Used when supplying data to the database.
             * 
             * @param severity event severity
             * @param source event source
             * @param timestamp event timestamp
             * @param message event message
             */
            LogDataContainer(LogSeverity severity, std::string source,
                             Timestamp timestamp, std::string message)
            : LogDataContainer(boost::uuids::random_generator()(),
              severity, source, timestamp, message)
            {}
            
            /**
             * Creates a new log data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id log ID
             * @param severity event severity
             * @param source event source
             * @param timestamp event timestamp
             * @param message event message
             */
            LogDataContainer(LogID id, LogSeverity severity, std::string source,
                             Timestamp timestamp, std::string message)
            : DataContainer(id, DatabaseObjectType::LOG), eventSeverity(severity),
              sourceName(source), eventTimestamp(timestamp), eventMessage(message)
            {}
            
            LogDataContainer() = delete;
            LogDataContainer(const LogDataContainer&) = default;
            ~LogDataContainer() = default;
            LogDataContainer& operator=(const LogDataContainer&) = default;
            
            LogID getLogID()                const { return containerID; }
            LogSeverity getLogSeverity()    const { return eventSeverity; }
            std::string getLogSourceName()  const { return sourceName; }
            Timestamp getLogTimestamp()     const { return eventTimestamp; }
            std::string getLogMessage()     const { return eventMessage; }

        private:
            LogSeverity eventSeverity;
            std::string sourceName;
            Timestamp eventTimestamp;
            std::string eventMessage;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::LogDataContainer> LogDataContainerPtr;
}

#endif	/* LOGDATACONTAINER_H */

