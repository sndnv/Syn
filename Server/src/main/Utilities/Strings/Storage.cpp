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

#include "Storage.h"
#include <boost/unordered_map.hpp>

struct StorageMaps
{
    static const boost::unordered_map<DataPoolType, std::string> dataPoolTypeToString;
    static const boost::unordered_map<std::string, DataPoolType> stringToDataPoolType;
    static const boost::unordered_map<PoolMode, std::string> poolModeToString;
    static const boost::unordered_map<std::string, PoolMode> stringToPoolMode;
    static const boost::unordered_map<PoolState, std::string> poolStateToString;
    static const boost::unordered_map<std::string, PoolState> stringToPoolState;
    static const boost::unordered_map<LinkActionType, std::string> linkActionTypeToString;
    static const boost::unordered_map<std::string, LinkActionType> stringToLinkActionType;
    static const boost::unordered_map<LinkActionConditionType, std::string> linkActionConditionTypeToString;
    static const boost::unordered_map<std::string, LinkActionConditionType> stringToLinkActionConditionType;
};

using Maps = StorageMaps;

const boost::unordered_map<DataPoolType, std::string> Maps::dataPoolTypeToString
{
    {DataPoolType::AGGREGATE,       "AGGREGATE"},
    {DataPoolType::LOCAL_DISK,      "LOCAL_DISK"},
    {DataPoolType::LOCAL_MEMORY,    "LOCAL_MEMORY"},
    {DataPoolType::REMOTE_DISK,     "REMOTE_DISK"},
    {DataPoolType::REMOTE_MEMORY,   "REMOTE_MEMORY"},
    {DataPoolType::INVALID,         "INVALID"},
};

const boost::unordered_map<std::string, DataPoolType> Maps::stringToDataPoolType
{
    {"AGGREGATE",       DataPoolType::AGGREGATE},
    {"LOCAL_DISK",      DataPoolType::LOCAL_DISK},
    {"LOCAL_MEMORY",    DataPoolType::LOCAL_MEMORY},
    {"REMOTE_DISK",     DataPoolType::REMOTE_DISK},
    {"REMOTE_MEMORY",   DataPoolType::REMOTE_MEMORY},
    {"INVALID",         DataPoolType::INVALID},
};

const boost::unordered_map<PoolMode, std::string> Maps::poolModeToString
{
    {PoolMode::READ_WRITE,  "READ_WRITE"},
    {PoolMode::READ_ONLY,   "READ_ONLY"},
    {PoolMode::INVALID,     "INVALID"}
};

const boost::unordered_map<std::string, PoolMode> Maps::stringToPoolMode
{
    {"READ_WRITE",  PoolMode::READ_WRITE},
    {"READ_ONLY",   PoolMode::READ_ONLY},
    {"INVALID",     PoolMode::INVALID}
};

const boost::unordered_map<PoolState, std::string> Maps::poolStateToString
{
    {PoolState::OPEN,       "OPEN"},
    {PoolState::CLOSED,     "CLOSED"},
    {PoolState::FAILED,     "FAILED"},
    {PoolState::INVALID,    "INVALID"},
};

const boost::unordered_map<std::string, PoolState> Maps::stringToPoolState
{
    {"OPEN",    PoolState::OPEN},
    {"CLOSED",  PoolState::CLOSED},
    {"FAILED",  PoolState::FAILED},
    {"INVALID", PoolState::INVALID}
};

const boost::unordered_map<LinkActionType, std::string> Maps::linkActionTypeToString
{
    {LinkActionType::DISTRIBUTE, "DISTRIBUTE"},
    {LinkActionType::COPY,       "COPY"},
    {LinkActionType::MOVE,       "MOVE"},
    {LinkActionType::DISCARD,    "DISCARD"},
    {LinkActionType::SKIP,       "SKIP"},
    {LinkActionType::INVALID,    "INVALID"},
};

const boost::unordered_map<std::string, LinkActionType> Maps::stringToLinkActionType
{
    {"DISTRIBUTE",  LinkActionType::DISTRIBUTE},
    {"COPY",        LinkActionType::COPY},
    {"MOVE",        LinkActionType::MOVE},
    {"DISCARD",     LinkActionType::DISCARD},
    {"SKIP",        LinkActionType::SKIP},
    {"INVALID",     LinkActionType::INVALID},
};

