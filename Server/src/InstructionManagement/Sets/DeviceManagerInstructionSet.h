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

#ifndef DEVICEMANAGERINSTRUCTIONSET_H
#define	DEVICEMANAGERINSTRUCTIONSET_H

#include <vector>
#include <string>
#include <boost/any.hpp>

#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../EntityManagement/Types/Types.h"
#include "../../SecurityManagement/Rules/AuthorizationRules.h"
#include "../../DatabaseManagement/Types/Types.h"
#include "../../DatabaseManagement/Containers/DeviceDataContainer.h"

using Common_Types::IPAddress;
using Common_Types::IPPort;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::INVALID_IP_ADDRESS;
using Common_Types::INVALID_IP_PORT;
using Common_Types::INVALID_DEVICE_ID;
using Common_Types::INVALID_USER_ID;
using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::DeviceManagerAdminInstructionType;
using InstructionManagement_Types::DeviceManagerUserInstructionType;
using SecurityManagement_Rules::UserAuthorizationRule;
using DatabaseManagement_Containers::DeviceDataContainerPtr;
using DatabaseManagement_Types::DatabaseSelectConstraints;
using DatabaseManagement_Types::DataTransferType;

namespace InstructionManagement_Sets
{
    namespace DeviceManagerInstructions
    {
        struct AdminGetDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminGetDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::GET_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminGetDevicesByConstraint : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminGetDevicesByConstraint(DatabaseSelectConstraints::DEVICES type, boost::any value)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() { return true; }
            DatabaseSelectConstraints::DEVICES constraintType;
            boost::any constraintValue;
        };
        
        struct AdminAddDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminAddDevice(const std::string & name, const std::string & password, UserID owner, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::ADD_DEVICE),
              deviceName(name), rawPassword(password), ownerID(owner), transferType(xferType)
            {}
            
            bool isValid() { return (!deviceName.empty() && !rawPassword.empty()
                    && ownerID != INVALID_USER_ID && transferType != DataTransferType::INVALID); }
            
