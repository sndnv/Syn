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

#include <vector>
#include <boost/uuid/random_generator.hpp>
#include "../BasicSpec.h"
#include "../Fixtures.h"
#include "../InstructionManagement/TestInstructionSet.h"
#include "TestSecurable.h"
#include "../../main/Utilities/Strings/Common.h"
#include "../../main/Utilities/FileLogger.h"
#include "../../main/DatabaseManagement/DatabaseManager.h"
#include "../../main/InstructionManagement/InstructionDispatcher.h"
#include "../../main/SecurityManagement/SecurityManager.h"
#include "../../main/SecurityManagement/Rules/AuthenticationRules.h"

SCENARIO("A security manager is created, managed and can process requests",
         "[SecurityManager][SecurityManagement][Managers][Core]")
{
    GIVEN("a SecurityManager with a basic configuration")
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "./SecurityManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        SyncServer_Core::InstructionDispatcher * dispatcher = Testing::Fixtures::createInstructionDispatcher();
        SyncServer_Core::DatabaseManager * dbManager = Testing::Fixtures::createDatabaseManager();
        SyncServer_Core::SecurityManager * testManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, logger);
        
        PasswordData user1_pass(testManager->hashUserPassword("passw0rd"));
        PasswordData device1_pass(testManager->hashDevicePassword("PassW0rd1"));
        UserDataContainerPtr user1(new UserDataContainer("TEST_USER_1", user1_pass, UserAccessLevel::ADMIN, false));
        DeviceDataContainerPtr device1(new DeviceDataContainer("TEST_DEVICE_1", device1_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        
        user1->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        
        dbManager->Users().addUser(user1);
        dbManager->Devices().addDevice(device1);
        
        WHEN("new user and device name/password rules are added")
        {
            SecurityManagement_Rules::MinNameLength * rule_1 = new SecurityManagement_Rules::MinNameLength(3);
            SecurityManagement_Rules::MaxNameLength * rule_2 = new SecurityManagement_Rules::MaxNameLength(3);
            SecurityManagement_Rules::MinPasswordLength * rule_3 = new SecurityManagement_Rules::MinPasswordLength(5);
            SecurityManagement_Rules::AllowedPasswordStructure * rule_4 = new SecurityManagement_Rules::AllowedPasswordStructure("(?=.*\\d)(?=.*[a-z])(?=.*[A-Z]).{6,}");
            
            auto rule_id_1 = testManager->addDeviceNameRule(rule_1);
            auto rule_id_2 = testManager->addUserNameRule(rule_2);
            auto rule_id_3 = testManager->addDevicePasswordRule(rule_3);
            auto rule_id_4 = testManager->addUserPasswordRule(rule_4);
            
            THEN("the new rules are used to validate names/passwords")
            {
                std::string result;
                CHECK_FALSE(testManager->isDeviceNameValid("", result));
                CHECK_FALSE(testManager->isDeviceNameValid("A", result));
                CHECK_FALSE(testManager->isDeviceNameValid("AB", result));
                CHECK(testManager->isDeviceNameValid("ABC", result));
                CHECK(testManager->isDeviceNameValid("ABC123", result));
                
                CHECK(testManager->isUserNameValid("", result));
                CHECK(testManager->isUserNameValid("A", result));
                CHECK(testManager->isUserNameValid("AB", result));
                CHECK(testManager->isUserNameValid("ABC", result));
                CHECK_FALSE(testManager->isUserNameValid("ABC1", result));
                CHECK_FALSE(testManager->isUserNameValid("ABC12", result));
                
                CHECK_THROWS_AS(testManager->hashDevicePassword(""), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashDevicePassword("1"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashDevicePassword("1A"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashDevicePassword("1A2"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashDevicePassword("1A2B"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_FALSE(testManager->hashDevicePassword("1A2B3").empty());
                CHECK_FALSE(testManager->hashDevicePassword("1A2B3C").empty());
                CHECK_FALSE(testManager->hashDevicePassword("1A2B3C4").empty());
                
                CHECK_THROWS_AS(testManager->hashUserPassword(""), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashUserPassword("test"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashUserPassword("password"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashUserPassword("1"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashUserPassword("ABC"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(testManager->hashUserPassword("Ab12C"), SecurityManagement_Types::InvalidPassswordException);
                CHECK_FALSE(testManager->hashUserPassword("PassW0rd1").empty());
                CHECK_FALSE(testManager->hashUserPassword("PassW0rd2").empty());
            }
            
            AND_THEN("the rules can be removed")
            {
                CHECK_NOTHROW(testManager->removeDeviceNameRule(rule_id_1));
                CHECK_NOTHROW(testManager->removeUserNameRule(rule_id_2));
                CHECK_NOTHROW(testManager->removeDevicePasswordRule(rule_id_3));
                CHECK_NOTHROW(testManager->removeUserPasswordRule(rule_id_4));
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("new hashing parameters are successfully added")
        {
            SyncServer_Core::SecurityManager::PasswordHashingParameters newHashingParams
            (
                10,                             //userPasswordSaltSize
                20,                             //devicePasswordSaltSize
                HashAlgorithmType::SHA3_224,    //userPasswordHashAlgorithm
                HashAlgorithmType::SHA_384      //devicePasswordHashAlgorithm
            );
            
            CHECK_NOTHROW(testManager->updatePasswordHashingParameters(newHashingParams));
            SecurityManagement_Rules::MinPasswordLength * rule_1 = new SecurityManagement_Rules::MinPasswordLength(3);
            SecurityManagement_Rules::MinPasswordLength * rule_2 = new SecurityManagement_Rules::MinPasswordLength(3);
            CHECK_NOTHROW(testManager->addDevicePasswordRule(rule_1));
            CHECK_NOTHROW(testManager->addUserPasswordRule(rule_2));
            
            THEN("the new parameters are used for hashing")
            {
                unsigned int expectedUserPasswordHashSize = newHashingParams.userPasswordSaltSize + (224/8);
                unsigned int expectedDevicePasswordHashSize = newHashingParams.devicePasswordSaltSize + (384/8);
                
                CHECK(testManager->hashDevicePassword("1A2B3").size() == expectedDevicePasswordHashSize);
                CHECK(testManager->hashDevicePassword("ABC").size() == expectedDevicePasswordHashSize);
                CHECK(testManager->hashDevicePassword("TEST").size() == expectedDevicePasswordHashSize);
                CHECK(testManager->hashUserPassword("ABCDEFG").size() == expectedUserPasswordHashSize);
                CHECK(testManager->hashUserPassword("123").size() == expectedUserPasswordHashSize);
                CHECK(testManager->hashUserPassword("Password!@#").size() == expectedUserPasswordHashSize);
            }
            
            AND_THEN("the old parameters can be discarded")
            {
                CHECK_NOTHROW(testManager->discardPreviousPasswordHashingParameters());
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("a Securable component is registered")
        {
            Testing::TestSecurable newSecurable;
            testManager->registerSecurableComponent(newSecurable);
            
            THEN("security requests can be created for it")
            {
                for(unsigned int i = 0; i < 1000; i++)
                {
                    INFO("Iteration [" + Utilities::Strings::toString(i) + "] ...");
                    
                    boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestOne> testInstruction_1(
                        new Testing::InstructionManagement_Sets::TestInstructions::DoTestOne());

                    boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo> testInstruction_2(
                        new Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo("test"));
                    
                    std::string rawPassword_1 = "passw0rd";
                    SecurityManagement_Types::UserAuthenticationRequest
                        userAnRequest(user1->getUsername(), rawPassword_1, newSecurable);

                    std::string rawPassword_2 = "PassW0rd1";
                    SecurityManagement_Types::DeviceAuthenticationRequest
                        devAnRequest(device1->getDeviceID(), rawPassword_2, newSecurable);

                    SecurityManagement_Types::AuthorizationRequest userAzRequest
                    (
                        user1->getUserID(),
                        newSecurable,
                        newSecurable.getComponentType(),
                        testInstruction_1
                    );
                    
                    SecurityManagement_Types::AuthorizationRequest devAzRequest
                    (
                        user1->getUserID(),
                        device1->getDeviceID(),
                        newSecurable,
                        newSecurable.getComponentType(),
                        testInstruction_2
                    );
                    
                    std::string rawPassword_3 = "some_password";
                    SecurityManagement_Types::DerivedCryptoDataGenerationRequest
                        dcdGenRequest(newSecurable, rawPassword_3);

                    SecurityManagement_Types::SymmetricCryptoDataGenerationRequest
                        scdGenRequest(newSecurable);

                    auto token_1 = testManager->postRequest(userAnRequest);
                    auto token_2 = testManager->postRequest(devAnRequest);
                    
                    CHECK_NOTHROW(token_1->get_future().get());
                    CHECK_NOTHROW(token_2->get_future().get());
                    
                    auto token_3 = testManager->postRequest(userAzRequest);
                    auto token_4 = testManager->postRequest(devAzRequest);
                    auto token_5 = testManager->postRequest(dcdGenRequest);
                    auto token_6 = testManager->postRequest(scdGenRequest);
                    
                    CHECK_NOTHROW(token_3->get_future().get());
                    CHECK_NOTHROW(token_4->get_future().get());
                    CHECK_NOTHROW(token_5->get_future().get());
                    CHECK_NOTHROW(token_6->get_future().get());
                }
            }
            
            AND_WHEN("it is deregistered")
            {
                CHECK_NOTHROW(testManager->deregisterSecurableComponent(newSecurable.getComponentType()));
                
                THEN("no security requests can be created for it")
                {
                    for(unsigned int i = 0; i < 1000; i++)
                    {
                        INFO("Iteration [" + Utilities::Strings::toString(i) + "] ...");

                        boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestOne> testInstruction_1(
                            new Testing::InstructionManagement_Sets::TestInstructions::DoTestOne());

                        boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo> testInstruction_2(
                            new Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo("test"));

                        std::string rawPassword_1 = "passw0rd";
                        SecurityManagement_Types::UserAuthenticationRequest
                            userAnRequest(user1->getUsername(), rawPassword_1, newSecurable);

                        std::string rawPassword_2 = "PassW0rd1";
                        SecurityManagement_Types::DeviceAuthenticationRequest
                            devAnRequest(device1->getDeviceID(), rawPassword_2, newSecurable);

                        SecurityManagement_Types::AuthorizationRequest userAzRequest
                        (
                            user1->getUserID(),
                            newSecurable,
                            newSecurable.getComponentType(),
                            testInstruction_1
                        );

                        SecurityManagement_Types::AuthorizationRequest devAzRequest
                        (
                            user1->getUserID(),
                            device1->getDeviceID(),
                            newSecurable,
                            newSecurable.getComponentType(),
                            testInstruction_2
                        );

                        std::string rawPassword_3 = "some_password";
                        SecurityManagement_Types::DerivedCryptoDataGenerationRequest
                            dcdGenRequest(newSecurable, rawPassword_3);

                        SecurityManagement_Types::SymmetricCryptoDataGenerationRequest
                            scdGenRequest(newSecurable);

                        auto token_1 = testManager->postRequest(userAnRequest);
                        auto token_2 = testManager->postRequest(devAnRequest);

                        CHECK_THROWS_AS(token_1->get_future().get(), std::logic_error);
                        CHECK_THROWS_AS(token_2->get_future().get(), std::logic_error);

                        auto token_3 = testManager->postRequest(userAzRequest);
                        auto token_4 = testManager->postRequest(devAzRequest);
                        auto token_5 = testManager->postRequest(dcdGenRequest);
                        auto token_6 = testManager->postRequest(scdGenRequest);

                        CHECK_THROWS_AS(token_3->get_future().get(), std::logic_error);
                        CHECK_THROWS_AS(token_4->get_future().get(), std::logic_error);
                        CHECK_THROWS_AS(token_5->get_future().get(), std::logic_error);
                        CHECK_THROWS_AS(token_6->get_future().get(), std::logic_error);
                    }
                }
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}

SCENARIO("A security manager rejects invalid requests",
         "[SecurityManager][SecurityManagement][Managers][Core]")
{
    GIVEN("a SecurityManager and initialization data")
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "./SecurityManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        SyncServer_Core::InstructionDispatcher * dispatcher = Testing::Fixtures::createInstructionDispatcher();
        SyncServer_Core::DatabaseManager * dbManager = Testing::Fixtures::createDatabaseManager();
        SyncServer_Core::SecurityManager * testManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, logger);
        
        PasswordData user1_pass(testManager->hashUserPassword("passw0rd"));
        PasswordData user2_pass(testManager->hashUserPassword("passw1rd"));
        PasswordData user3_pass(testManager->hashUserPassword("passw2rd"));
        PasswordData user4_pass(testManager->hashUserPassword("passw3rd"));
        PasswordData user5_pass(testManager->hashUserPassword("passw4rd"));
        PasswordData user6_pass(testManager->hashUserPassword("passw5rd"));
        UserDataContainerPtr user1(new UserDataContainer("TEST_USER_1", user1_pass, UserAccessLevel::ADMIN, false));
        UserDataContainerPtr user2(new UserDataContainer("TEST_USER_2", user2_pass, UserAccessLevel::ADMIN, false));
        UserDataContainerPtr user3(new UserDataContainer("TEST_USER_3", user3_pass, UserAccessLevel::NONE, false));
        UserDataContainerPtr user4(new UserDataContainer("TEST_USER_4", user4_pass, UserAccessLevel::USER, false));
        UserDataContainerPtr user5(new UserDataContainer("TEST_USER_5", user5_pass, UserAccessLevel::USER, false));
        UserDataContainerPtr user6(new UserDataContainer("TEST_USER_6", user6_pass, UserAccessLevel::USER, false));
        
        user1->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        user2->setLockedState(true);
        user5->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        user6->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DAL_CACHE));
        
        PasswordData device1_pass(testManager->hashDevicePassword("PassW0rd1"));
        PasswordData device2_pass(testManager->hashDevicePassword("PassW1rd1"));
        PasswordData device3_pass(testManager->hashDevicePassword("PassW2rd1"));
        DeviceDataContainerPtr device1(new DeviceDataContainer("TEST_DEVICE_1", device1_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        DeviceDataContainerPtr device2(new DeviceDataContainer("TEST_DEVICE_2", device2_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        DeviceDataContainerPtr device3(new DeviceDataContainer("TEST_DEVICE_3", device3_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        
        device1->setLockedState(true);
        
        dbManager->Users().addUser(user1);
        dbManager->Users().addUser(user2);
        dbManager->Users().addUser(user3);
        dbManager->Users().addUser(user4);
        dbManager->Users().addUser(user5);
        dbManager->Users().addUser(user6);
        dbManager->Devices().addDevice(device1);
        dbManager->Devices().addDevice(device2);
        dbManager->Devices().addDevice(device3);
        
        Testing::TestSecurable newSecurable;
        testManager->registerSecurableComponent(newSecurable);
            
        WHEN("invalid authentication requests are posted")
        {
            //invalid username
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_1("invalid_user", "passw0rd", newSecurable);

            auto token_1 = testManager->postRequest(userAnRequest_1);
            
            //user locked
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_2(user2->getUsername(), "passw1rd", newSecurable);

            auto token_2 = testManager->postRequest(userAnRequest_2);
            
            //invalid password
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_3(user1->getUsername(), "passw1rd", newSecurable);

            auto token_3 = testManager->postRequest(userAnRequest_3);
            
            //insufficient access level
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_4(user3->getUsername(), "passw2rd", newSecurable);

            auto token_4 = testManager->postRequest(userAnRequest_4);
            
            //no permissions
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_5(user4->getUsername(), "passw3rd", newSecurable);

            auto token_5 = testManager->postRequest(userAnRequest_5);
            
            //invalid device
            auto invalidDeviceID = boost::uuids::random_generator()();
            SecurityManagement_Types::DeviceAuthenticationRequest
                devAnRequest_1(invalidDeviceID, "PassW0rd1", newSecurable);
            
            auto token_6 = testManager->postRequest(devAnRequest_1);
            
            //device locked
            SecurityManagement_Types::DeviceAuthenticationRequest
                devAnRequest_2(device1->getDeviceID(), "PassW0rd1", newSecurable);
            
            auto token_7 = testManager->postRequest(devAnRequest_2);
            
            //invalid password
            SecurityManagement_Types::DeviceAuthenticationRequest
                devAnRequest_3(device2->getDeviceID(), "test123", newSecurable);
            
            auto token_8 = testManager->postRequest(devAnRequest_3);
            
            THEN("they are rejected")
            {
                CHECK_THROWS_AS(token_1->get_future().get(), SecurityManagement_Types::UserNotFoundException);
                CHECK_THROWS_AS(token_2->get_future().get(), SecurityManagement_Types::UserLockedException);
                CHECK_THROWS_AS(token_3->get_future().get(), SecurityManagement_Types::InvalidPassswordException);
                CHECK_THROWS_AS(token_4->get_future().get(), SecurityManagement_Types::InsufficientUserAccessException);
                //TODO - enable after fixing/replacing DebugDAL
                //CHECK_THROWS_AS(token_5->get_future().get(), SecurityManagement_Types::InsufficientUserAccessException);
                //TODO - remove after fixing/replacing DebugDAL
                CHECK_NOTHROW(token_5->get_future().get_or(nullptr));
                
                CHECK_THROWS_AS(token_6->get_future().get(), SecurityManagement_Types::DeviceNotFoundException);
                CHECK_THROWS_AS(token_7->get_future().get(), SecurityManagement_Types::DeviceLockedException);
                CHECK_THROWS_AS(token_8->get_future().get(), SecurityManagement_Types::InvalidPassswordException);
            }
        }
        
        WHEN("invalid authorization requests are posted")
        {
            boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestOne> testInstruction_1(
                new Testing::InstructionManagement_Sets::TestInstructions::DoTestOne());

            boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo> testInstruction_2(
                new Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo("test"));
            
            //invalid user ID
            auto invalidUserID = boost::uuids::random_generator()();
            SecurityManagement_Types::AuthorizationRequest userAzRequest_1
            (
                invalidUserID,
                newSecurable,
                newSecurable.getComponentType(),
                testInstruction_1
            );
            
            auto token_1 = testManager->postRequest(userAzRequest_1);
            
            //not authenticated
            SecurityManagement_Types::AuthorizationRequest userAzRequest_2
            (
                user1->getUserID(),
                newSecurable,
                newSecurable.getComponentType(),
                testInstruction_1
            );
            
            auto token_2 = testManager->postRequest(userAzRequest_2);

            //authenticated users are needed for the next requests
            std::string rawPassword_1 = "passw4rd";
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_1(user5->getUsername(), rawPassword_1, newSecurable);
            auto authToken_1 = testManager->postRequest(userAnRequest_1);
            CHECK_NOTHROW(authToken_1->get_future().get());
            
            std::string rawPassword_2 = "passw5rd";
            SecurityManagement_Types::UserAuthenticationRequest
                userAnRequest_2(user6->getUsername(), rawPassword_2, newSecurable);
            auto authToken_2 = testManager->postRequest(userAnRequest_2);
            CHECK_NOTHROW(authToken_2->get_future().get());
            
            //insufficient access level
            SecurityManagement_Types::AuthorizationRequest userAzRequest_3
            (
                user5->getUserID(),
                newSecurable,
                newSecurable.getComponentType(),
                testInstruction_1
            );
            
            auto token_3 = testManager->postRequest(userAzRequest_3);
            
            //insufficient permissions
            SecurityManagement_Types::AuthorizationRequest userAzRequest_4
            (
                user6->getUserID(),
                newSecurable,
                newSecurable.getComponentType(),
                testInstruction_1
            );
            
            auto token_4 = testManager->postRequest(userAzRequest_4);

            THEN("they are rejected")
            {
                CHECK_THROWS_AS(token_1->get_future().get(), SecurityManagement_Types::UserNotFoundException);
                CHECK_THROWS_AS(token_2->get_future().get(), SecurityManagement_Types::UserNotAuthenticatedException);
                CHECK_THROWS_AS(token_3->get_future().get(), SecurityManagement_Types::InsufficientUserAccessException);
                CHECK_THROWS_AS(token_4->get_future().get(), SecurityManagement_Types::InsufficientUserAccessException);
            }
        }
        
        dbManager->Users().removeUser(user1->getUserID());
        dbManager->Users().removeUser(user2->getUserID());
        dbManager->Users().removeUser(user3->getUserID());
        dbManager->Users().removeUser(user4->getUserID());
        dbManager->Users().removeUser(user5->getUserID());
        dbManager->Users().removeUser(user6->getUserID());
        dbManager->Devices().removeDevice(device1->getDeviceID());
        dbManager->Devices().removeDevice(device2->getDeviceID());
        dbManager->Devices().removeDevice(device3->getDeviceID());
        
        delete testManager;
        delete dbManager;
        delete dispatcher;
    }
}