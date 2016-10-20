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

#include "DatabaseLogger.h"
#include <vector>
#include <boost/thread/lock_guard.hpp>
#include "../Utilities/Strings/Common.h"

namespace Convert = Utilities::Strings;
namespace Instructions = InstructionManagement_Sets::DatabaseLoggerInstructions;
namespace InstructionResults = InstructionManagement_Sets::DatabaseLoggerInstructions::Results;

EntityManagement::DatabaseLogger::DatabaseLogger
(const DatabaseLoggerParameters & params,  Utilities::FileLoggerPtr debugLogger)
: debugLogger(debugLogger), databaseManager(params.databaseManager),
  securityManager(params.securityManager), isDisabled(false),
  defaultMinSeverity(params.defaultMinSeverity), lastSourceID(0),
  instructionsReceived(0), instructionsProcessed(0)
{
    if(defaultMinSeverity == LogSeverity::INVALID)
    {
        throw std::invalid_argument("DatabaseLogger::() > Invalid default minimum"
                                    " log severity encountered.");
    }
}

EntityManagement::DatabaseLogger::~DatabaseLogger()
{
    logDebugMessage("(~) > Destruction initiated.");

    isDisabled = true;

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);
    authorizationTokens.clear();

    sourcesMinLogSeverity.clear();
}

void EntityManagement::DatabaseLogger::postAuthorizationToken
(const SecurityManagement_Types::AuthorizationTokenPtr token)
{
    if(getType() != token->getAuthorizedSet())
     {
         throw std::logic_error("DatabaseLogger::postAuthorizationToken() > The token with ID ["
                 + Convert::toString(token->getID()) + "] is not for the expected instruction set.");
     }

     boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

     if(authorizationTokens.find(token->getID()) == authorizationTokens.end())
     {
         authorizationTokens.insert({token->getID(), token});
     }
     else
     {
         throw std::logic_error("DatabaseLogger::postAuthorizationToken() > A token with ID ["
                 + Convert::toString(token->getID()) + "] is already present.");
     }
}

