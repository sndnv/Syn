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

#ifndef SYSTEMDATACONTAINER_H
#define	SYSTEMDATACONTAINER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"

using DatabaseManagement_Types::SystemParameterType;

namespace DatabaseManagement_Containers
{
    class SystemDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            SystemDataContainer(SystemParameterType type, boost::any value)
                : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::SYSTEM_SETTINGS), paramType(type), paramValue(value) {}
            
            SystemDataContainer() = delete;                                         //No default constructor
            SystemDataContainer(const SystemDataContainer&) = default;              //Default copy constructor
            ~SystemDataContainer() = default;                                       //Default destructor
            SystemDataContainer& operator=(const SystemDataContainer&) = default;   //Default assignment operator
            
            SystemParameterType getSystemParameterType()    const { return paramType; }
            boost::any getSystemParameterValue()            const { return paramValue; }
            
            void setSystemParameterValue(boost::any value) { paramValue = value; modified = true; }

        private:
            SystemParameterType paramType;
            boost::any paramValue;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::SystemDataContainer> SystemDataContainerPtr;
}

#endif	/* SYSTEMDATACONTAINER_H */

