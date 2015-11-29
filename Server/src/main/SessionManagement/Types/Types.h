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

#ifndef SESSION_MANAGEMENT_TYPES_H
#define	SESSION_MANAGEMENT_TYPES_H

namespace SessionManagement_Types
{
    typedef unsigned long InternalSessionID;
    const InternalSessionID INVALID_INTERNAL_SESSION_ID = 0;
    
    enum class SessionDataCommitType
    {
        INVALID,
        NEVER,      //data is never stored in the DB
        ON_UPDATE,  //data is pushed to DB on each update & on close
        ON_REAUTH,  //data is pushed to DB on each refresh/renew & on close
        ON_CLOSE    //data is pushed to DB on close
    };
    
    enum class GetSessionsConstraintType
    {
        INVALID,
        ALL,            //for all sessions
        ALL_DEVICE,     //for all device sessions
        ALL_USER,       //for all user sessions
        ALL_FOR_DEVICE, //for all sessions for a specific device
        ALL_FOR_USER    //for all sessions for a specific user
    };
}

#endif	/* SESSION_MANAGEMENT_TYPES_H */

