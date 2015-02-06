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

#include "Sessions.h"

using Maps = Utilities::Strings::SessionsMaps;

const boost::unordered_map<SessionDataCommitType, std::string> Maps::sessionDataCommitTypeToString
{
    {SessionDataCommitType::NEVER,      "NEVER"},
    {SessionDataCommitType::ON_CLOSE,   "ON_CLOSE"},
    {SessionDataCommitType::ON_REAUTH,  "ON_REAUTH"},
    {SessionDataCommitType::ON_UPDATE,  "ON_UPDATE"},
    {SessionDataCommitType::INVALID,    "INVALID"}
};

const boost::unordered_map<std::string, SessionDataCommitType> Maps::stringToSessionDataCommitType
{
    {"NEVER",       SessionDataCommitType::NEVER},
    {"ON_CLOSE",    SessionDataCommitType::ON_CLOSE},
    {"ON_REAUTH",   SessionDataCommitType::ON_REAUTH},
    {"ON_UPDATE",   SessionDataCommitType::ON_UPDATE},
    {"INVALID",     SessionDataCommitType::INVALID},
};

std::string Utilities::Strings::toString(SessionDataCommitType var)
{
    if(Maps::sessionDataCommitTypeToString.find(var) != Maps::sessionDataCommitTypeToString.end())
        return Maps::sessionDataCommitTypeToString.at(var);
    else
        return "INVALID";
}

SessionDataCommitType Utilities::Strings::toSessionDataCommitType(std::string var)
{
    if(Maps::stringToSessionDataCommitType.find(var) != Maps::stringToSessionDataCommitType.end())
        return Maps::stringToSessionDataCommitType.at(var);
    else
        return SessionDataCommitType::INVALID;
}
