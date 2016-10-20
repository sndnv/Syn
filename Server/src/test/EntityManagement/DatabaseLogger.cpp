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
#include "../../main/EntityManagement/Interfaces/DatabaseLoggingSource.h"
#include "../../main/EntityManagement/DatabaseLogger.h"
#include "TestLoggingSource.h"
#include "EntityInstructionSources.h"

using EntityManagement::DatabaseLogger;
using Testing::TestLoggingSource;
using Testing::DatabaseLoggerInstructionSource;

SCENARIO("A database logger is created, registers logging sources and can process messages & instructions",
         "[DatabaseLogger][EntityManagement][Managers]")
{
    GIVEN("A new DatabaseLogger")
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "test_data/DatabaseLogger.log",  //logFilePath
            32*1024*1024,                    //maximumFileSize
            Utilities::FileLogSeverity::Debug
        };

        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        auto dbManager = Testing::Fixtures::createDatabaseManager();
        std::vector<InstructionManagement_Types::InstructionSetType> dispatcherSets;
        dispatcherSets.push_back(InstructionManagement_Types::InstructionSetType::DATABASE_LOGGER);
        auto dispatcher = Testing::Fixtures::createInstructionDispatcher(dispatcherSets, logger);
        auto secManager = Testing::Fixtures::createSecurityManager(dispatcher, dbManager, logger);
        auto sessManager = Testing::Fixtures::createSessionManager(dbManager, secManager, logger);
        
        DatabaseLogger::DatabaseLoggerParameters dbLoggerParams
        {
            *dbManager,
            *secManager,
            LogSeverity::Debug
        };

        DatabaseLogger dbLogger(dbLoggerParams, logger);
        secManager->registerSecurableComponent(dbLogger);
        dispatcher->registerInstructionTarget<DatabaseLoggerInstructionType>(dbLogger);
        
        std::string rawPassword = "passw0rd1";
        PasswordData adminPassword(secManager->hashUserPassword(rawPassword));
        UserDataContainerPtr testAdminUser(new UserDataContainer("TEST_ADMIN_1", adminPassword, UserAccessLevel::ADMIN, false));
        testAdminUser->addAccessRule(UserAuthorizationRule(InstructionManagement_Types::InstructionSetType::DATABASE_LOGGER));
        dbManager->Users().addUser(testAdminUser);
        CHECK_NOTHROW(sessManager->openSession("TEST_ADMIN_1", rawPassword, SessionType::COMMAND, true));
        
        DatabaseLoggerInstructionSource testInstructionSource(secManager, &dbLogger, testAdminUser->getUserID());
        dispatcher->registerInstructionSource(testInstructionSource);
        
        TestLoggingSource testSource;
        dbLogger.registerLoggingSource(testSource);
        
        WHEN("new messages are added")
        {
            testSource.logTestMessage(LogSeverity::Debug, "test_message_1");
            testSource.logTestMessage(LogSeverity::Error, "test_message_2");
            testSource.logTestMessage(LogSeverity::Warning, "test_message_3");
            testSource.logTestMessage(LogSeverity::Info, "test_message_4");
            testSource.logTestMessage(LogSeverity::None, "test_message_5");
            
            waitFor(1);
            
            THEN("they can be retrieved from the database")
            {
                auto logs = dbManager->Logs().getLogsBySource(testSource.getSourceName());
                CHECK(logs.size() == 5);
            }
            
            AND_WHEN("instructions are sent to the DatabaseLogger")
            {
                CHECK_NOTHROW(testInstructionSource.doInstruction_DebugGetState());
                auto result_1 = testInstructionSource.doInstruction_GetLogsByConstraint(DatabaseSelectConstraints::LOGS::GET_ALL, 0);
                auto result_2 = testInstructionSource.doInstruction_UpdateDefaultLoggingLevel(LogSeverity::Info);
                auto result_3 = testInstructionSource.doInstruction_UpdateSourceLoggingLevel(1, LogSeverity::Warning);

                THEN("it processes them successfully")
                {
                    CHECK(result_1->result.size() == 5);
                    CHECK(result_2->result);
                    CHECK(result_3->result);
                }
            }
            
            delete secManager;
            delete dbManager;
            delete dispatcher;
        }
    }
}
