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

#include "Tools.h"

using Utilities::Tools;

const boost::unordered_map<DatabaseObjectType, std::string> Tools::databaseObjectTypesToString
{
    {DatabaseObjectType::DEVICE,            "DEVICE"},
    {DatabaseObjectType::INVALID,           "INVALID"},
    {DatabaseObjectType::LOG,               "LOG"},
    {DatabaseObjectType::SCHEDULE,          "SCHEDULE"},
    {DatabaseObjectType::SESSION,           "SESSION"},
    {DatabaseObjectType::STATISTICS,        "STATISTICS"},
    {DatabaseObjectType::SYNC_FILE,         "SYNC_FILE"},
    {DatabaseObjectType::SYSTEM_SETTINGS,   "SYSTEM_SETTINGS"},
    {DatabaseObjectType::USER,              "USER"}
};

const boost::unordered_map<std::string, DatabaseObjectType> Tools::stringToDatabaseObjectTypes
{
    {"DEVICE",          DatabaseObjectType::DEVICE},
    {"INVALID",         DatabaseObjectType::INVALID},
    {"LOG",             DatabaseObjectType::LOG},
    {"SCHEDULE",        DatabaseObjectType::SCHEDULE},
    {"SESSION",         DatabaseObjectType::SESSION},
    {"STATISTICS",      DatabaseObjectType::STATISTICS},
    {"SYNC_FILE",       DatabaseObjectType::SYNC_FILE},
    {"SYSTEM_SETTINGS", DatabaseObjectType::SYSTEM_SETTINGS},
    {"USER",            DatabaseObjectType::USER}
};

const boost::unordered_map<DatabaseManagerOperationMode, std::string> Tools::databaseManagerOperationModeToString
{
    {DatabaseManagerOperationMode::CRCW,    "CRCW"},
    {DatabaseManagerOperationMode::INVALID, "INVALID"},
    {DatabaseManagerOperationMode::PRCW,    "PRCW"},
    {DatabaseManagerOperationMode::PRPW,    "PRPW"}
};

const boost::unordered_map<std::string, DatabaseManagerOperationMode> Tools::stringToDatabaseManagerOperationMode
{
    {"CRCW",    DatabaseManagerOperationMode::CRCW},
    {"INVALID", DatabaseManagerOperationMode::INVALID},
    {"PRCW",    DatabaseManagerOperationMode::PRCW},
    {"PRPW",    DatabaseManagerOperationMode::PRPW}
};

const boost::unordered_map<DatabaseFailureAction, std::string> Tools::databaseFailureActionToString
{
    {DatabaseFailureAction::DROP_DAL,           "DROP_DAL"},
    {DatabaseFailureAction::DROP_IF_NOT_LAST,   "DROP_IF_NOT_LAST"},
    {DatabaseFailureAction::IGNORE_FAILURE,     "IGNORE_FAILURE"},
    {DatabaseFailureAction::INITIATE_RECONNECT, "INITIATE_RECONNECT"},
    {DatabaseFailureAction::PUSH_TO_BACK,       "PUSH_TO_BACK"},
    {DatabaseFailureAction::INVALID,            "INVALID"}
};

const boost::unordered_map<std::string, DatabaseFailureAction> Tools::stringToDatabaseFailureAction
{
    {"DROP_DAL",            DatabaseFailureAction::DROP_DAL},
    {"DROP_IF_NOT_LAST",    DatabaseFailureAction::DROP_IF_NOT_LAST},
    {"IGNORE_FAILURE",      DatabaseFailureAction::IGNORE_FAILURE},
    {"INITIATE_RECONNECT",  DatabaseFailureAction::INITIATE_RECONNECT},
    {"PUSH_TO_BACK",        DatabaseFailureAction::PUSH_TO_BACK},
    {"INVALID",             DatabaseFailureAction::INVALID}
};

const boost::unordered_map<UserAccessLevel, std::string> Tools::userAccessLevelToString
{
    {UserAccessLevel::ADMIN,    "ADMIN"},
    {UserAccessLevel::NONE,     "NONE"},
    {UserAccessLevel::USER,     "USER"}
};

const boost::unordered_map<std::string, UserAccessLevel> Tools::stringToUserAccessLevel
{
    {"ADMIN",   UserAccessLevel::ADMIN},
    {"NONE",    UserAccessLevel::NONE},
    {"USER",    UserAccessLevel::USER}
};

