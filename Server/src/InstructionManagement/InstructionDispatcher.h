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

#ifndef INSTRUCTIONDISPATCHER_H
#define	INSTRUCTIONDISPATCHER_H

#include <vector>
#include <string>
#include <boost/unordered_map.hpp>
#include <boost/thread/future.hpp>
#include "../Common/Types.h"
#include "Types/Types.h"
#include "Sets/InstructionSet.h"
#include "../Utilities/FileLogger.h"
#include "../Utilities/Tools.h"

#include "Interfaces/InstructionSource.h"
#include "Interfaces/InstructionTarget.h"

#include "../SecurityManagement/Types/SecurityTokens.h"

using Common_Types::UserAccessLevel;
using SecurityManagement_Types::AuthorizationTokenPtr;
using InstructionManagement_Sets::InstructionPtr;
using InstructionManagement_Sets::InstructionSet;
using InstructionManagement_Sets::InstructionSetPtr;
using InstructionManagement_Sets::InstructionBasePtr;
using InstructionManagement_Sets::InstructionSetBasePtr;
using InstructionManagement_Sets::InstructionResultFuture;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::InstructionSourceID;
using InstructionManagement_Types::INVALID_INSTRUCTION_SOURCE_ID;
using InstructionManagement_Interfaces::InstructionSource;
using InstructionManagement_Interfaces::InstructionTarget;

namespace SyncServer_Core
{
    //TODO - add support for multiple sets from single target
    /**
     * Class for managing instruction managing and dispatching.\n
     * 
     * Note #1: All registration functions are NOT thread-safe and must be called from 
     * the same thread. They must not be called while instructions are
     * being processed.
     * 
     * Note #2: Instruction processing IS thread-safe.
     * 
     * Note #3: In order for any instructions to be dispatched, at least one
     * source and one target must be registered.
     * 
     */
    class InstructionDispatcher
    {
        public:
            /** Parameters structure for holding <code>InstructionDispatcher</code> configuration data. */
            struct InstructionDispatcherParameters
            {
                /** Allowed/expected set types for the dispatcher. */
                std::vector<InstructionSetType> expectedSetTypes;
            };
            
            /**
             * Constructs a new instruction dispatcher with the specified configuration.\n
             * 
             * Note #1: All registration functions are NOT thread-safe and must be called from 
             * the same thread. They must not be called while instructions are
             * being processed.
             * 
             * Note #2: Instruction processing IS thread-safe.
             * 
             * Note #3: In order for any instructions to be dispatched, at least one
             * source and one target must be registered.
             * 
             * @param parameters the dispatcher's configuration data
             * @param debugLogger logger for debugging, if any
             */
            InstructionDispatcher(InstructionDispatcherParameters parameters, Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Clears the source and target data structures.
             */
            ~InstructionDispatcher();
            
            InstructionDispatcher() = delete;                                         //No default constructor
            InstructionDispatcher(const InstructionDispatcher&) = delete;             //Copying not allowed (pass/access only by reference/pointer)
            InstructionDispatcher& operator=(const InstructionDispatcher&) = delete;  //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Registers a new instruction source with the dispatcher.\n
             * 
             * Note: NOT thread-safe. All registration functions must be called from 
             * the same thread AND they must not be called while instructions are
             * being processed.
             * 
             * @param source the source to be registered
             */
            void registerInstructionSource(InstructionSource & source);
            
            /**
             * Registers a new instruction target with the dispatcher.
             * 
             * Note: NOT thread-safe. All registration functions must be called from 
             * the same thread AND they must not be called while instructions are
             * being processed.
             * 
             * @param (template) TInstructionTypeEnum template parameter denoting the type of 
             *         instructions that will be processed by the target
             * @param target the instruction target to be registered
             */
            template <typename TInstructionTypeEnum>
            void registerInstructionTarget(InstructionTarget<TInstructionTypeEnum> & target)
            {
                //restricts the template parameter
                static_assert(std::is_enum<TInstructionTypeEnum>::value, "registerInstructionTarget() > Supplied instruction type for <TInstructionTypeEnum> is not an enum class.");
                
                if(std::find(expectedSetTypes.begin(), expectedSetTypes.end(), target.getType()) == expectedSetTypes.end())
                {
                    logDebugMessage("(registerInstructionTarget) > Failed to register target; the instruction set of the target is not expected.");
                    return;
                }
                
                if(targetSets.find(target.getType()) == targetSets.end())
                {
                    InstructionSetPtr<TInstructionTypeEnum> targetSet(new InstructionSet<TInstructionTypeEnum>());
                    if(target.registerInstructionSet(targetSet))
                        targetSets.insert({target.getType(), targetSet});
                    else
                        logDebugMessage("(registerInstructionTarget) > Failed to register a new instruction set with the supplied target.");
                }
                else
                    logDebugMessage("(registerInstructionTarget) > The supplied target is already registered.");
            }
            
            /**
             * Retrieves the minimum required user access level for the specified 
             * instruction set type.
             * 
             * @param set the set type for which to retrieve the access level
             * @return the requested minimum access level or <code>INVALID</code>,
             * if the set is not found
             */
            UserAccessLevel getMinimumAccessLevelForSet(InstructionSetType set) const
            {
                auto setData = targetSets.find(set);
                if(setData != targetSets.end())
                    return setData->second->getMinimumAccessLevel();
                else
                    return UserAccessLevel::INVALID;
            }
            
        private:
            //Configuration
            std::vector<InstructionSetType> expectedSetTypes;
            Utilities::FileLogger * debugLogger;
            
            //Targets
            boost::unordered_map<InstructionSetType, InstructionSetBasePtr> targetSets;
            
            //Sources
            InstructionSourceID nextSourceID = INVALID_INSTRUCTION_SOURCE_ID;
            boost::unordered_map<InstructionSourceID, std::vector<InstructionSetType>> sources;
            
            /**
             * Processes the supplied instruction from the specified source.
             * 
             * @param sourceID the ID of the source that sent the instruction
             * @param instruction the instruction to be processed
             * @param token the authorization token associated with the instruction
             */
            void processInstruction(InstructionSourceID sourceID, InstructionBasePtr instruction, AuthorizationTokenPtr token);
            
            /**
             * Logs the specified message, if a debugging file logger is assigned to the dispatcher.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string message)
            {
                if(debugLogger != nullptr)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "InstructionDispatcher " + message);
            }
    };
}
#endif	/* INSTRUCTIONDISPATCHER_H */

