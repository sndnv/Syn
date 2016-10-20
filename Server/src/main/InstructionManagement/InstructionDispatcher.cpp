/**
 * Copyright (C) 2014 https://github.com/sndnv
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

#include "InstructionDispatcher.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Instructions.h"

SyncServer_Core::InstructionDispatcher::InstructionDispatcher
(InstructionDispatcherParameters parameters, Utilities::FileLoggerPtr logger)
        : expectedSetTypes(parameters.expectedSetTypes), debugLogger(logger)
{
    logMessage(LogSeverity::Debug, "() > Dispatcher created.");
}

SyncServer_Core::InstructionDispatcher::~InstructionDispatcher()
{
    logMessage(LogSeverity::Debug, "(~) > Destruction initiated.");
    expectedSetTypes.clear();
    targetSets.clear();
    sources.clear();
}

void SyncServer_Core::InstructionDispatcher::registerInstructionSource(InstructionSource & source)
{
    std::vector<InstructionSetType> requiredTypes = source.getRequiredInstructionSetTypes();

    if(requiredTypes.size() == 0)
    {
        logMessage(LogSeverity::Error, "(registerInstructionSource) > Failed to register source;"
                " no instruction set types specified.");
        
        return;
    }

    for(InstructionSetType currentType : requiredTypes)
    {
        if(std::find(expectedSetTypes.begin(), expectedSetTypes.end(), currentType) == expectedSetTypes.end())
        {
            logMessage(LogSeverity::Error, "(registerInstructionSource) > Failed to register source;"
                    " one or more of the required instructions are not expected.");
            
            return;
        }
    }

    InstructionSourceID currentSourceID = ++nextSourceID;
    auto currentSourceInstructionHandler = [&, currentSourceID]
    (InstructionBasePtr instruction, AuthorizationTokenPtr token)
    {
        processInstruction(currentSourceID, instruction, token);
    };

    if(source.registerInstructionHandler(currentSourceInstructionHandler))
        sources.insert({currentSourceID, requiredTypes});
    else
        logMessage(LogSeverity::Error, "(registerInstructionSource) > Failed to register"
                " a new instruction handler with the supplied source.");
}

void SyncServer_Core::InstructionDispatcher::processInstruction
(InstructionSourceID sourceID, InstructionBasePtr instruction, AuthorizationTokenPtr token)
{
    if(!instruction->isValid())
    {
        logMessage(LogSeverity::Error, "(processInstruction) > Instruction processing failed;"
                " the specified instruction is not valid.");
        
        return;
    }
    
    auto source = sources.find(sourceID);
    if(source != sources.end())
    {
        if(std::find((*source).second.begin(), (*source).second.end(), instruction->getParentSet()) != (*source).second.end())
        {
            targetSets[instruction->getParentSet()]->processInstruction(instruction, token);
            logMessage(LogSeverity::Debug, "(processInstruction) > Instruction from source ["
                    + Convert::toString(sourceID) + "] sent to target [" 
                    + Convert::toString(instruction->getParentSet()) + "].");
        }
        else
        {
            logMessage(LogSeverity::Error, "(processInstruction) > Instruction processing failed;"
                    " the required instruction set is not allowed for the specified source.");
        }
    }
    else
    {
        logMessage(LogSeverity::Error, "(processInstruction) > Instruction processing failed;"
                " the source of the instruction was not found.");
    }
}

