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

#ifndef ENTITY_MANAGEMENT_DEVICES_H
#define	ENTITY_MANAGEMENT_DEVICES_H
#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/FileLogger.h"
#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Types/SecurityTokens.h"
#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Types/Exceptions.h"
#include "../SecurityManagement/Interfaces/Securable.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"
#include "../InstructionManagement/Sets/DeviceManagerInstructionSet.h"

#include "Types/Types.h"

//Database and Security Management
using SyncServer_Core::DatabaseManager;
using SyncServer_Core::SecurityManager;

//Exceptions
using SecurityManagement_Types::InvalidAuthorizationTokenException;

//Misc
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::INVALID_TOKEN_ID;
using SecurityManagement_Types::AuthorizationTokenPtr;
using DatabaseManagement_Containers::DeviceDataContainer;
using DatabaseManagement_Containers::DeviceDataContainerPtr;
using InstructionManagement_Types::DeviceManagerAdminInstructionType;
using InstructionManagement_Types::DeviceManagerUserInstructionType;

namespace EntityManagement
{
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class DeviceManagerAdminInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<DeviceManagerAdminInstructionType>
    {
        public:
            virtual ~DeviceManagerAdminInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const
            {
                return InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_ADMIN;
            }
    };
    
    /**
     * Class for enabling support for multiple instruction sets by a single target.
     */
    class DeviceManagerUserInstructionTarget
    : public InstructionManagement_Interfaces::InstructionTarget<DeviceManagerUserInstructionType>
    {
        public:
            virtual ~DeviceManagerUserInstructionTarget() {}
            
            InstructionManagement_Types::InstructionSetType getType() const
            {
                return InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_USER;
            }
    };
    
    /**
     * Class for managing device-related activities.
     */
    class DeviceManager
    : public SecurityManagement_Interfaces::Securable,
      public DeviceManagerAdminInstructionTarget,
      public DeviceManagerUserInstructionTarget
    {
        public:
            /** Parameters structure holding <code>DeviceManager</code> configuration. */
            struct DeviceManagerParameters
            {
                /** Reference to a valid database manager instance */
                DatabaseManager & databaseManager;
                /** Reference to a valid security manager instance */
                SecurityManager & securityManager;
            };

            /**
             * Constructs a new device manager object with the specified configuration.
             * 
             * @param params the manager configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             */
            DeviceManager(const DeviceManagerParameters & params,  Utilities::FileLogger * debugLogger = nullptr);

            /**
             * Clears all data structures.
             */
            ~DeviceManager();

            DeviceManager() = delete;
            DeviceManager(const DeviceManager&) = delete;
            DeviceManager& operator=(const DeviceManager&) = delete;

            void postAuthorizationToken(const SecurityManagement_Types::AuthorizationTokenPtr token);

            SecurityManagement_Types::SecurableComponentType getComponentType() const
            {
                return SecurityManagement_Types::SecurableComponentType::DEVICE_MANAGER;
            }

            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<DeviceManagerAdminInstructionType> set) const;
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<DeviceManagerUserInstructionType> set) const;

        private:
            Utilities::FileLogger * debugLogger;//logger for debugging
            
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
            void throwInstructionException(const std::string & message, InstructionPtr<DeviceManagerAdminInstructionType> instruction)
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
            void throwInstructionException(const std::string & message, InstructionPtr<DeviceManagerUserInstructionType> instruction)
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
             * Attempts to add a new device with the specified parameters.
             * 
             * @param name device name
             * @param password raw device password
             * @param owner user ID of the device's owner
             * @param xferType data transfer type for the device
             * 
             * @return <code>true</code> if the operation completed successfully
             * 
             * @throw runtime_error if the specified device name is not valid
             */
            bool addDeviceOperation(const std::string & name, const std::string & password, UserID owner, DataTransferType xferType);
            
            //Instruction Handlers (admin)
            void adminGetDeviceHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminGetDevicesByConstraintHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminAddDeviceHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminRemoveDeviceHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminResetDevicePasswordHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminUpdateConnectionInfoHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminUpdateGeneralInfoHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminLockDeviceHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminUnlockDeviceHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void adminResetFailedAuthenticationAttemptsHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            void debugGetStateHandler(InstructionPtr<DeviceManagerAdminInstructionType> instruction);
            
            //Instruction Handlers (user)
            void userGetDeviceHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userGetDevicesHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userAddDeviceHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userRemoveDeviceHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userResetDevicePasswordHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userUpdateConnectionInfoHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userUpdateGeneralInfoHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userLockDeviceHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userUnlockDeviceHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            void userResetFailedAuthenticationAttemptsHandler(InstructionPtr<DeviceManagerUserInstructionType> instruction);
            
            //Instruction Handlers Function Binds
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminGetDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminGetDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminGetDevicesByConstraintHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminGetDevicesByConstraintHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminAddDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminAddDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminRemoveDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminRemoveDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminResetDevicePasswordHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminResetDevicePasswordHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminUpdateConnectionInfoHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminUpdateConnectionInfoHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminUpdateGeneralInfoHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminUpdateGeneralInfoHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminLockDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminLockDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminUnlockDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminUnlockDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> adminResetFailedAuthenticationAttemptsHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::adminResetFailedAuthenticationAttemptsHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerAdminInstructionType>)> debugGetStateHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::debugGetStateHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userGetDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userGetDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userGetDevicesHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userGetDevicesHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userAddDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userAddDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userRemoveDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userRemoveDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userResetDevicePasswordHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userResetDevicePasswordHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userUpdateConnectionInfoHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userUpdateConnectionInfoHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userUpdateGeneralInfoHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userUpdateGeneralInfoHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userLockDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userLockDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userUnlockDeviceHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userUnlockDeviceHandler, this, _1);
            
            std::function<void(InstructionPtr<DeviceManagerUserInstructionType>)> userResetFailedAuthenticationAttemptsHandlerBind =
                boost::bind(&EntityManagement::DeviceManager::userResetFailedAuthenticationAttemptsHandler, this, _1);
            
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
             * Logs the specified message, if a debugging file logger is assigned to the manager.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string message) const
            {
                if(debugLogger != nullptr)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "DeviceManager " + message);
            }
    };

}
#endif	/* ENTITY_MANAGEMENT_DEVICES_H */

