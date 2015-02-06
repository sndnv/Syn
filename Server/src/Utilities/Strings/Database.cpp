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

#include "Database.h"

using Maps = Utilities::Strings::DatabaseMaps;

const boost::unordered_map<DatabaseObjectType, std::string> Maps::databaseObjectTypesToString
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

const boost::unordered_map<std::string, DatabaseObjectType> Maps::stringToDatabaseObjectTypes
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

const boost::unordered_map<DatabaseManagerOperationMode, std::string> Maps::databaseManagerOperationModeToString
{
    {DatabaseManagerOperationMode::CRCW,    "CRCW"},
    {DatabaseManagerOperationMode::INVALID, "INVALID"},
    {DatabaseManagerOperationMode::PRCW,    "PRCW"},
    {DatabaseManagerOperationMode::PRPW,    "PRPW"}
};

const boost::unordered_map<std::string, DatabaseManagerOperationMode> Maps::stringToDatabaseManagerOperationMode
{
    {"CRCW",    DatabaseManagerOperationMode::CRCW},
    {"INVALID", DatabaseManagerOperationMode::INVALID},
    {"PRCW",    DatabaseManagerOperationMode::PRCW},
    {"PRPW",    DatabaseManagerOperationMode::PRPW}
};

const boost::unordered_map<DatabaseFailureAction, std::string> Maps::databaseFailureActionToString
{
    {DatabaseFailureAction::DROP_DAL,           "DROP_DAL"},
    {DatabaseFailureAction::DROP_IF_NOT_LAST,   "DROP_IF_NOT_LAST"},
    {DatabaseFailureAction::IGNORE_FAILURE,     "IGNORE_FAILURE"},
    {DatabaseFailureAction::INITIATE_RECONNECT, "INITIATE_RECONNECT"},
    {DatabaseFailureAction::PUSH_TO_BACK,       "PUSH_TO_BACK"},
    {DatabaseFailureAction::INVALID,            "INVALID"}
};

const boost::unordered_map<std::string, DatabaseFailureAction> Maps::stringToDatabaseFailureAction
{
    {"DROP_DAL",            DatabaseFailureAction::DROP_DAL},
    {"DROP_IF_NOT_LAST",    DatabaseFailureAction::DROP_IF_NOT_LAST},
    {"IGNORE_FAILURE",      DatabaseFailureAction::IGNORE_FAILURE},
    {"INITIATE_RECONNECT",  DatabaseFailureAction::INITIATE_RECONNECT},
    {"PUSH_TO_BACK",        DatabaseFailureAction::PUSH_TO_BACK},
    {"INVALID",             DatabaseFailureAction::INVALID}
};

const boost::unordered_map<StatisticType, std::string> Maps::statisticTypeToString
{
    {StatisticType::INSTALL_TIMESTAMP,          "INSTALL_TIMESTAMP"},
    {StatisticType::START_TIMESTAMP,            "START_TIMESTAMP"},
    {StatisticType::TOTAL_FAILED_TRANSFERS,     "TOTAL_FAILED_TRANSFERS"},
    {StatisticType::TOTAL_RETRIED_TRANSFERS,    "TOTAL_RETRIED_TRANSFERS"},
    {StatisticType::TOTAL_TRANSFERRED_DATA,     "TOTAL_TRANSFERRED_DATA"},
    {StatisticType::TOTAL_TRANSFERRED_FILES,    "TOTAL_TRANSFERRED_FILES"},
    {StatisticType::INVALID,                    "INVALID"}
};

const boost::unordered_map<std::string, StatisticType> Maps::stringToStatisticType
{
    {"INSTALL_TIMESTAMP",       StatisticType::INSTALL_TIMESTAMP},
    {"START_TIMESTAMP",         StatisticType::START_TIMESTAMP},
    {"TOTAL_FAILED_TRANSFERS",  StatisticType::TOTAL_FAILED_TRANSFERS},
    {"TOTAL_RETRIED_TRANSFERS", StatisticType::TOTAL_RETRIED_TRANSFERS},
    {"TOTAL_TRANSFERRED_DATA",  StatisticType::TOTAL_TRANSFERRED_DATA},
    {"TOTAL_TRANSFERRED_FILES", StatisticType::TOTAL_TRANSFERRED_FILES},
    {"INVALID",                 StatisticType::INVALID}
};

const boost::unordered_map<SystemParameterType, std::string> Maps::systemParameterTypeToString
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
    {SystemParameterType::SUPPORTED_PROTOCOLS,      "SUPPORTED_PROTOCOLS"},
    {SystemParameterType::INVALID,                  "INVALID"}
};

const boost::unordered_map<std::string, SystemParameterType> Maps::stringToSystemParameterType
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
    {"SUPPORTED_PROTOCOLS",     SystemParameterType::SUPPORTED_PROTOCOLS},
    {"INVALID",                 SystemParameterType::INVALID}
};

