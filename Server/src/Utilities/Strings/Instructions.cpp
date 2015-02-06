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

#include "Instructions.h"

using Maps = Utilities::Strings::InstructionsMaps;

const boost::unordered_map<InstructionSetType, std::string> Maps::instructionSetTypeToString
{
    {InstructionSetType::DATABASE_MANAGER,  "DATABASE_MANAGER"},
    {InstructionSetType::SESSION_MANAGER,   "SESSION_MANAGER"},
    {InstructionSetType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, InstructionSetType> Maps::stringToInstructionSetType
{
    {"DATABASE_MANAGER",    InstructionSetType::DATABASE_MANAGER},
    {"SESSION_MANAGER",     InstructionSetType::SESSION_MANAGER},
    {"INVALID", InstructionSetType::INVALID}
};

std::string Utilities::Strings::toString(InstructionSetType var)
{
    if(Maps::instructionSetTypeToString.find(var) != Maps::instructionSetTypeToString.end())
        return Maps::instructionSetTypeToString.at(var);
    else
        return "INVALID";
}

InstructionSetType Utilities::Strings::toInstructionSetType(std::string var)
{
    if(Maps::stringToInstructionSetType.find(var) != Maps::stringToInstructionSetType.end())
        return Maps::stringToInstructionSetType.at(var);
    else
        return InstructionSetType::INVALID;
}
