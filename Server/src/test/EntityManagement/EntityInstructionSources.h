/**
 * Copyright (C) 2016 https://github.com/sndnv
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

#ifndef ENTITYINSTRUCTIONSOURCES_H
#define ENTITYINSTRUCTIONSOURCES_H

#include <boost/shared_ptr.hpp>
#include "../../main/EntityManagement/DatabaseLogger.h"
#include "../../main/InstructionManagement/Sets/DatabaseLoggerInstructionSet.h"
#include "../../main/InstructionManagement/Sets/DeviceManagerInstructionSet.h"
#include "../../main/InstructionManagement/Sets/UserManagerInstructionSet.h"
#include "../../main/InstructionManagement/Sets/InstructionSet.h"
#include "../../main/InstructionManagement/Interfaces/InstructionSource.h"

using InstructionManagement_Sets::InstructionBasePtr;
using EntityManagement::DatabaseLogger;
using EntityManagement::DeviceManager;
using EntityManagement::UserManager;
namespace DatabaseLoggerInstructions = InstructionManagement_Sets::DatabaseLoggerInstructions;
namespace DeviceManagerInstructions = InstructionManagement_Sets::DeviceManagerInstructions;
namespace UserManagerInstructions = InstructionManagement_Sets::UserManagerInstructions;

namespace Testing
{
    class DatabaseLoggerInstructionSource : public InstructionManagement_Interfaces::InstructionSource
    {
        public:
            DatabaseLoggerInstructionSource(SyncServer_Core::SecurityManager * security, DatabaseLogger * logger, UserID adminID)
            : securityManager(security), testLogger(logger), testAdminID(adminID)
            {}
            
            boost::shared_ptr<DatabaseLoggerInstructions::Results::GetLogsByConstraint>
            doInstruction_GetLogsByConstraint(DatabaseSelectConstraints::LOGS type, boost::any value)
            {
                boost::shared_ptr<DatabaseLoggerInstructions::GetLogsByConstraint> testInstruction(
                    new DatabaseLoggerInstructions::GetLogsByConstraint(type, value));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testLogger, SecurableComponentType::DATABASE_LOGGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DatabaseLoggerInstructions::Results::GetLogsByConstraint>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DatabaseLoggerInstructions::Results::UpdateSourceLoggingLevel>
            doInstruction_UpdateSourceLoggingLevel(DatabaseLoggingSourceID id, LogSeverity severity)
            {
                boost::shared_ptr<DatabaseLoggerInstructions::UpdateSourceLoggingLevel> testInstruction(
                    new DatabaseLoggerInstructions::UpdateSourceLoggingLevel(id, severity));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testLogger, SecurableComponentType::DATABASE_LOGGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DatabaseLoggerInstructions::Results::UpdateSourceLoggingLevel>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DatabaseLoggerInstructions::Results::UpdateDefaultLoggingLevel>
            doInstruction_UpdateDefaultLoggingLevel(LogSeverity severity)
            {
                boost::shared_ptr<DatabaseLoggerInstructions::UpdateDefaultLoggingLevel> testInstruction(
                    new DatabaseLoggerInstructions::UpdateDefaultLoggingLevel(severity));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testLogger, SecurableComponentType::DATABASE_LOGGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DatabaseLoggerInstructions::Results::UpdateDefaultLoggingLevel>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DatabaseLoggerInstructions::Results::DebugGetState>
            doInstruction_DebugGetState()
            {
                boost::shared_ptr<DatabaseLoggerInstructions::DebugGetState> testInstruction(
                    new DatabaseLoggerInstructions::DebugGetState());
            
                AuthorizationRequest authorizationRequest(testAdminID, *testLogger, SecurableComponentType::DATABASE_LOGGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DatabaseLoggerInstructions::Results::DebugGetState>(testInstruction->getFuture().get());
            }
            
            bool registerInstructionHandler(
                    const std::function<void(InstructionBasePtr,
                    SecurityManagement_Types::AuthorizationTokenPtr)> handler) override
            {
                hndlr = handler;
                return true;
            }

            std::vector<InstructionManagement_Types::InstructionSetType> getRequiredInstructionSetTypes() override
            {
                return std::vector<InstructionManagement_Types::InstructionSetType>(
                {
                    InstructionManagement_Types::InstructionSetType::DATABASE_LOGGER
                });
            }

        private:
            std::function<void(InstructionBasePtr, SecurityManagement_Types::AuthorizationTokenPtr)> hndlr;
            SyncServer_Core::SecurityManager * securityManager;
            DatabaseLogger * testLogger;
            UserID testAdminID;
    };
    
    class DeviceManagerInstructionSource : public InstructionManagement_Interfaces::InstructionSource
    {
        public:
            DeviceManagerInstructionSource(SyncServer_Core::SecurityManager * security, DeviceManager * manager, UserID adminID, UserID userID)
            : securityManager(security), testManager(manager), testAdminID(adminID), testUserID(userID)
            {}
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminGetDevice>
            doInstruction_AdminGetDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminGetDevice> testInstruction(
                    new DeviceManagerInstructions::AdminGetDevice(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminGetDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminGetDevicesByConstraint>
            doInstruction_AdminGetDevicesByConstraint(DatabaseSelectConstraints::DEVICES type, boost::any value)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminGetDevicesByConstraint> testInstruction(
                    new DeviceManagerInstructions::AdminGetDevicesByConstraint(type, value));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminGetDevicesByConstraint>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminAddDevice>
            doInstruction_AdminAddDevice(const std::string & name, const std::string & password, UserID owner, DataTransferType xferType, PeerType type)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminAddDevice> testInstruction(
                    new DeviceManagerInstructions::AdminAddDevice(name, password, owner, xferType, type));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminAddDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminRemoveDevice>
            doInstruction_AdminRemoveDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminRemoveDevice> testInstruction(
                    new DeviceManagerInstructions::AdminRemoveDevice(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminRemoveDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminResetDevicePassword>
            doInstruction_AdminResetDevicePassword(DeviceID id, const std::string & password)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminResetDevicePassword> testInstruction(
                    new DeviceManagerInstructions::AdminResetDevicePassword(id, password));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminResetDevicePassword>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminUpdateConnectionInfo>
            doInstruction_AdminUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminUpdateConnectionInfo> testInstruction(
                    new DeviceManagerInstructions::AdminUpdateConnectionInfo(id, ip, port, xferType));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminUpdateConnectionInfo>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminUpdateGeneralInfo>
            doInstruction_AdminUpdateGeneralInfo(DeviceID id, const std::string & name, const std::string & info)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminUpdateGeneralInfo> testInstruction(
                    new DeviceManagerInstructions::AdminUpdateGeneralInfo(id, name, info));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminUpdateGeneralInfo>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminLockDevice>
            doInstruction_AdminLockDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminLockDevice> testInstruction(
                    new DeviceManagerInstructions::AdminLockDevice(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminLockDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminUnlockDevice>
            doInstruction_AdminUnlockDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminUnlockDevice> testInstruction(
                    new DeviceManagerInstructions::AdminUnlockDevice(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminUnlockDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::AdminResetFailedAuthenticationAttempts>
            doInstruction_AdminResetFailedAuthenticationAttempts(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::AdminResetFailedAuthenticationAttempts> testInstruction(
                    new DeviceManagerInstructions::AdminResetFailedAuthenticationAttempts(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::AdminResetFailedAuthenticationAttempts>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::DebugGetState>
            doInstruction_DebugGetState()
            {
                boost::shared_ptr<DeviceManagerInstructions::DebugGetState> testInstruction(
                    new DeviceManagerInstructions::DebugGetState());
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::DebugGetState>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserGetDevice>
            doInstruction_UserGetDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserGetDevice> testInstruction(
                    new DeviceManagerInstructions::UserGetDevice(id));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserGetDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserGetDevices>
            doInstruction_UserGetDevices()
            {
                boost::shared_ptr<DeviceManagerInstructions::UserGetDevices> testInstruction(
                    new DeviceManagerInstructions::UserGetDevices());
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserGetDevices>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserAddDevice>
            doInstruction_UserAddDevice(const std::string & name, const std::string & password, DataTransferType xferType, PeerType type)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserAddDevice> testInstruction(
                    new DeviceManagerInstructions::UserAddDevice(name, password, xferType, type));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserAddDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserRemoveDevice>
            doInstruction_UserRemoveDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserRemoveDevice> testInstruction(
                    new DeviceManagerInstructions::UserRemoveDevice(id));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserRemoveDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserResetDevicePassword>
            doInstruction_UserResetDevicePassword(DeviceID id, const std::string & password)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserResetDevicePassword> testInstruction(
                    new DeviceManagerInstructions::UserResetDevicePassword(id, password));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserResetDevicePassword>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserUpdateConnectionInfo>
            doInstruction_UserUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserUpdateConnectionInfo> testInstruction(
                    new DeviceManagerInstructions::UserUpdateConnectionInfo(id, ip, port, xferType));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserUpdateConnectionInfo>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserUpdateGeneralInfo>
            doInstruction_UserUpdateGeneralInfo(DeviceID id, const std::string & name, const std::string & info)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserUpdateGeneralInfo> testInstruction(
                    new DeviceManagerInstructions::UserUpdateGeneralInfo(id, name, info));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserUpdateGeneralInfo>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserLockDevice>
            doInstruction_UserLockDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserLockDevice> testInstruction(
                    new DeviceManagerInstructions::UserLockDevice(id));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserLockDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserUnlockDevice>
            doInstruction_UserUnlockDevice(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserUnlockDevice> testInstruction(
                    new DeviceManagerInstructions::UserUnlockDevice(id));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserUnlockDevice>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<DeviceManagerInstructions::Results::UserResetFailedAuthenticationAttempts>
            doInstruction_UserResetFailedAuthenticationAttempts(DeviceID id)
            {
                boost::shared_ptr<DeviceManagerInstructions::UserResetFailedAuthenticationAttempts> testInstruction(
                    new DeviceManagerInstructions::UserResetFailedAuthenticationAttempts(id));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::DEVICE_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<DeviceManagerInstructions::Results::UserResetFailedAuthenticationAttempts>(testInstruction->getFuture().get());
            }
            
            bool registerInstructionHandler(
                    const std::function<void(InstructionBasePtr,
                    SecurityManagement_Types::AuthorizationTokenPtr)> handler) override
            {
                hndlr = handler;
                return true;
            }

            std::vector<InstructionManagement_Types::InstructionSetType> getRequiredInstructionSetTypes() override
            {
                return std::vector<InstructionManagement_Types::InstructionSetType>(
                {
                    InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_USER,
                    InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_ADMIN
                });
            }

        private:
            std::function<void(InstructionBasePtr, SecurityManagement_Types::AuthorizationTokenPtr)> hndlr;
            SyncServer_Core::SecurityManager * securityManager;
            DeviceManager * testManager;
            UserID testAdminID;
            UserID testUserID;
    };
    
    class UserManagerInstructionSource : public InstructionManagement_Interfaces::InstructionSource
    {
        public:
            UserManagerInstructionSource(SyncServer_Core::SecurityManager * security, UserManager * manager, UserID adminID, UserID userID)
            : securityManager(security), testManager(manager), testAdminID(adminID), testUserID(userID)
            {}
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminGetUser>
            doInstruction_AdminGetUser(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminGetUser> testInstruction(
                    new UserManagerInstructions::AdminGetUser(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminGetUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminGetUser>
            doInstruction_AdminGetUser(const std::string & user)
            {
                boost::shared_ptr<UserManagerInstructions::AdminGetUser> testInstruction(
                    new UserManagerInstructions::AdminGetUser(user));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminGetUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminGetUsersByConstraint>
            doInstruction_AdminGetUsersByConstraint(DatabaseSelectConstraints::USERS type, boost::any value)
            {
                boost::shared_ptr<UserManagerInstructions::AdminGetUsersByConstraint> testInstruction(
                    new UserManagerInstructions::AdminGetUsersByConstraint(type, value));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminGetUsersByConstraint>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminAddUser>
            doInstruction_AdminAddUser(const std::string & user, const std::string & password, UserAccessLevel access, bool forcePassReset)
            {
                boost::shared_ptr<UserManagerInstructions::AdminAddUser> testInstruction(
                    new UserManagerInstructions::AdminAddUser(user, password, access, forcePassReset));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminAddUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminRemoveUser>
            doInstruction_AdminRemoveUser(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminRemoveUser> testInstruction(
                    new UserManagerInstructions::AdminRemoveUser(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminRemoveUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminResetPassword>
            doInstruction_AdminResetPassword(UserID id, const std::string & password)
            {
                boost::shared_ptr<UserManagerInstructions::AdminResetPassword> testInstruction(
                    new UserManagerInstructions::AdminResetPassword(id, password));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminResetPassword>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminForcePasswordReset>
            doInstruction_AdminForcePasswordReset(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminForcePasswordReset> testInstruction(
                    new UserManagerInstructions::AdminForcePasswordReset(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminForcePasswordReset>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminLockUser>
            doInstruction_AdminLockUser(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminLockUser> testInstruction(
                    new UserManagerInstructions::AdminLockUser(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminLockUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminUnlockUser>
            doInstruction_AdminUnlockUser(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminUnlockUser> testInstruction(
                    new UserManagerInstructions::AdminUnlockUser(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminUnlockUser>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminUpdateAccessLevel>
            doInstruction_AdminUpdateAccessLevel(UserID id, UserAccessLevel newLevel)
            {
                boost::shared_ptr<UserManagerInstructions::AdminUpdateAccessLevel> testInstruction(
                    new UserManagerInstructions::AdminUpdateAccessLevel(id, newLevel));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminUpdateAccessLevel>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminResetFailedAuthenticationAttempts>
            doInstruction_AdminResetFailedAuthenticationAttempts(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminResetFailedAuthenticationAttempts> testInstruction(
                    new UserManagerInstructions::AdminResetFailedAuthenticationAttempts(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminResetFailedAuthenticationAttempts>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminAddAuthorizationRule>
            doInstruction_AdminAddAuthorizationRule(UserID id, UserAuthorizationRule newRule)
            {
                boost::shared_ptr<UserManagerInstructions::AdminAddAuthorizationRule> testInstruction(
                    new UserManagerInstructions::AdminAddAuthorizationRule(id, newRule));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminAddAuthorizationRule>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminRemoveAuthorizationRule>
            doInstruction_AdminRemoveAuthorizationRule(UserID id, UserAuthorizationRule oldRule)
            {
                boost::shared_ptr<UserManagerInstructions::AdminRemoveAuthorizationRule> testInstruction(
                    new UserManagerInstructions::AdminRemoveAuthorizationRule(id, oldRule));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminRemoveAuthorizationRule>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::AdminClearAuthorizationRules>
            doInstruction_AdminClearAuthorizationRules(UserID id)
            {
                boost::shared_ptr<UserManagerInstructions::AdminClearAuthorizationRules> testInstruction(
                    new UserManagerInstructions::AdminClearAuthorizationRules(id));
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::AdminClearAuthorizationRules>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::DebugGetState>
            doInstruction_DebugGetState()
            {
                boost::shared_ptr<UserManagerInstructions::DebugGetState> testInstruction(
                    new UserManagerInstructions::DebugGetState());
            
                AuthorizationRequest authorizationRequest(testAdminID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::DebugGetState>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::SelfResetPassword>
            doInstruction_SelfResetPassword(const std::string & password)
            {
                boost::shared_ptr<UserManagerInstructions::SelfResetPassword> testInstruction(
                    new UserManagerInstructions::SelfResetPassword(password));
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::SelfResetPassword>(testInstruction->getFuture().get());
            }
            
            boost::shared_ptr<UserManagerInstructions::Results::SelfGetUser>
            doInstruction_SelfGetUser()
            {
                boost::shared_ptr<UserManagerInstructions::SelfGetUser> testInstruction(
                    new UserManagerInstructions::SelfGetUser());
            
                AuthorizationRequest authorizationRequest(testUserID, *testManager, SecurableComponentType::USER_MANAGER, testInstruction);
                auto authorizationToken = securityManager->postRequest(authorizationRequest)->get_future().get();

                hndlr(testInstruction, authorizationToken);
                return boost::dynamic_pointer_cast<UserManagerInstructions::Results::SelfGetUser>(testInstruction->getFuture().get());
            }
            
            bool registerInstructionHandler(
                    const std::function<void(InstructionBasePtr,
                    SecurityManagement_Types::AuthorizationTokenPtr)> handler) override
            {
                hndlr = handler;
                return true;
            }

            std::vector<InstructionManagement_Types::InstructionSetType> getRequiredInstructionSetTypes() override
            {
                return std::vector<InstructionManagement_Types::InstructionSetType>(
                {
                    InstructionManagement_Types::InstructionSetType::USER_MANAGER_SELF,
                    InstructionManagement_Types::InstructionSetType::USER_MANAGER_ADMIN
                });
            }

        private:
            std::function<void(InstructionBasePtr, SecurityManagement_Types::AuthorizationTokenPtr)> hndlr;
            SyncServer_Core::SecurityManager * securityManager;
            UserManager * testManager;
            UserID testAdminID;
            UserID testUserID;
    };
}

#endif /* ENTITYINSTRUCTIONSOURCES_H */
