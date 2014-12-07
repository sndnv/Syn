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

namespace StorageManagement_Types
{
    typedef unsigned long StoredDataID;
    const StoredDataID INVALID_STORED_DATA_ID = 0;
    
    typedef StoredDataID EntitiesCountType;
    
    typedef unsigned long DiskDataAddress;
    const DiskDataAddress INVALID_DISK_DATA_ADDRESS = 0;
    
    typedef unsigned long DiskDataSize;
    const DiskDataSize INVALID_DISK_DATA_SIZE = 0;
    
    enum class DataPoolType { INVALID, LOCAL_DISK, LOCAL_MEMORY, REMOTE_DISK, REMOTE_MEMORY };
    enum class PoolState { INVALID, OPEN, CLOSED, FAILED };
    enum class PoolMode { INVALID, READ_WRITE, READ_ONLY };
}

#endif	/* STORAGE_MANAGEMENT_TYPES_H */

