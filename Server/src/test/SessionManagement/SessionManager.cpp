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

#include "../BasicSpec.h"
#include "../Fixtures.h"
#include "../../main/Utilities/Strings/Common.h"

SCENARIO("A session manager is created and can manage sessions",
         "[SessionManager][SessionManagement][Managers][Core]")
{
    GIVEN("a SessionManager with a basic configuration")
    {
        Utilities::FileLoggerParameters loggerParams_1
        {
            "test_data/SecurityManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters loggerParams_2
        {
            "test_data/SessionManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLogger secLogger(loggerParams_1), sessLogger(loggerParams_2);
        
        SyncServer_Core::InstructionDispatcher * dispatcher = Testing::Fixtures::createInstructionDispatcher();
        SyncServer_Core::DatabaseManager * dbManager = Testing::Fixtures::createDatabaseManager();
        SyncServer_Core::SecurityManager * secManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, &secLogger);
        SyncServer_Core::SessionManager * testManager = Testing::Fixtures::createSessionManager(dbManager, secManager, &sessLogger);
        
        std::string user1_rawPass = "passw0rd", device1_rawPass = "PassW0rd1";
        
        PasswordData user1_pass(secManager->hashUserPassword(user1_rawPass));
        UserDataContainerPtr user1(new UserDataContainer("TEST_USER_1", user1_pass, UserAccessLevel::ADMIN, false));
        user1->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        
        PasswordData device1_pass(secManager->hashDevicePassword(device1_rawPass));
        DeviceDataContainerPtr device1(new DeviceDataContainer("TEST_DEVICE_1", device1_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        
        dbManager->Users().addUser(user1);
        dbManager->Devices().addDevice(device1);
        
        WHEN("sessions are opened")
        {
            auto userSession_1 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, true);
            auto userSession_2 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::DATA, true);
            auto deviceSession_1 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, true);
            auto deviceSession_2 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::DATA, true);
            
            THEN("their stats can be updated")
            {
                CHECK_NOTHROW(testManager->addCommandsReceived(userSession_1, 123));
                CHECK_NOTHROW(testManager->addCommandsSent(deviceSession_1, 456));
                CHECK_NOTHROW(testManager->addDataReceived(userSession_2, 789));
                CHECK_NOTHROW(testManager->addDataSent(deviceSession_2, 111));
            }
            
            AND_THEN("they can be re-authenticated")
            {
                waitFor(6);
                CHECK_NOTHROW(testManager->reauthenticateSession(userSession_1, user1->getUsername(), user1_rawPass));
                CHECK_NOTHROW(testManager->reauthenticateSession(userSession_2, user1->getUsername(), user1_rawPass));
                CHECK_NOTHROW(testManager->reauthenticateSession(deviceSession_1, device1->getDeviceID(), device1_rawPass));
                CHECK_NOTHROW(testManager->reauthenticateSession(deviceSession_2, device1->getDeviceID(), device1_rawPass));
            }
            
            AND_THEN("they can be closed")
            {
                CHECK_NOTHROW(testManager->closeSession(userSession_1));
                CHECK_NOTHROW(testManager->closeSession(userSession_2));
                CHECK_NOTHROW(testManager->closeSession(deviceSession_1));
                CHECK_NOTHROW(testManager->closeSession(deviceSession_2));
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("sessions are opened and closed multiple times")
        {
            unsigned int maxTestCount = 1000;
            
            for(unsigned int i = 0; i < maxTestCount; i++)
            {
                INFO("Iteration [" + Utilities::Strings::toString(i) + "] ...");
                
                auto userSession_1 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, true);
                auto userSession_2 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::DATA, true);
                auto deviceSession_1 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, true);
                auto deviceSession_2 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::DATA, true);
                
                CHECK_NOTHROW(testManager->closeSession(userSession_1));
                CHECK_NOTHROW(testManager->closeSession(userSession_2));
                CHECK_NOTHROW(testManager->closeSession(deviceSession_1));
                CHECK_NOTHROW(testManager->closeSession(deviceSession_2));
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}

