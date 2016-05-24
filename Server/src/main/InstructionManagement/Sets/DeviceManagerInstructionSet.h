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

#include "../../Utilities/Strings/Instructions.h"
#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../EntityManagement/Types/Types.h"
#include "../../SecurityManagement/Rules/AuthorizationRules.h"
#include "../../DatabaseManagement/Types/Types.h"
#include "../../DatabaseManagement/Containers/DeviceDataContainer.h"
#include "../../NetworkManagement/Types/Types.h"

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
using NetworkManagement_Types::PeerType;

namespace InstructionManagement_Sets
{
    namespace DeviceManagerInstructions
    {
        struct AdminGetDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            explicit AdminGetDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::GET_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminGetDevicesByConstraint : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminGetDevicesByConstraint(DatabaseSelectConstraints::DEVICES type, boost::any value)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT),
              constraintType(type), constraintValue(value)
            {}
            bool isValid() override { return true; }
            DatabaseSelectConstraints::DEVICES constraintType;
            boost::any constraintValue;
        };
        
        struct AdminAddDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminAddDevice(const std::string & name, const std::string & password, UserID owner, DataTransferType xferType, PeerType type)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::ADD_DEVICE),
              deviceName(name), rawPassword(password), ownerID(owner), transferType(xferType), peerType(type)
            {}
            
            bool isValid() override { return (!deviceName.empty() && !rawPassword.empty()
                    && ownerID != INVALID_USER_ID && transferType != DataTransferType::INVALID); }
            
            std::string deviceName;
            const std::string rawPassword;
            UserID ownerID;
            DataTransferType transferType;
            PeerType peerType;
        };
        
        struct AdminRemoveDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            explicit AdminRemoveDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::REMOVE_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminResetDevicePassword : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminResetDevicePassword(DeviceID id, const std::string & password)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD),
              deviceID(id), rawPassword(password)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && !rawPassword.empty()); }
            DeviceID deviceID;
            const std::string rawPassword;
        };
        
        struct AdminUpdateConnectionInfo : public Instruction<DeviceManagerAdminInstructionType>
        {
            AdminUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO),
              deviceID(id), ipAddress(ip), ipPort(port), transferType(xferType)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && ipAddress != INVALID_IP_ADDRESS
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
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && !deviceName.empty()); }
            DeviceID deviceID;
            std::string deviceName;
            std::string deviceInfo;
        };
        
        struct AdminLockDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            explicit AdminLockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::LOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminUnlockDevice : public Instruction<DeviceManagerAdminInstructionType>
        {
            explicit AdminUnlockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::UNLOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct AdminResetFailedAuthenticationAttempts : public Instruction<DeviceManagerAdminInstructionType>
        {
            explicit AdminResetFailedAuthenticationAttempts(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct DebugGetState : public Instruction<DeviceManagerAdminInstructionType>
        {
            DebugGetState()
            : Instruction(InstructionSetType::DEVICE_MANAGER_ADMIN, DeviceManagerAdminInstructionType::DEBUG_GET_STATE)
            {}
            
            bool isValid() override { return true; }
        };
        
        struct UserGetDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            explicit UserGetDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::GET_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserGetDevices : public Instruction<DeviceManagerUserInstructionType>
        {
            UserGetDevices()
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::GET_DEVICES)
            {}
            
            bool isValid() override { return true; }
        };
        
        struct UserAddDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            UserAddDevice(const std::string & name, const std::string & password, DataTransferType xferType, PeerType type)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::ADD_DEVICE),
              deviceName(name), rawPassword(password), transferType(xferType), peerType(type)
            {}
            
            bool isValid() override { return (!deviceName.empty() && !rawPassword.empty() && transferType != DataTransferType::INVALID); }
            std::string deviceName;
            const std::string rawPassword;
            DataTransferType transferType;
            PeerType peerType;
        };
        
        struct UserRemoveDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            explicit UserRemoveDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::REMOVE_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserResetDevicePassword : public Instruction<DeviceManagerUserInstructionType>
        {
            UserResetDevicePassword(DeviceID id, const std::string & password)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD),
              deviceID(id), rawPassword(password)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && !rawPassword.empty()); }
            DeviceID deviceID;
            const std::string rawPassword;
        };
        
        struct UserUpdateConnectionInfo : public Instruction<DeviceManagerUserInstructionType>
        {
            UserUpdateConnectionInfo(DeviceID id, IPAddress ip, IPPort port, DataTransferType xferType)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO),
              deviceID(id), ipAddress(ip), ipPort(port), transferType(xferType)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && ipAddress != INVALID_IP_ADDRESS
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
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID && !deviceName.empty()); }
            DeviceID deviceID;
            std::string deviceName;
            std::string deviceInfo;
        };
        
        struct UserLockDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            explicit UserLockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::LOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserUnlockDevice : public Instruction<DeviceManagerUserInstructionType>
        {
            explicit UserUnlockDevice(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::UNLOCK_DEVICE),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        struct UserResetFailedAuthenticationAttempts : public Instruction<DeviceManagerUserInstructionType>
        {
            explicit UserResetFailedAuthenticationAttempts(DeviceID id)
            : Instruction(InstructionSetType::DEVICE_MANAGER_USER, DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS),
              deviceID(id)
            {}
            
            bool isValid() override { return (deviceID != INVALID_DEVICE_ID); }
            DeviceID deviceID;
        };
        
        namespace Results
        {
            struct AdminGetDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminGetDevice(DeviceDataContainerPtr input)
                : InstructionResult(DeviceManagerAdminInstructionType::GET_DEVICE), result(input) {}
                
                DeviceDataContainerPtr result;
            };
            
            struct AdminGetDevicesByConstraint : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminGetDevicesByConstraint(std::vector<DeviceDataContainerPtr> input)
                : InstructionResult(DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT), result(input) {}
                
                std::vector<DeviceDataContainerPtr> result;
            };
            
            struct AdminAddDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminAddDevice(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::ADD_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct AdminRemoveDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminRemoveDevice(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::REMOVE_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct AdminResetDevicePassword : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminResetDevicePassword(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD), result(input) {}
                
                bool result;
            };
            
            struct AdminUpdateConnectionInfo : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminUpdateConnectionInfo(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO), result(input) {}
                
                bool result;
            };
            
            struct AdminUpdateGeneralInfo : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminUpdateGeneralInfo(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO), result(input) {}
                
                bool result;
            };
            
            struct AdminLockDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminLockDevice(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::LOCK_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct AdminUnlockDevice : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminUnlockDevice(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::UNLOCK_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct AdminResetFailedAuthenticationAttempts : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit AdminResetFailedAuthenticationAttempts(bool input)
                : InstructionResult(DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS), result(input) {}
                
                bool result;
            };
            
            struct DebugGetState : public InstructionResult<DeviceManagerAdminInstructionType>
            {
                explicit DebugGetState(std::string input)
                : InstructionResult(DeviceManagerAdminInstructionType::DEBUG_GET_STATE), result(input) {}
                
                std::string result;
            };
            
            struct UserGetDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserGetDevice(DeviceDataContainerPtr input)
                : InstructionResult(DeviceManagerUserInstructionType::GET_DEVICE), result(input) {}
                
                DeviceDataContainerPtr result;
            };
            
            struct UserGetDevices : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserGetDevices(std::vector<DeviceDataContainerPtr> input)
                : InstructionResult(DeviceManagerUserInstructionType::GET_DEVICES), result(input) {}
                
                std::vector<DeviceDataContainerPtr> result;
            };
            
            struct UserAddDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserAddDevice(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::ADD_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct UserRemoveDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserRemoveDevice(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::REMOVE_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct UserResetDevicePassword : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserResetDevicePassword(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD), result(input) {}
                
                bool result;
            };
            
            struct UserUpdateConnectionInfo : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserUpdateConnectionInfo(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO), result(input) {}
                
                bool result;
            };
            
            struct UserUpdateGeneralInfo : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserUpdateGeneralInfo(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO), result(input) {}
                
                bool result;
            };
            
            struct UserLockDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserLockDevice(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::LOCK_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct UserUnlockDevice : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserUnlockDevice(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::UNLOCK_DEVICE), result(input) {}
                
                bool result;
            };
            
            struct UserResetFailedAuthenticationAttempts : public InstructionResult<DeviceManagerUserInstructionType>
            {
                explicit UserResetFailedAuthenticationAttempts(bool input)
                : InstructionResult(DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS), result(input) {}
                
                bool result;
            };
        }
    }
}

#endif	/* DEVICEMANAGERINSTRUCTIONSET_H */