bool EntityManagement::DatabaseLogger::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<DatabaseLoggerInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::ADMIN);

        try
        {
            set->bindInstructionHandler(DatabaseLoggerInstructionType::GET_LOG,
                                        getLogHandlerBind);
            
            set->bindInstructionHandler(DatabaseLoggerInstructionType::GET_LOGS_BY_CONSTRAINT,
                                        getLogsByConstraintHandlerBind);
            
            set->bindInstructionHandler(DatabaseLoggerInstructionType::UPDATE_SOURCE_LOGGING_LEVEL,
                                        updateSourceLoggingLevelHandlerBind);
            
            set->bindInstructionHandler(DatabaseLoggerInstructionType::UPDATE_DEFAULT_LOGGING_LEVEL,
                                        updateDefaultLoggingLevelHandlerBind);
            
            set->bindInstructionHandler(DatabaseLoggerInstructionType::DEBUG_GET_STATE,
                                        debugGetStateHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logDebugMessage("(registerInstructionSet) > Exception encountered: <"
                            + std::string(ex.what()) + ">");
            return false;
        }

        return true;
    }
    else
    {
        logDebugMessage("(registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}

void EntityManagement::DatabaseLogger::registerLoggingSource(DatabaseLoggingSource & source)
{
    DatabaseLoggingSourceID currentSourceID = ++lastSourceID;
    auto currentSourceLoggingHandler = [&, currentSourceID](LogSeverity severity, const std::string & message)
    {
        if(isDisabled)
            return;

        if(severity >= sourcesMinLogSeverity.at(currentSourceID))
        {
            LogDataContainerPtr newLogContainer
            (
                new LogDataContainer
                (
                    severity,
                    source.getSourceName(),
                    boost::posix_time::second_clock::universal_time(),
                    message
                )
            );
            
            databaseManager.Logs().addLogAsync(newLogContainer);
        }
    };

    if(source.registerLoggingHandler(currentSourceLoggingHandler))
    {
        sourcesMinLogSeverity.insert({currentSourceID, defaultMinSeverity});
    }
    else
        logDebugMessage("(registerLoggingSource) > Failed to register a new logging handler"
                        " with the supplied source.");
}

void EntityManagement::DatabaseLogger::getLogHandler
(InstructionPtr<DatabaseLoggerInstructionType> instruction)
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

    LogDataContainerPtr resultData;
    boost::shared_ptr<Instructions::GetLog> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::GetLog>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Logs().getLog(actualInstruction->logID);
    }

    auto result = boost::shared_ptr<InstructionResults::GetLog>(
        new InstructionResults::GetLog{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DatabaseLogger::getLogsByConstraintHandler
(InstructionPtr<DatabaseLoggerInstructionType> instruction)
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

    std::vector<LogDataContainerPtr> resultData;
    boost::shared_ptr<Instructions::GetLogsByConstraint> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::GetLogsByConstraint>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Logs().getLogsByConstraint(actualInstruction->constraintType,
                                                                actualInstruction->constraintValue);
    }

    auto result = boost::shared_ptr<InstructionResults::GetLogsByConstraint>(
        new InstructionResults::GetLogsByConstraint{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DatabaseLogger::updateSourceLoggingLevelHandler
(InstructionPtr<DatabaseLoggerInstructionType> instruction)
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
    boost::shared_ptr<Instructions::UpdateSourceLoggingLevel> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UpdateSourceLoggingLevel>(instruction);

    if(actualInstruction)
    {
        auto currentSourceData = sourcesMinLogSeverity.find(actualInstruction->sourceID);
        if(currentSourceData != sourcesMinLogSeverity.end())
        {
            if(currentSourceData->second != actualInstruction->newLogSeverity)
            {
                currentSourceData->second = actualInstruction->newLogSeverity;
                resultValue = true;
            }
        }
        else
        {
            logDebugMessage("(updateSourceLoggingLevelHandler) > The specified source ["
                            + Convert::toString(actualInstruction->sourceID) + "] was not found.");
            
            throwInstructionException("DatabaseLogger::updateSourceLoggingLevelHandler() > The specified source ["
                                      + Convert::toString(actualInstruction->sourceID)
                                      + "] was not found..", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UpdateSourceLoggingLevel>(
        new InstructionResults::UpdateSourceLoggingLevel{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DatabaseLogger::updateDefaultLoggingLevelHandler
(InstructionPtr<DatabaseLoggerInstructionType> instruction)
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
    boost::shared_ptr<Instructions::UpdateDefaultLoggingLevel> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::UpdateDefaultLoggingLevel>(instruction);

    if(actualInstruction)
    {
        if(defaultMinSeverity != actualInstruction->newLogSeverity)
        {
            defaultMinSeverity = actualInstruction->newLogSeverity;
            resultValue = true;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::UpdateDefaultLoggingLevel>(
        new InstructionResults::UpdateDefaultLoggingLevel{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DatabaseLogger::debugGetStateHandler
(InstructionPtr<DatabaseLoggerInstructionType> instruction)
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
        resultData += "isDisabled;" + Convert::toString(isDisabled) + "\n";
        resultData += "defaultMinSeverity;" + Convert::toString(defaultMinSeverity) + "\n";
        resultData += "nextSourceID;" + Convert::toString(lastSourceID) + "\n";
        resultData += "sourcesMinLogSeverity size;" + Convert::toString(sourcesMinLogSeverity.size()) + "\n";

        if(sourcesMinLogSeverity.size() > 0)
        {
            for(auto currentSourceData : sourcesMinLogSeverity)
            {
                resultData += "sourcesMinLogSeverity [" + Convert::toString(currentSourceData.first) + "];"
                              + Convert::toString(currentSourceData.second) + "\n";
            }
        }
    }

    auto result = boost::shared_ptr<InstructionResults::DebugGetState>(
        new InstructionResults::DebugGetState{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::DatabaseLogger::verifyAuthorizationToken(AuthorizationTokenPtr token)
{
    ++instructionsReceived;
    if(!token)
    {
       throw InvalidAuthorizationTokenException("DatabaseLogger::verifyAuthorizationToken() > "
                "An empty token was supplied."); 
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    auto requestedToken = authorizationTokens.find(token->getID());
    if(requestedToken != authorizationTokens.end())
    {
        if(*(requestedToken->second) == *token && token->getAuthorizedSet() == getType())
        {
            authorizationTokens.erase(requestedToken);
            ++instructionsProcessed;
        }
        else
        {
            throw InvalidAuthorizationTokenException("DatabaseLogger::verifyAuthorizationToken() > "
                    "The supplied token [" + Convert::toString(token->getID())
                    + "] does not match the one expected by the logger.");
        }
    }
    else
    {
        throw InvalidAuthorizationTokenException("DatabaseLogger::verifyAuthorizationToken() > "
                "The supplied token [" + Convert::toString(token->getID()) + "] was not found.");
    }
}