const boost::unordered_map<StatisticType, std::string> Tools::statisticTypeToString
{
    {StatisticType::INSTALL_TIMESTAMP,          "INSTALL_TIMESTAMP"},
    {StatisticType::START_TIMESTAMP,            "START_TIMESTAMP"},
    {StatisticType::TOTAL_FAILED_TRANSFERS,     "TOTAL_FAILED_TRANSFERS"},
    {StatisticType::TOTAL_RETRIED_TRANSFERS,    "TOTAL_RETRIED_TRANSFERS"},
    {StatisticType::TOTAL_TRANSFERRED_DATA,     "TOTAL_TRANSFERRED_DATA"},
    {StatisticType::TOTAL_TRANSFERRED_FILES,    "TOTAL_TRANSFERRED_FILES"}
};

const boost::unordered_map<std::string, StatisticType> Tools::stringToStatisticType
{
    {"INSTALL_TIMESTAMP",       StatisticType::INSTALL_TIMESTAMP},
    {"START_TIMESTAMP",         StatisticType::START_TIMESTAMP},
    {"TOTAL_FAILED_TRANSFERS",  StatisticType::TOTAL_FAILED_TRANSFERS},
    {"TOTAL_RETRIED_TRANSFERS", StatisticType::TOTAL_RETRIED_TRANSFERS},
    {"TOTAL_TRANSFERRED_DATA",  StatisticType::TOTAL_TRANSFERRED_DATA},
    {"TOTAL_TRANSFERRED_FILES", StatisticType::TOTAL_TRANSFERRED_FILES}
};

const boost::unordered_map<SystemParameterType, std::string> Tools::systemParameterTypeToString
{
    {SystemParameterType::COMMAND_IP_ADDRESS,       "COMMAND_IP_ADDRESS"},
    {SystemParameterType::COMMAND_IP_PORT,          "COMMAND_IP_PORT"},
    {SystemParameterType::COMMAND_RETRIES_MAX,      "COMMAND_RETRIES_MAX"},
    {SystemParameterType::DATA_IP_ADDRESS,          "DATA_IP_ADDRESS"},
    {SystemParameterType::DATA_IP_PORT,             "DATA_IP_PORT"},
    {SystemParameterType::DATA_RETRIES_MAX,         "DATA_RETRIES_MAX"},
    {SystemParameterType::DB_CACHE_FLUSH_INTERVAL,  "DB_CACHE_FLUSH_INTERVAL"},
    {SystemParameterType::DB_IMMEDIATE_FLUSH,       "DB_IMMEDIATE_FLUSH"},
    {SystemParameterType::DB_MAX_READ_RETRIES,      "DB_MAX_READ_RETRIES"},
    {SystemParameterType::DB_MAX_WRITE_RETRIES,     "DB_MAX_WRITE_RETRIES"},
    {SystemParameterType::DB_OPERATION_MODE,        "DB_OPERATION_MODE"},
    {SystemParameterType::FORCE_COMMAND_ENCRYPTION, "FORCE_COMMAND_ENCRYPTION"},
    {SystemParameterType::FORCE_DATA_COMPRESSION,   "FORCE_DATA_COMPRESSION"},
    {SystemParameterType::FORCE_DATA_ENCRYPTION,    "FORCE_DATA_ENCRYPTION"},
    {SystemParameterType::IN_MEMORY_POOL_RETENTION, "IN_MEMORY_POOL_RETENTION"},
    {SystemParameterType::IN_MEMORY_POOL_SIZE,      "IN_MEMORY_POOL_SIZE"},
    {SystemParameterType::MINIMIZE_MEMORY_USAGE,    "MINIMIZE_MEMORY_USAGE"},
    {SystemParameterType::PENDING_DATA_POOL_PATH,   "PENDING_DATA_POOL_PATH"},
    {SystemParameterType::PENDING_DATA_POOL_SIZE,   "PENDING_DATA_POOL_SIZE"},
    {SystemParameterType::PENDING_DATA_RETENTION,   "PENDING_DATA_RETENTION"},
    {SystemParameterType::SESSION_KEEP_ALIVE,       "SESSION_KEEP_ALIVE"},
    {SystemParameterType::SESSION_TIMEOUT,          "SESSION_TIMEOUT"},
    {SystemParameterType::SUPPORTED_PROTOCOLS,      "SUPPORTED_PROTOCOLS"}
};

