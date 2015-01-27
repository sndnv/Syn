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

#ifndef UTILITIES_STRINGS_INSTRUCTIONS_H
#define	UTILITIES_STRINGS_INSTRUCTIONS_H

#include <string>
#include <boost/unordered_map.hpp>
#include "../../InstructionManagement/Types/Types.h"

using InstructionManagement_Types::InstructionSetType;

namespace Utilities
{
    namespace Strings
    {
        struct InstructionsMaps
        {
            static const boost::unordered_map<InstructionSetType, std::string> instructionSetTypeToString;
            static const boost::unordered_map<std::string, InstructionSetType> stringToInstructionSetType;
        };
        
        std::string toString(InstructionSetType var);
        
        InstructionSetType toInstructionSetType(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_INSTRUCTIONS_H */
