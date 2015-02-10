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

#include "UserManagerInstructionSet.h"

namespace InstructionManagement_Sets
{
    template <>
    void InstructionSet<UserManagerAdminInstructionType>::buildTable()
    {
        instructionHandlers.insert({UserManagerAdminInstructionType::GET_USER,                              &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT,               &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::ADD_USER,                              &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::REMOVE_USER,                           &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::RESET_PASSWORD,                        &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::FORCE_PASSWORD_RESET,                  &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::LOCK_USER,                             &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::UNLOCK_USER,                           &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL,                   &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,  &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE,                &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE,             &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES,             &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerAdminInstructionType::DEBUG_GET_STATE,                       &InstructionManagement_Sets::InstructionSet<UserManagerAdminInstructionType>::instructionNotSet});
    }
    
    template <>
    void InstructionSet<UserManagerSelfInstructionType>::buildTable()
    {
        instructionHandlers.insert({UserManagerSelfInstructionType::GET_USER,       &InstructionManagement_Sets::InstructionSet<UserManagerSelfInstructionType>::instructionNotSet});
        instructionHandlers.insert({UserManagerSelfInstructionType::RESET_PASSWORD, &InstructionManagement_Sets::InstructionSet<UserManagerSelfInstructionType>::instructionNotSet});
    }
}