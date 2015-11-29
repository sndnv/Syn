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

#include "DeviceManagerInstructionSet.h"

namespace InstructionManagement_Sets
{
    template <>
    void InstructionSet<DeviceManagerAdminInstructionType>::buildTable()
    {
        instructionHandlers.insert({DeviceManagerAdminInstructionType::GET_DEVICE,              &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT, &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::ADD_DEVICE,              &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::REMOVE_DEVICE,           &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD,   &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO,  &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO,     &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::LOCK_DEVICE,             &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::UNLOCK_DEVICE,           &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS, &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerAdminInstructionType::DEBUG_GET_STATE,         &InstructionManagement_Sets::InstructionSet<DeviceManagerAdminInstructionType>::instructionNotSet});
    }
    
    template <>
    void InstructionSet<DeviceManagerUserInstructionType>::buildTable()
    {
        instructionHandlers.insert({DeviceManagerUserInstructionType::GET_DEVICE,               &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::GET_DEVICES,              &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::ADD_DEVICE,               &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::REMOVE_DEVICE,            &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD,    &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO,   &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO,      &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::LOCK_DEVICE,              &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::UNLOCK_DEVICE,            &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
        instructionHandlers.insert({DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS, &InstructionManagement_Sets::InstructionSet<DeviceManagerUserInstructionType>::instructionNotSet});
    }
}