            std::string deviceName;
            const std::string rawPassword;
            UserID ownerID;
            DataTransferType transferType;
        };
        
        struct AdminRemoveDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminRemoveDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::REMOVE_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminResetDevicePassword : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminResetDevicePassword(DeviceID id, const std::string & password)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD),
              deviceID(id), rawPassword(password)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && !rawPassword.empty()); }
            DeviceID deviceID;
            const std::string rawPassword;
        };
        
        struct AdminUpdateConnectionInfo : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO),
              deviceID(id), ipAddress(ip), ipPort(port), transferType(xferType)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && ipAddress != INVALID_IP_ADDRESS
                    && ipPort != INVALID_IP_PORT && transferType != DataTransferType::INVALID); }
            
            DeviceID deviceID;
            IPAddress ipAddress;
            IPPort ipPort;
            DataTransferType transferType;
        };
        
        struct AdminUpdateGeneralInfo : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminUpdateGeneralInfo(DeviceID id, const std::string & name, const std::string & info)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO),
              deviceID(id), deviceName(name), deviceInfo(info)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && !deviceName.empty()); }
            DeviceID deviceID;
            std::string deviceName;
            std::string deviceInfo;
        };
        
        struct AdminLockDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminLockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::LOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminUnlockDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminUnlockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::UNLOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminResetFailedAuthenticationAttempts : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminResetFailedAuthenticationAttempts(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct DebugGetState : public Instruction<DeviceManagerAdminInstructionType>
        {
            DebugGetState()
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::DEBUG_GET_STATE)
            {}
            
            bool isValid() { return true; }
        };
        
        struct UserGetDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserGetDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::GET_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserGetDevices : public Instruction<DeviceManagerUserInstructionType>
        {
            UserGetDevices()
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::GET_DEVICES)
            {}
            
            bool isValid() { return true; }
        };
        
        struct UserAddDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserAddDevice(const std::string & name, const std::string & password, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::ADD_DEVICE),
              deviceName(name), rawPassword(password), transferType(xferType)
            {}
            
            bool isValid() { return (!deviceName.empty() && !rawPassword.empty() && transferType != DataTransferType::INVALID); }
            std::string deviceName;
            const std::string rawPassword;
            DataTransferType transferType;
        };
        
        struct UserRemoveDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserRemoveDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::REMOVE_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserResetDevicePassword : public Instruction<DeviceManagerUserInstructionType>
        {
            UserResetDevicePassword(DeviceID id, const std::string & password)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD),
              deviceID(id), rawPassword(password)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && !rawPassword.empty()); }
            DeviceID deviceID;
            const std::string rawPassword;
        };
        
        struct UserUpdateConnectionInfo : public Instruction<DeviceManagerUserInstructionType>
        {
            UserUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO),
              deviceID(id), ipAddress(ip), ipPort(port), transferType(xferType)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && ipAddress != INVALID_IP_ADDRESS
                    && ipPort != INVALID_IP_PORT && transferType != DataTransferType::INVALID); }
            
            DeviceID deviceID;
            IPAddress ipAddress;
            IPPort ipPort;
            DataTransferType transferType;
        };
        
        struct UserUpdateGeneralInfo : public Instruction<DeviceManagerUserInstructionType>
        {
            UserUpdateGeneralInfo(DeviceID id, const std::string & name, const std::string & info)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO),
              deviceID(id), deviceName(name), deviceInfo(info)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID && !deviceName.empty()); }
            DeviceID deviceID;
            std::string deviceName;
            std::string deviceInfo;
        };
        
        struct UserLockDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserLockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::LOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserUnlockDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserUnlockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::UNLOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserResetFailedAuthenticationAttempts : public Instruction<DeviceManagerUserInstructionType>
        {
            UserResetFailedAuthenticationAttempts(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS),
              deviceID(id)
            {}
            
            bool isValid() { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        namespace Results
        {
            struct AdminGetDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminGetDevice(DeviceDataContainerPtr input) : result(input) {}
                DeviceDataContainerPtr result;
            };
            
            struct AdminGetDevicesByConstraint : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminGetDevicesByConstraint(std::vector<DeviceDataContainerPtr> input) : result(input) {}
                std::vector<DeviceDataContainerPtr> result;
            };
            
            struct AdminAddDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminAddDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminRemoveDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminRemoveDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminResetDevicePassword : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminResetDevicePassword(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminUpdateConnectionInfo : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminUpdateConnectionInfo(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminUpdateGeneralInfo : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminUpdateGeneralInfo(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminLockDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminLockDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminUnlockDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminUnlockDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct AdminResetFailedAuthenticationAttempts : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                AdminResetFailedAuthenticationAttempts(bool input) : result(input) {}
                bool result;
            };
            
            struct DebugGetState : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                DebugGetState(std::string input) : result(input) {}
                std::string result;
            };
            
            struct UserGetDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserGetDevice(DeviceDataContainerPtr input) : result(input) {}
                DeviceDataContainerPtr result;
            };
            
            struct UserGetDevices : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserGetDevices(std::vector<DeviceDataContainerPtr> input) : result(input) {}
                std::vector<DeviceDataContainerPtr> result;
            };
            
            struct UserAddDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserAddDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct UserRemoveDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserRemoveDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct UserResetDevicePassword : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserResetDevicePassword(bool input) : result(input) {}
                bool result;
            };
            
            struct UserUpdateConnectionInfo : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserUpdateConnectionInfo(bool input) : result(input) {}
                bool result;
            };
            
            struct UserUpdateGeneralInfo : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserUpdateGeneralInfo(bool input) : result(input) {}
                bool result;
            };
            
            struct UserLockDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserLockDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct UserUnlockDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserUnlockDevice(bool input) : result(input) {}
                bool result;
            };
            
            struct UserResetFailedAuthenticationAttempts : public InstructionResult<DeviceManagerUserInstructionType>
            {
                UserResetFailedAuthenticationAttempts(bool input) : result(input) {}
                bool result;
            };
        }
    }
}

#endif	/* DEVICEMANAGERINSTRUCTIONSET_H */
