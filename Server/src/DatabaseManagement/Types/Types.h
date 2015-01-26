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

#ifndef DATABASE_MANAGEMENT_TYPES_H
#define	DATABASE_MANAGEMENT_TYPES_H

#include <string>
#include "../../Common/Types.h"

namespace DatabaseManagement_Types
{
    typedef int DatabaseAbstractionLayerID;
    const DatabaseAbstractionLayerID INVALID_DAL_ID = -1;
    
    typedef unsigned int FunctionCallTimeoutPeriod;
    
    typedef unsigned long ObjectCacheAge;
    
    typedef unsigned long DatabaseRequestID;
    const DatabaseRequestID INVALID_DATABASE_REQUEST_ID = 0; //TODO - value?
        
    enum class DatabaseObjectType { INVALID, VECTOR, STATISTICS, SYSTEM_SETTINGS, SYNC_FILE, DEVICE, SCHEDULE, USER, LOG, SESSION };
    enum class DatabaseManagerOperationMode { PRPW, PRCW, CRCW, INVALID };
    enum class DatabaseFailureAction { INVALID, IGNORE_FAILURE, DROP_IF_NOT_LAST, DROP_DAL, PUSH_TO_BACK, INITIATE_RECONNECT };
    enum class StatisticType { INSTALL_TIMESTAMP, START_TIMESTAMP, TOTAL_TRANSFERRED_DATA, TOTAL_TRANSFERRED_FILES, TOTAL_FAILED_TRANSFERS, TOTAL_RETRIED_TRANSFERS };
    enum class SystemParameterType { DATA_IP_ADDRESS, DATA_IP_PORT, COMMAND_IP_ADDRESS, COMMAND_IP_PORT, FORCE_COMMAND_ENCRYPTION, FORCE_DATA_ENCRYPTION, 
                                     FORCE_DATA_COMPRESSION, PENDING_DATA_POOL_SIZE, PENDING_DATA_POOL_PATH, PENDING_DATA_RETENTION, IN_MEMORY_POOL_SIZE, 
                                     IN_MEMORY_POOL_RETENTION, SUPPORTED_PROTOCOLS, COMMAND_RETRIES_MAX, DATA_RETRIES_MAX, SESSION_TIMEOUT, SESSION_KEEP_ALIVE,
                                     MINIMIZE_MEMORY_USAGE, DB_IMMEDIATE_FLUSH, DB_CACHE_FLUSH_INTERVAL, DB_OPERATION_MODE, DB_MAX_READ_RETRIES, DB_MAX_WRITE_RETRIES };
    enum class LogSeverity { Info, Warning, Error, Debug };
    enum class DataTransferType { PUSH, PULL };
    enum class SessionType { COMMAND, DATA, ADMIN };
    enum class ScheduleIntervalType { SECONDS, MINUTES, HOURS, DAYS, MONTHS };
    enum class ConflictResolutionRule_Directory { OVERWRITE_SOURCE, OVERWRITE_DESTINATION, MERGE, RENAME_AND_COPY, COPY_AND_RENAME, STOP, ASK };
    enum class ConflictResolutionRule_File { OVERWRITE_SOURCE, OVERWRITE_DESTINATION, RENAME_AND_COPY, COPY_AND_RENAME, STOP, ASK };
    enum class SyncFailureAction { SKIP, RETRY_NOW, RETRY_LATER, STOP };
    enum class SyncResult { NONE, SUCCESSFUL, FAILED, PARTIAL };
    
    enum class DatabaseIDType { INTEGER, UUID, STRING, INVALID };
    
    class DatabaseSelectConstraints
    {
        private:
            DatabaseSelectConstraints() {}
            
        public:
            enum class STATISTCS { GET_ALL, LIMIT_BY_TYPE };
            enum class SYSTEM { GET_ALL, LIMIT_BY_TYPE };
            enum class SYNC { GET_ALL, LIMIT_BY_ID, LIMIT_BY_OWNER, LIMIT_BY_DEVICE, LIMIT_BY_PATH, LIMIT_BY_ENCRYPTION, LIMIT_BY_COMPRESSION, LIMIT_BY_OFFLINE_SYNC, LIMIT_BY_DIFFERENTIAL_SYNC };
            enum class DEVICES { GET_ALL, LIMIT_BY_ID, LIMIT_BY_TRANSFER_TYPE, LIMIT_BY_OWNER, LIMIT_BY_ADDRESS };
            enum class SCHEDULES { GET_ALL, LIMIT_BY_ID, LIMIT_BY_STATE, LIMIT_BY_SYNC };
            enum class USERS { GET_ALL, LIMIT_BY_ID, LIMIT_BY_NAME, LIMIT_BY_ACCESS_LEVEL, LIMIT_BY_LOCKED_STATE };
            enum class LOGS { GET_ALL, LIMIT_BY_ID, LIMIT_BY_SEVERITY, LIMIT_BY_SOURCE };
            enum class SESSIONS { GET_ALL, LIMIT_BY_ID, LIMIT_BY_TYPE, LIMIT_BY_DEVICE, LIMIT_BY_USER, LIMIT_BY_STATE, LIMIT_BY_PERSISTENCY };
    };
}

#endif	/* DATABASE_MANAGEMENT_TYPES_H */

