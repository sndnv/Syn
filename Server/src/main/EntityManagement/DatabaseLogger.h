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

#ifndef ENTITY_MANAGEMENT_DATABASE_LOGGER_H
#define	ENTITY_MANAGEMENT_DATABASE_LOGGER_H

#include <atomic>
#include <string>

#include <boost/thread/mutex.hpp>

#include "../Common/Types.h"
#include "../Utilities/FileLogger.h"
#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Types/SecurityTokens.h"
#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Types/Exceptions.h"
#include "../SecurityManagement/Interfaces/Securable.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../InstructionManagement/Sets/DatabaseLoggerInstructionSet.h"

#include "Interfaces/DatabaseLoggingSource.h"
#include "Types/Types.h"

//Database and Security Management
using SyncServer_Core::DatabaseManager;
using SyncServer_Core::SecurityManager;

//Exceptions
using SecurityManagement_Types::InvalidAuthorizationTokenException;

//Entity Management
using EntityManagement_Interfaces::DatabaseLoggingSource;
using EntityManagement_Types::DatabaseLoggingSourceID;
using EntityManagement_Types::INVALID_DATABASE_LOGGING_SOURCE_ID;

//Misc
using Common_Types::LogSeverity;
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::INVALID_TOKEN_ID;
using SecurityManagement_Types::AuthorizationTokenPtr;
using DatabaseManagement_Containers::LogDataContainer;
using DatabaseManagement_Containers::LogDataContainerPtr;
using InstructionManagement_Types::DatabaseLoggerInstructionType;

namespace EntityManagement
{
    /**
     * Class for managing database logging activities.
     */
    class DatabaseLogger final
    : public SecurityManagement_Interfaces::Securable,
      public InstructionManagement_Interfaces::InstructionTarget<DatabaseLoggerInstructionType>
    {
        public:
            /** Parameters structure holding <code>DatabaseLogger</code> configuration. */
            struct DatabaseLoggerParameters
            {
                /** Reference to a valid database manager instance */
                DatabaseManager & databaseManager;
                /** Reference to a valid security manager instance */
                SecurityManager & securityManager;
                /** Minimum default log severity for all source components */
                LogSeverity defaultMinSeverity;
            };
            
            /**
             * Constructs a new database logger object with the specified configuration.
             * 
             * @param params the logger configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             */
            DatabaseLogger(const DatabaseLoggerParameters & params,  Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());

            /**
             * Clears all data structures.
             */
            ~DatabaseLogger();
            
            DatabaseLogger() = delete;
            DatabaseLogger(const DatabaseLogger&) = delete;
            DatabaseLogger& operator=(const DatabaseLogger&) = delete;
            
            void postAuthorizationToken(const SecurityManagement_Types::AuthorizationTokenPtr token) override;
            
            SecurityManagement_Types::SecurableComponentType getComponentType() const override
            {
                return SecurityManagement_Types::SecurableComponentType::DATABASE_LOGGER;
            }
            
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<DatabaseLoggerInstructionType> set) const override;
            
            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::DATABASE_LOGGER;
            }
            
            /**
             * Registers a new logging source component.
             * 
             * Note: NOT thread-safe. All component registration must be done during
             * system initialization.
             * 
             * @param source the source component to be registered
             */
            void registerLoggingSource(DatabaseLoggingSource & source);
            
        private:
            Utilities::FileLoggerPtr debugLogger;//logger for debugging
            
            //Required Managers
            DatabaseManager & databaseManager;
            SecurityManager & securityManager;
            
            //Configuration
            std::atomic<bool> isDisabled;           //denotes whether the logger is disabled or not
            LogSeverity defaultMinSeverity;         //default minimum log severity for all new sources
            DatabaseLoggingSourceID lastSourceID;   //the ID of the last source to be registered
            boost::unordered_map<DatabaseLoggingSourceID, LogSeverity> sourcesMinLogSeverity; //minimum severity for each source
            
            //Instruction Management
            boost::mutex instructionDataMutex;  //instruction data mutex
            boost::unordered_map<TokenID, AuthorizationTokenPtr> authorizationTokens; //expected authorization tokens
            
            //Stats
            unsigned long instructionsReceived; //number of instructions received
            unsigned long instructionsProcessed;//number of instructions processed
            
            /**
             * Sets an exception with the specified message in the supplied 
             * instruction's promise.
             * 
             * Note: Always sets <code>std::runtime_error</code> exception.
             * 
             * @param message the message for the exception
             * @param instruction the instruction in which the exception is to be set
             */
            void throwInstructionException(const std::string & message, InstructionPtr<DatabaseLoggerInstructionType> instruction)
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
            
            //Instruction Handlers
            void getLogHandler(InstructionPtr<DatabaseLoggerInstructionType> instruction);
            void getLogsByConstraintHandler(InstructionPtr<DatabaseLoggerInstructionType> instruction);
            void updateSourceLoggingLevelHandler(InstructionPtr<DatabaseLoggerInstructionType> instruction);
            void updateDefaultLoggingLevelHandler(InstructionPtr<DatabaseLoggerInstructionType> instruction);
            void debugGetStateHandler(InstructionPtr<DatabaseLoggerInstructionType> instruction);
            
            //Instruction Handlers Function Binds
            std::function<void(InstructionPtr<DatabaseLoggerInstructionType>)> getLogHandlerBind =
                boost::bind(&EntityManagement::DatabaseLogger::getLogHandler, this, _1);
            
            std::function<void(InstructionPtr<DatabaseLoggerInstructionType>)> getLogsByConstraintHandlerBind =
                boost::bind(&EntityManagement::DatabaseLogger::getLogsByConstraintHandler, this, _1);
            
            std::function<void(InstructionPtr<DatabaseLoggerInstructionType>)> updateSourceLoggingLevelHandlerBind =
                boost::bind(&EntityManagement::DatabaseLogger::updateSourceLoggingLevelHandler, this, _1);
            
            std::function<void(InstructionPtr<DatabaseLoggerInstructionType>)> updateDefaultLoggingLevelHandlerBind =
                boost::bind(&EntityManagement::DatabaseLogger::updateDefaultLoggingLevelHandler, this, _1);
            
            std::function<void(InstructionPtr<DatabaseLoggerInstructionType>)> debugGetStateHandlerBind =
                boost::bind(&EntityManagement::DatabaseLogger::debugGetStateHandler, this, _1);
            
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
             * Logs the specified message, if a debugging file logger is assigned to the logger.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string & message) const
            {
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "DatabaseLogger " + message);
            }
    };
}

#endif	/* ENTITY_MANAGEMENT_DATABASE_LOGGER_H */

