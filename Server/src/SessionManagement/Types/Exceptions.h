/* 
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

#ifndef SESSIONS_EXCEPTIONS_H
#define	SESSIONS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace SessionManagement_Types
{
    /**
     * Exception class signifying that the maximum number of user sessions has been reached.
     */
    class TooManyUserSessionsException : public std::runtime_error
    {
        public:
            TooManyUserSessionsException(const std::string & message)
            : std::runtime_error("TooManyUserSessionsException {" + message + "}.")
            {}
            
            ~TooManyUserSessionsException() noexcept {}
    };
    
    /**
     * Exception class signifying that the maximum number of device sessions has been reached.
     */
    class TooManyDeviceSessionsException : public std::runtime_error
    {
        public:
            TooManyDeviceSessionsException(const std::string & message)
            : std::runtime_error("TooManyDeviceSessionsException {" + message + "}.")
            {}
            
            ~TooManyDeviceSessionsException() noexcept {}
    };
}

#endif	/* SESSIONS_EXCEPTIONS_H */

