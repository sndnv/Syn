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

#include "Common.h"

using Maps = Utilities::Strings::CommonMaps;

const boost::unordered_map<UserAccessLevel, std::string> Maps::userAccessLevelToString
{
    {UserAccessLevel::ADMIN,    "ADMIN"},
    {UserAccessLevel::NONE,     "NONE"},
    {UserAccessLevel::USER,     "USER"},
    {UserAccessLevel::INVALID,  "INVALID"}
};

const boost::unordered_map<std::string, UserAccessLevel> Maps::stringToUserAccessLevel
{
    {"ADMIN",   UserAccessLevel::ADMIN},
    {"NONE",    UserAccessLevel::NONE},
    {"USER",    UserAccessLevel::USER},
    {"INVALID", UserAccessLevel::INVALID}
};

std::string Utilities::Strings::toString(bool var) { return (var) ? "TRUE" : "FALSE"; }
std::string Utilities::Strings::toString(int var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(long var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(short var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(unsigned int var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(unsigned long var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(unsigned short var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(unsigned long long var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(boost::uuids::uuid var) { return boost::lexical_cast<std::string>(var); }
std::string Utilities::Strings::toString(boost::thread::id var) { return boost::lexical_cast<std::string>(var); }

std::string Utilities::Strings::toString(CryptoPP::SecByteBlock var)
{
    unsigned int resultLength = var.size() * 2;
    char result[resultLength + 1];

    Byte * varDataPtr = var.BytePtr();

    for(unsigned int i = 0; i < var.size(); i++)
        snprintf(&result[2*i], resultLength+1, "%02X", varDataPtr[i]);

    result[resultLength] = '\0';

    return std::string(result);
}

std::string Utilities::Strings::toString(ByteVector var)
{
    unsigned int resultLength = var.size() * 2;
    char result[resultLength + 1];

    for(unsigned int i = 0; i < var.size(); i++)
        snprintf(&result[2*i], resultLength+1, "%02X", var[i]);

    result[resultLength] = '\0';

    return std::string(result);
}

std::string Utilities::Strings::toString(boost::posix_time::ptime var)
{
    unsigned int bufferSize = 20;
    char timestampBuffer[bufferSize];
    snprintf(timestampBuffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", 
            (unsigned short)var.date().year(), (unsigned short)var.date().month(), (unsigned short)var.date().day(), 
            var.time_of_day().hours(), var.time_of_day().minutes(), var.time_of_day().seconds());

    return std::string(timestampBuffer);
}

std::string Utilities::Strings::toString(UserAccessLevel var)
{
    if(Maps::userAccessLevelToString.find(var) != Maps::userAccessLevelToString.end())
        return Maps::userAccessLevelToString.at(var);
    else
        return "UNDEFINED";
}

CryptoPP::SecByteBlock Utilities::Strings::toSecByteBlock(std::string var)
{
    if(var.size() == 0 || var.size() % 2 != 0)
        return CryptoPP::SecByteBlock();

    std::size_t blockSize = var.size()/2;
    CryptoPP::SecByteBlock result(blockSize);
    for(std::size_t i = 0; i < blockSize; i++)
    {
        std::string currentRawByte = var.substr(i*2, 2);
        result.data()[i] = strtol(currentRawByte.c_str(), 0, 16);
    }

    return result;
}

boost::posix_time::ptime Utilities::Strings::toTimestamp(std::string var)
{
    boost::smatch timestampMatch;

    if(boost::regex_search(var, timestampMatch, boost::regex("(\\d{4})-(\\d{2})-(\\d{2}) (\\d{2}):(\\d{2}):(\\d{2})")))
    {
        boost::gregorian::date date(boost::lexical_cast<unsigned short>(timestampMatch[1]), boost::lexical_cast<unsigned short>(timestampMatch[2]), boost::lexical_cast<unsigned short>(timestampMatch[3]));
        boost::posix_time::time_duration time(boost::lexical_cast<int>(timestampMatch[4]), boost::lexical_cast<int>(timestampMatch[5]), boost::lexical_cast<int>(timestampMatch[6]));
        return boost::posix_time::ptime(date, time);
    }
    else
        return boost::posix_time::ptime();
}

UserAccessLevel Utilities::Strings::toUserAccessLevel(std::string var)
{
    return Maps::stringToUserAccessLevel.at(var);
}
