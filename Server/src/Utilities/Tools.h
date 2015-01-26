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

#ifndef TOOLS_H
#define	TOOLS_H

#include <string>
#include <boost/thread.hpp>
#include <boost/regex.hpp>
#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cryptopp/secblock.h>
#include "../DatabaseManagement/Types/Types.h"
#include "../NetworkManagement/Types/Types.h"
#include "../NetworkManagement/Types/Packets.h"
#include "../InstructionManagement/Types/Types.h"
#include "../Common/Types.h"
#include "../StorageManagement/Types/Types.h"
#include "../SecurityManagement/Types/Types.h"

using Common_Types::Byte;
using Common_Types::UserAccessLevel;

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

using NetworkManagement_Types::ConnectionRequest;
using NetworkManagement_Types::ConnectionState;
using NetworkManagement_Types::ConnectionSubstate;
using NetworkManagement_Types::ConnectionType;
using NetworkManagement_Types::PeerType;
using NetworkManagement_Types::ConnectionInitiation;

using InstructionManagement_Types::InstructionSetType;

using StorageManagement_Types::PoolMode;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::LinkActionType;
using StorageManagement_Types::LinkActionConditionType;
using StorageManagement_Types::DataPoolType;

using SecurityManagement_Types::SecurableComponentType;
using SecurityManagement_Types::CacheEvictionType;
using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::CryptoPPByte;

namespace Utilities
{
    class Tools
    {
        public:
            static unsigned long powerof(unsigned long base, unsigned long exponent)
            {
                if(exponent != 0)
                {
                    unsigned long intermediateResult = powerof(base, (exponent - 1));
                    unsigned long result = base*intermediateResult;
                    
                    if(result/intermediateResult != base)
                        throw std::overflow_error("Tools::powerof() > Overflow encountered.");
                    else
                        return result;
                }
                else
                    return 1;
            }
            
