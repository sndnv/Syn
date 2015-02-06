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

#ifndef SECURITY_EXCEPTIONS_H
#define	SECURITY_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace SecurityManagement_Types
{
    /**
     * Exception class signifying that an instruction was not allowed.
     */
    class InstructionNotAllowedException : public std::runtime_error
    {
        public:
            InstructionNotAllowedException(const std::string & message)
            : std::runtime_error("InstructionNotAllowedException {" + message + "}.")
            {}
            
            ~InstructionNotAllowedException() noexcept {}
    };
    
    /**
     * Exception class signifying that a user has insufficient access.
     */
    class InsufficientUserAccessException : public std::runtime_error
    {
        public:
            InsufficientUserAccessException(const std::string & message)
            : std::runtime_error("InsufficientUserAccessException {" + message + "}.")
            {}
            
            ~InsufficientUserAccessException() noexcept {}
    };
    
    /**
     * Exception class signifying that a device was encountered in a context that was not expected.
     */
    class UnexpectedDeviceException : public std::runtime_error
    {
        public:
            UnexpectedDeviceException(const std::string & message)
            : std::runtime_error("UnrecognisedDeviceException {" + message + "}.")
            {}
            
            ~UnexpectedDeviceException() noexcept {}
    };
    
    /**
     * Exception class signifying that a device is locked.
     */
    class DeviceLockedException : public std::runtime_error
    {
        public:
            DeviceLockedException(const std::string & message)
            : std::runtime_error("DeviceLockedException {" + message + "}.")
            {}
            
            ~DeviceLockedException() noexcept {}
    };
    
    /**
     * Exception class signifying that a device was not found.
     */
    class DeviceNotFoundException : public std::runtime_error
    {
        public:
            DeviceNotFoundException(const std::string & message)
            : std::runtime_error("DeviceNotFoundException {" + message + "}.")
            {}
            
            ~DeviceNotFoundException() noexcept {}
    };
    
    /**
     * Exception class signifying that a user was not found.
     */
    class UserNotFoundException : public std::runtime_error
    {
        public:
            UserNotFoundException(const std::string & message)
            : std::runtime_error("UserNotFoundException {" + message + "}.")
            {}
            
            ~UserNotFoundException() noexcept {}
    };
    
    /**
     * Exception class signifying that a password mismatch was encountered.
     */
    class InvalidPassswordException : public std::runtime_error
    {
        public:
            InvalidPassswordException(const std::string & message)
            : std::runtime_error("InvalidPassswordException {" + message + "}.")
            {}
            
            ~InvalidPassswordException() noexcept {}
    };
    
    /**
     * Exception class signifying that a user is locked.
     */
    class UserLockedException : public std::runtime_error
    {
        public:
            UserLockedException(const std::string & message)
            : std::runtime_error("UserLockedException {" + message + "}.")
            {}
            
            ~UserLockedException() noexcept {}
    };
    
    /**
     * Exception class signifying that a user is not authenticated.
     */
    class UserNotAuthenticatedException : public std::runtime_error
    {
        public:
            UserNotAuthenticatedException(const std::string & message)
            : std::runtime_error("UserNotAuthenticatedException {" + message + "}.")
            {}
            
            ~UserNotAuthenticatedException() noexcept {}
    };
    
    /**
     * Exception class signifying that an invalid authorization token was encountered.
     */
    class InvalidAuthorizationTokenException : public std::runtime_error
    {
        public:
            InvalidAuthorizationTokenException(const std::string & message)
            : std::runtime_error("InvalidAuthorizationTokenException {" + message + "}.")
            {}
            
            ~InvalidAuthorizationTokenException() noexcept {}
    };
}

#endif	/* SECURITY_EXCEPTIONS_H */

