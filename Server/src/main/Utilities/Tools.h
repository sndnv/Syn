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

#ifndef TOOLS_H
#define	TOOLS_H

#include <string>
#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "../DatabaseManagement/Types/Types.h"

using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DatabaseSelectConstraints;

namespace Utilities
{
    namespace Tools
    {
        unsigned long powerof(unsigned long base, unsigned long exponent);
        
        Common_Types::DBObjectID getIDFromString(std::string var);
        Common_Types::DBObjectID getIDFromConstraint(DatabaseObjectType objectType, boost::any constraintType, boost::any constraintValue);
    }
}

#endif	/* TOOLS_H */

