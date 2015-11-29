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

#ifndef UTILITIES_STRINGS_INSTRUCTIONS_H
#define	UTILITIES_STRINGS_INSTRUCTIONS_H

#include <string>
#include <boost/unordered_map.hpp>
#include "../../InstructionManagement/Types/Types.h"

using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::NetworkManagerConnectionLifeCycleInstructionType;
using InstructionManagement_Types::DatabaseManagerInstructionType;
using InstructionManagement_Types::SessionManagerInstructionType;
using InstructionManagement_Types::UserManagerAdminInstructionType;
using InstructionManagement_Types::UserManagerSelfInstructionType;
using InstructionManagement_Types::DeviceManagerAdminInstructionType;
using InstructionManagement_Types::DeviceManagerUserInstructionType;
using InstructionManagement_Types::DatabaseLoggerInstructionType;

namespace Utilities
{
    namespace Strings
    {
        struct InstructionsMaps
        {
            static const boost::unordered_map<InstructionSetType, std::string> instructionSetTypeToString;
            static const boost::unordered_map<std::string, InstructionSetType> stringToInstructionSetType;
            
            static const boost::unordered_map<DatabaseManagerInstructionType, std::string> databaseManagerInstructionTypeToString;
            static const boost::unordered_map<std::string, DatabaseManagerInstructionType> stringToDatabaseManagerInstructionType;
            static const boost::unordered_map<SessionManagerInstructionType, std::string> sessionManagerInstructionTypeToString;
            static const boost::unordered_map<std::string, SessionManagerInstructionType> stringToSessionManagerInstructionType;
            static const boost::unordered_map<UserManagerAdminInstructionType, std::string> userManagerAdminInstructionTypeToString;
            static const boost::unordered_map<std::string, UserManagerAdminInstructionType> stringToUserManagerAdminInstructionType;
            static const boost::unordered_map<UserManagerSelfInstructionType, std::string> userManagerSelfInstructionTypeToString;
            static const boost::unordered_map<std::string, UserManagerSelfInstructionType> stringToUserManagerSelfInstructionType;
            static const boost::unordered_map<DeviceManagerAdminInstructionType, std::string> deviceManagerAdminInstructionTypeToString;
            static const boost::unordered_map<std::string, DeviceManagerAdminInstructionType> stringToDeviceManagerAdminInstructionType;
            static const boost::unordered_map<DeviceManagerUserInstructionType, std::string> deviceManagerUserInstructionTypeToString;
            static const boost::unordered_map<std::string, DeviceManagerUserInstructionType> stringToDeviceManagerUserInstructionType;
            static const boost::unordered_map<DatabaseLoggerInstructionType, std::string> databaseLoggerInstructionTypeToString;
            static const boost::unordered_map<std::string, DatabaseLoggerInstructionType> stringToDatabaseLoggerInstructionType;
            static const boost::unordered_map<NetworkManagerConnectionLifeCycleInstructionType, std::string> networkManagerConnectionLifeCycleInstructionTypeToString;
            static const boost::unordered_map<std::string, NetworkManagerConnectionLifeCycleInstructionType> stringToNetworkManagerConnectionLifeCycleInstructionType;
        };
        
        std::string toString(InstructionSetType var);
        InstructionSetType toInstructionSetType(std::string var);
        
        std::string toString(DatabaseManagerInstructionType var);
        DatabaseManagerInstructionType toDatabaseManagerInstructionType(std::string var);
        std::string toString(SessionManagerInstructionType var);
        SessionManagerInstructionType toSessionManagerInstructionType(std::string var);
        std::string toString(UserManagerAdminInstructionType var);
        UserManagerAdminInstructionType toUserManagerAdminInstructionType(std::string var);
        std::string toString(UserManagerSelfInstructionType var);
        UserManagerSelfInstructionType toUserManagerSelfInstructionType(std::string var);
        std::string toString(DeviceManagerAdminInstructionType var);
        DeviceManagerAdminInstructionType toDeviceManagerAdminInstructionType(std::string var);
        std::string toString(DeviceManagerUserInstructionType var);
        DeviceManagerUserInstructionType toDeviceManagerUserInstructionType(std::string var);
        std::string toString(DatabaseLoggerInstructionType var);
        DatabaseLoggerInstructionType toDatabaseLoggerInstructionType(std::string var);
        NetworkManagerConnectionLifeCycleInstructionType toNetworkManagerConnectionLifeCycleInstructionType(std::string var);
        std::string toString(NetworkManagerConnectionLifeCycleInstructionType var);
    }
}

#endif	/* UTILITIES_STRINGS_INSTRUCTIONS_H */
