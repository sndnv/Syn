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

#include "DeviceManager.h"

namespace Convert = Utilities::Strings;
namespace Instructions = InstructionManagement_Sets::DeviceManagerInstructions;
namespace InstructionResults = InstructionManagement_Sets::DeviceManagerInstructions::Results;

EntityManagement::DeviceManager::DeviceManager
(const DeviceManagerParameters & params,  Utilities::FileLogger * debugLogger)
: debugLogger(debugLogger), databaseManager(params.databaseManager),
  securityManager(params.securityManager), instructionsReceived(0), instructionsProcessed(0)
{}

EntityManagement::DeviceManager::~DeviceManager()
{
    logMessage(LogSeverity::Debug, "(~) > Destruction initiated.");

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    authorizationTokens.clear();

    debugLogger = nullptr;
}

void EntityManagement::DeviceManager::postAuthorizationToken
(const SecurityManagement_Types::AuthorizationTokenPtr token)
{
    if(DeviceManagerAdminInstructionTarget::getType() != token->getAuthorizedSet()
        && DeviceManagerUserInstructionTarget::getType() != token->getAuthorizedSet())
     {
         throw std::logic_error("DeviceManager::postAuthorizationToken() > The token with ID ["
                 + Convert::toString(token->getID()) + "] is not for the expected instruction sets.");
     }

     boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

     if(authorizationTokens.find(token->getID()) == authorizationTokens.end())
     {
         authorizationTokens.insert({token->getID(), token});
     }
     else
     {
         throw std::logic_error("DeviceManager::postAuthorizationToken() > A token with ID ["
                 + Convert::toString(token->getID()) + "] is already present.");
     }
}

bool EntityManagement::DeviceManager::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<DeviceManagerAdminInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::ADMIN);

        try
        {
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::GET_DEVICE,
                                        adminGetDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::GET_DEVICES_BY_CONSTRAINT,
                                        adminGetDevicesByConstraintHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::ADD_DEVICE,
                                        adminAddDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::REMOVE_DEVICE,
                                        adminRemoveDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::RESET_DEVICE_PASSWORD,
                                        adminResetDevicePasswordHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::UPDATE_CONNECTION_INFO,
                                        adminUpdateConnectionInfoHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::UPDATE_GENERAL_INFO,
                                        adminUpdateGeneralInfoHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::LOCK_DEVICE,
                                        adminLockDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::UNLOCK_DEVICE,
                                        adminUnlockDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,
                                        adminResetFailedAuthenticationAttemptsHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerAdminInstructionType::DEBUG_GET_STATE,
                                        debugGetStateHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) > Exception encountered: <"
                    + std::string(ex.what()) + ">");
            
            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}