SCENARIO("A session manager rejects invalid session requests",
         "[SessionManager][SessionManagement][Managers][Core]")
{
    GIVEN("a SessionManager with a basic configuration")
    {
        Utilities::FileLoggerParameters loggerParams_1
        {
            "test_data/SecurityManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };
        
        Utilities::FileLoggerParameters loggerParams_2
        {
            "test_data/SessionManager.log",  //logFilePath
            32*1024*1024,           //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLogger secLogger(loggerParams_1), sessLogger(loggerParams_2);
        
        SyncServer_Core::InstructionDispatcher * dispatcher = Testing::Fixtures::createInstructionDispatcher();
        SyncServer_Core::DatabaseManager * dbManager = Testing::Fixtures::createDatabaseManager();
        SyncServer_Core::SecurityManager * secManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, &secLogger);
        SyncServer_Core::SessionManager * testManager = Testing::Fixtures::createSessionManager(dbManager, secManager, &sessLogger);
        
        std::string user1_rawPass = "passw0rd", device1_rawPass = "PassW0rd1";
        
        PasswordData user1_pass(secManager->hashUserPassword(user1_rawPass));
        UserDataContainerPtr user1(new UserDataContainer("TEST_USER_1", user1_pass, UserAccessLevel::ADMIN, false));
        user1->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::TEST));
        
        PasswordData device1_pass(secManager->hashDevicePassword(device1_rawPass));
        DeviceDataContainerPtr device1(new DeviceDataContainer("TEST_DEVICE_1", device1_pass, user1->getUserID(), DataTransferType::PULL, PeerType::SERVER));
        
        dbManager->Users().addUser(user1);
        dbManager->Devices().addDevice(device1);
        
        WHEN("an invalid session type is specified")
        {
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::INVALID, false), std::invalid_argument);
                CHECK_THROWS_AS(testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::INVALID, false), std::invalid_argument);
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("too many sessions have already been opened")
        {
            auto userSession_1 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, false);
            auto userSession_2 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, false);
            auto userSession_3 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, false);
            auto deviceSession_1 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, false);
            auto deviceSession_2 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, false);
            auto deviceSession_3 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, false);
            
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, false), SessionManagement_Types::TooManyUserSessionsException);
                CHECK_THROWS_AS(testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::DATA, false), SessionManagement_Types::TooManyUserSessionsException);
                CHECK_THROWS_AS(testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, false), SessionManagement_Types::TooManyDeviceSessionsException);
                CHECK_THROWS_AS(testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::DATA, false), SessionManagement_Types::TooManyDeviceSessionsException);
            }
            
            CHECK_NOTHROW(testManager->closeSession(userSession_1));
            CHECK_NOTHROW(testManager->closeSession(userSession_2));
            CHECK_NOTHROW(testManager->closeSession(userSession_3));
            CHECK_NOTHROW(testManager->closeSession(deviceSession_1));
            CHECK_NOTHROW(testManager->closeSession(deviceSession_2));
            CHECK_NOTHROW(testManager->closeSession(deviceSession_3));
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("re-authenticating a non-existing session")
        {
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->reauthenticateSession(100000123, user1->getUsername(), user1_rawPass), std::invalid_argument);
                CHECK_THROWS_AS(testManager->reauthenticateSession(100000456, user1->getUsername(), user1_rawPass), std::invalid_argument);
                CHECK_THROWS_AS(testManager->reauthenticateSession(100000789, device1->getDeviceID(), device1_rawPass), std::invalid_argument);
                CHECK_THROWS_AS(testManager->reauthenticateSession(100000111, device1->getDeviceID(), device1_rawPass), std::invalid_argument);
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("re-authenticating a session that is not eligible")
        {
            auto userSession_1 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::COMMAND, false);
            auto userSession_2 = testManager->openSession(user1->getUsername(), user1_rawPass, SessionType::DATA, false);
            auto deviceSession_1 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::COMMAND, false);
            auto deviceSession_2 = testManager->openSession(device1->getDeviceID(), device1_rawPass, SessionType::DATA, false);
            
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->reauthenticateSession(userSession_1, user1->getUsername(), user1_rawPass), std::logic_error);
                CHECK_THROWS_AS(testManager->reauthenticateSession(userSession_2, user1->getUsername(), user1_rawPass), std::logic_error);
                CHECK_THROWS_AS(testManager->reauthenticateSession(deviceSession_1, device1->getDeviceID(), device1_rawPass), std::logic_error);
                CHECK_THROWS_AS(testManager->reauthenticateSession(deviceSession_2, device1->getDeviceID(), device1_rawPass), std::logic_error);
            }
            
            CHECK_NOTHROW(testManager->closeSession(userSession_1));
            CHECK_NOTHROW(testManager->closeSession(userSession_2));
            CHECK_NOTHROW(testManager->closeSession(deviceSession_1));
            CHECK_NOTHROW(testManager->closeSession(deviceSession_2));
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("closing a non-existing session")
        {
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->closeSession(200000123), std::invalid_argument);
                CHECK_THROWS_AS(testManager->closeSession(200000456), std::invalid_argument);
                CHECK_THROWS_AS(testManager->closeSession(200000789), std::invalid_argument);
                CHECK_THROWS_AS(testManager->closeSession(200000111), std::invalid_argument);
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
        
        WHEN("making invalid stats updates")
        {
            THEN("the requests are rejected")
            {
                CHECK_THROWS_AS(testManager->addCommandsReceived(300000123, 123), std::invalid_argument);
                CHECK_THROWS_AS(testManager->addCommandsSent(300000456, 456), std::invalid_argument);
                CHECK_THROWS_AS(testManager->addDataReceived(300000789, 789), std::invalid_argument);
                CHECK_THROWS_AS(testManager->addDataSent(300000111, 12), std::invalid_argument);
            }
            
            dbManager->Users().removeUser(user1->getUserID());
            dbManager->Devices().removeDevice(device1->getDeviceID());

            delete testManager;
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}