const boost::unordered_map<std::string, SystemParameterType> Tools::stringToSystemParameterType
{
    {"COMMAND_IP_ADDRESS",      SystemParameterType::COMMAND_IP_ADDRESS},
    {"COMMAND_IP_PORT",         SystemParameterType::COMMAND_IP_PORT},
    {"COMMAND_RETRIES_MAX",     SystemParameterType::COMMAND_RETRIES_MAX},
    {"DATA_IP_ADDRESS",         SystemParameterType::DATA_IP_ADDRESS},
    {"DATA_IP_PORT",            SystemParameterType::DATA_IP_PORT},
    {"DATA_RETRIES_MAX",        SystemParameterType::DATA_RETRIES_MAX},
    {"DB_CACHE_FLUSH_INTERVAL", SystemParameterType::DB_CACHE_FLUSH_INTERVAL},
    {"DB_IMMEDIATE_FLUSH",      SystemParameterType::DB_IMMEDIATE_FLUSH},
    {"DB_MAX_READ_RETRIES",     SystemParameterType::DB_MAX_READ_RETRIES},
    {"DB_MAX_WRITE_RETRIES",    SystemParameterType::DB_MAX_WRITE_RETRIES},
    {"DB_OPERATION_MODE",       SystemParameterType::DB_OPERATION_MODE},
    {"FORCE_COMMAND_ENCRYPTION",SystemParameterType::FORCE_COMMAND_ENCRYPTION},
    {"FORCE_DATA_COMPRESSION",  SystemParameterType::FORCE_DATA_COMPRESSION},
    {"FORCE_DATA_ENCRYPTION",   SystemParameterType::FORCE_DATA_ENCRYPTION},
    {"IN_MEMORY_POOL_RETENTION",SystemParameterType::IN_MEMORY_POOL_RETENTION},
    {"IN_MEMORY_POOL_SIZE",     SystemParameterType::IN_MEMORY_POOL_SIZE},
    {"MINIMIZE_MEMORY_USAGE",   SystemParameterType::MINIMIZE_MEMORY_USAGE},
    {"PENDING_DATA_POOL_PATH",  SystemParameterType::PENDING_DATA_POOL_PATH},
    {"PENDING_DATA_POOL_SIZE",  SystemParameterType::PENDING_DATA_POOL_SIZE},
    {"PENDING_DATA_RETENTION",  SystemParameterType::PENDING_DATA_RETENTION},
    {"SESSION_KEEP_ALIVE",      SystemParameterType::SESSION_KEEP_ALIVE},
    {"SESSION_TIMEOUT",         SystemParameterType::SESSION_TIMEOUT},
    {"SUPPORTED_PROTOCOLS",     SystemParameterType::SUPPORTED_PROTOCOLS}
};

const boost::unordered_map<LogSeverity, std::string> Tools::logSeverityToString
{
    {LogSeverity::Debug,    "DEBUG"},
    {LogSeverity::Error,    "ERROR"},
    {LogSeverity::Info,     "INFO"},
    {LogSeverity::Warning,     "WARN"}
};

const boost::unordered_map<std::string, LogSeverity> Tools::stringToLogSeverity
{
    {"DEBUG",   LogSeverity::Debug},
    {"ERROR",   LogSeverity::Error},
    {"INFO",    LogSeverity::Info},
    {"WARN",    LogSeverity::Warning}
};

const boost::unordered_map<DataTransferType, std::string> Tools::dataTransferTypeToString
{
    {DataTransferType::PULL, "PULL"},
    {DataTransferType::PUSH, "PUSH"}
};

const boost::unordered_map<std::string, DataTransferType> Tools::stringToDataTransferType
{
    {"PULL", DataTransferType::PULL},
    {"PUSH", DataTransferType::PUSH}
};

const boost::unordered_map<SessionType, std::string> Tools::sessionTypeToString
{
    {SessionType::ADMIN, "ADMIN"},
    {SessionType::COMMAND, "COMMAND"},
    {SessionType::DATA, "DATA"}
};

const boost::unordered_map<std::string, SessionType> Tools::stringToSessionType
{
    {"ADMIN", SessionType::ADMIN},
    {"COMMAND", SessionType::COMMAND},
    {"DATA", SessionType::DATA}
};

