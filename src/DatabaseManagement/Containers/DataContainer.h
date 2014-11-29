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

#ifndef DATACONTAINER_H
#define	DATACONTAINER_H

#include "../Types/Types.h"
#include "../../Utilities/Tools.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/uuid.hpp>

using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DBObjectID;

namespace DatabaseManagement_Containers
{
    /**
     * Parent container class for all data handled by the database management.
     */
    class DataContainer
    {
        public:
            virtual ~DataContainer() {}
            
            /**
             * Resets the state of the container to "unmodified".
             */
            void resetModifiedState() { modified = false; }
            
            /**
             * Retrieves the container ID.
             * 
             * @return the container ID
             */
            DBObjectID getContainerID() const { return containerID; }
            
            /**
             * Retrieves the container type.
             * 
             * @return the container type
             */
            DatabaseObjectType getDataType() const { return dataType; }
            
            /**
             * Retrieves the state of the container.
             * 
             * @return true, if the container was modified
             */
            bool isModified() const { return modified; }
            
            /**
             * Retrieves a string representation of the base container
             * 
             * @return the container, as a string
             */
            std::string toString() const { return Utilities::Tools::toString(containerID) + "," + Utilities::Tools::toString(dataType); }
            
        protected:
            DataContainer(DBObjectID id, DatabaseObjectType type) : containerID(id), dataType(type) {}
            
            DBObjectID containerID;     //container ID
            DatabaseObjectType dataType;//container type
            bool modified = false;      //denotes whether any changes have been made to the container (enforced by subclasses)
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::DataContainer> DataContainerPtr;
}

#endif	/* DATACONTAINER_H */

