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

#ifndef INSTRUCTION_MANAGEMENT_TYPES_H
#define	INSTRUCTION_MANAGEMENT_TYPES_H

//Note: 'Utilities/Strings/Instructions.cpp' needs to be updated when changes to enums are made.
namespace InstructionManagement_Types
{
    typedef unsigned int InstructionSourceID;
    const InstructionSourceID INVALID_INSTRUCTION_SOURCE_ID = 0;
    
    enum class InstructionSetType
    {
        INVALID, TEST,
        CONNECTION_MANAGER, NETWORK_MANAGER_ADMIN, NETWORK_MANAGER_USER, NETWORK_MANAGER_STATE, //Networking
        NETWORK_MANAGER_CONNECTION_LIFE_CYCLE, NETWORK_MANAGER_CONNECTION_BRIDGING , //Networking
        DATABASE_MANAGER, DAL, DAL_CACHE, DAL_QUEUE, DAL_MIGRATOR, DAL_DISTRIBUTED_CACHE, //Database Management
        STORAGE_MANAGER, //Storage
        SESSION_MANAGER, //Sessions
        FILE_LOGGER, THREAD_POOL, //Utilities
        USER_MANAGER_ADMIN, USER_MANAGER_SELF, DEVICE_MANAGER_ADMIN, DEVICE_MANAGER_USER, DATABASE_LOGGER //Entities
    };
    
    enum class TestInstructionType
    {
        DO_TEST_1,
        DO_TEST_2,
        DO_TEST_3,
        INVALID
    };
    
    enum class DatabaseManagerInstructionType
    {
        /* CORE */
        GET_QUEUES_LIST,
        GET_CACHES_LIST,
        GET_DALS_LIST,
        SET_DEFAULT_CACHE_PARAMS,
        GET_DEFAULT_CACHE_PARAMS,
        SET_CACHE_PARAMS,
        GET_CACHE_PARAMS,
        SET_QUEUE_PARAMS,
        GET_QUEUE_PARAMS,
        SET_FUNCTION_TIMEOUT,
        GET_FUNCTION_TIMEOUT,
        ADD_DAL,
        ADD_DAL_WITH_CACHE_PARAMS,
        REMOVE_DAL,
        
        /* FUNCTIONS_STATISTICS */
        GET_SYSTEM_INSTALL_TIMESTAMP,
        GET_SYSTEM_START_TIMESTAMP,
        GET_TOTAL_TRANSFERRED_DATA,
        GET_TOTAL_NUMBER_TRANSFERRED_FILES,
        GET_TOTAL_NUMBER_FAILED_TRANSFERS,
        GET_TOTAL_NUMBER_RETRIED_TRANSFERS,
        GET_ALL_STATS,
        
        /* FUNCTIONS_SYSTEM */
        SET_DATA_IP_ADDRESS,
        GET_DATA_IP_ADDRESS,
        SET_DATA_IP_PORT,
        GET_DATA_IP_PORT,
        SET_COMMAND_IP_ADDRESS,
        GET_COMMAND_IP_ADDRESS,
        SET_COMMAND_IP_PORT,
        GET_COMMAND_IP_PORT,
        SET_FORCE_COMMAND_ENCRYPTION,
        GET_FORCE_COMMAND_ENCRYPTION,
        SET_FORCE_DATA_ENCRYPTION,
        GET_FORCE_DATA_ENCRYPTION,
        SET_FORCE_DATA_COMPRESSION,
        GET_FORCE_DATA_COMPRESSION,
        SET_PENDING_DATA_POOL_SIZE,
        GET_PENDING_DATA_POOL_SIZE,
        SET_PENDING_DATA_POOL_PATH,
        GET_PENDING_DATA_POOL_PATH,
        SET_PENDING_DATA_POOL_RETENTION,
        GET_PENDING_DATA_POOL_RETENTION,
        SET_IN_MEMORY_DATA_POOL_SIZE,
        GET_IN_MEMORY_DATA_POOL_SIZE,
        SET_IN_MEMORY_DATA_POOL_RETENTION,
        GET_IN_MEMORY_DATA_POOL_RETENTION,
        SET_COMMAND_RESEND_RETRIES,
        GET_COMMAND_RESEND_RETRIES,
        SET_DATA_RESEND_RETRIES,
        GET_DATA_RESENT_RETRIES,
        SET_SESSION_TIMEOUT,
        GET_SESSION_TIMEOUT,
        SET_SESSION_KEEP_ALIVE,
        GET_SESSION_KEEP_ALIVE,
        SET_MINIMIZE_MEMORY_USAGE,
        GET_MINIMIZE_MEMORY_USAGE,
        ADD_SUPPORTED_PROTOCOL,
        REMOVE_SUPPORTED_PROTOCOL,
        SET_DB_IMMEDIATE_LOG_FLUSH,
        GET_DB_IMMEDIATE_LOG_FLUSH,
        SET_DB_CACHE_FLUSH_INTERVAL,
        GET_DB_CACHE_FLUSH_INTERVAL,
        SET_DB_OPERATION_MODE,
        GET_DB_OPERATION_MODE,
        