const boost::unordered_map<ScheduleIntervalType, std::string> Tools::scheduleIntervalTypeToString
{
    {ScheduleIntervalType::DAYS,    "DAYS"},
    {ScheduleIntervalType::HOURS,   "HOURS"},
    {ScheduleIntervalType::MINUTES, "MINUTES"},
    {ScheduleIntervalType::MONTHS,  "MONTHS"},
    {ScheduleIntervalType::SECONDS, "SECONDS"}
};

const boost::unordered_map<std::string, ScheduleIntervalType> Tools::stringToScheduleIntervalType
{
    {"DAYS",    ScheduleIntervalType::DAYS},
    {"HOURS",   ScheduleIntervalType::HOURS},
    {"MINUTES", ScheduleIntervalType::MINUTES},
    {"MONTHS",  ScheduleIntervalType::MONTHS},
    {"SECONDS", ScheduleIntervalType::SECONDS}
};

const boost::unordered_map<ConflictResolutionRule_Directory, std::string> Tools::dirResolutionRuleToString
{
    {ConflictResolutionRule_Directory::ASK,                     "ASK"},
    {ConflictResolutionRule_Directory::COPY_AND_RENAME,         "COPY_AND_RENAME"},
    {ConflictResolutionRule_Directory::MERGE,                   "MERGE"},
    {ConflictResolutionRule_Directory::OVERWRITE_DESTINATION,   "OVERWRITE_DESTINATION"},
    {ConflictResolutionRule_Directory::OVERWRITE_SOURCE,        "OVERWRITE_SOURCE"},
    {ConflictResolutionRule_Directory::RENAME_AND_COPY,         "RENAME_AND_COPY"},
    {ConflictResolutionRule_Directory::STOP,                    "STOP"}
};

const boost::unordered_map<std::string, ConflictResolutionRule_Directory> Tools::stringToDirResolutionRule
{
    {"ASK",                     ConflictResolutionRule_Directory::ASK},
    {"COPY_AND_RENAME",         ConflictResolutionRule_Directory::COPY_AND_RENAME},
    {"MERGE",                   ConflictResolutionRule_Directory::MERGE},
    {"OVERWRITE_DESTINATION",   ConflictResolutionRule_Directory::OVERWRITE_DESTINATION},
    {"OVERWRITE_SOURCE",        ConflictResolutionRule_Directory::OVERWRITE_SOURCE},
    {"RENAME_AND_COPY",         ConflictResolutionRule_Directory::RENAME_AND_COPY},
    {"STOP",                    ConflictResolutionRule_Directory::STOP}
};

const boost::unordered_map<ConflictResolutionRule_File, std::string> Tools::fileResolutionRuleToString
{
    {ConflictResolutionRule_File::ASK,                  "ASK"},
    {ConflictResolutionRule_File::COPY_AND_RENAME,      "COPY_AND_RENAME"},
    {ConflictResolutionRule_File::OVERWRITE_DESTINATION,"OVERWRITE_DESTINATION"},
    {ConflictResolutionRule_File::OVERWRITE_SOURCE,     "OVERWRITE_SOURCE"},
    {ConflictResolutionRule_File::RENAME_AND_COPY,      "RENAME_AND_COPY"},
    {ConflictResolutionRule_File::STOP,                 "STOP"}
};

const boost::unordered_map<std::string, ConflictResolutionRule_File> Tools::stringToFileResolutionRule
{
    {"ASK",                     ConflictResolutionRule_File::ASK},
    {"COPY_AND_RENAME",         ConflictResolutionRule_File::COPY_AND_RENAME},
    {"OVERWRITE_DESTINATION",   ConflictResolutionRule_File::OVERWRITE_DESTINATION},
    {"OVERWRITE_SOURCE",        ConflictResolutionRule_File::OVERWRITE_SOURCE},
    {"RENAME_AND_COPY",         ConflictResolutionRule_File::RENAME_AND_COPY},
    {"STOP",                    ConflictResolutionRule_File::STOP}
};

const boost::unordered_map<SyncFailureAction, std::string> Tools::syncFailureActionToString
{
    {SyncFailureAction::RETRY_LATER,"RETRY_LATER"},
    {SyncFailureAction::RETRY_NOW,  "RETRY_NOW"},
    {SyncFailureAction::SKIP,       "SKIP"},
    {SyncFailureAction::STOP,       "STOP"}
};

