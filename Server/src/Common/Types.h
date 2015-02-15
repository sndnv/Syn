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

#ifndef COMMON_TYPES_H
#define	COMMON_TYPES_H

#include <limits>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace Common_Types
{
    typedef unsigned char Byte;
    typedef std::vector<Byte> ByteVector;
    typedef boost::shared_ptr<ByteVector> ByteVectorPtr;
    
    typedef unsigned long TransferredDataAmount;
    const TransferredDataAmount INVALID_TRANSFERRED_DATA_AMOUNT = 0;
    
    typedef unsigned long TransferredFilesAmount;
    const TransferredFilesAmount INVALID_TRANSFERRED_FILES_AMOUNT = 0;
    
    typedef std::string IPAddress;
    const IPAddress INVALID_IP_ADDRESS = "0/0"; //for IPv4 and IPv6
    
    typedef unsigned int IPPort;
    const IPPort INVALID_IP_PORT = 0;
    
    typedef boost::posix_time::ptime Timestamp;
    const Timestamp INVALID_DATE_TIME = boost::posix_time::ptime(boost::posix_time::min_date_time);
    
    typedef unsigned long Seconds;
    const Seconds MAX_SECONDS = ULONG_MAX;
    
    //Database Management Types
    typedef boost::uuids::uuid DBObjectID;
    const DBObjectID INVALID_OBJECT_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID LogID;
    const LogID INVALID_LOG_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID SessionID;
    const SessionID INVALID_SESSION_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID UserID;
    const UserID INVALID_USER_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID DeviceID;
    const DeviceID INVALID_DEVICE_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID SyncID;
    const SyncID INVALID_SYNC_ID = boost::uuids::nil_uuid();
    
    typedef DBObjectID ScheduleID;
    const ScheduleID INVALID_SCHEDULE_ID = boost::uuids::nil_uuid();
    
    //Data Pools Management Types
    typedef unsigned long DataPoolSize;
    const DataPoolSize INVALID_DATA_POOL_SIZE = 0;
    
    typedef std::string DataPoolPath;
    const DataPoolPath INVALID_DATA_POOL_PATH = "";
    
    typedef unsigned long DataPoolRetention;
    const DataPoolRetention INVALID_DATA_POOL_RETENTION = 0;
    
    enum class SessionType { INVALID, COMMAND, DATA };
    
    //User Management Types
    enum class UserLockType { NONE, FAILED_LOGIN, ADMIN_FORCED, USER_FORCED, INACTIVITY }; //TODO - implement
    
    enum class UserAccessLevel { INVALID, NONE, USER, ADMIN };
    
    /**
     * Converts the specified user access level to an integer.
     * 
     * The following rules hold for the values returned by the function:<br>
     * - <code>UserAccessLevel::ADMIN</code> will always be the highest possible value<br>
     * - An invalid/unexpected value will always throw an exception<br>
     * - All values in between will be ordered based on the level of access that the
     * value implies (for example: ADMIN(UINT_MAX) > USER(1) > NONE(0))<br>
     * 
     * Warning: The actual value returned by the function can change from one
     * code revision to another and, therefore, should not be used anywhere
     * as a hard-coded integer or for permanent storage.
     * 
     * @param level the access level to be converted
     * @return the representation of the access level as an integer
     * 
     * @throw logic_error if an unexpected/invalid access level is encountered
     */
    inline unsigned int userAccessLevelToInt(const UserAccessLevel level)
    {
        switch(level)
        {
            case UserAccessLevel::NONE: return 0;
            case UserAccessLevel::USER: return 1;
            case UserAccessLevel::ADMIN: return UINT_MAX;
            default: throw std::logic_error("userAccessLevelToInt() > An unexpected user access level was encountered.");
        }
    }
    
    inline bool operator>(const UserAccessLevel a, const UserAccessLevel b)
    {
        return (userAccessLevelToInt(a) > userAccessLevelToInt(b));
    }
    
    inline bool operator<(const UserAccessLevel a, const UserAccessLevel b)
    {
        return (userAccessLevelToInt(a) < userAccessLevelToInt(b));
    }
    
    inline bool operator>=(const UserAccessLevel a, const UserAccessLevel b)
    {
        return (userAccessLevelToInt(a) >= userAccessLevelToInt(b));
    }
    
    inline bool operator<=(const UserAccessLevel a, const UserAccessLevel b)
    {
        return (userAccessLevelToInt(a) <= userAccessLevelToInt(b));
    }
    
    enum class LogSeverity { INVALID, None, Info, Warning, Error, Debug };
    
    /**
     * Converts the specified log severity to an integer.
     * 
     * The following rules hold for the values returned by the function:<br>
     * - <code>Debug \< Info \< Warning \< Error \< None</code><br>
     * - <code>INVALID == 0</code>
     * 
     * Warning: The actual value returned by the function can change from one
     * code revision to another and, therefore, should not be used anywhere
     * as a hard-coded integer or for permanent storage.
     * 
     * @param severity the log severity to be converted
     * @return the representation of the severity as an integer
     * 
     * @throw logic_error if an unexpected/invalid severity is encountered
     */
    inline unsigned int logSeverityToInt(const LogSeverity severity)
    {
        switch(severity)
        {
            case LogSeverity::INVALID: return 0;
            case LogSeverity::Debug: return 1;
            case LogSeverity::Info: return 2;
            case LogSeverity::Warning: return 3;
            case LogSeverity::Error: return 4;
            case LogSeverity::None: return 5;
            default: throw std::logic_error("logSeverityToInt() > An unexpected log severity was encountered.");
        }
    }
    
    inline bool operator>(const LogSeverity a, const LogSeverity b)
    {
        return (logSeverityToInt(a) > logSeverityToInt(b));
    }
    
    inline bool operator<(const LogSeverity a, const LogSeverity b)
    {
        return (logSeverityToInt(a) < logSeverityToInt(b));
    }
    
    inline bool operator>=(const LogSeverity a, const LogSeverity b)
    {
        return (logSeverityToInt(a) >= logSeverityToInt(b));
    }
    
    inline bool operator<=(const LogSeverity a, const LogSeverity b)
    {
        return (logSeverityToInt(a) <= logSeverityToInt(b));
    }
}

#endif	/* COMMON_TYPES_H */

