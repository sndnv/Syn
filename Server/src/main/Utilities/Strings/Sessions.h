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

#ifndef UTILITIES_STRINGS_SESSIONS_H
#define	UTILITIES_STRINGS_SESSIONS_H

#include <string>
#include "../../SessionManagement/Types/Types.h"

using SessionManagement_Types::SessionDataCommitType;

namespace Utilities
{
    namespace Strings
    {
        std::string toString(SessionDataCommitType var);
        SessionDataCommitType toSessionDataCommitType(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_SESSIONS_H */