const boost::unordered_map<LogSeverity, std::string> Maps::logSeverityToString
{
    {LogSeverity::Debug,    "DEBUG"},
    {LogSeverity::Error,    "ERROR"},
    {LogSeverity::Info,     "INFO"},
    {LogSeverity::Warning,  "WARN"},
    {LogSeverity::INVALID,  "INVALID"}
};

const boost::unordered_map<std::string, LogSeverity> Maps::stringToLogSeverity
{
    {"DEBUG",   LogSeverity::Debug},
    {"ERROR",   LogSeverity::Error},
    {"INFO",    LogSeverity::Info},
    {"WARN",    LogSeverity::Warning},
    {"INVALID", LogSeverity::INVALID}
};

const boost::unordered_map<DataTransferType, std::string> Maps::dataTransferTypeToString
{
    {DataTransferType::PULL,    "PULL"},
    {DataTransferType::PUSH,    "PUSH"},
    {DataTransferType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, DataTransferType> Maps::stringToDataTransferType
{
    {"PULL",    DataTransferType::PULL},
    {"PUSH",    DataTransferType::PUSH},
    {"INVALID", DataTransferType::INVALID}
};

const boost::unordered_map<ScheduleIntervalType, std::string> Maps::scheduleIntervalTypeToString
{
    {ScheduleIntervalType::DAYS,    "DAYS"},
    {ScheduleIntervalType::HOURS,   "HOURS"},
    {ScheduleIntervalType::MINUTES, "MINUTES"},
    {ScheduleIntervalType::MONTHS,  "MONTHS"},
    {ScheduleIntervalType::SECONDS, "SECONDS"},
    {ScheduleIntervalType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, ScheduleIntervalType> Maps::stringToScheduleIntervalType
{
    {"DAYS",    ScheduleIntervalType::DAYS},
    {"HOURS",   ScheduleIntervalType::HOURS},
    {"MINUTES", ScheduleIntervalType::MINUTES},
    {"MONTHS",  ScheduleIntervalType::MONTHS},
    {"SECONDS", ScheduleIntervalType::SECONDS},
    {"INVALID", ScheduleIntervalType::INVALID}
};

const boost::unordered_map<ConflictResolutionRule_Directory, std::string> Maps::dirResolutionRuleToString
{
    {ConflictResolutionRule_Directory::ASK,                     "ASK"},
    {ConflictResolutionRule_Directory::COPY_AND_RENAME,         "COPY_AND_RENAME"},
    {ConflictResolutionRule_Directory::MERGE,                   "MERGE"},
    {ConflictResolutionRule_Directory::OVERWRITE_DESTINATION,   "OVERWRITE_DESTINATION"},
    {ConflictResolutionRule_Directory::OVERWRITE_SOURCE,        "OVERWRITE_SOURCE"},
    {ConflictResolutionRule_Directory::RENAME_AND_COPY,         "RENAME_AND_COPY"},
    {ConflictResolutionRule_Directory::STOP,                    "STOP"},
    {ConflictResolutionRule_Directory::INVALID,                 "INVALID"}
};

const boost::unordered_map<std::string, ConflictResolutionRule_Directory> Maps::stringToDirResolutionRule
{
    {"ASK",                     ConflictResolutionRule_Directory::ASK},
    {"COPY_AND_RENAME",         ConflictResolutionRule_Directory::COPY_AND_RENAME},
    {"MERGE",                   ConflictResolutionRule_Directory::MERGE},
    {"OVERWRITE_DESTINATION",   ConflictResolutionRule_Directory::OVERWRITE_DESTINATION},
    {"OVERWRITE_SOURCE",        ConflictResolutionRule_Directory::OVERWRITE_SOURCE},
    {"RENAME_AND_COPY",         ConflictResolutionRule_Directory::RENAME_AND_COPY},
    {"STOP",                    ConflictResolutionRule_Directory::STOP},
    {"INVALID",                 ConflictResolutionRule_Directory::INVALID}
};

const boost::unordered_map<ConflictResolutionRule_File, std::string> Maps::fileResolutionRuleToString
{
    {ConflictResolutionRule_File::ASK,                  "ASK"},
    {ConflictResolutionRule_File::COPY_AND_RENAME,      "COPY_AND_RENAME"},
    {ConflictResolutionRule_File::OVERWRITE_DESTINATION,"OVERWRITE_DESTINATION"},
    {ConflictResolutionRule_File::OVERWRITE_SOURCE,     "OVERWRITE_SOURCE"},
    {ConflictResolutionRule_File::RENAME_AND_COPY,      "RENAME_AND_COPY"},
    {ConflictResolutionRule_File::STOP,                 "STOP"},
    {ConflictResolutionRule_File::INVALID,              "INVALID"}
};

const boost::unordered_map<std::string, ConflictResolutionRule_File> Maps::stringToFileResolutionRule
{
    {"ASK",                     ConflictResolutionRule_File::ASK},
    {"COPY_AND_RENAME",         ConflictResolutionRule_File::COPY_AND_RENAME},
    {"OVERWRITE_DESTINATION",   ConflictResolutionRule_File::OVERWRITE_DESTINATION},
    {"OVERWRITE_SOURCE",        ConflictResolutionRule_File::OVERWRITE_SOURCE},
    {"RENAME_AND_COPY",         ConflictResolutionRule_File::RENAME_AND_COPY},
    {"STOP",                    ConflictResolutionRule_File::STOP},
    {"INVALID",                 ConflictResolutionRule_File::INVALID}
};

const boost::unordered_map<SyncFailureAction, std::string> Maps::syncFailureActionToString
{
    {SyncFailureAction::RETRY_LATER,"RETRY_LATER"},
    {SyncFailureAction::RETRY_NOW,  "RETRY_NOW"},
    {SyncFailureAction::SKIP,       "SKIP"},
    {SyncFailureAction::STOP,       "STOP"},
    {SyncFailureAction::INVALID,    "INVALID"}
};

const boost::unordered_map<std::string, SyncFailureAction> Maps::stringToSyncFailureAction
{
    {"RETRY_LATER", SyncFailureAction::RETRY_LATER},
    {"RETRY_NOW",   SyncFailureAction::RETRY_NOW},
    {"SKIP",        SyncFailureAction::SKIP},
    {"STOP",        SyncFailureAction::STOP},
    {"INVALID",     SyncFailureAction::INVALID}
};

const boost::unordered_map<SyncResult, std::string> Maps::syncResultToString
{
    {SyncResult::NONE,      "NONE"},
    {SyncResult::FAILED,    "FAILED"},
    {SyncResult::PARTIAL,   "PARTIAL"},
    {SyncResult::SUCCESSFUL,"SUCCESSFUL"},
    {SyncResult::INVALID,   "INVALID"}
};

const boost::unordered_map<std::string, SyncResult> Maps::stringToSyncResult
{
    {"NONE",        SyncResult::NONE},
    {"FAILED",      SyncResult::FAILED},
    {"PARTIAL",     SyncResult::PARTIAL},
    {"SUCCESSFUL",  SyncResult::SUCCESSFUL},
    {"INVALID",     SyncResult::INVALID}
};

std::string Utilities::Strings::toString(DatabaseObjectType var)
{
    if(Maps::databaseObjectTypesToString.find(var) != Maps::databaseObjectTypesToString.end())
        return Maps::databaseObjectTypesToString.at(var);
    else
        return "INVALID";
}

DatabaseObjectType Utilities::Strings::toDatabaseObjectType(std::string var)
{
    if(Maps::stringToDatabaseObjectTypes.find(var) != Maps::stringToDatabaseObjectTypes.end())
        return Maps::stringToDatabaseObjectTypes.at(var);
    else
        return DatabaseObjectType::INVALID;
}

std::string Utilities::Strings::toString(DatabaseManagerOperationMode var)
{
    if(Maps::databaseManagerOperationModeToString.find(var) != Maps::databaseManagerOperationModeToString.end())
        return Maps::databaseManagerOperationModeToString.at(var);
    else
        return "INVALID";
}

DatabaseManagerOperationMode Utilities::Strings::toDatabaseManagerOperationMode(std::string var)
{
    if(Maps::stringToDatabaseManagerOperationMode.find(var) != Maps::stringToDatabaseManagerOperationMode.end())
        return Maps::stringToDatabaseManagerOperationMode.at(var);
    else
        return DatabaseManagerOperationMode::INVALID;
}

std::string Utilities::Strings::toString(DatabaseFailureAction var)
{
    if(Maps::databaseFailureActionToString.find(var) != Maps::databaseFailureActionToString.end())
        return Maps::databaseFailureActionToString.at(var);
    else
        return "INVALID";
}

DatabaseFailureAction Utilities::Strings::toDatabaseFailureAction(std::string var)
{
    if(Maps::stringToDatabaseFailureAction.find(var) != Maps::stringToDatabaseFailureAction.end())
        return Maps::stringToDatabaseFailureAction.at(var);
    else
        return DatabaseFailureAction::INVALID;
}

std::string Utilities::Strings::toString(StatisticType var)
{
    if(Maps::statisticTypeToString.find(var) != Maps::statisticTypeToString.end())
        return Maps::statisticTypeToString.at(var);
    else
        return "INVALID";
}

StatisticType Utilities::Strings::toStatisticType(std::string var)
{
    if(Maps::stringToStatisticType.find(var) != Maps::stringToStatisticType.end())
        return Maps::stringToStatisticType.at(var);
    else
        return StatisticType::INVALID;
}

std::string Utilities::Strings::toString(SystemParameterType var)
{
    if(Maps::systemParameterTypeToString.find(var) != Maps::systemParameterTypeToString.end())
        return Maps::systemParameterTypeToString.at(var);
    else
        return "INVALID";
}

SystemParameterType Utilities::Strings::toSystemParameterType(std::string var)
{
    if(Maps::stringToSystemParameterType.find(var) != Maps::stringToSystemParameterType.end())
        return Maps::stringToSystemParameterType.at(var);
    else
        return SystemParameterType::INVALID;
}

std::string Utilities::Strings::toString(LogSeverity var)
{
    if(Maps::logSeverityToString.find(var) != Maps::logSeverityToString.end())
        return Maps::logSeverityToString.at(var);
    else
        return "INVALID";
}

LogSeverity Utilities::Strings::toLogSeverity(std::string var)
{
    if(Maps::stringToLogSeverity.find(var) != Maps::stringToLogSeverity.end())
        return Maps::stringToLogSeverity.at(var);
    else
        return LogSeverity::INVALID;
}

std::string Utilities::Strings::toString(DataTransferType var)
{
    if(Maps::dataTransferTypeToString.find(var) != Maps::dataTransferTypeToString.end())
        return Maps::dataTransferTypeToString.at(var);
    else
        return "INVALID";
}

DataTransferType Utilities::Strings::toDataTransferType(std::string var)
{
    if(Maps::stringToDataTransferType.find(var) != Maps::stringToDataTransferType.end())
        return Maps::stringToDataTransferType.at(var);
    else
        return DataTransferType::INVALID;
}

std::string Utilities::Strings::toString(ScheduleIntervalType var)
{
    if(Maps::scheduleIntervalTypeToString.find(var) != Maps::scheduleIntervalTypeToString.end())
        return Maps::scheduleIntervalTypeToString.at(var);
    else
        return "INVALID";
}

ScheduleIntervalType Utilities::Strings::toScheduleIntervalType(std::string var)
{
    if(Maps::stringToScheduleIntervalType.find(var) != Maps::stringToScheduleIntervalType.end())
        return Maps::stringToScheduleIntervalType.at(var);
    else
        return ScheduleIntervalType::INVALID;
}

std::string Utilities::Strings::toString(ConflictResolutionRule_Directory var)
{
    if(Maps::dirResolutionRuleToString.find(var) != Maps::dirResolutionRuleToString.end())
        return Maps::dirResolutionRuleToString.at(var);
    else
        return "INVALID";
}

ConflictResolutionRule_Directory Utilities::Strings::toDirConflictResolutionRule(std::string var)
{
    if(Maps::stringToDirResolutionRule.find(var) != Maps::stringToDirResolutionRule.end())
        return Maps::stringToDirResolutionRule.at(var);
    else
        return ConflictResolutionRule_Directory::INVALID;
}

std::string Utilities::Strings::toString(ConflictResolutionRule_File var)
{
    if(Maps::fileResolutionRuleToString.find(var) != Maps::fileResolutionRuleToString.end())
        return Maps::fileResolutionRuleToString.at(var);
    else
        return "INVALID";
}

ConflictResolutionRule_File Utilities::Strings::toFileConflictResolutionRule(std::string var)
{
    if(Maps::stringToFileResolutionRule.find(var) != Maps::stringToFileResolutionRule.end())
        return Maps::stringToFileResolutionRule.at(var);
    else
        return ConflictResolutionRule_File::INVALID;
}

std::string Utilities::Strings::toString(SyncFailureAction var)
{
    if(Maps::syncFailureActionToString.find(var) != Maps::syncFailureActionToString.end())
        return Maps::syncFailureActionToString.at(var);
    else
        return "INVALID";
}

SyncFailureAction Utilities::Strings::toSyncFailureAction(std::string var)
{
    if(Maps::stringToSyncFailureAction.find(var) != Maps::stringToSyncFailureAction.end())
        return Maps::stringToSyncFailureAction.at(var);
    else
        return SyncFailureAction::INVALID;
}

std::string Utilities::Strings::toString(SyncResult var)
{
    if(Maps::syncResultToString.find(var) != Maps::syncResultToString.end())
        return Maps::syncResultToString.at(var);
    else
        return "INVALID";
}

SyncResult Utilities::Strings::toSyncResult(std::string var)
{
    if(Maps::stringToSyncResult.find(var) != Maps::stringToSyncResult.end())
        return Maps::stringToSyncResult.at(var);
    else
        return SyncResult::INVALID;
}