const boost::unordered_map<LinkActionConditionType, std::string> Maps::linkActionConditionTypeToString
{
    {LinkActionConditionType::NONE,                 "NONE"},
    {LinkActionConditionType::TIMED,                "TIMED"},
    {LinkActionConditionType::SOURCE_MIN_FULL,      "SOURCE_MIN_FULL"},
    {LinkActionConditionType::TARGET_MIN_FULL,      "TARGET_MIN_FULL"},
    {LinkActionConditionType::SOURCE_MAX_FULL,      "SOURCE_MAX_FULL"},
    {LinkActionConditionType::TARGET_MAX_FULL,      "TARGET_MAX_FULL"},
    {LinkActionConditionType::SOURCE_MIN_ENTITIES,  "SOURCE_MIN_ENTITIES"},
    {LinkActionConditionType::SOURCE_MAX_ENTITIES,  "SOURCE_MAX_ENTITIES"},
    {LinkActionConditionType::TARGET_MIN_ENTITIES,  "TARGET_MIN_ENTITIES"},
    {LinkActionConditionType::TARGET_MAX_ENTITIES,  "TARGET_MAX_ENTITIES"},
    {LinkActionConditionType::DATA_MIN_SIZE,        "DATA_MIN_SIZE"},
    {LinkActionConditionType::DATA_MAX_SIZE,        "DATA_MAX_SIZE"},
    {LinkActionConditionType::INVALID,              "INVALID"},
};

const boost::unordered_map<std::string, LinkActionConditionType> Maps::stringToLinkActionConditionType
{
    {"NONE",                LinkActionConditionType::NONE},
    {"TIMED",               LinkActionConditionType::TIMED},
    {"SOURCE_MIN_FULL",     LinkActionConditionType::SOURCE_MIN_FULL},
    {"TARGET_MIN_FULL",     LinkActionConditionType::TARGET_MIN_FULL},
    {"SOURCE_MAX_FULL",     LinkActionConditionType::SOURCE_MAX_FULL},
    {"TARGET_MAX_FULL",     LinkActionConditionType::TARGET_MAX_FULL},
    {"SOURCE_MIN_ENTITIES", LinkActionConditionType::SOURCE_MIN_ENTITIES},
    {"SOURCE_MAX_ENTITIES", LinkActionConditionType::SOURCE_MAX_ENTITIES},
    {"TARGET_MIN_ENTITIES", LinkActionConditionType::TARGET_MIN_ENTITIES},
    {"TARGET_MAX_ENTITIES", LinkActionConditionType::TARGET_MAX_ENTITIES},
    {"DATA_MIN_SIZE",       LinkActionConditionType::DATA_MIN_SIZE},
    {"DATA_MAX_SIZE",       LinkActionConditionType::DATA_MAX_SIZE},
    {"INVALID",             LinkActionConditionType::INVALID}
};

std::string Utilities::Strings::toString(DataPoolType var)
{
    if(Maps::dataPoolTypeToString.find(var) != Maps::dataPoolTypeToString.end())
        return Maps::dataPoolTypeToString.at(var);
    else
        return "INVALID";
}

DataPoolType Utilities::Strings::toDataPoolType(std::string var)
{
    if(Maps::stringToDataPoolType.find(var) != Maps::stringToDataPoolType.end())
        return Maps::stringToDataPoolType.at(var);
    else
        return DataPoolType::INVALID;
}

std::string Utilities::Strings::toString(PoolMode var)
{
    if(Maps::poolModeToString.find(var) != Maps::poolModeToString.end())
        return Maps::poolModeToString.at(var);
    else
        return "INVALID";
}

PoolMode Utilities::Strings::toPoolMode(std::string var)
{
    if(Maps::stringToPoolMode.find(var) != Maps::stringToPoolMode.end())
        return Maps::stringToPoolMode.at(var);
    else
        return PoolMode::INVALID;
}

std::string Utilities::Strings::toString(PoolState var)
{
    if(Maps::poolStateToString.find(var) != Maps::poolStateToString.end())
        return Maps::poolStateToString.at(var);
    else
        return "INVALID";
}

PoolState Utilities::Strings::toPoolState(std::string var)
{
    if(Maps::stringToPoolState.find(var) != Maps::stringToPoolState.end())
        return Maps::stringToPoolState.at(var);
    else
        return PoolState::INVALID;
}

std::string Utilities::Strings::toString(LinkActionType var)
{
    if(Maps::linkActionTypeToString.find(var) != Maps::linkActionTypeToString.end())
        return Maps::linkActionTypeToString.at(var);
    else
        return "INVALID";
}

LinkActionType Utilities::Strings::toLinkActionType(std::string var)
{
    if(Maps::stringToLinkActionType.find(var) != Maps::stringToLinkActionType.end())
        return Maps::stringToLinkActionType.at(var);
    else
        return LinkActionType::INVALID;
}

std::string Utilities::Strings::toString(LinkActionConditionType var)
{
    if(Maps::linkActionConditionTypeToString.find(var) != Maps::linkActionConditionTypeToString.end())
        return Maps::linkActionConditionTypeToString.at(var);
    else
        return "INVALID";
}

LinkActionConditionType Utilities::Strings::toLinkActionConditionType(std::string var)
{
    if(Maps::stringToLinkActionConditionType.find(var) != Maps::stringToLinkActionConditionType.end())
        return Maps::stringToLinkActionConditionType.at(var);
    else
        return LinkActionConditionType::INVALID;
}