const boost::unordered_map<std::string, SyncFailureAction> Tools::stringToSyncFailureAction
{
    {"RETRY_LATER", SyncFailureAction::RETRY_LATER},
    {"RETRY_NOW",   SyncFailureAction::RETRY_NOW},
    {"SKIP",        SyncFailureAction::SKIP},
    {"STOP",        SyncFailureAction::STOP}
};

const boost::unordered_map<SyncResult, std::string> Tools::syncResultToString
{
    {SyncResult::NONE,      "NONE"},
    {SyncResult::FAILED,    "FAILED"},
    {SyncResult::PARTIAL,   "PARTIAL"},
    {SyncResult::SUCCESSFUL,"SUCCESSFUL"}
};

const boost::unordered_map<std::string, SyncResult> Tools::stringToSyncResult
{
    {"NONE",        SyncResult::NONE},
    {"FAILED",      SyncResult::FAILED},
    {"PARTIAL",     SyncResult::PARTIAL},
    {"SUCCESSFUL",  SyncResult::SUCCESSFUL}
};

const boost::unordered_map<PeerType, std::string> Tools::peerTypeToString
{
    {PeerType::CLIENT,  "CLIENT"},
    {PeerType::SERVER,  "SERVER"},
    {PeerType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, PeerType> Tools::stringToPeerType
{
    {"CLIENT",  PeerType::CLIENT},
    {"SERVER",  PeerType::SERVER},
    {"INVALID", PeerType::INVALID}
};

const boost::unordered_map<ConnectionType, std::string> Tools::connectionTypeToString
{
    {ConnectionType::COMMAND,           "COMMAND"},
    {ConnectionType::DATA,              "DATA"},
    {ConnectionType::INVALID,           "INVALID"}
};

const boost::unordered_map<std::string, ConnectionType> Tools::stringToConnectionType
{
    {"COMMAND",             ConnectionType::COMMAND},
    {"DATA",                ConnectionType::DATA},
    {"INVALID",             ConnectionType::INVALID}
};

const boost::unordered_map<ConnectionState, std::string> Tools::connectionStateToString
{
    {ConnectionState::CLOSED,       "CLOSED"},
    {ConnectionState::ESTABLISHED,  "ESTABLISHED"},
    {ConnectionState::INVALID,      "INVALID"}
};

const boost::unordered_map<std::string, ConnectionState> Tools::stringToConnectionState
{
    {"CLOSED",      ConnectionState::CLOSED},
    {"ESTABLISHED", ConnectionState::ESTABLISHED},
    {"INVALID",     ConnectionState::INVALID}
};

const boost::unordered_map<ConnectionSubstate, std::string> Tools::connectionSubstateToString
{
    {ConnectionSubstate::DROPPED,   "DROPPED"},
    {ConnectionSubstate::NONE,      "NONE"},
    {ConnectionSubstate::READING,   "READING"},
    {ConnectionSubstate::FAILED,    "FAILED"},
    {ConnectionSubstate::WAITING,   "WAITING"},
    {ConnectionSubstate::WRITING,   "WRITING"}
};

const boost::unordered_map<std::string, ConnectionSubstate> Tools::stringToConnectionSubstate
{
    {"DROPPED", ConnectionSubstate::DROPPED},
    {"NONE",    ConnectionSubstate::NONE},
    {"READING", ConnectionSubstate::READING},
    {"FAILED",  ConnectionSubstate::FAILED},
    {"WAITING", ConnectionSubstate::WAITING},
    {"WRITING", ConnectionSubstate::WRITING}
};


const boost::unordered_map<ConnectionInitiation, std::string> Tools::connectionInitiationToString
{
    {ConnectionInitiation::LOCAL,   "LOCAL"},
    {ConnectionInitiation::REMOTE,  "REMOTE"},
    {ConnectionInitiation::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, ConnectionInitiation> Tools::stringToConnectionInitiation
{
    {"LOCAL",   ConnectionInitiation::LOCAL},
    {"REMOTE",  ConnectionInitiation::REMOTE},
    {"INVALID", ConnectionInitiation::INVALID}
};

const boost::unordered_map<InstructionSetType, std::string> Tools::instructionSetTypeToString
{
    {InstructionSetType::DATABASE_MANAGER, "DATABASE_MANAGER"},
    {InstructionSetType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, InstructionSetType> Tools::stringToInstructionSetType
{
    {"DATABASE_MANAGER", InstructionSetType::DATABASE_MANAGER},
    {"INVALID", InstructionSetType::INVALID}
};
