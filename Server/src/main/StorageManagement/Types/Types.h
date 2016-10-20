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

#ifndef STORAGE_MANAGEMENT_TYPES_H
#define	STORAGE_MANAGEMENT_TYPES_H

#include <climits>
#include <boost/uuid/uuid_generators.hpp>

namespace StorageManagement_Types
{
    typedef unsigned long StoredDataID;
    const StoredDataID INVALID_STORED_DATA_ID = 0;
    
    typedef StoredDataID EntitiesCountType;
    
    typedef unsigned long DiskDataAddress;
    const DiskDataAddress INVALID_DISK_DATA_ADDRESS = 0;
    
    typedef unsigned long DataSize;
    const DataSize MAX_DATA_SIZE = ULONG_MAX;
    const DataSize INVALID_DATA_SIZE = 0;
    
    enum class DataPoolType { INVALID, AGGREGATE, LOCAL_DISK, LOCAL_MEMORY, REMOTE_DISK, REMOTE_MEMORY };
    enum class PoolState { INVALID, OPEN, CLOSED, FAILED };
    enum class PoolMode { INVALID, READ_WRITE, READ_ONLY };
    
    typedef unsigned int PoolID;
    const PoolID INVALID_POOL_ID = 0;
    
    typedef boost::uuids::uuid PoolUUID;
    const PoolUUID INVALID_POOL_UUID = boost::uuids::nil_uuid();
    
    enum class SimpleLinkActionType
    {
        INVALID,
        COPY,   //denotes copying data from a source pool to a target pool
        REMOVE  //denotes removal of data from a source pool
    };
    
    enum class LinkActionType
    {
        INVALID,
        DISTRIBUTE, //distributes incoming entities between all target pools
        COPY,       //copies incoming entities from the source to the target pool
        MOVE,       //moves incoming entities from the source to the target pool
        DISCARD,    //discards incoming entities from the source pool; no target pool is defined
        SKIP        //skips the action defined between the specified source and target pools
                    //or performs a copy, based on the specified condition
    };
    
    enum class LinkActionConditionType
    {
        INVALID,
        NONE,               //performs the action immediately
        TIMED,              //performs the action after X number of seconds
        SOURCE_MIN_FULL,    //performs the action if the source is at least X% full
        TARGET_MIN_FULL,    //performs the action if the target is at least X% full
        SOURCE_MAX_FULL,    //performs the action if the source is at most X% full
        TARGET_MAX_FULL,    //performs the action if the target is at most X% full
        SOURCE_MIN_ENTITIES,//performs the action if the source has at least X number of entities
        SOURCE_MAX_ENTITIES,//performs the action if the source has at most X number of entities
        TARGET_MIN_ENTITIES,//performs the action if the target has at least X number of entities
        TARGET_MAX_ENTITIES,//performs the action if the target has at most X number of entities
        DATA_MIN_SIZE,      //performs the action if the supplied data is at least X bytes in size
        DATA_MAX_SIZE       //performs the action if the supplied data is at most X bytes in size
    };
    
    typedef unsigned int LinkActionConditionValue;
}

#endif	/* STORAGE_MANAGEMENT_TYPES_H */

