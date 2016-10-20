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

#ifndef UTILITIES_STRINGS_COMMON_H
#define	UTILITIES_STRINGS_COMMON_H

#include <string>
#include <boost/thread.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cryptopp/secblock.h>
#include "../../Common/Types.h"

using Common_Types::Byte;
using Common_Types::ByteVector;
using Common_Types::UserAccessLevel;
using Common_Types::SessionType;
using Common_Types::LogSeverity;
using Common_Types::DeviceID;
using Common_Types::UserID;
using Common_Types::SyncID;
using Common_Types::DBObjectID;
using Common_Types::LogID;
using Common_Types::ScheduleID;
using Common_Types::SessionID;

namespace Utilities
{
    namespace Strings
    {
        std::string toString(bool var);
        std::string toString(int var);
        std::string toString(long var);
        std::string toString(short var);
        std::string toString(unsigned int var);
        std::string toString(unsigned long var);
        std::string toString(unsigned short var);
        std::string toString(unsigned long long var);
        std::string toString(boost::uuids::uuid var);
        std::string toString(boost::thread::id var);
        std::string toString(CryptoPP::SecByteBlock var);
        std::string toString(ByteVector var);
        std::string toString(boost::posix_time::ptime var);
        std::string toString(UserAccessLevel var);
        std::string toString(SessionType var);
        std::string toString(LogSeverity var);
        std::string toStringFromBytes(const std::string & var);
        
        boost::uuids::uuid toUUID(std::string var);
        DBObjectID toDBObjectID(std::string var);
        DeviceID toDeviceID(std::string var);
        UserID toUserID(std::string var);
        SyncID toSyncID(std::string var);
        LogID toLogID(std::string var);
        ScheduleID toScheduleID(std::string var);
        SessionID toSessionID(std::string var);
        std::string toBytesFromString(const std::string & var);
        CryptoPP::SecByteBlock toSecByteBlock(std::string var);
        boost::posix_time::ptime toTimestamp(std::string var);
        UserAccessLevel toUserAccessLevel(std::string var);
        SessionType toSessionType(std::string var);
        LogSeverity toLogSeverity(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_COMMON_H */