            static std::string toString(bool var) { return (var) ? "TRUE" : "FALSE"; }
            static std::string toString(int var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(long var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(short var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(unsigned int var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(unsigned long var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(unsigned short var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(unsigned long long var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(boost::uuids::uuid var) { return boost::lexical_cast<std::string>(var); }
            static std::string toString(boost::thread::id var) { return boost::lexical_cast<std::string>(var); }
            
            static std::string toString(CryptoPP::SecByteBlock var)
            {
                unsigned int resultLength = var.size() * 2;
                char result[resultLength + 1];
                
                Byte * varDataPtr = var.BytePtr();
                
                for(unsigned int i = 0; i < var.size(); i++)
                    snprintf(&result[2*i], resultLength+1, "%02X", varDataPtr[i]);
                
                result[resultLength] = '\0';
                
                return std::string(result);
            }
            
            static CryptoPP::SecByteBlock toSecByteBlock(std::string var)
            {
                if(var.size() == 0 || var.size() % 2 != 0)
                    return CryptoPP::SecByteBlock();
                
                std::size_t blockSize = var.size()/2;
                CryptoPP::SecByteBlock result(blockSize);
                for(std::size_t i = 0; i < blockSize; i++)
                {
                    std::string currentRawByte = var.substr(i*2, 2);
                    result.data()[i] = strtol(currentRawByte.c_str(), 0, 16);
                }
                
                return result;
            }
            
            static std::string toString(ByteVector var)
            {
                unsigned int resultLength = var.size() * 2;
                char result[resultLength + 1];
                
                for(unsigned int i = 0; i < var.size(); i++)
                    snprintf(&result[2*i], resultLength+1, "%02X", var[i]);
                
                result[resultLength] = '\0';
                
                return std::string(result);
            }
            
            static std::string toString(boost::posix_time::ptime var)
            {
                unsigned int bufferSize = 20;
                char timestampBuffer[bufferSize];
                snprintf(timestampBuffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", 
                        (unsigned short)var.date().year(), (unsigned short)var.date().month(), (unsigned short)var.date().day(), 
                        var.time_of_day().hours(), var.time_of_day().minutes(), var.time_of_day().seconds());

                return std::string(timestampBuffer);
            }
            
            static boost::posix_time::ptime toTimestamp(std::string var)
            {
                boost::smatch timestampMatch;
                
                if(boost::regex_search(var, timestampMatch, boost::regex("(\\d{4})-(\\d{2})-(\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})")))
                {
                    boost::gregorian::date date(boost::lexical_cast<unsigned short>(timestampMatch[1]), boost::lexical_cast<unsigned short>(timestampMatch[2]), boost::lexical_cast<unsigned short>(timestampMatch[3]));
                    boost::posix_time::time_duration time(boost::lexical_cast<int>(timestampMatch[4]), boost::lexical_cast<int>(timestampMatch[5]), boost::lexical_cast<int>(timestampMatch[6]));
                    return boost::posix_time::ptime(date, time);
                }
                else
                    return boost::posix_time::ptime();
            }
            
            static std::string toString(DatabaseObjectType var)
            {
                if(databaseObjectTypesToString.find(var) != databaseObjectTypesToString.end())
                    return databaseObjectTypesToString.at(var);
                else
                    return "INVALID";
            }
            
            static DatabaseObjectType toDatabaseObjectType(std::string var)
            {
                if(stringToDatabaseObjectTypes.find(var) != stringToDatabaseObjectTypes.end())
                    return stringToDatabaseObjectTypes.at(var);
                else
                    return DatabaseObjectType::INVALID;
            }
            
            static std::string toString(DatabaseManagerOperationMode var)
            {
                if(databaseManagerOperationModeToString.find(var) != databaseManagerOperationModeToString.end())
                    return databaseManagerOperationModeToString.at(var);
                else
                    return "INVALID";
            }
            
            static DatabaseManagerOperationMode toDatabaseManagerOperationMode(std::string var)
            {
                if(stringToDatabaseManagerOperationMode.find(var) != stringToDatabaseManagerOperationMode.end())
                    return stringToDatabaseManagerOperationMode.at(var);
                else
                    return DatabaseManagerOperationMode::INVALID;
            }
            
            static std::string toString(DatabaseFailureAction var)
            {
                if(databaseFailureActionToString.find(var) != databaseFailureActionToString.end())
                    return databaseFailureActionToString.at(var);
                else
                    return "INVALID";
            }
            
            static DatabaseFailureAction toDatabaseFailureAction(std::string var)
            {
                return stringToDatabaseFailureAction.at(var);
            }
            
            static std::string toString(UserAccessLevel var)
            {
                if(userAccessLevelToString.find(var) != userAccessLevelToString.end())
                    return userAccessLevelToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static UserAccessLevel toUserAccessLevel(std::string var)
            {
                return stringToUserAccessLevel.at(var);
            }
            
            static std::string toString(StatisticType var)
            {
                if(statisticTypeToString.find(var) != statisticTypeToString.end())
                    return statisticTypeToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static StatisticType toStatisticType(std::string var)
            {
                return stringToStatisticType.at(var);
            }
            
            static std::string toString(SystemParameterType var)
            {
                if(systemParameterTypeToString.find(var) != systemParameterTypeToString.end())
                    return systemParameterTypeToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static SystemParameterType toSystemParameterType(std::string var)
            {
                return stringToSystemParameterType.at(var);
            }
            
            static std::string toString(LogSeverity var)
            {
                if(logSeverityToString.find(var) != logSeverityToString.end())
                    return logSeverityToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static LogSeverity toLogSeverity(std::string var)
            {
                return stringToLogSeverity.at(var);
            }
            
            static std::string toString(DataTransferType var)
            {
                if(dataTransferTypeToString.find(var) != dataTransferTypeToString.end())
                    return dataTransferTypeToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static DataTransferType toDataTransferType(std::string var)
            {
                return stringToDataTransferType.at(var);
            }
            
            static std::string toString(SessionType var)
            {
                if(sessionTypeToString.find(var) != sessionTypeToString.end())
                    return sessionTypeToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static SessionType toSessionType(std::string var)
            {
                return stringToSessionType.at(var);
            }
            
            static std::string toString(ScheduleIntervalType var)
            {
                if(scheduleIntervalTypeToString.find(var) != scheduleIntervalTypeToString.end())
                    return scheduleIntervalTypeToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static ScheduleIntervalType toScheduleIntervalType(std::string var)
            {
                return stringToScheduleIntervalType.at(var);
            }
            
            static std::string toString(ConflictResolutionRule_Directory var)
            {
                if(dirResolutionRuleToString.find(var) != dirResolutionRuleToString.end())
                    return dirResolutionRuleToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static ConflictResolutionRule_Directory toDirConflictResolutionRule(std::string var)
            {
                return stringToDirResolutionRule.at(var);
            }
            
            static std::string toString(ConflictResolutionRule_File var)
            {
                if(fileResolutionRuleToString.find(var) != fileResolutionRuleToString.end())
                    return fileResolutionRuleToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static ConflictResolutionRule_File toFileConflictResolutionRule(std::string var)
            {
                return stringToFileResolutionRule.at(var);
            }
            
            static std::string toString(SyncFailureAction var)
            {
                if(syncFailureActionToString.find(var) != syncFailureActionToString.end())
                    return syncFailureActionToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static SyncFailureAction toSyncFailureAction(std::string var)
            {
                return stringToSyncFailureAction.at(var);
            }
            
            static std::string toString(SyncResult var)
            {
                if(syncResultToString.find(var) != syncResultToString.end())
                    return syncResultToString.at(var);
                else
                    return "UNDEFINED";
            }
            
            static SyncResult toSyncResult(std::string var)
            {
                return stringToSyncResult.at(var);
            }
            
            static Common_Types::DBObjectID getIDFromString(std::string var)
            {
                try { return boost::lexical_cast<boost::uuids::uuid>(var); }
                catch(boost::bad_lexical_cast) { return Common_Types::INVALID_OBJECT_ID;}
            }
            
            static Common_Types::DBObjectID getIDFromConstraint(DatabaseObjectType objectType, boost::any constraintType, boost::any constraintValue)
            {
                Common_Types::DBObjectID objectID = Common_Types::INVALID_OBJECT_ID;

                switch(objectType)
                {
                    case DatabaseObjectType::DEVICE:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::DEVICES>(constraintType) == DatabaseSelectConstraints::DEVICES::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::DeviceID>(constraintValue);
                    } break;

                    case DatabaseObjectType::LOG:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::LOGS>(constraintType) == DatabaseSelectConstraints::LOGS::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::LogID>(constraintValue);
                    } break;

                    case DatabaseObjectType::SCHEDULE:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::SCHEDULES>(constraintType) == DatabaseSelectConstraints::SCHEDULES::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::ScheduleID>(constraintValue);
                    } break;

                    case DatabaseObjectType::SESSION:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::SESSIONS>(constraintType) == DatabaseSelectConstraints::SESSIONS::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::SessionID>(constraintValue);
                    } break;

                    case DatabaseObjectType::STATISTICS:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::STATISTCS>(constraintType) == DatabaseSelectConstraints::STATISTCS::LIMIT_BY_TYPE)
                            objectID = boost::any_cast<boost::uuids::uuid>(constraintValue);
                    } break;

                    case DatabaseObjectType::SYNC_FILE:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::SYNC>(constraintType) == DatabaseSelectConstraints::SYNC::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::SyncID>(constraintValue);
                    } break;

                    case DatabaseObjectType::SYSTEM_SETTINGS:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::SYSTEM>(constraintType) == DatabaseSelectConstraints::SYSTEM::LIMIT_BY_TYPE)
                            objectID = boost::any_cast<boost::uuids::uuid>(constraintValue);
                    } break;

                    case DatabaseObjectType::USER:
                    {
                        if(boost::any_cast<DatabaseSelectConstraints::USERS>(constraintType) == DatabaseSelectConstraints::USERS::LIMIT_BY_ID)
                            objectID = boost::any_cast<Common_Types::UserID>(constraintValue);
                    } break;

                    default: ; break; //ignore
                }

                return objectID;
            }
            
            static std::string toString(PeerType var)
            {
                if(peerTypeToString.find(var) != peerTypeToString.end())
                    return peerTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static PeerType toPeerType(std::string var)
            {
                if(stringToPeerType.find(var) != stringToPeerType.end())
                    return stringToPeerType.at(var);
                else
                    return PeerType::INVALID;
            }
            
            static std::string toString(ConnectionType var)
            {
                if(connectionTypeToString.find(var) != connectionTypeToString.end())
                    return connectionTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static ConnectionType toConnectionType(std::string var)
            {
                if(stringToConnectionType.find(var) != stringToConnectionType.end())
                    return stringToConnectionType.at(var);
                else
                    return ConnectionType::INVALID;
            }
            
            static std::string toString(ConnectionState var)
            {
                if(connectionStateToString.find(var) != connectionStateToString.end())
                    return connectionStateToString.at(var);
                else
                    return "INVALID";
            }
            
            static ConnectionState toConnectionState(std::string var)
            {
                if(stringToConnectionState.find(var) != stringToConnectionState.end())
                    return stringToConnectionState.at(var);
                else
                    return ConnectionState::INVALID;
            }
            
            static std::string toString(ConnectionRequest var)
            {
                return toString(ConnectionRequest::VERSION) + "," + toString(ConnectionRequest::BYTE_LENGTH) + ","
                       + toString(var.senderPeerType) + "," + toString(var.connectionType);
            }
            
            static std::string toString(ConnectionSubstate var)
            {
                if(connectionSubstateToString.find(var) != connectionSubstateToString.end())
                    return connectionSubstateToString.at(var);
                else
                    return "NONE";
            }
            
            static ConnectionSubstate toConnectionSubstate(std::string var)
            {
                if(stringToConnectionSubstate.find(var) != stringToConnectionSubstate.end())
                    return stringToConnectionSubstate.at(var);
                else
                    return ConnectionSubstate::NONE;
            }
            
            static std::string toString(ConnectionInitiation var)
            {
                if(connectionInitiationToString.find(var) != connectionInitiationToString.end())
                    return connectionInitiationToString.at(var);
                else
                    return "INVALID";
            }
            
            static ConnectionInitiation toConnectionInitiation(std::string var)
            {
                if(stringToConnectionInitiation.find(var) != stringToConnectionInitiation.end())
                    return stringToConnectionInitiation.at(var);
                else
                    return ConnectionInitiation::INVALID;
            }
            
            static std::string toString(InstructionSetType var)
            {
                if(instructionSetTypeToString.find(var) != instructionSetTypeToString.end())
                    return instructionSetTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static InstructionSetType toInstructionSetType(std::string var)
            {
                if(stringToInstructionSetType.find(var) != stringToInstructionSetType.end())
                    return stringToInstructionSetType.at(var);
                else
                    return InstructionSetType::INVALID;
            }
            
            static std::string toString(DataPoolType var)
            {
                if(dataPoolTypeToString.find(var) != dataPoolTypeToString.end())
                    return dataPoolTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static DataPoolType toDataPoolType(std::string var)
            {
                if(stringToDataPoolType.find(var) != stringToDataPoolType.end())
                    return stringToDataPoolType.at(var);
                else
                    return DataPoolType::INVALID;
            }
            
            static std::string toString(PoolMode var)
            {
                if(poolModeToString.find(var) != poolModeToString.end())
                    return poolModeToString.at(var);
                else
                    return "INVALID";
            }
            
            static PoolMode toPoolMode(std::string var)
            {
                if(stringToPoolMode.find(var) != stringToPoolMode.end())
                    return stringToPoolMode.at(var);
                else
                    return PoolMode::INVALID;
            }
            
            static std::string toString(PoolState var)
            {
                if(poolStateToString.find(var) != poolStateToString.end())
                    return poolStateToString.at(var);
                else
                    return "INVALID";
            }
            
            static PoolState toPoolState(std::string var)
            {
                if(stringToPoolState.find(var) != stringToPoolState.end())
                    return stringToPoolState.at(var);
                else
                    return PoolState::INVALID;
            }
            
            static std::string toString(LinkActionType var)
            {
                if(linkActionTypeToString.find(var) != linkActionTypeToString.end())
                    return linkActionTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static LinkActionType toLinkActionType(std::string var)
            {
                if(stringToLinkActionType.find(var) != stringToLinkActionType.end())
                    return stringToLinkActionType.at(var);
                else
                    return LinkActionType::INVALID;
            }
            
            static std::string toString(LinkActionConditionType var)
            {
                if(linkActionConditionTypeToString.find(var) != linkActionConditionTypeToString.end())
                    return linkActionConditionTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static LinkActionConditionType toLinkActionConditionType(std::string var)
            {
                if(stringToLinkActionConditionType.find(var) != stringToLinkActionConditionType.end())
                    return stringToLinkActionConditionType.at(var);
                else
                    return LinkActionConditionType::INVALID;
            }
            
            static std::string toString(SecurableComponentType var)
            {
                if(securableComponentTypeToString.find(var) != securableComponentTypeToString.end())
                    return securableComponentTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static SecurableComponentType toSecurableComponentType(std::string var)
            {
                if(stringToSecurableComponentType.find(var) != stringToSecurableComponentType.end())
                    return stringToSecurableComponentType.at(var);
                else
                    return SecurableComponentType::INVALID;
            }
            
            static std::string toString(CacheEvictionType var)
            {
                if(cacheEvictionTypeToString.find(var) != cacheEvictionTypeToString.end())
                    return cacheEvictionTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static CacheEvictionType toCacheEvictionType(std::string var)
            {
                if(stringToCacheEvictionType.find(var) != stringToCacheEvictionType.end())
                    return stringToCacheEvictionType.at(var);
                else
                    return CacheEvictionType::INVALID;
            }
            
            static std::string toString(HashAlgorithmType var)
            {
                if(hashAlgorithmTypeToString.find(var) != hashAlgorithmTypeToString.end())
                    return hashAlgorithmTypeToString.at(var);
                else
                    return "INVALID";
            }
            
            static HashAlgorithmType toHashAlgorithmType(std::string var)
            {
                if(stringToHashAlgorithmType.find(var) != stringToHashAlgorithmType.end())
                    return stringToHashAlgorithmType.at(var);
                else
                    return HashAlgorithmType::INVALID;
            }
            
        private:
            Tools();
            
            static const boost::unordered_map<DatabaseObjectType, std::string> databaseObjectTypesToString;
            static const boost::unordered_map<std::string, DatabaseObjectType> stringToDatabaseObjectTypes;
            static const boost::unordered_map<DatabaseManagerOperationMode, std::string> databaseManagerOperationModeToString;
            static const boost::unordered_map<std::string, DatabaseManagerOperationMode> stringToDatabaseManagerOperationMode;
            static const boost::unordered_map<DatabaseFailureAction, std::string> databaseFailureActionToString;
            static const boost::unordered_map<std::string, DatabaseFailureAction> stringToDatabaseFailureAction;
            static const boost::unordered_map<UserAccessLevel, std::string> userAccessLevelToString;
            static const boost::unordered_map<std::string, UserAccessLevel> stringToUserAccessLevel;
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
            
            static const boost::unordered_map<PeerType, std::string> peerTypeToString;
            static const boost::unordered_map<std::string, PeerType> stringToPeerType;
            static const boost::unordered_map<ConnectionType, std::string> connectionTypeToString;
            static const boost::unordered_map<std::string, ConnectionType> stringToConnectionType;
            static const boost::unordered_map<ConnectionState, std::string> connectionStateToString;
            static const boost::unordered_map<std::string, ConnectionState> stringToConnectionState;
            static const boost::unordered_map<ConnectionSubstate, std::string> connectionSubstateToString;
            static const boost::unordered_map<std::string, ConnectionSubstate> stringToConnectionSubstate;
            static const boost::unordered_map<ConnectionInitiation, std::string> connectionInitiationToString;
            static const boost::unordered_map<std::string, ConnectionInitiation> stringToConnectionInitiation;
            
            static const boost::unordered_map<InstructionSetType, std::string> instructionSetTypeToString;
            static const boost::unordered_map<std::string, InstructionSetType> stringToInstructionSetType;
            
            static const boost::unordered_map<DataPoolType, std::string> dataPoolTypeToString;
            static const boost::unordered_map<std::string, DataPoolType> stringToDataPoolType;
            static const boost::unordered_map<PoolMode, std::string> poolModeToString;
            static const boost::unordered_map<std::string, PoolMode> stringToPoolMode;
            static const boost::unordered_map<PoolState, std::string> poolStateToString;
            static const boost::unordered_map<std::string, PoolState> stringToPoolState;
            static const boost::unordered_map<LinkActionType, std::string> linkActionTypeToString;
            static const boost::unordered_map<std::string, LinkActionType> stringToLinkActionType;
            static const boost::unordered_map<LinkActionConditionType, std::string> linkActionConditionTypeToString;
            static const boost::unordered_map<std::string, LinkActionConditionType> stringToLinkActionConditionType;
            
            static const boost::unordered_map<SecurableComponentType, std::string> securableComponentTypeToString;
            static const boost::unordered_map<std::string, SecurableComponentType> stringToSecurableComponentType;
            static const boost::unordered_map<CacheEvictionType, std::string> cacheEvictionTypeToString;
            static const boost::unordered_map<std::string, CacheEvictionType> stringToCacheEvictionType;
            static const boost::unordered_map<HashAlgorithmType, std::string> hashAlgorithmTypeToString;
            static const boost::unordered_map<std::string, HashAlgorithmType> stringToHashAlgorithmType;
    };
}

#endif	/* TOOLS_H */

