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

#ifndef UTILITIES_STRINGS_STORAGE_H
#define	UTILITIES_STRINGS_STORAGE_H

#include <string>
#include "../../StorageManagement/Types/Types.h"

using StorageManagement_Types::PoolMode;
using StorageManagement_Types::PoolState;
using StorageManagement_Types::LinkActionType;
using StorageManagement_Types::LinkActionConditionType;
using StorageManagement_Types::DataPoolType;

namespace Utilities
{
    namespace Strings
    {
        std::string toString(DataPoolType var);
        std::string toString(PoolMode var);
        std::string toString(PoolState var);
        std::string toString(LinkActionType var);
        std::string toString(LinkActionConditionType var);
        
        DataPoolType toDataPoolType(std::string var);
        PoolMode toPoolMode(std::string var);
        PoolState toPoolState(std::string var);
        LinkActionType toLinkActionType(std::string var);
        LinkActionConditionType toLinkActionConditionType(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_STORAGE_H */
