/**
 * Copyright (C) 2015 https://github.com/sndnv
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

#ifndef DATABASELOGGINGSOURCE_H
#define	DATABASELOGGINGSOURCE_H

#include <string>
#include "../../Common/Types.h"

namespace EntityManagement_Interfaces
{
    /**
     * Interface for defining a source component for database logging.\n
     * 
     * A source needs to be registered with a <code>DatabaseLogger</code>
     * before it is able to log messages.
     */
    class DatabaseLoggingSource
    {
        public:
            /**
             * Retrieves the name of the source component.
             * 
             * @return the name of the source
             */
            virtual std::string getSourceName() const = 0;
            
            /**
             * Registers the specified logging handler with the source component.\n
             * 
             * Note: The signature of the handler is:
             * <code>void(Common_Types::LogSeverity, const std::string &)</code>.
             * 
             * @param handler the handler to be used by the source component to send logging messages
             * @return <code>true</code>, if the registration was successful
             */
            virtual bool registerLoggingHandler(const std::function<void(Common_Types::LogSeverity, const std::string &)> handler) = 0;
    };
}

#endif	/* DATABASELOGGINGSOURCE_H */