        GET_ALL_SYSTEM_PARAMETERS,
        
        /* FUNCTIONS_SYNC_FILES */
        ADD_SYNC,
        UPDATE_SYNC,
        REMOVE_SYNC,
        GET_SYNCS_BY_CONSTRAINT,
        GET_SYNC,
        
        /* FUNCTIONS_DEVICES */
        ADD_DEVICE,
        UPDATE_DEVICE,
        REMOVE_DEVICE,
        GET_DEVICES_BY_CONSTRAINT,
        GET_DEVICE,
        
        /* FUNCTIONS_SCHEDULES */
        ADD_SCHEDULE,
        UPDATE_SCHEDULE,
        REMOVE_SCHEDULE,
        GET_SCHEDULES_BY_CONSTRAINT,
        GET_SCHEDULE,
        
        /* FUNCTIONS_USERS */
        ADD_USER,
        UPDATE_USER,
        REMOVE_USER,
        GET_USERS_BY_CONSTRAINT,
        GET_USER,
        
        /* FUNCTIONS_LOGS */
        ADD_LOG,
        ADD_LOG_ASYNC,
        GET_LOGS_BY_CONSTRAINT,
        GET_LOG,
        
        /* FUNCTIONS_SESSIONS */
        GET_SESSIONS_BY_CONSTRAINT,
        GET_SESSION,
        
        INVALID
    };
    
    enum class SessionManagerInstructionType
    {
        GET_SESSION,
        GET_SESSIONS_BY_CONSTRAINT,
        FORCE_SESSION_EXPIRATION,
        FORCE_SESSION_REAUTHENTICATION,
        FORCE_EXPIRATION_PROCESS,
        DEBUG_GET_STATE,
        INVALID
    };
    
    enum class UserManagerAdminInstructionType
    {
        GET_USER,
        GET_USERS_BY_CONSTRAINT,
        ADD_USER,
        REMOVE_USER,
        RESET_PASSWORD,
        FORCE_PASSWORD_RESET,
        LOCK_USER,
        UNLOCK_USER,
        UPDATE_ACCESS_LEVEL,
        RESET_FAILED_AUTHENTICATION_ATTEMPTS,
        ADD_AUTHORIZATION_RULE,
        REMOVE_AUTHORIZATION_RULE,
        CLEAR_AUTHORIZATION_RULES,
        DEBUG_GET_STATE,
        INVALID
    };
    
    enum class UserManagerSelfInstructionType
    {
        GET_USER,
        RESET_PASSWORD,
        INVALID
    };
    
    enum class DeviceManagerAdminInstructionType
    {
        GET_DEVICE,
        GET_DEVICES_BY_CONSTRAINT,
        ADD_DEVICE,
        REMOVE_DEVICE,
        RESET_DEVICE_PASSWORD,
        UPDATE_CONNECTION_INFO,
        UPDATE_GENERAL_INFO,
        LOCK_DEVICE,
        UNLOCK_DEVICE,
        RESET_FAILED_AUTHENTICATION_ATTEMPTS,
        DEBUG_GET_STATE,
        INVALID
    };
    
    enum class DeviceManagerUserInstructionType
    {
        GET_DEVICE,
        GET_DEVICES,
        ADD_DEVICE,
        REMOVE_DEVICE,
        RESET_DEVICE_PASSWORD,
        UPDATE_CONNECTION_INFO,
        UPDATE_GENERAL_INFO,
        LOCK_DEVICE,
        UNLOCK_DEVICE,
        RESET_FAILED_AUTHENTICATION_ATTEMPTS,
        INVALID
    };
    
    enum class DatabaseLoggerInstructionType
    {
        GET_LOG,
        GET_LOGS_BY_CONSTRAINT,
        UPDATE_SOURCE_LOGGING_LEVEL,
        UPDATE_DEFAULT_LOGGING_LEVEL,
        DEBUG_GET_STATE,
        INVALID
    };
    
    enum class NetworkManagerAdminInstructionType
    {
        START_CONNECTION_MANAGER,
        STOP_CONNECTION_MANAGER,
        RESTART_CONNECTION_MANAGER,
        INVALID
    };
    
    enum class NetworkManagerUserInstructionType
    {
        SEND_INSTRUCTION,
        SEND_DATA,
        INVALID
    };
    
    enum class NetworkManagerStateInstructionType
    {
        GET_CONNECTION,
        GET_CONNECTIONS_BY_TYPE,
        GET_CONNECTIONS_BY_STATE,
        DEBUG_GET_STATE,
        INVALID
    };
    
    enum class NetworkManagerConnectionLifeCycleInstructionType
    {
        TERMINATE,
        KEEP_ALIVE,
        RELOAD_KEYS,
        REAUTHENTICATE,
        OPEN_INIT_CONNECTION,
        OPEN_DATA_CONNECTION,
        INVALID
    };
    
    enum class NetworkManagerConnectionBridgingInstructionType
    {
        OPEN_BRIDGE,
        CLOSE_BRIDGE,
        FORWARD_PAYLOAD,
        INVALID
    };
}

#endif	/* INSTRUCTION_MANAGEMENT_TYPES_H */

