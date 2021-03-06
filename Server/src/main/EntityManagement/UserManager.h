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

#ifndef ENTITY_MANAGEMENT_USERS_H
#define	ENTITY_MANAGEMENT_USERS_H

#include <string>

#include <boost/thread/mutex.hpp>

#include "../Common/Types.h"
#include "../Utilities/FileLogger.h"
#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Types/SecurityTokens.h"
#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Types/Exceptions.h"
#include "../SecurityManagement/Interfaces/Securable.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../DatabaseManagement/Containers/UserDataContainer.h"
#include "../InstructionManagement/Sets/UserManagerInstructionSet.h"

#include "Interfaces/DatabaseLoggingSource.h"

//Database and Security Management
using SyncServer_Core::DatabaseManager;
using SyncServer_Core::SecurityManager;

//Exceptions
using SecurityManagement_Types::InvalidAuthorizationTokenException;

//Misc
using Common_Types::LogSeverity;
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::INVALID_TOKEN_ID;
using SecurityManagement_Types::AuthorizationTokenPtr;
using DatabaseManagement_Containers::UserDataContainer;
using DatabaseManagement_Containers::UserDataContainerPtr;
using InstructionManagement_Types::UserManagerAdminInstructionType;
using InstructionManagement_Types::UserManagerSelfInstructionType;

namespace EntityManagement
{
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class UserManagerAdminInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<UserManagerAdminInstructionType>
    {
        public:
            virtual ~UserManagerAdminInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::USER_MANAGER_ADMIN;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class UserManagerSelfInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<UserManagerSelfInstructionType>
    {
        public:
            virtual ~UserManagerSelfInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::USER_MANAGER_SELF;
            }
    };
    
    /**
     * Class for managing user-related activities.
     */
    class UserManager final
    : public SecurityManagement_Interfaces::Securable,
      public UserManagerAdminInstructionTarget,
      public UserManagerSelfInstructionTarget,
      public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure holding <code>UserManager</code> configuration. */
            struct UserManagerParameters
            {
                /** Reference to a valid database manager instance */
                DatabaseManager & databaseManager;
                /** Reference to a valid security manager instance */
                SecurityManager & securityManager;
            };
            
