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

#include "../BasicSpec.h"
#include "../Fixtures.h"
#include "../../main/EntityManagement/DeviceManager.h"
#include "EntityInstructionSources.h"

using EntityManagement::DeviceManager;
using Testing::DeviceManagerInstructionSource;

SCENARIO("A device manager is created and can process instructions",
         "[DeviceManager][EntityManagement][Managers]")
{
    GIVEN("A new DeviceManager")
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "./DeviceManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        auto dbManager = Testing::Fixtures::createDatabaseManager();
        std::vector<InstructionManagement_Types::InstructionSetType> dispatcherSets;
        dispatcherSets.push_back(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_ADMIN);
        dispatcherSets.push_back(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_USER);
        auto dispatcher = Testing::Fixtures::createInstructionDispatcher(dispatcherSets, logger);
        auto secManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, logger);
        auto sessManager = Testing::Fixtures::createSessionManager(dbManager, secManager, logger);
        
        DeviceManager::DeviceManagerParameters mgrParams
        {
            *dbManager,
            *secManager
        };

        DeviceManager devManager(mgrParams, logger);
        secManager->registerSecurableComponent(devManager);
        dispatcher->registerInstructionTarget<DeviceManagerAdminInstructionType>(devManager);
        dispatcher->registerInstructionTarget<DeviceManagerUserInstructionType>(devManager);
        
        std::string rawAdminPassword = "passw0rd1";
        PasswordData adminPassword(secManager->hashUserPassword(rawAdminPassword));
        UserDataContainerPtr testAdminUser(new UserDataContainer("TEST_ADMIN_1", adminPassword, UserAccessLevel::ADMIN, false));
        testAdminUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_ADMIN));
        dbManager->Users().addUser(testAdminUser);
        CHECK_NOTHROW(sessManager->openSession("TEST_ADMIN_1", rawAdminPassword, SessionType::COMMAND, true));
        
        std::string rawUserPassword = "passw0rd2";
        PasswordData userPassword(secManager->hashUserPassword(rawUserPassword));
        UserDataContainerPtr testUser(new UserDataContainer("TEST_USER_1", userPassword, UserAccessLevel::USER, false));
        testUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DEVICE_MANAGER_USER));
        dbManager->Users().addUser(testUser);
        CHECK_NOTHROW(sessManager->openSession("TEST_USER_1", rawUserPassword, SessionType::COMMAND, true));
        
        DeviceManagerInstructionSource testInstructionSource(secManager, &devManager, testAdminUser->getUserID(), testUser->getUserID());
        dispatcher->registerInstructionSource(testInstructionSource);
        
        WHEN("instructions are sent to the DeviceManager, it processes them successfully")
        {
            CHECK_NOTHROW(testInstructionSource.doInstruction_DebugGetState());
            
            std::string rawPassword_1 = "Passw0rd1";
            auto result_1 = testInstructionSource.doInstruction_AdminAddDevice("TEST_DEV_1", rawPassword_1, testUser->getUserID(), DataTransferType::PULL, PeerType::CLIENT);
            CHECK(result_1->result);
            
            auto result_2 = testInstructionSource.doInstruction_AdminGetDevicesByConstraint(DatabaseSelectConstraints::DEVICES::GET_ALL, 0);
            CHECK(result_2->result.size() == 1);
            auto newDevice = result_2->result.front();
            
            auto result_3 = testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID());
            CHECK(result_3->result->getDeviceID() == newDevice->getDeviceID());
            
            auto result_4 = testInstructionSource.doInstruction_AdminLockDevice(newDevice->getDeviceID());
            CHECK(result_4->result);
            CHECK(testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID())->result->isDeviceLocked());
            
            auto result_5 = testInstructionSource.doInstruction_AdminUnlockDevice(newDevice->getDeviceID());
            CHECK(result_5->result);
            CHECK_FALSE(testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID())->result->isDeviceLocked());
            
            std::string rawPassword_2 = "Some0therPassW0rd!";
            auto result_6 = testInstructionSource.doInstruction_AdminResetDevicePassword(newDevice->getDeviceID(), rawPassword_2);
            CHECK(result_6->result);
            bool testDevicePasswordsEqual = testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID())->result->getPasswordData() == newDevice->getPasswordData();
            CHECK_FALSE(testDevicePasswordsEqual);
            
            std::string rawPassword_3 = "incorrectPassword";
            CHECK_THROWS_AS(sessManager->openSession(newDevice->getDeviceID(), rawPassword_3, SessionType::COMMAND), InvalidPassswordException);
            auto result_7 = testInstructionSource.doInstruction_AdminResetFailedAuthenticationAttempts(newDevice->getDeviceID());
            CHECK(result_7->result);
            
            auto result_8 = testInstructionSource.doInstruction_AdminUpdateConnectionInfo(newDevice->getDeviceID(), "1.2.3.4", 1234, DataTransferType::PUSH);
            CHECK(result_8->result);
            auto updatedDevice_1 = testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID())->result;
            CHECK_FALSE(updatedDevice_1->getDeviceCommandAddress() == newDevice->getDeviceCommandAddress());
            CHECK_FALSE(updatedDevice_1->getDeviceCommandPort() == newDevice->getDeviceCommandPort());
            CHECK_FALSE(updatedDevice_1->getTransferType() == newDevice->getTransferType());
            
            auto result_9 = testInstructionSource.doInstruction_AdminUpdateGeneralInfo(newDevice->getDeviceID(), "UPDATED_NAME_1", "test info 1");
            CHECK(result_9->result);
            auto updatedDevice_2 = testInstructionSource.doInstruction_AdminGetDevice(newDevice->getDeviceID())->result;
            CHECK_FALSE(updatedDevice_2->getDeviceName() == newDevice->getDeviceName());
            CHECK_FALSE(updatedDevice_2->getDeviceInfo() == newDevice->getDeviceInfo());
            
            auto result_10 = testInstructionSource.doInstruction_AdminRemoveDevice(newDevice->getDeviceID());
            CHECK(result_10->result);
            
            auto result_11 = testInstructionSource.doInstruction_AdminGetDevicesByConstraint(DatabaseSelectConstraints::DEVICES::GET_ALL, 0);
            CHECK(result_11->result.empty());
            
            std::string rawPassword_4 = "Passw0rd2";
            auto result_12 = testInstructionSource.doInstruction_UserAddDevice("TEST_DEV_2", rawPassword_4, DataTransferType::PULL, PeerType::CLIENT);
            CHECK(result_12->result);
            
            auto result_13 = testInstructionSource.doInstruction_UserGetDevices();
            CHECK(result_13->result.size() == 1);
            auto newUserDevice = result_13->result.front();
            
            auto result_14 = testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID());
            CHECK(result_14->result->getDeviceID() == newUserDevice->getDeviceID());
            
            auto result_15 = testInstructionSource.doInstruction_UserLockDevice(newUserDevice->getDeviceID());
            CHECK(result_15->result);
            CHECK(testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID())->result->isDeviceLocked());
            
            auto result_16 = testInstructionSource.doInstruction_UserUnlockDevice(newUserDevice->getDeviceID());
            CHECK(result_16->result);
            CHECK_FALSE(testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID())->result->isDeviceLocked());
            
            std::string rawPassword_5 = "TesDevPassw0rd!!";
            auto result_17 = testInstructionSource.doInstruction_UserResetDevicePassword(newUserDevice->getDeviceID(), rawPassword_5);
            CHECK(result_17->result);
            bool testUserDevicePasswordsEqual = testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID())->result->getPasswordData() == newUserDevice->getPasswordData();
            CHECK_FALSE(testUserDevicePasswordsEqual);
            
            std::string rawPassword_6 = "incorrectPassword";
            CHECK_THROWS_AS(sessManager->openSession(newUserDevice->getDeviceID(), rawPassword_6, SessionType::COMMAND), InvalidPassswordException);
            auto result_18 = testInstructionSource.doInstruction_UserResetFailedAuthenticationAttempts(newUserDevice->getDeviceID());
            CHECK(result_18->result);
            
            auto result_19 = testInstructionSource.doInstruction_UserUpdateConnectionInfo(newUserDevice->getDeviceID(), "5.6.7.8", 5678, DataTransferType::PUSH);
            CHECK(result_19->result);
            auto updatedUserDevice_1 = testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID())->result;
            CHECK_FALSE(updatedUserDevice_1->getDeviceCommandAddress() == newUserDevice->getDeviceCommandAddress());
            CHECK_FALSE(updatedUserDevice_1->getDeviceCommandPort() == newUserDevice->getDeviceCommandPort());
            CHECK_FALSE(updatedUserDevice_1->getTransferType() == newUserDevice->getTransferType());
            
            auto result_20 = testInstructionSource.doInstruction_UserUpdateGeneralInfo(newUserDevice->getDeviceID(), "UPDATED_NANE_2", "test info 2");
            CHECK(result_20->result);
            auto updatedUserDevice_2 = testInstructionSource.doInstruction_UserGetDevice(newUserDevice->getDeviceID())->result;
            CHECK_FALSE(updatedUserDevice_2->getDeviceName() == newUserDevice->getDeviceName());
            CHECK_FALSE(updatedUserDevice_2->getDeviceInfo() == newUserDevice->getDeviceInfo());
            
            auto result_21 = testInstructionSource.doInstruction_UserRemoveDevice(newUserDevice->getDeviceID());
            CHECK(result_21->result);
            
            auto result_22 = testInstructionSource.doInstruction_UserGetDevices();
            CHECK(result_22->result.empty());
            
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}
