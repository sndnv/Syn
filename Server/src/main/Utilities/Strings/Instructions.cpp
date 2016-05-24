/**
 * Copyright (C) 2015 https://github.com/sndnv
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

#include "Instructions.h"

using Maps = Utilities::Strings::InstructionsMaps;

const boost::unordered_map<InstructionSetType, std::string> Maps::instructionSetTypeToString
{
    {InstructionSetType::CONNECTION_MANAGER,                    "CONNECTION_MANAGER"},
    {InstructionSetType::NETWORK_MANAGER_ADMIN,                 "NETWORK_MANAGER_ADMIN"},
    {InstructionSetType::NETWORK_MANAGER_USER,                  "NETWORK_MANAGER_USER"},
    {InstructionSetType::NETWORK_MANAGER_STATE,                 "NETWORK_MANAGER_STATE"},
    {InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE, "NETWORK_MANAGER_CONNECTION_LIFE_CYCLE"},
    {InstructionSetType::NETWORK_MANAGER_CONNECTION_BRIDGING,   "NETWORK_MANAGER_CONNECTION_BRIDGING"},
    {InstructionSetType::DATABASE_MANAGER,                      "DATABASE_MANAGER"},
    {InstructionSetType::DAL,                                   "DAL"},
    {InstructionSetType::DAL_CACHE,                             "DAL_CACHE"},
    {InstructionSetType::DAL_QUEUE,                             "DAL_QUEUE"},
    {InstructionSetType::DAL_MIGRATOR,                          "DAL_MIGRATOR"},
    {InstructionSetType::DAL_DISTRIBUTED_CACHE,                 "DAL_DISTRIBUTED_CACHE"},
    {InstructionSetType::STORAGE_MANAGER,                       "STORAGE_MANAGER"},
    {InstructionSetType::SESSION_MANAGER,                       "SESSION_MANAGER"},
    {InstructionSetType::FILE_LOGGER,                           "FILE_LOGGER"},
    {InstructionSetType::THREAD_POOL,                           "THREAD_POOL"},
    {InstructionSetType::USER_MANAGER_ADMIN,                    "USER_MANAGER_ADMIN"},
    {InstructionSetType::USER_MANAGER_SELF,                     "USER_MANAGER_SELF"},
    {InstructionSetType::DEVICE_MANAGER_ADMIN,                  "DEVICE_MANAGER_ADMIN"},
    {InstructionSetType::DEVICE_MANAGER_USER,                   "DEVICE_MANAGER_USER"},
    {InstructionSetType::DATABASE_LOGGER,                       "DATABASE_LOGGER"},
    {InstructionSetType::INVALID,                               "INVALID"}
};

const boost::unordered_map<std::string, InstructionSetType> Maps::stringToInstructionSetType
{
    {"CONNECTION_MANAGER",                      InstructionSetType::CONNECTION_MANAGER},
    {"NETWORK_MANAGER_ADMIN",                   InstructionSetType::NETWORK_MANAGER_ADMIN},
    {"NETWORK_MANAGER_USER",                    InstructionSetType::NETWORK_MANAGER_USER},
    {"NETWORK_MANAGER_STATE",                   InstructionSetType::NETWORK_MANAGER_STATE},
    {"NETWORK_MANAGER_CONNECTION_LIFE_CYCLE",   InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE},
    {"NETWORK_MANAGER_CONNECTION_BRIDGING",     InstructionSetType::NETWORK_MANAGER_CONNECTION_BRIDGING},
    {"DATABASE_MANAGER",                        InstructionSetType::DATABASE_MANAGER},
    {"DAL",                                     InstructionSetType::DAL},
    {"DAL_CACHE",                               InstructionSetType::DAL_CACHE},
    {"DAL_QUEUE",                               InstructionSetType::DAL_QUEUE},
    {"DAL_MIGRATOR",                            InstructionSetType::DAL_MIGRATOR},
    {"DAL_DISTRIBUTED_CACHE",                   InstructionSetType::DAL_DISTRIBUTED_CACHE},
    {"STORAGE_MANAGER",                         InstructionSetType::STORAGE_MANAGER},
    {"SESSION_MANAGER",                         InstructionSetType::SESSION_MANAGER},
    {"FILE_LOGGER",                             InstructionSetType::FILE_LOGGER},
    {"THREAD_POOL",                             InstructionSetType::THREAD_POOL},
    {"USER_MANAGER_ADMIN",                      InstructionSetType::USER_MANAGER_ADMIN},
    {"USER_MANAGER_SELF",                       InstructionSetType::USER_MANAGER_SELF},
    {"DEVICE_MANAGER_ADMIN",                    InstructionSetType::DEVICE_MANAGER_ADMIN},
    {"DEVICE_MANAGER_USER",                     InstructionSetType::DEVICE_MANAGER_USER},
    {"DATABASE_LOGGER",                         InstructionSetType::DATABASE_LOGGER},
    {"INVALID",                                 InstructionSetType::INVALID}
};

const boost::unordered_map<TestInstructionType, std::string> Maps::testInstructionTypeToString
{
    {TestInstructionType::DO_TEST_1,    "DO_TEST_1"},
    {TestInstructionType::DO_TEST_2,    "DO_TEST_2"},
    {TestInstructionType::DO_TEST_3,    "DO_TEST_3"},
    {TestInstructionType::INVALID,      "INVALID"}
};

const boost::unordered_map<std::string, TestInstructionType> Maps::stringToTestInstructionType
{
    {"DO_TEST_1",   TestInstructionType::DO_TEST_1},
    {"DO_TEST_2",   TestInstructionType::DO_TEST_2},
    {"DO_TEST_3",   TestInstructionType::DO_TEST_3},
    {"INVALID",     TestInstructionType::INVALID}
};

const boost::unordered_map<DatabaseManagerInstructionType, std::string> Maps::databaseManagerInstructionTypeToString
{
    /* CORE */
    {DatabaseManagerInstructionType::GET_QUEUES_LIST,           "DMIT_GET_QUEUES_LIST"},
    {DatabaseManagerInstructionType::GET_CACHES_LIST,           "DMIT_GET_CACHES_LIST"},
    {DatabaseManagerInstructionType::GET_DALS_LIST,             "DMIT_GET_DALS_LIST"},
    {DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS,  "DMIT_SET_DEFAULT_CACHE_PARAMS"},
    {DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS,  "DMIT_GET_DEFAULT_CACHE_PARAMS"},
    {DatabaseManagerInstructionType::SET_CACHE_PARAMS,          "DMIT_SET_CACHE_PARAMS"},
    {DatabaseManagerInstructionType::GET_CACHE_PARAMS,          "DMIT_GET_CACHE_PARAMS"},
    {DatabaseManagerInstructionType::SET_QUEUE_PARAMS,          "DMIT_SET_QUEUE_PARAMS"},
    {DatabaseManagerInstructionType::GET_QUEUE_PARAMS,          "DMIT_GET_QUEUE_PARAMS"},
    {DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT,      "DMIT_SET_FUNCTION_TIMEOUT"},
    {DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT,      "DMIT_GET_FUNCTION_TIMEOUT"},
    {DatabaseManagerInstructionType::ADD_DAL,                   "DMIT_ADD_DAL"},
    {DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS, "DMIT_ADD_DAL_WITH_CACHE_PARAMS"},
    {DatabaseManagerInstructionType::REMOVE_DAL,                "DMIT_REMOVE_DAL"},

    /* FUNCTIONS_STATISTICS */
    {DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP,          "DMIT_GET_SYSTEM_INSTALL_TIMESTAMP"},
    {DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP,            "DMIT_GET_SYSTEM_START_TIMESTAMP"},
    {DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA,            "DMIT_GET_TOTAL_TRANSFERRED_DATA"},
    {DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES,    "DMIT_GET_TOTAL_NUMBER_TRANSFERRED_FILES"},
    {DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS,     "DMIT_GET_TOTAL_NUMBER_FAILED_TRANSFERS"},
    {DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS,    "DMIT_GET_TOTAL_NUMBER_RETRIED_TRANSFERS"},
    {DatabaseManagerInstructionType::GET_ALL_STATS,                         "DMIT_GET_ALL_STATS"},
    
    /* FUNCTIONS_SYSTEM */
    {DatabaseManagerInstructionType::SET_DATA_IP_ADDRESS,               "DMIT_SET_DATA_IP_ADDRESS"},
    {DatabaseManagerInstructionType::GET_DATA_IP_ADDRESS,               "DMIT_GET_DATA_IP_ADDRESS"},
    {DatabaseManagerInstructionType::SET_DATA_IP_PORT,                  "DMIT_SET_DATA_IP_PORT"},
    {DatabaseManagerInstructionType::GET_DATA_IP_PORT,                  "DMIT_GET_DATA_IP_PORT"},
    {DatabaseManagerInstructionType::SET_COMMAND_IP_ADDRESS,            "DMIT_SET_COMMAND_IP_ADDRESS"},
    {DatabaseManagerInstructionType::GET_COMMAND_IP_ADDRESS,            "DMIT_GET_COMMAND_IP_ADDRESS"},
    {DatabaseManagerInstructionType::SET_COMMAND_IP_PORT,               "DMIT_SET_COMMAND_IP_PORT"},
    {DatabaseManagerInstructionType::GET_COMMAND_IP_PORT,               "DMIT_GET_COMMAND_IP_PORT"},
    {DatabaseManagerInstructionType::SET_FORCE_COMMAND_ENCRYPTION,      "DMIT_SET_FORCE_COMMAND_ENCRYPTION"},
    {DatabaseManagerInstructionType::GET_FORCE_COMMAND_ENCRYPTION,      "DMIT_GET_FORCE_COMMAND_ENCRYPTION"},
    {DatabaseManagerInstructionType::SET_FORCE_DATA_ENCRYPTION,         "DMIT_SET_FORCE_DATA_ENCRYPTION"},
    {DatabaseManagerInstructionType::GET_FORCE_DATA_ENCRYPTION,         "DMIT_GET_FORCE_DATA_ENCRYPTION"},
    {DatabaseManagerInstructionType::SET_FORCE_DATA_COMPRESSION,        "DMIT_SET_FORCE_DATA_COMPRESSION"},
    {DatabaseManagerInstructionType::GET_FORCE_DATA_COMPRESSION,        "DMIT_GET_FORCE_DATA_COMPRESSION"},
    {DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_SIZE,        "DMIT_SET_PENDING_DATA_POOL_SIZE"},
    {DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_SIZE,        "DMIT_GET_PENDING_DATA_POOL_SIZE"},
    {DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_PATH,        "DMIT_SET_PENDING_DATA_POOL_PATH"},
    {DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_PATH,        "DMIT_GET_PENDING_DATA_POOL_PATH"},
    {DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_RETENTION,   "DMIT_SET_PENDING_DATA_POOL_RETENTION"},
    {DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_RETENTION,   "DMIT_GET_PENDING_DATA_POOL_RETENTION"},
    {DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_SIZE,      "DMIT_SET_IN_MEMORY_DATA_POOL_SIZE"},
    {DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_SIZE,      "DMIT_GET_IN_MEMORY_DATA_POOL_SIZE"},
    {DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_RETENTION, "DMIT_SET_IN_MEMORY_DATA_POOL_RETENTION"},
    {DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_RETENTION, "DMIT_GET_IN_MEMORY_DATA_POOL_RETENTION"},
    {DatabaseManagerInstructionType::SET_COMMAND_RESEND_RETRIES,        "DMIT_SET_COMMAND_RESEND_RETRIES"},
    {DatabaseManagerInstructionType::GET_COMMAND_RESEND_RETRIES,        "DMIT_GET_COMMAND_RESEND_RETRIES"},
    {DatabaseManagerInstructionType::SET_DATA_RESEND_RETRIES,           "DMIT_SET_DATA_RESEND_RETRIES"},
    {DatabaseManagerInstructionType::GET_DATA_RESENT_RETRIES,           "DMIT_GET_DATA_RESENT_RETRIES"},
    {DatabaseManagerInstructionType::SET_SESSION_TIMEOUT,               "DMIT_SET_SESSION_TIMEOUT"},
    {DatabaseManagerInstructionType::GET_SESSION_TIMEOUT,               "DMIT_GET_SESSION_TIMEOUT"},
    {DatabaseManagerInstructionType::SET_SESSION_KEEP_ALIVE,            "DMIT_SET_SESSION_KEEP_ALIVE"},
    {DatabaseManagerInstructionType::GET_SESSION_KEEP_ALIVE,            "DMIT_GET_SESSION_KEEP_ALIVE"},
    {DatabaseManagerInstructionType::SET_MINIMIZE_MEMORY_USAGE,         "DMIT_SET_MINIMIZE_MEMORY_USAGE"},
    {DatabaseManagerInstructionType::GET_MINIMIZE_MEMORY_USAGE,         "DMIT_GET_MINIMIZE_MEMORY_USAGE"},
    {DatabaseManagerInstructionType::ADD_SUPPORTED_PROTOCOL,            "DMIT_ADD_SUPPORTED_PROTOCOL"},
    {DatabaseManagerInstructionType::REMOVE_SUPPORTED_PROTOCOL,         "DMIT_REMOVE_SUPPORTED_PROTOCOL"},
    {DatabaseManagerInstructionType::SET_DB_IMMEDIATE_LOG_FLUSH,        "DMIT_SET_DB_IMMEDIATE_LOG_FLUSH"},
    {DatabaseManagerInstructionType::GET_DB_IMMEDIATE_LOG_FLUSH,        "DMIT_GET_DB_IMMEDIATE_LOG_FLUSH"},
    {DatabaseManagerInstructionType::SET_DB_CACHE_FLUSH_INTERVAL,       "DMIT_SET_DB_CACHE_FLUSH_INTERVAL"},
    {DatabaseManagerInstructionType::GET_DB_CACHE_FLUSH_INTERVAL,       "DMIT_GET_DB_CACHE_FLUSH_INTERVAL"},
    {DatabaseManagerInstructionType::SET_DB_OPERATION_MODE,             "DMIT_SET_DB_OPERATION_MODE"},
    {DatabaseManagerInstructionType::GET_DB_OPERATION_MODE,             "DMIT_GET_DB_OPERATION_MODE"},
    {DatabaseManagerInstructionType::GET_ALL_SYSTEM_PARAMETERS,         "DMIT_GET_ALL_SYSTEM_PARAMETERS"},

    /* FUNCTIONS_SYNC_FILES */
    {DatabaseManagerInstructionType::ADD_SYNC,                  "DMIT_ADD_SYNC"},
    {DatabaseManagerInstructionType::UPDATE_SYNC,               "DMIT_UPDATE_SYNC"},
    {DatabaseManagerInstructionType::REMOVE_SYNC,               "DMIT_REMOVE_SYNC"},
    {DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT,   "DMIT_GET_SYNCS_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_SYNC,                  "DMIT_GET_SYNC"},

    /* FUNCTIONS_DEVICES */
    {DatabaseManagerInstructionType::ADD_DEVICE,                "DMIT_ADD_DEVICE"},
    {DatabaseManagerInstructionType::UPDATE_DEVICE,             "DMIT_UPDATE_DEVICE"},
    {DatabaseManagerInstructionType::REMOVE_DEVICE,             "DMIT_REMOVE_DEVICE"},
    {DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT, "DMIT_GET_DEVICES_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_DEVICE,                "DMIT_GET_DEVICE"},

    /* FUNCTIONS_SCHEDULES */
    {DatabaseManagerInstructionType::ADD_SCHEDULE,                  "DMIT_ADD_SCHEDULE"},
    {DatabaseManagerInstructionType::UPDATE_SCHEDULE,               "DMIT_UPDATE_SCHEDULE"},
    {DatabaseManagerInstructionType::REMOVE_SCHEDULE,               "DMIT_REMOVE_SCHEDULE"},
    {DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT,   "DMIT_GET_SCHEDULES_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_SCHEDULE,                  "DMIT_GET_SCHEDULE"},

    /* FUNCTIONS_USERS */
    {DatabaseManagerInstructionType::ADD_USER,                  "DMIT_ADD_USER"},
    {DatabaseManagerInstructionType::UPDATE_USER,               "DMIT_UPDATE_USER"},
    {DatabaseManagerInstructionType::REMOVE_USER,               "DMIT_REMOVE_USER"},
    {DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT,   "DMIT_GET_USERS_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_USER,                  "DMIT_GET_USER"},

    /* FUNCTIONS_LOGS */
    {DatabaseManagerInstructionType::ADD_LOG,                   "DMIT_ADD_LOG"},
    {DatabaseManagerInstructionType::ADD_LOG_ASYNC,             "DMIT_ADD_LOG_ASYNC"},
    {DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT,    "DMIT_GET_LOGS_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_LOG,                   "DMIT_GET_LOG"},
            
    /* FUNCTIONS_SESSIONS */
    {DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT,    "DMIT_GET_SESSIONS_BY_CONSTRAINT"},
    {DatabaseManagerInstructionType::GET_SESSION,                   "DMIT_GET_SESSION"},

    {DatabaseManagerInstructionType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, DatabaseManagerInstructionType> Maps::stringToDatabaseManagerInstructionType
{
    /* CORE */
    {"DMIT_GET_QUEUES_LIST",             DatabaseManagerInstructionType::GET_QUEUES_LIST},
    {"DMIT_GET_CACHES_LIST",             DatabaseManagerInstructionType::GET_CACHES_LIST},
    {"DMIT_GET_DALS_LIST",               DatabaseManagerInstructionType::GET_DALS_LIST},
    {"DMIT_SET_DEFAULT_CACHE_PARAMS",    DatabaseManagerInstructionType::SET_DEFAULT_CACHE_PARAMS},
    {"DMIT_GET_DEFAULT_CACHE_PARAMS",    DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS},
    {"DMIT_SET_CACHE_PARAMS",            DatabaseManagerInstructionType::SET_CACHE_PARAMS},
    {"DMIT_GET_CACHE_PARAMS",            DatabaseManagerInstructionType::GET_CACHE_PARAMS},
    {"DMIT_SET_QUEUE_PARAMS",            DatabaseManagerInstructionType::SET_QUEUE_PARAMS},
    {"DMIT_GET_QUEUE_PARAMS",            DatabaseManagerInstructionType::GET_QUEUE_PARAMS},
    {"DMIT_SET_FUNCTION_TIMEOUT",        DatabaseManagerInstructionType::SET_FUNCTION_TIMEOUT},
    {"DMIT_GET_FUNCTION_TIMEOUT",        DatabaseManagerInstructionType::GET_FUNCTION_TIMEOUT},
    {"DMIT_ADD_DAL",                     DatabaseManagerInstructionType::ADD_DAL},
    {"DMIT_ADD_DAL_WITH_CACHE_PARAMS",   DatabaseManagerInstructionType::ADD_DAL_WITH_CACHE_PARAMS},
    {"DMIT_REMOVE_DAL",                  DatabaseManagerInstructionType::REMOVE_DAL},

    /* FUNCTIONS_STATISTICS */
    {"DMIT_GET_SYSTEM_INSTALL_TIMESTAMP",        DatabaseManagerInstructionType::GET_SYSTEM_INSTALL_TIMESTAMP},
    {"DMIT_GET_SYSTEM_START_TIMESTAMP",          DatabaseManagerInstructionType::GET_SYSTEM_START_TIMESTAMP},
    {"DMIT_GET_TOTAL_TRANSFERRED_DATA",          DatabaseManagerInstructionType::GET_TOTAL_TRANSFERRED_DATA},
    {"DMIT_GET_TOTAL_NUMBER_TRANSFERRED_FILES",  DatabaseManagerInstructionType::GET_TOTAL_NUMBER_TRANSFERRED_FILES},
    {"DMIT_GET_TOTAL_NUMBER_FAILED_TRANSFERS",   DatabaseManagerInstructionType::GET_TOTAL_NUMBER_FAILED_TRANSFERS},
    {"DMIT_GET_TOTAL_NUMBER_RETRIED_TRANSFERS",  DatabaseManagerInstructionType::GET_TOTAL_NUMBER_RETRIED_TRANSFERS},
    {"DMIT_GET_ALL_STATS",                       DatabaseManagerInstructionType::GET_ALL_STATS},
    
    /* FUNCTIONS_SYSTEM */
    {"DMIT_SET_DATA_IP_ADDRESS",                 DatabaseManagerInstructionType::SET_DATA_IP_ADDRESS},
    {"DMIT_GET_DATA_IP_ADDRESS",                 DatabaseManagerInstructionType::GET_DATA_IP_ADDRESS},
    {"DMIT_SET_DATA_IP_PORT",                    DatabaseManagerInstructionType::SET_DATA_IP_PORT},
    {"DMIT_GET_DATA_IP_PORT",                    DatabaseManagerInstructionType::GET_DATA_IP_PORT},
    {"DMIT_SET_COMMAND_IP_ADDRESS",              DatabaseManagerInstructionType::SET_COMMAND_IP_ADDRESS},
    {"DMIT_GET_COMMAND_IP_ADDRESS",              DatabaseManagerInstructionType::GET_COMMAND_IP_ADDRESS},
    {"DMIT_SET_COMMAND_IP_PORT",                 DatabaseManagerInstructionType::SET_COMMAND_IP_PORT},
    {"DMIT_GET_COMMAND_IP_PORT",                 DatabaseManagerInstructionType::GET_COMMAND_IP_PORT},
    {"DMIT_SET_FORCE_COMMAND_ENCRYPTION",        DatabaseManagerInstructionType::SET_FORCE_COMMAND_ENCRYPTION},
    {"DMIT_GET_FORCE_COMMAND_ENCRYPTION",        DatabaseManagerInstructionType::GET_FORCE_COMMAND_ENCRYPTION},
    {"DMIT_SET_FORCE_DATA_ENCRYPTION",           DatabaseManagerInstructionType::SET_FORCE_DATA_ENCRYPTION},
    {"DMIT_GET_FORCE_DATA_ENCRYPTION",           DatabaseManagerInstructionType::GET_FORCE_DATA_ENCRYPTION},
    {"DMIT_SET_FORCE_DATA_COMPRESSION",          DatabaseManagerInstructionType::SET_FORCE_DATA_COMPRESSION},
    {"DMIT_GET_FORCE_DATA_COMPRESSION",          DatabaseManagerInstructionType::GET_FORCE_DATA_COMPRESSION},
    {"DMIT_SET_PENDING_DATA_POOL_SIZE",          DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_SIZE},
    {"DMIT_GET_PENDING_DATA_POOL_SIZE",          DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_SIZE},
    {"DMIT_SET_PENDING_DATA_POOL_PATH",          DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_PATH},
    {"DMIT_GET_PENDING_DATA_POOL_PATH",          DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_PATH},
    {"DMIT_SET_PENDING_DATA_POOL_RETENTION",     DatabaseManagerInstructionType::SET_PENDING_DATA_POOL_RETENTION},
    {"DMIT_GET_PENDING_DATA_POOL_RETENTION",     DatabaseManagerInstructionType::GET_PENDING_DATA_POOL_RETENTION},
    {"DMIT_SET_IN_MEMORY_DATA_POOL_SIZE",        DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_SIZE},
    {"DMIT_GET_IN_MEMORY_DATA_POOL_SIZE",        DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_SIZE},
    {"DMIT_SET_IN_MEMORY_DATA_POOL_RETENTION",   DatabaseManagerInstructionType::SET_IN_MEMORY_DATA_POOL_RETENTION},
    {"DMIT_GET_IN_MEMORY_DATA_POOL_RETENTION",   DatabaseManagerInstructionType::GET_IN_MEMORY_DATA_POOL_RETENTION},
    {"DMIT_SET_COMMAND_RESEND_RETRIES",          DatabaseManagerInstructionType::SET_COMMAND_RESEND_RETRIES},
    {"DMIT_GET_COMMAND_RESEND_RETRIES",          DatabaseManagerInstructionType::GET_COMMAND_RESEND_RETRIES},
    {"DMIT_SET_DATA_RESEND_RETRIES",             DatabaseManagerInstructionType::SET_DATA_RESEND_RETRIES},
    {"DMIT_GET_DATA_RESENT_RETRIES",             DatabaseManagerInstructionType::GET_DATA_RESENT_RETRIES},
    {"DMIT_SET_SESSION_TIMEOUT",                 DatabaseManagerInstructionType::SET_SESSION_TIMEOUT},
    {"DMIT_GET_SESSION_TIMEOUT",                 DatabaseManagerInstructionType::GET_SESSION_TIMEOUT},
    {"DMIT_SET_SESSION_KEEP_ALIVE",              DatabaseManagerInstructionType::SET_SESSION_KEEP_ALIVE},
    {"DMIT_GET_SESSION_KEEP_ALIVE",              DatabaseManagerInstructionType::GET_SESSION_KEEP_ALIVE},
    {"DMIT_SET_MINIMIZE_MEMORY_USAGE",           DatabaseManagerInstructionType::SET_MINIMIZE_MEMORY_USAGE},
    {"DMIT_GET_MINIMIZE_MEMORY_USAGE",           DatabaseManagerInstructionType::GET_MINIMIZE_MEMORY_USAGE},
    {"DMIT_ADD_SUPPORTED_PROTOCOL",              DatabaseManagerInstructionType::ADD_SUPPORTED_PROTOCOL},
    {"DMIT_REMOVE_SUPPORTED_PROTOCOL",           DatabaseManagerInstructionType::REMOVE_SUPPORTED_PROTOCOL},
    {"DMIT_SET_DB_IMMEDIATE_LOG_FLUSH",          DatabaseManagerInstructionType::SET_DB_IMMEDIATE_LOG_FLUSH},
    {"DMIT_GET_DB_IMMEDIATE_LOG_FLUSH",          DatabaseManagerInstructionType::GET_DB_IMMEDIATE_LOG_FLUSH},
    {"DMIT_SET_DB_CACHE_FLUSH_INTERVAL",         DatabaseManagerInstructionType::SET_DB_CACHE_FLUSH_INTERVAL},
    {"DMIT_GET_DB_CACHE_FLUSH_INTERVAL",         DatabaseManagerInstructionType::GET_DB_CACHE_FLUSH_INTERVAL},
    {"DMIT_SET_DB_OPERATION_MODE",               DatabaseManagerInstructionType::SET_DB_OPERATION_MODE},
    {"DMIT_GET_DB_OPERATION_MODE",               DatabaseManagerInstructionType::GET_DB_OPERATION_MODE},
    {"DMIT_GET_ALL_SYSTEM_PARAMETERS",           DatabaseManagerInstructionType::GET_ALL_SYSTEM_PARAMETERS},

    /* FUNCTIONS_SYNC_FILES */
    {"DMIT_ADD_SYNC",                DatabaseManagerInstructionType::ADD_SYNC},
    {"DMIT_UPDATE_SYNC",             DatabaseManagerInstructionType::UPDATE_SYNC},
    {"DMIT_REMOVE_SYNC",             DatabaseManagerInstructionType::REMOVE_SYNC},
    {"DMIT_GET_SYNCS_BY_CONSTRAINT", DatabaseManagerInstructionType::GET_SYNCS_BY_CONSTRAINT},
    {"DMIT_GET_SYNC",                DatabaseManagerInstructionType::GET_SYNC},

    /* FUNCTIONS_DEVICES */
    {"DMIT_ADD_DEVICE",                  DatabaseManagerInstructionType::ADD_DEVICE},
    {"DMIT_UPDATE_DEVICE",               DatabaseManagerInstructionType::UPDATE_DEVICE},
    {"DMIT_REMOVE_DEVICE",               DatabaseManagerInstructionType::REMOVE_DEVICE},
    {"DMIT_GET_DEVICES_BY_CONSTRAINT",   DatabaseManagerInstructionType::GET_DEVICES_BY_CONSTRAINT},
    {"DMIT_GET_DEVICE",                  DatabaseManagerInstructionType::GET_DEVICE},

    /* FUNCTIONS_SCHEDULES */
    {"DMIT_ADD_SCHEDULE",                DatabaseManagerInstructionType::ADD_SCHEDULE},
    {"DMIT_UPDATE_SCHEDULE",             DatabaseManagerInstructionType::UPDATE_SCHEDULE},
    {"DMIT_REMOVE_SCHEDULE",             DatabaseManagerInstructionType::REMOVE_SCHEDULE},
    {"DMIT_GET_SCHEDULES_BY_CONSTRAINT", DatabaseManagerInstructionType::GET_SCHEDULES_BY_CONSTRAINT},
    {"DMIT_GET_SCHEDULE",                DatabaseManagerInstructionType::GET_SCHEDULE},

    /* FUNCTIONS_USERS */
    {"DMIT_ADD_USER",                DatabaseManagerInstructionType::ADD_USER},
    {"DMIT_UPDATE_USER",             DatabaseManagerInstructionType::UPDATE_USER},
    {"DMIT_REMOVE_USER",             DatabaseManagerInstructionType::REMOVE_USER},
    {"DMIT_GET_USERS_BY_CONSTRAINT", DatabaseManagerInstructionType::GET_USERS_BY_CONSTRAINT},
    {"DMIT_GET_USER",                DatabaseManagerInstructionType::GET_USER},

    /* FUNCTIONS_LOGS */
    {"DMIT_ADD_LOG",                 DatabaseManagerInstructionType::ADD_LOG},
    {"DMIT_ADD_LOG_ASYNC",           DatabaseManagerInstructionType::ADD_LOG_ASYNC},
    {"DMIT_GET_LOGS_BY_CONSTRAINT",  DatabaseManagerInstructionType::GET_LOGS_BY_CONSTRAINT},
    {"DMIT_GET_LOG",                 DatabaseManagerInstructionType::GET_LOG},
            
    /* FUNCTIONS_SESSIONS */
    {"DMIT_GET_SESSIONS_BY_CONSTRAINT",  DatabaseManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT},
    {"DMIT_GET_SESSION",                 DatabaseManagerInstructionType::GET_SESSION},

    {"INVALID", DatabaseManagerInstructionType::INVALID}
};

const boost::unordered_map<SessionManagerInstructionType, std::string> Maps::sessionManagerInstructionTypeToString
{
    {SessionManagerInstructionType::GET_SESSION,                    "SMIT_GET_SESSION"},
    {SessionManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT,     "SMIT_GET_SESSIONS_BY_CONSTRAINT"},
    {SessionManagerInstructionType::FORCE_SESSION_EXPIRATION,       "SMIT_FORCE_SESSION_EXPIRATION"},
    {SessionManagerInstructionType::FORCE_SESSION_REAUTHENTICATION, "SMIT_FORCE_SESSION_REAUTHENTICATION"},
    {SessionManagerInstructionType::FORCE_EXPIRATION_PROCESS,       "SMIT_FORCE_EXPIRATION_PROCESS"},
    {SessionManagerInstructionType::DEBUG_GET_STATE,                "SMIT_DEBUG_GET_STATE"},
    {SessionManagerInstructionType::INVALID,                        "INVALID"}
};

const boost::unordered_map<std::string, SessionManagerInstructionType> Maps::stringToSessionManagerInstructionType
{
    {"SMIT_GET_SESSION",                     SessionManagerInstructionType::GET_SESSION},
    {"SMIT_GET_SESSIONS_BY_CONSTRAINT",      SessionManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT},
    {"SMIT_FORCE_SESSION_EXPIRATION",        SessionManagerInstructionType::FORCE_SESSION_EXPIRATION},
    {"SMIT_FORCE_SESSION_REAUTHENTICATION",  SessionManagerInstructionType::FORCE_SESSION_REAUTHENTICATION},
    {"SMIT_FORCE_EXPIRATION_PROCESS",        SessionManagerInstructionType::FORCE_EXPIRATION_PROCESS},
    {"SMIT_DEBUG_GET_STATE",                 SessionManagerInstructionType::DEBUG_GET_STATE},
    {"INVALID",                              SessionManagerInstructionType::INVALID}
};

const boost::unordered_map<UserManagerAdminInstructionType, std::string> Maps::userManagerAdminInstructionTypeToString
{
    {UserManagerAdminInstructionType::GET_USER,                             "UMAIT_GET_USER"},
    {UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT,              "UMAIT_GET_USERS_BY_CONSTRAINT"},
    {UserManagerAdminInstructionType::ADD_USER,                             "UMAIT_ADD_USER"},
    {UserManagerAdminInstructionType::REMOVE_USER,                          "UMAIT_REMOVE_USER"},
    {UserManagerAdminInstructionType::RESET_PASSWORD,                       "UMAIT_RESET_PASSWORD"},
    {UserManagerAdminInstructionType::FORCE_PASSWORD_RESET,                 "UMAIT_FORCE_PASSWORD_RESET"},
    {UserManagerAdminInstructionType::LOCK_USER,                            "UMAIT_LOCK_USER"},
    {UserManagerAdminInstructionType::UNLOCK_USER,                          "UMAIT_UNLOCK_USER"},
    {UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL,                  "UMAIT_UPDATE_ACCESS_LEVEL"},
    {UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS, "UMAIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS"},
    {UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE,               "UMAIT_ADD_AUTHORIZATION_RULE"},
    {UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE,            "UMAIT_REMOVE_AUTHORIZATION_RULE"},
    {UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES,            "UMAIT_CLEAR_AUTHORIZATION_RULES"},
    {UserManagerAdminInstructionType::DEBUG_GET_STATE,                      "UMAIT_DEBUG_GET_STATE"},
    {UserManagerAdminInstructionType::INVALID,                              "INVALID"}
};

const boost::unordered_map<std::string, UserManagerAdminInstructionType> Maps::stringToUserManagerAdminInstructionType
{
    {"UMAIT_GET_USER",                              UserManagerAdminInstructionType::GET_USER},
    {"UMAIT_GET_USERS_BY_CONSTRAINT",               UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT},
    {"UMAIT_ADD_USER",                              UserManagerAdminInstructionType::ADD_USER},
    {"UMAIT_REMOVE_USER",                           UserManagerAdminInstructionType::REMOVE_USER},
    {"UMAIT_RESET_PASSWORD",                        UserManagerAdminInstructionType::RESET_PASSWORD},
    {"UMAIT_FORCE_PASSWORD_RESET",                  UserManagerAdminInstructionType::FORCE_PASSWORD_RESET},
    {"UMAIT_LOCK_USER",                             UserManagerAdminInstructionType::LOCK_USER},
    {"UMAIT_UNLOCK_USER",                           UserManagerAdminInstructionType::UNLOCK_USER},
    {"UMAIT_UPDATE_ACCESS_LEVEL",                   UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL},
    {"UMAIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS",  UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS},
    {"UMAIT_ADD_AUTHORIZATION_RULE",                UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE},
    {"UMAIT_REMOVE_AUTHORIZATION_RULE",             UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE},
    {"UMAIT_CLEAR_AUTHORIZATION_RULES",             UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES},
    {"UMAIT_DEBUG_GET_STATE",                       UserManagerAdminInstructionType::DEBUG_GET_STATE},
    {"INVALID",                                     UserManagerAdminInstructionType::INVALID}
};

const boost::unordered_map<UserManagerSelfInstructionType, std::string> Maps::userManagerSelfInstructionTypeToString
{
    {UserManagerSelfInstructionType::GET_USER,          "UMSIT_GET_USER"},
    {UserManagerSelfInstructionType::RESET_PASSWORD,    "UMSIT_RESET_PASSWORD"},
    {UserManagerSelfInstructionType::INVALID,           "INVALID"}
};

const boost::unordered_map<std::string, UserManagerSelfInstructionType> Maps::stringToUserManagerSelfInstructionType
{
    {"UMSIT_GET_USER",        UserManagerSelfInstructionType::GET_USER},
    {"UMSIT_RESET_PASSWORD",  UserManagerSelfInstructionType::RESET_PASSWORD},
    {"INVALID",               UserManagerSelfInstructionType::INVALID},
};

const boost::unordered_map<DeviceManagerAdminInstructionType, std::string> Maps::deviceManagerAdminInstructionTypeToString
{
    {DeviceManagerAdminInstructionType::GET_DEVICE,                             "DMAIT_GET_DEVICE"},
    {DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT,              "DMAIT_GET_DEVICES_BY_CONSTRAINT"},
    {DeviceManagerAdminInstructionType::ADD_DEVICE,                             "DMAIT_ADD_DEVICE"},
    {DeviceManagerAdminInstructionType::REMOVE_DEVICE,                          "DMAIT_REMOVE_DEVICE"},
    {DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD,                  "DMAIT_RESET_DEVICE_PASSWORD"},
    {DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO,                 "DMAIT_UPDATE_CONNECTION_INFO"},
    {DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO,                    "DMAIT_UPDATE_GENERAL_INFO"},
    {DeviceManagerAdminInstructionType::LOCK_DEVICE,                            "DMAIT_LOCK_DEVICE"},
    {DeviceManagerAdminInstructionType::UNLOCK_DEVICE,                          "DMAIT_UNLOCK_DEVICE"},
    {DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,   "DMAIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS"},
    {DeviceManagerAdminInstructionType::DEBUG_GET_STATE,                        "DMAIT_DEBUG_GET_STATE"},
    {DeviceManagerAdminInstructionType::INVALID,                                "INVALID"}
};

const boost::unordered_map<std::string, DeviceManagerAdminInstructionType> Maps::stringToDeviceManagerAdminInstructionType
{
    {"DMAIT_GET_DEVICE",                              DeviceManagerAdminInstructionType::GET_DEVICE},
    {"DMAIT_GET_DEVICES_BY_CONSTRAINT",               DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT},
    {"DMAIT_ADD_DEVICE",                              DeviceManagerAdminInstructionType::ADD_DEVICE},
    {"DMAIT_REMOVE_DEVICE",                           DeviceManagerAdminInstructionType::REMOVE_DEVICE},
    {"DMAIT_RESET_DEVICE_PASSWORD",                   DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD},
    {"DMAIT_UPDATE_CONNECTION_INFO",                  DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO},
    {"DMAIT_UPDATE_GENERAL_INFO",                     DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO},
    {"DMAIT_LOCK_DEVICE",                             DeviceManagerAdminInstructionType::LOCK_DEVICE},
    {"DMAIT_UNLOCK_DEVICE",                           DeviceManagerAdminInstructionType::UNLOCK_DEVICE},
    {"DMAIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS",    DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS},
    {"DMAIT_DEBUG_GET_STATE",                         DeviceManagerAdminInstructionType::DEBUG_GET_STATE},
    {"INVALID",                                       DeviceManagerAdminInstructionType::INVALID}
};

const boost::unordered_map<DeviceManagerUserInstructionType, std::string> Maps::deviceManagerUserInstructionTypeToString
{
    {DeviceManagerUserInstructionType::GET_DEVICE,                              "DMUIT_GET_DEVICE"},
    {DeviceManagerUserInstructionType::GET_DEVICES,                             "DMUIT_GET_DEVICES"},
    {DeviceManagerUserInstructionType::ADD_DEVICE,                              "DMUIT_ADD_DEVICE"},
    {DeviceManagerUserInstructionType::REMOVE_DEVICE,                           "DMUIT_REMOVE_DEVICE"},
    {DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD,                   "DMUIT_RESET_DEVICE_PASSWORD"},
    {DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO,                  "DMUIT_UPDATE_CONNECTION_INFO"},
    {DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO,                     "DMUIT_UPDATE_GENERAL_INFO"},
    {DeviceManagerUserInstructionType::LOCK_DEVICE,                             "DMUIT_LOCK_DEVICE"},
    {DeviceManagerUserInstructionType::UNLOCK_DEVICE,                           "DMUIT_UNLOCK_DEVICE"},
    {DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,    "DMUIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS"},
    {DeviceManagerUserInstructionType::INVALID,                                 "INVALID"}
};

const boost::unordered_map<std::string, DeviceManagerUserInstructionType> Maps::stringToDeviceManagerUserInstructionType
{
    {"DMUIT_GET_DEVICE",                              DeviceManagerUserInstructionType::GET_DEVICE},
    {"DMUIT_GET_DEVICES",                             DeviceManagerUserInstructionType::GET_DEVICES},
    {"DMUIT_ADD_DEVICE",                              DeviceManagerUserInstructionType::ADD_DEVICE},
    {"DMUIT_REMOVE_DEVICE",                           DeviceManagerUserInstructionType::REMOVE_DEVICE},
    {"DMUIT_RESET_DEVICE_PASSWORD",                   DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD},
    {"DMUIT_UPDATE_CONNECTION_INFO",                  DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO},
    {"DMUIT_UPDATE_GENERAL_INFO",                     DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO},
    {"DMUIT_LOCK_DEVICE",                             DeviceManagerUserInstructionType::LOCK_DEVICE},
    {"DMUIT_UNLOCK_DEVICE",                           DeviceManagerUserInstructionType::UNLOCK_DEVICE},
    {"DMUIT_RESET_FAILED_AUTHENTICATION_ATTEMPTS",    DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS},
    {"INVALID",                                       DeviceManagerUserInstructionType::INVALID}
};

const boost::unordered_map<DatabaseLoggerInstructionType, std::string> Maps::databaseLoggerInstructionTypeToString
{
    {DatabaseLoggerInstructionType::GET_LOG,                        "DLIT_GET_LOG"},
    {DatabaseLoggerInstructionType::GET_LOGS_BY_CONSTRAINT,         "DLIT_GET_LOGS_BY_CONSTRAINT"},
    {DatabaseLoggerInstructionType::UPDATE_SOURCE_LOGGING_LEVEL,    "DLIT_UPDATE_SOURCE_LOGGING_LEVEL"},
    {DatabaseLoggerInstructionType::UPDATE_DEFAULT_LOGGING_LEVEL,   "DLIT_UPDATE_DEFAULT_LOGGING_LEVEL"},
    {DatabaseLoggerInstructionType::DEBUG_GET_STATE,                "DLIT_DEBUG_GET_STATE"},
    {DatabaseLoggerInstructionType::INVALID,                        "INVALID"}
};

const boost::unordered_map<std::string, DatabaseLoggerInstructionType> Maps::stringToDatabaseLoggerInstructionType
{
    {"DLIT_GET_LOG",                         DatabaseLoggerInstructionType::GET_LOG},
    {"DLIT_GET_LOGS_BY_CONSTRAINT",          DatabaseLoggerInstructionType::GET_LOGS_BY_CONSTRAINT},
    {"DLIT_UPDATE_SOURCE_LOGGING_LEVEL",     DatabaseLoggerInstructionType::UPDATE_SOURCE_LOGGING_LEVEL},
    {"DLIT_UPDATE_DEFAULT_LOGGING_LEVEL",    DatabaseLoggerInstructionType::UPDATE_DEFAULT_LOGGING_LEVEL},
    {"DLIT_DEBUG_GET_STATE",                 DatabaseLoggerInstructionType::DEBUG_GET_STATE},
    {"INVALID",                              DatabaseLoggerInstructionType::INVALID}
};

const boost::unordered_map<NetworkManagerConnectionLifeCycleInstructionType, std::string> Maps::networkManagerConnectionLifeCycleInstructionTypeToString
{
    {NetworkManagerConnectionLifeCycleInstructionType::KEEP_ALIVE,                  "NMCLCIT_KEEP_ALIVE"},
    {NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION,        "NMCLCIT_OPEN_DATA_CONNECTION"},
    {NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION,        "NMCLCIT_OPEN_INIT_CONNECTION"},
    {NetworkManagerConnectionLifeCycleInstructionType::REAUTHENTICATE,              "NMCLCIT_REAUTHENTICATE"},
    {NetworkManagerConnectionLifeCycleInstructionType::RELOAD_KEYS,                 "NMCLCIT_RELOAD_KEYS"},
    {NetworkManagerConnectionLifeCycleInstructionType::TERMINATE,                   "NMCLCIT_TERMINATE"},
    {NetworkManagerConnectionLifeCycleInstructionType::INVALID,                     "INVALID"}
};

const boost::unordered_map<std::string, NetworkManagerConnectionLifeCycleInstructionType> Maps::stringToNetworkManagerConnectionLifeCycleInstructionType
{
    {"NMCLCIT_KEEP_ALIVE",                  NetworkManagerConnectionLifeCycleInstructionType::KEEP_ALIVE},
    {"NMCLCIT_OPEN_DATA_CONNECTION",        NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION},
    {"NMCLCIT_OPEN_INIT_CONNECTION",        NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION},
    {"NMCLCIT_REAUTHENTICATE",              NetworkManagerConnectionLifeCycleInstructionType::REAUTHENTICATE},
    {"NMCLCIT_RELOAD_KEYS",                 NetworkManagerConnectionLifeCycleInstructionType::RELOAD_KEYS},
    {"NMCLCIT_TERMINATE",                   NetworkManagerConnectionLifeCycleInstructionType::TERMINATE},
    {"INVALID",                             NetworkManagerConnectionLifeCycleInstructionType::INVALID}
};

std::string Utilities::Strings::toString(InstructionSetType var)
{
    if(Maps::instructionSetTypeToString.find(var) != Maps::instructionSetTypeToString.end())
        return Maps::instructionSetTypeToString.at(var);
    else
        return "INVALID";
}

InstructionSetType Utilities::Strings::toInstructionSetType(std::string var)
{
    if(Maps::stringToInstructionSetType.find(var) != Maps::stringToInstructionSetType.end())
        return Maps::stringToInstructionSetType.at(var);
    else
        return InstructionSetType::INVALID;
}


std::string Utilities::Strings::toString(TestInstructionType var)
{
    if(Maps::testInstructionTypeToString.find(var) != Maps::testInstructionTypeToString.end())
        return Maps::testInstructionTypeToString.at(var);
    else
        return "INVALID";
}

TestInstructionType Utilities::Strings::toTestInstructionType(std::string var)
{
    if(Maps::stringToTestInstructionType.find(var) != Maps::stringToTestInstructionType.end())
        return Maps::stringToTestInstructionType.at(var);
    else
        return TestInstructionType::INVALID;
}

std::string Utilities::Strings::toString(DatabaseManagerInstructionType var)
{
    if(Maps::databaseManagerInstructionTypeToString.find(var) != Maps::databaseManagerInstructionTypeToString.end())
        return Maps::databaseManagerInstructionTypeToString.at(var);
    else
        return "INVALID";
}

DatabaseManagerInstructionType Utilities::Strings::toDatabaseManagerInstructionType(std::string var)
{
    if(Maps::stringToDatabaseManagerInstructionType.find(var) != Maps::stringToDatabaseManagerInstructionType.end())
        return Maps::stringToDatabaseManagerInstructionType.at(var);
    else
        return DatabaseManagerInstructionType::INVALID;
}

std::string Utilities::Strings::toString(SessionManagerInstructionType var)
{
    if(Maps::sessionManagerInstructionTypeToString.find(var) != Maps::sessionManagerInstructionTypeToString.end())
        return Maps::sessionManagerInstructionTypeToString.at(var);
    else
        return "INVALID";
}

SessionManagerInstructionType Utilities::Strings::toSessionManagerInstructionType(std::string var)
{
    if(Maps::stringToSessionManagerInstructionType.find(var) != Maps::stringToSessionManagerInstructionType.end())
        return Maps::stringToSessionManagerInstructionType.at(var);
    else
        return SessionManagerInstructionType::INVALID;
}

std::string Utilities::Strings::toString(UserManagerAdminInstructionType var)
{
    if(Maps::userManagerAdminInstructionTypeToString.find(var) != Maps::userManagerAdminInstructionTypeToString.end())
        return Maps::userManagerAdminInstructionTypeToString.at(var);
    else
        return "INVALID";
}

UserManagerAdminInstructionType Utilities::Strings::toUserManagerAdminInstructionType(std::string var)
{
    if(Maps::stringToUserManagerAdminInstructionType.find(var) != Maps::stringToUserManagerAdminInstructionType.end())
        return Maps::stringToUserManagerAdminInstructionType.at(var);
    else
        return UserManagerAdminInstructionType::INVALID;
}

std::string Utilities::Strings::toString(UserManagerSelfInstructionType var)
{
    if(Maps::userManagerSelfInstructionTypeToString.find(var) != Maps::userManagerSelfInstructionTypeToString.end())
        return Maps::userManagerSelfInstructionTypeToString.at(var);
    else
        return "INVALID";
}

UserManagerSelfInstructionType Utilities::Strings::toUserManagerSelfInstructionType(std::string var)
{
    if(Maps::stringToUserManagerSelfInstructionType.find(var) != Maps::stringToUserManagerSelfInstructionType.end())
        return Maps::stringToUserManagerSelfInstructionType.at(var);
    else
        return UserManagerSelfInstructionType::INVALID;
}

std::string Utilities::Strings::toString(DeviceManagerAdminInstructionType var)
{
    if(Maps::deviceManagerAdminInstructionTypeToString.find(var) != Maps::deviceManagerAdminInstructionTypeToString.end())
        return Maps::deviceManagerAdminInstructionTypeToString.at(var);
    else
        return "INVALID";
}

DeviceManagerAdminInstructionType Utilities::Strings::toDeviceManagerAdminInstructionType(std::string var)
{
    if(Maps::stringToDeviceManagerAdminInstructionType.find(var) != Maps::stringToDeviceManagerAdminInstructionType.end())
        return Maps::stringToDeviceManagerAdminInstructionType.at(var);
    else
        return DeviceManagerAdminInstructionType::INVALID;
}

std::string Utilities::Strings::toString(DeviceManagerUserInstructionType var)
{
    if(Maps::deviceManagerUserInstructionTypeToString.find(var) != Maps::deviceManagerUserInstructionTypeToString.end())
        return Maps::deviceManagerUserInstructionTypeToString.at(var);
    else
        return "INVALID";
}

DeviceManagerUserInstructionType Utilities::Strings::toDeviceManagerUserInstructionType(std::string var)
{
    if(Maps::stringToDeviceManagerUserInstructionType.find(var) != Maps::stringToDeviceManagerUserInstructionType.end())
        return Maps::stringToDeviceManagerUserInstructionType.at(var);
    else
        return DeviceManagerUserInstructionType::INVALID;
}

std::string Utilities::Strings::toString(DatabaseLoggerInstructionType var)
{
    if(Maps::databaseLoggerInstructionTypeToString.find(var) != Maps::databaseLoggerInstructionTypeToString.end())
        return Maps::databaseLoggerInstructionTypeToString.at(var);
    else
        return "INVALID";
}

DatabaseLoggerInstructionType Utilities::Strings::toDatabaseLoggerInstructionType(std::string var)
{
    if(Maps::stringToDatabaseLoggerInstructionType.find(var) != Maps::stringToDatabaseLoggerInstructionType.end())
        return Maps::stringToDatabaseLoggerInstructionType.at(var);
    else
        return DatabaseLoggerInstructionType::INVALID;
}

std::string Utilities::Strings::toString(NetworkManagerConnectionLifeCycleInstructionType var)
{
    if(Maps::networkManagerConnectionLifeCycleInstructionTypeToString.find(var) != Maps::networkManagerConnectionLifeCycleInstructionTypeToString.end())
        return Maps::networkManagerConnectionLifeCycleInstructionTypeToString.at(var);
    else
        return "INVALID";
}

NetworkManagerConnectionLifeCycleInstructionType Utilities::Strings::toNetworkManagerConnectionLifeCycleInstructionType(std::string var)
{
    if(Maps::stringToNetworkManagerConnectionLifeCycleInstructionType.find(var) != Maps::stringToNetworkManagerConnectionLifeCycleInstructionType.end())
        return Maps::stringToNetworkManagerConnectionLifeCycleInstructionType.at(var);
    else
        return NetworkManagerConnectionLifeCycleInstructionType::INVALID;
}
