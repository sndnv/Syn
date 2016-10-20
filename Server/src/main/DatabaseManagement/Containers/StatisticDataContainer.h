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

#ifndef STATISTICSDATACONTAINER_H
#define	STATISTICSDATACONTAINER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <boost/uuid/random_generator.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"

using DatabaseManagement_Types::StatisticType;

namespace DatabaseManagement_Containers
{
    class StatisticDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            StatisticDataContainer(StatisticType statType, boost::any statValue)
                : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::STATISTICS), type(statType), value(statValue) {}
            
            StatisticDataContainer() = delete;                                          //No default constructor
            StatisticDataContainer(const StatisticDataContainer&) = default;            //Default copy constructor
            ~StatisticDataContainer() = default;                                        //Default destructor
            StatisticDataContainer& operator=(const StatisticDataContainer&) = default; //Default assignment operator
            
            StatisticType getStatisticType() const { return type; }
            boost::any getStatisticValue()   const { return value; }
            
            void setStatisticValue(boost::any newValue) { value = newValue; modified = true; }

        private:
            StatisticType type;
            boost::any value;
    };

    typedef boost::shared_ptr<DatabaseManagement_Containers::StatisticDataContainer> StatisticDataContainerPtr;
}

#endif	/* STATISTICSDATACONTAINER_H */

