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
#include "../../DatabaseManagement/Types/Types.h"

using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DatabaseManagerOperationMode;
using DatabaseManagement_Types::DatabaseFailureAction;
using DatabaseManagement_Types::StatisticType;
using DatabaseManagement_Types::SystemParameterType;
using DatabaseManagement_Types::DataTransferType;
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
        std::string toString(DatabaseObjectType var);
        std::string toString(DatabaseManagerOperationMode var);
        std::string toString(DatabaseFailureAction var);
        std::string toString(StatisticType var);
        std::string toString(SystemParameterType var);
        std::string toString(DataTransferType var);
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
        DataTransferType toDataTransferType(std::string var);
        ScheduleIntervalType toScheduleIntervalType(std::string var);
        ConflictResolutionRule_Directory toDirConflictResolutionRule(std::string var);
        ConflictResolutionRule_File toFileConflictResolutionRule(std::string var);
        SyncFailureAction toSyncFailureAction(std::string var);
        SyncResult toSyncResult(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_DATABASE_H */
