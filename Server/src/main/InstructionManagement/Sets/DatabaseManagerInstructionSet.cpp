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

#include "DatabaseManagerInstructionSet.h"

namespace InstructionManagement_Sets
{
    template <>
    void InstructionSet<DatabaseManagerInstructionType>::buildTable()
    {
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_QUEUES_LIST,                   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_CACHES_LIST,                   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DALS_LIST,                     &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS,          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS,          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_CACHE_PARAMS,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_CACHE_PARAMS,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_QUEUE_PARAMS,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_QUEUE_PARAMS,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT,              &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT,              &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_DAL,                           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_DAL,                        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_STATISTICS */
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP,      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES,&InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS, &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS,&InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_ALL_STATS,                     &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_SYSTEM */
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DATA_IP_ADDRESS,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DATA_IP_ADDRESS,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DATA_IP_PORT,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DATA_IP_PORT,                  &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_COMMAND_IP_ADDRESS,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_COMMAND_IP_ADDRESS,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_COMMAND_IP_PORT,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_COMMAND_IP_PORT,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_FORCE_COMMAND_ENCRYPTION,      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_FORCE_COMMAND_ENCRYPTION,      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_FORCE_DATA_ENCRYPTION,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_FORCE_DATA_ENCRYPTION,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_FORCE_DATA_COMPRESSION,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_FORCE_DATA_COMPRESSION,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_SIZE,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_SIZE,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_PATH,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_PATH,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_RETENTION,   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_RETENTION,   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_SIZE,      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_SIZE,      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_RETENTION, &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_RETENTION, &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_COMMAND_RESEND_RETRIES,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_COMMAND_RESEND_RETRIES,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DATA_RESEND_RETRIES,           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DATA_RESENT_RETRIES,           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_SESSION_TIMEOUT,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SESSION_TIMEOUT,               &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_SESSION_KEEP_ALIVE,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SESSION_KEEP_ALIVE,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_MINIMIZE_MEMORY_USAGE,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_MINIMIZE_MEMORY_USAGE,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_SUPPORTED_PROTOCOL,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_SUPPORTED_PROTOCOL,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DB_IMMEDIATE_LOG_FLUSH,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DB_IMMEDIATE_LOG_FLUSH,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DB_CACHE_FLUSH_INTERVAL,       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DB_CACHE_FLUSH_INTERVAL,       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::SET_DB_OPERATION_MODE,             &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DB_OPERATION_MODE,             &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_ALL_SYSTEM_PARAMETERS,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_SYNC_FILES */
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_SYNC,                          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::UPDATE_SYNC,                       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_SYNC,                       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT,           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SYNC,                          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_DEVICES */
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_DEVICE,                        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::UPDATE_DEVICE,                     &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_DEVICE,                     &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT,         &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_DEVICE,                        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_SCHEDULES */
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_SCHEDULE,                      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::UPDATE_SCHEDULE,                   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_SCHEDULE,                   &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT,       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SCHEDULE,                      &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_USERS */
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_USER,                          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::UPDATE_USER,                       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::REMOVE_USER,                       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT,           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_USER,                          &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_LOGS */
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_LOG,                           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::ADD_LOG_ASYNC,                     &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT,            &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_LOG,                           &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});

        /* FUNCTIONS_SESSIONS */
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT,        &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
        instructionHandlers.insert({DatabaseManagerInstructionType::GET_SESSION,                       &InstructionManagement_Sets::InstructionSet<DatabaseManagerInstructionType>::instructionNotSet});
    };
}
