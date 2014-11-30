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

#ifndef DATAPOOL_H
#define	DATAPOOL_H

#include "Types/Types.h"
#include "../Common/Types.h"

using Common_Types::ByteVector;
using StorageManagement_Types::DataPoolType;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::PoolSize;
using StorageManagement_Types::PoolState;

namespace StorageManagement_Interfaces
{
    class DataPool
    {
        public:
            virtual ~DataPool() {}
            
            ByteVector retrieveData(StoredDataID id) = 0;
            StoredDataID storeData(ByteVector data) = 0;
            DataPoolType getPoolType() = 0;
            PoolSize getPoolSize() = 0;
            PoolSize getFreeSpace() = 0;
            PoolState getPoolState() = 0;
    };
}

#endif	/* DATAPOOL_H */

