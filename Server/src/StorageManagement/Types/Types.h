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
    
    typedef unsigned long PoolSize;
    const PoolSize INVALID_POOL_SIZE = 0;
    
    enum class DataPoolType { INVALID, LOCAL_FS, LOCAL_MEMORY, REMOTE_FS, REMOTE_MEMORY };
    enum class PoolState { INVALID, READ_WRITE, READ_ONLY, ERROR };
}

#endif	/* STORAGE_MANAGEMENT_TYPES_H */

