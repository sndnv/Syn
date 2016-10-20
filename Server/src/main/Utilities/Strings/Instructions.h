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
#include "../../InstructionManagement/Types/Types.h"

using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::TestInstructionType;
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
        std::string toString(InstructionSetType var);
        InstructionSetType toInstructionSetType(std::string var);
        std::string toString(TestInstructionType var);
        TestInstructionType toTestInstructionType(std::string var);
        
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
