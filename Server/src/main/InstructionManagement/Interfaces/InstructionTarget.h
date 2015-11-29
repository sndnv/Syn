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

#ifndef INSTRUCTION_TARGET_H
#define	INSTRUCTION_TARGET_H

#include "../Types/Types.h"
#include "../Sets/InstructionSet.h"

namespace InstructionManagement_Interfaces
{
    /**
     * Interface for defining a target for instruction management.\n
     * 
     * A target needs to be registered with an <code>InstructionDispatcher</code>
     * before it is able to receive instructions from sources.
     * 
     * @param (template) TInstructionTypeEnum template parameter denoting the type of instructions
     * that will be processed by the target
     */
    template <typename TInstructionTypeEnum>
    class InstructionTarget
    {
        //restricts the template parameter
        static_assert(std::is_enum<TInstructionTypeEnum>::value, "InstructionTarget > Supplied instruction type for <TInstructionTypeEnum> is not an enum class.");
        
        public:
            /**
             * Registers the specified instruction set with the target.\n
             * 
             * Note #1: It is advised that, for security reasons, set registration is allowed
             * to be done only once and it is up to the target to ensure such behaviour.
             * 
             * Note #2: Individual instruction handler binding is done via
             * <code>InstructionSet::bindInstructionHandler()</code>.
             * 
             * @param set the instruction set to be registered
             * @return <code>true</code>, if the registration was successful
             */
            virtual bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<TInstructionTypeEnum> set) const = 0;
            
            /**
             * Retrieves the type of instructions that can be processed by this target.
             * 
             * @return the instruction set type
             */
            virtual InstructionManagement_Types::InstructionSetType getType() const = 0;
    };
}

#endif	/* INSTRUCTION_TARGET_H */