bool EntityManagement::DeviceManager::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<DeviceManagerUserInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            set->bindInstructionHandler(DeviceManagerUserInstructionType::GET_DEVICE,
                                        userGetDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::GET_DEVICES,
                                        userGetDevicesHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::ADD_DEVICE,
                                        userAddDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::REMOVE_DEVICE,
                                        userRemoveDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::RESET_DEVICE_PASSWORD,
                                        userResetDevicePasswordHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::UPDATE_CONNECTION_INFO,
                                        userUpdateConnectionInfoHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::UPDATE_GENERAL_INFO,
                                        userUpdateGeneralInfoHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::LOCK_DEVICE,
                                        userLockDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::UNLOCK_DEVICE,
                                        userUnlockDeviceHandlerBind);
            
            set->bindInstructionHandler(DeviceManagerUserInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,
                                        userResetFailedAuthenticationAttemptsHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logMessage(LogSeverity::Error, "(registerInstructionSet) > Exception encountered: <"
                    + std::string(ex.what()) + ">");
            
            return false;
        }

        return true;
    }
    else
    {
        logMessage(LogSeverity::Error, "(registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}

bool EntityManagement::DeviceManager::addDeviceOperation
(const std::string & name, const std::string & password, UserID owner, DataTransferType xferType)
{
    std::string nameValidationErrorMessage;
    if(securityManager.isDeviceNameValid(name, nameValidationErrorMessage))
    {
        PasswordData newDevicePassword(securityManager.hashDevicePassword(password));
        DeviceDataContainerPtr newDeviceContainer(
            new DeviceDataContainer(name, newDevicePassword, owner, xferType));
        
        return databaseManager.Devices().addDevice(newDeviceContainer);
    }
    else
    {
        throw std::runtime_error("DeviceManager::addDeviceOperation() > Invalid device name supplied: ["
                                 + nameValidationErrorMessage + "].");
    }
}

void EntityManagement::DeviceManager::adminGetDeviceHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    DeviceDataContainerPtr resultData;
    boost::shared_ptr<Instructions::AdminGetDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminGetDevice>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Devices().getDevice(actualInstruction->deviceID);
    }

    auto result = boost::shared_ptr<InstructionResults::AdminGetDevice>(
        new InstructionResults::AdminGetDevice{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminGetDevicesByConstraintHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    std::vector<DeviceDataContainerPtr> resultData;
    boost::shared_ptr<Instructions::AdminGetDevicesByConstraint> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminGetDevicesByConstraint>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Devices().getDevicesByConstraint(actualInstruction->constraintType,
                                                                      actualInstruction->constraintValue);
    }

    auto result = boost::shared_ptr<InstructionResults::AdminGetDevicesByConstraint>(
        new InstructionResults::AdminGetDevicesByConstraint{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminAddDeviceHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminAddDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminAddDevice>(instruction);

    if(actualInstruction)
    {
        try
        {
            resultValue = addDeviceOperation(actualInstruction->deviceName,
                                             actualInstruction->rawPassword,
                                             actualInstruction->ownerID,
                                             actualInstruction->transferType);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device [" + actualInstruction->deviceName
                        + "] for user [" + Convert::toString(actualInstruction->ownerID)
                        + "] added by user ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
        catch(const std::exception & ex)
        {
            logMessage(LogSeverity::Error, "(adminAddDeviceHandler) > Exception encountered: ["
                    + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminAddDevice>(
        new InstructionResults::AdminAddDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminRemoveDeviceHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminRemoveDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminRemoveDevice>(instruction);

    if(actualInstruction)
    {
        resultValue = databaseManager.Devices().removeDevice(actualInstruction->deviceID);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "Device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] removed by user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminRemoveDevice>(
        new InstructionResults::AdminRemoveDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminResetDevicePasswordHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminResetDevicePassword> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminResetDevicePassword>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(adminResetDevicePasswordHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::adminResetDevicePasswordHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        try
        {
            PasswordData newDevicePassword(securityManager.hashDevicePassword(actualInstruction->rawPassword));

            deviceData->resetPassword(newDevicePassword);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Password for device ["
                        + Convert::toString(actualInstruction->deviceID)
                        + "] reset by user ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
        catch(const std::exception & ex)
        {
            logMessage(LogSeverity::Error, "(adminResetDevicePasswordHandler) > Exception encountered: ["
                    + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminResetDevicePassword>(
        new InstructionResults::AdminResetDevicePassword{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminUpdateConnectionInfoHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminUpdateConnectionInfo> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminUpdateConnectionInfo>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(adminUpdateConnectionInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::adminUpdateConnectionInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }


        deviceData->setDeviceAddress(actualInstruction->ipAddress);
        deviceData->setDevicePort(actualInstruction->ipPort);
        deviceData->setTransferType(actualInstruction->transferType);
        resultValue = databaseManager.Devices().updateDevice(deviceData);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "Connection info about device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] updated by user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminUpdateConnectionInfo>(
        new InstructionResults::AdminUpdateConnectionInfo{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminUpdateGeneralInfoHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminUpdateGeneralInfo> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminUpdateGeneralInfo>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(adminUpdateGeneralInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::adminUpdateGeneralInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        deviceData->setDeviceName(actualInstruction->deviceName);
        deviceData->setDeviceInfo(actualInstruction->deviceInfo);
        resultValue = databaseManager.Devices().updateDevice(deviceData);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "General info about device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] updated by user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminUpdateGeneralInfo>(
        new InstructionResults::AdminUpdateGeneralInfo{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminLockDeviceHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminLockDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminLockDevice>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(adminLockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::adminLockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(!deviceData->isDeviceLocked())
        {
            deviceData->setLockedState(true);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device ["
                        + Convert::toString(actualInstruction->deviceID) + "] locked by user ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminLockDevice>(
        new InstructionResults::AdminLockDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminUnlockDeviceHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminUnlockDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminUnlockDevice>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(adminUnlockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::adminUnlockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->isDeviceLocked())
        {
            deviceData->setLockedState(false);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device ["
                        + Convert::toString(actualInstruction->deviceID) + "] unlocked by user ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminUnlockDevice>(
        new InstructionResults::AdminUnlockDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::adminResetFailedAuthenticationAttemptsHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminResetFailedAuthenticationAttempts> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminResetFailedAuthenticationAttempts>(instruction);

    if(actualInstruction)
    {
        try
        {
            DeviceDataContainerPtr deviceData =
                    databaseManager.Devices().getDevice(actualInstruction->deviceID);
            
            if(!deviceData)
            {
                logMessage(LogSeverity::Error, "(adminResetFailedAuthenticationAttemptsHandler) > Device ["
                        + Convert::toString(actualInstruction->deviceID) + "] not found.");
                
                throwInstructionException("DeviceManager::adminResetFailedAuthenticationAttemptsHandler() > Device ["
                                          + Convert::toString(actualInstruction->deviceID)
                                          + "] not found.", instruction);
                return;
            }

            if(deviceData->getFailedAuthenticationAttempts() > 0)
            {
                deviceData->resetFailedAuthenticationAttempts();
                resultValue = databaseManager.Devices().updateDevice(deviceData);
                
                if(resultValue)
                {
                    logMessage(LogSeverity::Info, "Failed authentication attempts for device ["
                            + Convert::toString(actualInstruction->deviceID)
                            + "] reset by user ["
                            + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
                }
            }
        }
        catch(const std::exception & ex)
        {
            logMessage(LogSeverity::Error, "(adminResetFailedAuthenticationAttemptsHandler) > Exception encountered: ["
                    + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminResetFailedAuthenticationAttempts>(
        new InstructionResults::AdminResetFailedAuthenticationAttempts{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::debugGetStateHandler
(InstructionPtr<DeviceManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    std::string resultData;
    boost::shared_ptr<Instructions::DebugGetState> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::DebugGetState>(instruction);

    if(actualInstruction)
    {
        resultData += "instructionsReceived;" + Convert::toString(instructionsReceived) + "\n";
        resultData += "instructionsProcessed;" + Convert::toString(instructionsProcessed) + "\n";
        resultData += "authorizationTokens size;" + Convert::toString(authorizationTokens.size()) + "\n";
    }

    auto result = boost::shared_ptr<InstructionResults::DebugGetState>(
        new InstructionResults::DebugGetState{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userGetDeviceHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    DeviceDataContainerPtr resultData;
    boost::shared_ptr<Instructions::UserGetDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserGetDevice>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Devices().getDevice(actualInstruction->deviceID);

        if(!resultData)
        {
            logMessage(LogSeverity::Error, "(userGetDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userGetDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(resultData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userGetDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userGetDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserGetDevice>(
        new InstructionResults::UserGetDevice{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userGetDevicesHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    std::vector<DeviceDataContainerPtr> resultData;
    boost::shared_ptr<Instructions::UserGetDevices> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserGetDevices>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Devices().getDevicesByConstraint
                (
                    DatabaseSelectConstraints::DEVICES::LIMIT_BY_OWNER,
                    actualInstruction->getToken()->getUserID()
                );
    }

    auto result = boost::shared_ptr<InstructionResults::UserGetDevices>(
        new InstructionResults::UserGetDevices{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userAddDeviceHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserAddDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserAddDevice>(instruction);

    if(actualInstruction)
    {
        try
        {
            resultValue = addDeviceOperation(actualInstruction->deviceName,
                                             actualInstruction->rawPassword,
                                             actualInstruction->getToken()->getUserID(),
                                             actualInstruction->transferType);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device [" + actualInstruction->deviceName
                        + "] added by owner ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
        catch(const std::exception & ex)
        {
            logMessage(LogSeverity::Error, "(userAddDeviceHandler) > Exception encountered: ["
                    + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserAddDevice>(
        new InstructionResults::UserAddDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userRemoveDeviceHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserRemoveDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserRemoveDevice>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userRemoveDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userRemoveDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userRemoveDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userRemoveDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            
            return;
        }

        resultValue = databaseManager.Devices().removeDevice(actualInstruction->deviceID);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "Device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] removed by owner ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserRemoveDevice>(
        new InstructionResults::UserRemoveDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userResetDevicePasswordHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserResetDevicePassword> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserResetDevicePassword>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userResetDevicePasswordHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userResetDevicePasswordHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userResetDevicePasswordHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userResetDevicePasswordHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        try
        {
            PasswordData newDevicePassword(securityManager.hashDevicePassword(actualInstruction->rawPassword));

            deviceData->resetPassword(newDevicePassword);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Password for device ["
                        + Convert::toString(actualInstruction->deviceID)
                        + "] reset by owner ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
        catch(const std::exception & ex)
        {
            logMessage(LogSeverity::Error, "(userResetDevicePasswordHandler) > Exception encountered: ["
                    + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserResetDevicePassword>(
        new InstructionResults::UserResetDevicePassword{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userUpdateConnectionInfoHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserUpdateConnectionInfo> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserUpdateConnectionInfo>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userUpdateConnectionInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userUpdateConnectionInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userUpdateConnectionInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userUpdateConnectionInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        deviceData->setDeviceAddress(actualInstruction->ipAddress);
        deviceData->setDevicePort(actualInstruction->ipPort);
        deviceData->setTransferType(actualInstruction->transferType);
        resultValue = databaseManager.Devices().updateDevice(deviceData);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "Connection info for device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] updated by owner ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserUpdateConnectionInfo>(
        new InstructionResults::UserUpdateConnectionInfo{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userUpdateGeneralInfoHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserUpdateGeneralInfo> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserUpdateGeneralInfo>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userUpdateGeneralInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userUpdateGeneralInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userUpdateGeneralInfoHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userUpdateGeneralInfoHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        deviceData->setDeviceName(actualInstruction->deviceName);
        deviceData->setDeviceInfo(actualInstruction->deviceInfo);
        resultValue = databaseManager.Devices().updateDevice(deviceData);
        
        if(resultValue)
        {
            logMessage(LogSeverity::Info, "General info for device ["
                    + Convert::toString(actualInstruction->deviceID)
                    + "] updated by owner ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserUpdateGeneralInfo>(
        new InstructionResults::UserUpdateGeneralInfo{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userLockDeviceHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserLockDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserLockDevice>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userLockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userLockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userLockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userLockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        if(!deviceData->isDeviceLocked())
        {
            deviceData->setLockedState(true);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device ["
                        + Convert::toString(actualInstruction->deviceID)
                        + "] locked by owner ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserLockDevice>(
        new InstructionResults::UserLockDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userUnlockDeviceHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserUnlockDevice> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserUnlockDevice>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userUnlockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userUnlockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userUnlockDeviceHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userUnlockDeviceHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        if(deviceData->isDeviceLocked())
        {
            deviceData->setLockedState(false);
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Device ["
                        + Convert::toString(actualInstruction->deviceID)
                        + "] unlocked by owner ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserUnlockDevice>(
        new InstructionResults::UserUnlockDevice{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::userResetFailedAuthenticationAttemptsHandler
(InstructionPtr<DeviceManagerUserInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::UserResetFailedAuthenticationAttempts> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UserResetFailedAuthenticationAttempts>(instruction);

    if(actualInstruction)
    {
        DeviceDataContainerPtr deviceData =
                databaseManager.Devices().getDevice(actualInstruction->deviceID);
        
        if(!deviceData)
        {
            logMessage(LogSeverity::Error, "(userResetFailedAuthenticationAttemptsHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] not found.");
            
            throwInstructionException("DeviceManager::userResetFailedAuthenticationAttemptsHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] not found.", instruction);
            return;
        }

        if(deviceData->getDeviceOwner() != actualInstruction->getToken()->getUserID())
        {
            logMessage(LogSeverity::Error, "(userResetFailedAuthenticationAttemptsHandler) > Device ["
                    + Convert::toString(actualInstruction->deviceID) + "] does not belong to user ["
                    + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            
            throwInstructionException("DeviceManager::userResetFailedAuthenticationAttemptsHandler() > Device ["
                                      + Convert::toString(actualInstruction->deviceID)
                                      + "] does not belong to user ["
                                      + Convert::toString(actualInstruction->getToken()->getUserID())
                                      + "].", instruction);
            return;
        }

        if(deviceData->getFailedAuthenticationAttempts() > 0)
        {
            deviceData->resetFailedAuthenticationAttempts();
            resultValue = databaseManager.Devices().updateDevice(deviceData);
            
            if(resultValue)
            {
                logMessage(LogSeverity::Info, "Failed authentication attempts for device ["
                        + Convert::toString(actualInstruction->deviceID)
                        + "] reset by owner ["
                        + Convert::toString(actualInstruction->getToken()->getUserID()) + "].");
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UserResetFailedAuthenticationAttempts>(
        new InstructionResults::UserResetFailedAuthenticationAttempts{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DeviceManager::verifyAuthorizationToken(AuthorizationTokenPtr token)
{
    ++instructionsReceived;
    if(!token)
    {
       throw InvalidAuthorizationTokenException("DeviceManager::verifyAuthorizationToken() > "
                "An empty token was supplied."); 
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    auto requestedToken = authorizationTokens.find(token->getID());
    if(requestedToken != authorizationTokens.end())
    {
        if(*(requestedToken->second) == *token
           && (token->getAuthorizedSet() == DeviceManagerAdminInstructionTarget::getType()
               || token->getAuthorizedSet() == DeviceManagerUserInstructionTarget::getType()))
        {
            authorizationTokens.erase(requestedToken);
            ++instructionsProcessed;
        }
        else
        {
            throw InvalidAuthorizationTokenException("DeviceManager::verifyAuthorizationToken() > "
                    "The supplied token [" + Convert::toString(token->getID())
                    + "] does not match the one expected by the manager.");
        }
    }
    else
    {
        throw InvalidAuthorizationTokenException("DeviceManager::verifyAuthorizationToken() > "
                "The supplied token [" + Convert::toString(token->getID()) + "] was not found.");
    }
}