            /**
             * Constructs a new user manager object with the specified configuration.
             * 
             * @param params the manager configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             */
            UserManager(const UserManagerParameters & params,  Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Clears all data structures.
             */
            ~UserManager();
            
            UserManager() = delete;
            UserManager(const UserManager&) = delete;
            UserManager& operator=(const UserManager&) = delete;
            
            void postAuthorizationToken(const SecurityManagement_Types::AuthorizationTokenPtr token) override;
            
            SecurityManagement_Types::SecurableComponentType getComponentType() const override
            {
                return SecurityManagement_Types::SecurableComponentType::USER_MANAGER;
            }
            
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<UserManagerAdminInstructionType> set) const override;
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<UserManagerSelfInstructionType> set) const override;
            
            std::string getSourceName() const override
            {
                return "UserManager";
            }
            
            bool registerLoggingHandler(const std::function<void(LogSeverity, const std::string &)> handler) override
            {
                if(!dbLogHandler)
                {
                    dbLogHandler = handler;
                    return true;
                }
                else
                {
                    logMessage(LogSeverity::Error, "(registerLoggingHandler) > The database logging handler is already set.");
                    return false;
                }
            }
            
        private:
            Utilities::FileLoggerPtr debugLogger;//logger for debugging
            std::function<void(LogSeverity, const std::string &)> dbLogHandler;//database log handler
            
            //Required Managers
            DatabaseManager & databaseManager;
            SecurityManager & securityManager;
            
            //Instruction Management
            boost::mutex instructionDataMutex;  //instruction data mutex
            boost::unordered_map<TokenID, AuthorizationTokenPtr> authorizationTokens; //expected authorization tokens
            
            //Stats
            unsigned long instructionsReceived; //number of instructions received by manager
            unsigned long instructionsProcessed;//number of instructions processed by manager
            
            /**
             * Sets an exception with the specified message in the supplied 
             * instruction's promise.
             * 
             * Note: Always sets <code>std::runtime_error</code> exception.
             * 
             * @param message the message for the exception
             * @param instruction the instruction in which the exception is to be set
             */
            void throwInstructionException(const std::string & message, InstructionPtr<UserManagerAdminInstructionType> instruction)
            {
                try
                {
                    boost::throw_exception(std::runtime_error(message));
                }
                catch(const std::runtime_error &)
                {
                    instruction->getPromise().set_exception(boost::current_exception());
                    return;
                }
            }
            
            /**
             * Sets an exception with the specified message in the supplied 
             * instruction's promise.
             * 
             * Note: Always sets <code>std::runtime_error</code> exception.
             * 
             * @param message the message for the exception
             * @param instruction the instruction in which the exception is to be set
             */
            void throwInstructionException(const std::string & message, InstructionPtr<UserManagerSelfInstructionType> instruction)
            {
                try
                {
                    boost::throw_exception(std::runtime_error(message));
                }
                catch(const std::runtime_error &)
                {
                    instruction->getPromise().set_exception(boost::current_exception());
                    return;
                }
            }
            
            //Instruction Handlers (admin)
            void adminGetUserHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminGetUsersHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminAddUserHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminRemoveUserHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminResetPasswordHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminForcePasswordResetHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminLockUserHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminUnlockUserHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminUpdateAccessLevel(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminResetFailedAuthenticationAttemptsHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminAddAuthorizationRuleHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminRemoveAuthorizationRuleHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void adminClearAuthorizationRulesHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            void debugGetStateHandler(InstructionPtr<UserManagerAdminInstructionType> instruction);
            
            //Instruction Handlers (self)
            void selfGetUserHandler(InstructionPtr<UserManagerSelfInstructionType> instruction);
            void selfResetPasswordHandler(InstructionPtr<UserManagerSelfInstructionType> instruction);
            
            //Instruction Handlers Function Binds
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminGetUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminGetUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminGetUsersHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminGetUsersHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminAddUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminAddUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminRemoveUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminRemoveUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminResetPasswordHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminResetPasswordHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminForcePasswordResetHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminForcePasswordResetHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminLockUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminLockUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminUnlockUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminUnlockUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminUpdateAccessLevelBind =
                boost::bind(&EntityManagement::UserManager::adminUpdateAccessLevel, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminResetFailedAuthenticationAttemptsHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminResetFailedAuthenticationAttemptsHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminAddAuthorizationRuleHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminAddAuthorizationRuleHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminRemoveAuthorizationRuleHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminRemoveAuthorizationRuleHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> adminClearAuthorizationRulesHandlerBind =
                boost::bind(&EntityManagement::UserManager::adminClearAuthorizationRulesHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerAdminInstructionType>)> debugGetStateHandlerBind =
                boost::bind(&EntityManagement::UserManager::debugGetStateHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerSelfInstructionType>)> selfGetUserHandlerBind =
                boost::bind(&EntityManagement::UserManager::selfGetUserHandler, this, _1);
            
            std::function<void(InstructionPtr<UserManagerSelfInstructionType>)> selfResetPasswordHandlerBind =
                boost::bind(&EntityManagement::UserManager::selfResetPasswordHandler, this, _1);
            
            /**
             * Verifies the supplied authentication token.
             * 
             * Note: The token is removed the the list of expected authorization tokens
             * 
             * @param token the token to be verified
             * 
             * @throw InvalidAuthorizationTokenException if an invalid token is encountered
             */
            void verifyAuthorizationToken(AuthorizationTokenPtr token);
            
            /**
             * Logs the specified message, if the database log handler is set.
             * 
             * Note: If a debugging file logger is assigned, the message is sent to it.
             * 
             * @param severity the severity associated with the message/event
             * @param message the message to be logged
             */
            void logMessage(LogSeverity severity, const std::string & message) const
            {
                if(dbLogHandler)
                    dbLogHandler(severity, message);
                
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "UserManager " + message);
            }
    };
}
#endif	/* ENTITY_MANAGEMENT_USERS_H */

