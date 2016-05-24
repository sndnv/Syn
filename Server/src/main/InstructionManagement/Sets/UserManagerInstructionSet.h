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

#ifndef USERMANAGERINSTRUCTIONSET_H
#define	USERMANAGERINSTRUCTIONSET_H

#include <vector>
#include <string>
#include <boost/any.hpp>

#include "../../Utilities/Strings/Instructions.h"
#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../EntityManagement/Types/Types.h"
#include "../../SecurityManagement/Rules/AuthorizationRules.h"
#include "../../DatabaseManagement/Types/Types.h"
#include "../../DatabaseManagement/Containers/UserDataContainer.h"

using Common_Types::UserID;
using Common_Types::UserAccessLevel;
using Common_Types::INVALID_USER_ID;
using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::UserManagerAdminInstructionType;
using InstructionManagement_Types::UserManagerSelfInstructionType;
using SecurityManagement_Rules::UserAuthorizationRule;
using DatabaseManagement_Containers::UserDataContainerPtr;
using DatabaseManagement_Types::DatabaseSelectConstraints;

namespace InstructionManagement_Sets
{
    namespace UserManagerInstructions
    {
        struct AdminGetUser : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminGetUser(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::GET_USER),
              userID(id), username("")
            {}
            
            explicit AdminGetUser(const std::string & user)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::GET_USER),
              userID(INVALID_USER_ID), username(user)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID || !username.empty()); }
            UserID userID;
            std::string username;
        };
        
        struct AdminGetUsersByConstraint : public Instruction<UserManagerAdminInstructionType>
        {
            AdminGetUsersByConstraint(DatabaseSelectConstraints::USERS type, boost::any value)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::USERS constraintType;
            boost::any constraintValue;
        };
        
        struct AdminAddUser : public Instruction<UserManagerAdminInstructionType>
        {
            AdminAddUser(const std::string & user, const std::string & password, UserAccessLevel access, bool forcePassReset)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::ADD_USER),
              username(user), rawPassword(password), accessLevel(access), forcePasswordReset(forcePassReset)
            {}
            
            bool isValid() override { return (!username.empty() && !rawPassword.empty() && accessLevel != UserAccessLevel::INVALID); }
            
            std::string username;
            const std::string rawPassword;
            UserAccessLevel accessLevel;
            bool forcePasswordReset;
        };
        
        struct AdminRemoveUser : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminRemoveUser(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::REMOVE_USER),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct AdminResetPassword : public Instruction<UserManagerAdminInstructionType>
        {
            AdminResetPassword(UserID id, const std::string & password)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::RESET_PASSWORD),
              userID(id), rawPassword(password)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID && !rawPassword.empty()); }
            UserID userID;
            const std::string rawPassword;
        };
        
        struct AdminForcePasswordReset : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminForcePasswordReset(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::FORCE_PASSWORD_RESET),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct AdminLockUser : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminLockUser(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::LOCK_USER),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct AdminUnlockUser : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminUnlockUser(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::UNLOCK_USER),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct AdminUpdateAccessLevel : public Instruction<UserManagerAdminInstructionType>
        {
            AdminUpdateAccessLevel(UserID id, UserAccessLevel newLevel)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL),
              userID(id), level(newLevel)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
            UserAccessLevel level;
        };
        
        struct AdminResetFailedAuthenticationAttempts : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminResetFailedAuthenticationAttempts(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct AdminAddAuthorizationRule : public Instruction<UserManagerAdminInstructionType>
        {
            AdminAddAuthorizationRule(UserID id, UserAuthorizationRule newRule)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE),
              userID(id), rule(newRule)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
            UserAuthorizationRule rule;
        };
        
        struct AdminRemoveAuthorizationRule : public Instruction<UserManagerAdminInstructionType>
        {
            AdminRemoveAuthorizationRule(UserID id, UserAuthorizationRule oldRule)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE),
              userID(id), rule(oldRule)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
            UserAuthorizationRule rule;
        };
        
        struct AdminClearAuthorizationRules : public Instruction<UserManagerAdminInstructionType>
        {
            explicit AdminClearAuthorizationRules(UserID id)
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES),
              userID(id)
            {}
            
            bool isValid() override { return (userID != INVALID_USER_ID); }
            UserID userID;
        };
        
        struct DebugGetState : public Instruction<UserManagerAdminInstructionType>
        {
            DebugGetState()
            : Instruction(InstructionSetType::USER_MANAGER_ADMIN, UserManagerAdminInstructionType::DEBUG_GET_STATE)
            {}
            
            bool isValid() override { return true; }
        };
        
        struct SelfResetPassword : public Instruction<UserManagerSelfInstructionType>
        {
            explicit SelfResetPassword(const std::string & password)
            : Instruction(InstructionSetType::USER_MANAGER_SELF, UserManagerSelfInstructionType::RESET_PASSWORD),
              rawPassword(password)
            {}
            
            bool isValid() override { return !rawPassword.empty(); }
            const std::string rawPassword;
        };
        
        struct SelfGetUser : public Instruction<UserManagerSelfInstructionType>
        {
            SelfGetUser()
            : Instruction(InstructionSetType::USER_MANAGER_SELF, UserManagerSelfInstructionType::GET_USER)
            {}
            
            bool isValid() override { return true; }
        };
        
        namespace Results
        {
            struct AdminGetUser : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminGetUser(UserDataContainerPtr input)
                : InstructionResult(UserManagerAdminInstructionType::GET_USER), result(input) {}
                
                UserDataContainerPtr result;
            };
            
            struct AdminGetUsersByConstraint : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminGetUsersByConstraint(std::vector<UserDataContainerPtr> input)
                : InstructionResult(UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT), result(input) {}
                
                std::vector<UserDataContainerPtr> result;
            };
            
            struct AdminAddUser : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminAddUser(bool input)
                : InstructionResult(UserManagerAdminInstructionType::ADD_USER), result(input) {}
                
                bool result;
            };
            
            struct AdminRemoveUser : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminRemoveUser(bool input)
                : InstructionResult(UserManagerAdminInstructionType::REMOVE_USER), result(input) {}
                
                bool result;
            };
            
            struct AdminResetPassword : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminResetPassword(bool input)
                : InstructionResult(UserManagerAdminInstructionType::RESET_PASSWORD), result(input) {}
                
                bool result;
            };
            
            struct AdminForcePasswordReset : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminForcePasswordReset(bool input)
                : InstructionResult(UserManagerAdminInstructionType::FORCE_PASSWORD_RESET), result(input) {}
                
                bool result;
            };
            
            struct AdminLockUser : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminLockUser(bool input)
                : InstructionResult(UserManagerAdminInstructionType::LOCK_USER), result(input) {}
                
                bool result;
            };
            
            struct AdminUnlockUser : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminUnlockUser(bool input)
                : InstructionResult(UserManagerAdminInstructionType::UNLOCK_USER), result(input) {}
                
                bool result;
            };
            
            struct AdminUpdateAccessLevel : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminUpdateAccessLevel(bool input)
                : InstructionResult(UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL), result(input) {}
                
                bool result;
            };
            
            struct AdminResetFailedAuthenticationAttempts : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminResetFailedAuthenticationAttempts(bool input)
                : InstructionResult(UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS), result(input) {}
                
                bool result;
            };
            
            struct AdminAddAuthorizationRule : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminAddAuthorizationRule(bool input)
                : InstructionResult(UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE), result(input) {}
                
                bool result;
            };
            
            struct AdminRemoveAuthorizationRule : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminRemoveAuthorizationRule(bool input)
                : InstructionResult(UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE), result(input) {}
                
                bool result;
            };
            
            struct AdminClearAuthorizationRules : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit AdminClearAuthorizationRules(bool input)
                : InstructionResult(UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES), result(input) {}
                
                bool result;
            };
            
            struct SelfResetPassword : public InstructionResult<UserManagerSelfInstructionType>
            {
                explicit SelfResetPassword(bool input)
                : InstructionResult(UserManagerSelfInstructionType::GET_USER), result(input) {}
                
                bool result;
            };
            
            struct SelfGetUser : public InstructionResult<UserManagerSelfInstructionType>
            {
                explicit SelfGetUser(UserDataContainerPtr input)
                : InstructionResult(UserManagerSelfInstructionType::RESET_PASSWORD), result(input) {}
                
                UserDataContainerPtr result;
            };
            
            struct DebugGetState : public InstructionResult<UserManagerAdminInstructionType>
            {
                explicit DebugGetState(std::string input)
                : InstructionResult(UserManagerAdminInstructionType::DEBUG_GET_STATE), result(input) {}
                
                std::string result;
            };
        }
    }
}

#endif	/* USERMANAGERINSTRUCTIONSET_H */

