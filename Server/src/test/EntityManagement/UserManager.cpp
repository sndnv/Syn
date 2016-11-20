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
#include "../../main/EntityManagement/UserManager.h"
#include "EntityInstructionSources.h"

using EntityManagement::UserManager;
using Testing::UserManagerInstructionSource;

SCENARIO("A user manager is created and can process instructions",
         "[UserManager][EntityManagement][Managers]")
{
    GIVEN("A new UserManager")
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "./UserManager.log",  //logFilePath
            32*1024*1024,         //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        auto dbManager = Testing::Fixtures::createDatabaseManager();
        std::vector<InstructionManagement_Types::InstructionSetType> dispatcherSets;
        dispatcherSets.push_back(InstructionManagement_Types::InstructionSetType::USER_MANAGER_ADMIN);
        dispatcherSets.push_back(InstructionManagement_Types::InstructionSetType::USER_MANAGER_SELF);
        auto dispatcher = Testing::Fixtures::createInstructionDispatcher(dispatcherSets, logger);
        auto secManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, logger);
        auto sessManager = Testing::Fixtures::createSessionManager(dbManager, secManager, logger);
        
        UserManager::UserManagerParameters mgrParams
        {
            *dbManager,
            *secManager
        };
        
        SecurityManagement_Rules::MinNameLength * userNameRule = new SecurityManagement_Rules::MinNameLength(3);
        secManager->addUserNameRule(userNameRule);

        UserManager userManager(mgrParams, logger);
        secManager->registerSecurableComponent(userManager);
        dispatcher->registerInstructionTarget<UserManagerAdminInstructionType>(userManager);
        dispatcher->registerInstructionTarget<UserManagerSelfInstructionType>(userManager);
        
        std::string rawAdminPassword = "passw0rd1";
        PasswordData adminPassword(secManager->hashUserPassword(rawAdminPassword));
        UserDataContainerPtr testAdminUser(new UserDataContainer("TEST_ADMIN_1", adminPassword, UserAccessLevel::ADMIN, false));
        testAdminUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::USER_MANAGER_ADMIN));
        dbManager->Users().addUser(testAdminUser);
        CHECK_NOTHROW(sessManager->openSession("TEST_ADMIN_1", rawAdminPassword, SessionType::COMMAND, true));
        
        std::string rawUserPassword = "passw0rd2";
        PasswordData userPassword(secManager->hashUserPassword(rawUserPassword));
        UserDataContainerPtr testUser(new UserDataContainer("TEST_USER_1", userPassword, UserAccessLevel::USER, false));
        testUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::USER_MANAGER_SELF));
        dbManager->Users().addUser(testUser);
        sessManager->openSession("TEST_USER_1", rawUserPassword, SessionType::COMMAND, true);
        
        UserManagerInstructionSource testInstructionSource(secManager, &userManager, testAdminUser->getUserID(), testUser->getUserID());
        dispatcher->registerInstructionSource(testInstructionSource);
        
        WHEN("instructions are sent to the UserManager, it processes them successfully")
        {
            CHECK_NOTHROW(testInstructionSource.doInstruction_DebugGetState());
            
            std::string rawPassword_1 = "passw0rd3";
            auto result_1 = testInstructionSource.doInstruction_AdminAddUser("TEST_USER_2", rawPassword_1, UserAccessLevel::USER, false);
            CHECK(result_1->result);
            
            auto result_2 = testInstructionSource.doInstruction_AdminGetUsersByConstraint(DatabaseSelectConstraints::USERS::GET_ALL, 0);
            CHECK(result_2->result.size() == 3);
            
            auto result_3 = testInstructionSource.doInstruction_AdminGetUser("TEST_USER_2");
            UserDataContainerPtr newUser = result_3->result;
            
            auto result_4 = testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID());
            CHECK(newUser->getUserID() == result_4->result->getUserID());
            
            auto result_5 = testInstructionSource.doInstruction_AdminLockUser(newUser->getUserID());
            CHECK(result_5->result);
            CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->isUserLocked());
            
            auto result_6 = testInstructionSource.doInstruction_AdminUnlockUser(newUser->getUserID());
            CHECK(result_6->result);
            CHECK_FALSE(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->isUserLocked());
            
            auto result_7 = testInstructionSource.doInstruction_AdminUpdateAccessLevel(newUser->getUserID(), UserAccessLevel::ADMIN);
            CHECK(result_7->result);
            CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getUserAccessLevel() == UserAccessLevel::ADMIN);
            
            std::string rawPassword_2 = "passw0rd4";
            auto result_8 = testInstructionSource.doInstruction_AdminResetPassword(newUser->getUserID(), rawPassword_2);
            CHECK(result_8->result);
            bool testUserPasswordsEqual = newUser->getPasswordData() == testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getPasswordData();
            CHECK_FALSE(testUserPasswordsEqual);
            
            std::string rawPassword_3 = "invalidpassword";
            CHECK_THROWS_AS(sessManager->openSession("TEST_USER_2", rawPassword_3, SessionType::COMMAND), InvalidPassswordException);
            auto result_9 = testInstructionSource.doInstruction_AdminResetFailedAuthenticationAttempts(newUser->getUserID());
            CHECK(result_9->result);
            
            auto result_10 = testInstructionSource.doInstruction_AdminForcePasswordReset(newUser->getUserID());
            CHECK(result_10->result);
            CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getForcePasswordReset());
            
            UserAuthorizationRule newRule_1(InstructionManagement_Types::InstructionSetType::TEST);
            auto result_11 = testInstructionSource.doInstruction_AdminAddAuthorizationRule(newUser->getUserID(), newRule_1);
            CHECK(result_11->result);
            
            UserAuthorizationRule newRule_2(InstructionManagement_Types::InstructionSetType::DATABASE_LOGGER);
            auto result_12 = testInstructionSource.doInstruction_AdminAddAuthorizationRule(newUser->getUserID(), newRule_2);
            CHECK(result_12->result);
            
            //TODO - re-enable -> CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getAccessRules().size() == 2);
            
            auto result_13 = testInstructionSource.doInstruction_AdminRemoveAuthorizationRule(newUser->getUserID(), newRule_1);
            CHECK(result_13->result);
            //TODO - re-enable -> CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getAccessRules().size() == 1);
            
            auto result_14 = testInstructionSource.doInstruction_AdminClearAuthorizationRules(newUser->getUserID());
            CHECK(result_14->result);
            //TODO - re-enable -> CHECK(testInstructionSource.doInstruction_AdminGetUser(newUser->getUserID())->result->getAccessRules().empty());
            
            auto result_15 = testInstructionSource.doInstruction_AdminRemoveUser(newUser->getUserID());
            CHECK(result_15->result);
            
            auto result_16 = testInstructionSource.doInstruction_AdminGetUsersByConstraint(DatabaseSelectConstraints::USERS::GET_ALL, 0);
            CHECK(result_16->result.size() == 2);
            
            auto result_17 = testInstructionSource.doInstruction_SelfGetUser();
            auto ownUser = result_17->result;
            CHECK(ownUser->getUsername() == "TEST_USER_1");
            
            std::string rawPassword_4 = "newPassw0rd1";
            auto result_18 = testInstructionSource.doInstruction_SelfResetPassword(rawPassword_4);
            CHECK(result_18->result);
            bool ownUserPasswordsEqual = ownUser->getPasswordData() == testInstructionSource.doInstruction_AdminGetUser(ownUser->getUserID())->result->getPasswordData();
            CHECK_FALSE(ownUserPasswordsEqual);
            
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}
