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

#ifndef VECTORDATACONTAINER_H
#define	VECTORDATACONTAINER_H

#include <vector>
#include "DataContainer.h"
#include "../Types/Types.h"

namespace DatabaseManagement_Containers
{
    //TODO - make container iterable
    /**
     * Vector container for storing other containers.
     */
    class VectorDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            VectorDataContainer()
            : DataContainer(Common_Types::INVALID_OBJECT_ID, DatabaseObjectType::VECTOR)
            {}
            
            VectorDataContainer(const VectorDataContainer&) = default;
            ~VectorDataContainer() = default;
            VectorDataContainer& operator=(const VectorDataContainer&) = default;
            
            /**
             * Retrieves the list of stored containers.
             * 
             * @return the containers
             */
            std::vector<DataContainerPtr> getContainers() const
            {
                return containers;
            }
            
            /**
             * Retrieves the state of the vector container.
             * 
             * @return true, if there are no stored containers
             */
            bool isEmpty() const
            {
                return (containers.size() == 0);
            };
            
            /**
             * Retrieves the number of stored containers.
             * 
             * @return the number of containers/size of internal vector
             */
            unsigned long size() const
            {
                return containers.size();
            }
            
            /**
             * Removes all stored containers.
             */
            void clear()
            {
                containers.clear();
            }
            
            /**
             * Stores the specified container.
             * 
             * @param container the container to be stored
             */
            void addDataContainer(DataContainerPtr container)
            {
                containers.push_back(container);
            }
            
        private:
            std::vector<DataContainerPtr> containers;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::VectorDataContainer> VectorDataContainerPtr;
}

#endif	/* VECTORDATACONTAINER_H */

