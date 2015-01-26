/* 
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

#ifndef AUTHORIZATIONRULES_H
#define	AUTHORIZATIONRULES_H

#include "../../Common/Types.h"
#include "../../InstructionManagement/Types/Types.h"

using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::INVALID_USER_ID;
using Common_Types::INVALID_DEVICE_ID;
using InstructionManagement_Types::InstructionSetType;

namespace SecurityManagement_Rules
{
    /**
     * Class for representing user authorization rules.
     */
    class UserAuthorizationRule
    {
        public:
            /**
             * Constructs a new authorization rule for the specified user with the
             * supplied instruction set type.
             * 
             * @param setType the instruction set which the user is authorized to access
             */
            UserAuthorizationRule(InstructionSetType setType)
            : set(setType) {}
            
            /** Retrieves the instruction set type associated with the rule.\n\n@return the instruction set type */
            InstructionSetType getSetType() const { return set; }
            /** Checks whether or not the rule is valid.\n\n@return <code>true</code>, if the rule is valid */
            bool isValid() const { return (set != InstructionSetType::INVALID); }
            
            bool operator==(const UserAuthorizationRule & rhs) const { return (set == rhs.set); }
            bool operator!=(const UserAuthorizationRule & rhs) const { return !(*this == rhs); }
            
        private:
            InstructionSetType set;
    };
}

#endif	/* AUTHORIZATIONRULES_H */

