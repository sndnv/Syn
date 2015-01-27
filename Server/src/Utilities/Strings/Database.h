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

#ifndef UTILITIES_STRINGS_DATABASE_H
#define	UTILITIES_STRINGS_DATABASE_H

#include <string>
#include <boost/unordered_map.hpp>
#include "../../DatabaseManagement/Types/Types.h"

using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DatabaseManagerOperationMode;
using DatabaseManagement_Types::DatabaseFailureAction;
using DatabaseManagement_Types::StatisticType;
using DatabaseManagement_Types::SystemParameterType;
using DatabaseManagement_Types::LogSeverity;
using DatabaseManagement_Types::DataTransferType;
using DatabaseManagement_Types::SessionType;
using DatabaseManagement_Types::ScheduleIntervalType;
using DatabaseManagement_Types::ConflictResolutionRule_Directory;
using DatabaseManagement_Types::ConflictResolutionRule_File;
using DatabaseManagement_Types::SyncFailureAction;
using DatabaseManagement_Types::SyncResult;
using DatabaseManagement_Types::DatabaseSelectConstraints;

namespace Utilities
{
    namespace Strings
    {
        struct DatabaseMaps
        {
            static const boost::unordered_map<DatabaseObjectType, std::string> databaseObjectTypesToString;
            static const boost::unordered_map<std::string, DatabaseObjectType> stringToDatabaseObjectTypes;
            static const boost::unordered_map<DatabaseManagerOperationMode, std::string> databaseManagerOperationModeToString;
            static const boost::unordered_map<std::string, DatabaseManagerOperationMode> stringToDatabaseManagerOperationMode;
            static const boost::unordered_map<DatabaseFailureAction, std::string> databaseFailureActionToString;
            static const boost::unordered_map<std::string, DatabaseFailureAction> stringToDatabaseFailureAction;
            static const boost::unordered_map<StatisticType, std::string> statisticTypeToString;
            static const boost::unordered_map<std::string, StatisticType> stringToStatisticType;
            static const boost::unordered_map<SystemParameterType, std::string> systemParameterTypeToString;
            static const boost::unordered_map<std::string, SystemParameterType> stringToSystemParameterType;
            static const boost::unordered_map<LogSeverity, std::string> logSeverityToString;
            static const boost::unordered_map<std::string, LogSeverity> stringToLogSeverity;
            static const boost::unordered_map<DataTransferType, std::string> dataTransferTypeToString;
            static const boost::unordered_map<std::string, DataTransferType> stringToDataTransferType;
            static const boost::unordered_map<SessionType, std::string> sessionTypeToString;
            static const boost::unordered_map<std::string, SessionType> stringToSessionType;
            static const boost::unordered_map<ScheduleIntervalType, std::string> scheduleIntervalTypeToString;
            static const boost::unordered_map<std::string, ScheduleIntervalType> stringToScheduleIntervalType;
            static const boost::unordered_map<ConflictResolutionRule_Directory, std::string> dirResolutionRuleToString;
            static const boost::unordered_map<std::string, ConflictResolutionRule_Directory> stringToDirResolutionRule;
            static const boost::unordered_map<ConflictResolutionRule_File, std::string> fileResolutionRuleToString;
            static const boost::unordered_map<std::string, ConflictResolutionRule_File> stringToFileResolutionRule;
            static const boost::unordered_map<SyncFailureAction, std::string> syncFailureActionToString;
            static const boost::unordered_map<std::string, SyncFailureAction> stringToSyncFailureAction;
            static const boost::unordered_map<SyncResult, std::string> syncResultToString;
            static const boost::unordered_map<std::string, SyncResult> stringToSyncResult;
        };
        
        std::string toString(DatabaseObjectType var);
        std::string toString(DatabaseManagerOperationMode var);
        std::string toString(DatabaseFailureAction var);
        std::string toString(StatisticType var);
        std::string toString(SystemParameterType var);
        std::string toString(LogSeverity var);
        std::string toString(DataTransferType var);
        std::string toString(SessionType var);
        std::string toString(ScheduleIntervalType var);
        std::string toString(ConflictResolutionRule_Directory var);
        std::string toString(ConflictResolutionRule_File var);
        std::string toString(SyncFailureAction var);
        std::string toString(SyncResult var);
        
        DatabaseObjectType toDatabaseObjectType(std::string var);
        DatabaseManagerOperationMode toDatabaseManagerOperationMode(std::string var);
        DatabaseFailureAction toDatabaseFailureAction(std::string var);
        StatisticType toStatisticType(std::string var);
        SystemParameterType toSystemParameterType(std::string var);
        LogSeverity toLogSeverity(std::string var);
        DataTransferType toDataTransferType(std::string var);
        SessionType toSessionType(std::string var);
        ScheduleIntervalType toScheduleIntervalType(std::string var);
        ConflictResolutionRule_Directory toDirConflictResolutionRule(std::string var);
        ConflictResolutionRule_File toFileConflictResolutionRule(std::string var);
        SyncFailureAction toSyncFailureAction(std::string var);
        SyncResult toSyncResult(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_DATABASE_H */
