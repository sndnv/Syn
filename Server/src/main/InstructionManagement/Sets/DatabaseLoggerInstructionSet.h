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

#ifndef DATABASELOGGERINSTRUCTIONSET_H
#define	DATABASELOGGERINSTRUCTIONSET_H

#include <vector>
#include <string>
#include <boost/any.hpp>

#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../EntityManagement/Types/Types.h"
#include "../../DatabaseManagement/Types/Types.h"
#include "../../DatabaseManagement/Containers/LogDataContainer.h"

using Common_Types::LogSeverity;
using Common_Types::LogID;
using Common_Types::INVALID_LOG_ID;
using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::DatabaseLoggerInstructionType;
using DatabaseManagement_Containers::LogDataContainerPtr;
using DatabaseManagement_Types::DatabaseSelectConstraints;
using EntityManagement_Types::DatabaseLoggingSourceID;
using EntityManagement_Types::INVALID_DATABASE_LOGGING_SOURCE_ID;

namespace InstructionManagement_Sets
{
    namespace DatabaseLoggerInstructions
    {
        struct GetLog : public Instruction<DatabaseLoggerInstructionType>
        {
            explicit GetLog(LogID id)
            : Instruction(InstructionSetType::DATABASE_LOGGER, DatabaseLoggerInstructionType::GET_LOG),
              logID(id)
            {}
            
            bool isValid() override { return (logID != INVALID_LOG_ID); }
            LogID logID;
        };
        
        struct GetLogsByConstraint : public Instruction<DatabaseLoggerInstructionType>
        {
            GetLogsByConstraint(DatabaseSelectConstraints::LOGS type, boost::any value)
            : Instruction(InstructionSetType::DATABASE_LOGGER, DatabaseLoggerInstructionType::GET_LOGS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::LOGS constraintType;
            boost::any constraintValue;
        };
        
        struct UpdateSourceLoggingLevel : public Instruction<DatabaseLoggerInstructionType>
        {
            UpdateSourceLoggingLevel(DatabaseLoggingSourceID id, LogSeverity severity)
            : Instruction(InstructionSetType::DATABASE_LOGGER, DatabaseLoggerInstructionType::UPDATE_SOURCE_LOGGING_LEVEL),
              sourceID(id), newLogSeverity(severity)
            {}
            
            bool isValid() override { return (sourceID != INVALID_DATABASE_LOGGING_SOURCE_ID && newLogSeverity != LogSeverity::INVALID); }
            DatabaseLoggingSourceID sourceID;
            LogSeverity newLogSeverity;
        };
        
        struct UpdateDefaultLoggingLevel : public Instruction<DatabaseLoggerInstructionType>
        {
            explicit UpdateDefaultLoggingLevel(LogSeverity severity)
            : Instruction(InstructionSetType::DATABASE_LOGGER, DatabaseLoggerInstructionType::UPDATE_DEFAULT_LOGGING_LEVEL),
              newLogSeverity(severity)
            {}
            
            bool isValid() override { return (newLogSeverity != LogSeverity::INVALID); }
            LogSeverity newLogSeverity;
        };
        
        struct DebugGetState : public Instruction<DatabaseLoggerInstructionType>
        {
            DebugGetState()
            : Instruction(InstructionSetType::DATABASE_LOGGER, DatabaseLoggerInstructionType::DEBUG_GET_STATE)
            {}
            
            bool isValid() override { return true; }
        };
        
        namespace Results
        {
            struct GetLog : public InstructionResult<DatabaseLoggerInstructionType>
            {
                explicit GetLog(LogDataContainerPtr input)
                : InstructionResult(DatabaseLoggerInstructionType::GET_LOG), result(input) {}
                
                LogDataContainerPtr result;
            };
            
            struct GetLogsByConstraint : public InstructionResult<DatabaseLoggerInstructionType>
            {
                explicit GetLogsByConstraint(std::vector<LogDataContainerPtr> input)
                : InstructionResult(DatabaseLoggerInstructionType::GET_LOGS_BY_CONSTRAINT), result(input) {}
                
                std::vector<LogDataContainerPtr> result;
            };
            
            struct UpdateSourceLoggingLevel : public InstructionResult<DatabaseLoggerInstructionType>
            {
                explicit UpdateSourceLoggingLevel(bool input)
                : InstructionResult(DatabaseLoggerInstructionType::UPDATE_SOURCE_LOGGING_LEVEL), result(input) {}
                
                bool result;
            };
            
            struct UpdateDefaultLoggingLevel : public InstructionResult<DatabaseLoggerInstructionType>
            {
                explicit UpdateDefaultLoggingLevel(bool input)
                : InstructionResult(DatabaseLoggerInstructionType::UPDATE_DEFAULT_LOGGING_LEVEL), result(input) {}
                
                bool result;
            };
            
            struct DebugGetState : public InstructionResult<DatabaseLoggerInstructionType>
            {
                explicit DebugGetState(std::string input)
                : InstructionResult(DatabaseLoggerInstructionType::DEBUG_GET_STATE), result(input) {}
                
                std::string result;
            };
        }
    }
}

#endif	/* DATABASELOGGERINSTRUCTIONSET_H */

