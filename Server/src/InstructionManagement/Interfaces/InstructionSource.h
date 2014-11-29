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

#ifndef INSTRUCTION_SOURCE_H
#define	INSTRUCTION_SOURCE_H

#include "../Types/Types.h"
#include "../Sets/InstructionSet.h"

namespace InstructionManagement_Interfaces
{
    /**
     * Interface for defining a source for instruction management.\n
     * 
     * A source needs to be registered with an <code>InstructionDispatcher</code>
     * before it is able to send instructions to targets.
     */
    class InstructionSource
    {
        public:
            /**
             * Registers the specified instruction handler with the source.\n
             * 
             * Note #1: It is advised that, for security reasons, handler registration is allowed
             * to be done only once and it is up to the source to ensure such behaviour.
             * 
             * Note #2: The signature of the handler is: <code>void(InstructionManagement_Sets::InstructionBasePtr)</code>.
             * 
             * @param handler the handler to be used by the source to send instructions
             * @return <code>true</code>, if the registration was successful
             */
            virtual bool registerInstructionHandler(const std::function<void(InstructionManagement_Sets::InstructionBasePtr)> handler) = 0;
            
            /**
             * Retrieves the types of instructions that the source will request.\n
             * 
             * Note: Any instruction type that is not specified will be rejected.
             * 
             * @return the instruction types that will come from this source
             */
            virtual std::vector<InstructionManagement_Types::InstructionSetType> getRequiredInstructionSetTypes() = 0;
    };
}

#endif	/* INSTRUCTION_SOURCE_H */

