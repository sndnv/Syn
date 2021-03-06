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

#include "Network.h"
#include <boost/unordered_map.hpp>

struct NetworkMaps
{
    static const boost::unordered_map<PeerType, std::string> peerTypeToString;
    static const boost::unordered_map<std::string, PeerType> stringToPeerType;
    static const boost::unordered_map<ConnectionType, std::string> connectionTypeToString;
    static const boost::unordered_map<std::string, ConnectionType> stringToConnectionType;
    static const boost::unordered_map<ConnectionState, std::string> connectionStateToString;
    static const boost::unordered_map<std::string, ConnectionState> stringToConnectionState;
    static const boost::unordered_map<ConnectionSubstate, std::string> connectionSubstateToString;
    static const boost::unordered_map<std::string, ConnectionSubstate> stringToConnectionSubstate;
    static const boost::unordered_map<ConnectionInitiation, std::string> connectionInitiationToString;
    static const boost::unordered_map<std::string, ConnectionInitiation> stringToConnectionInitiation;
    static const boost::unordered_map<ConnectionSetupState, std::string> connectionSetupStateToString;
    static const boost::unordered_map<std::string, ConnectionSetupState> stringToConnectionSetupState;
};

using Maps = NetworkMaps;

const boost::unordered_map<PeerType, std::string> Maps::peerTypeToString
{
    {PeerType::CLIENT,  "CLIENT"},
    {PeerType::SERVER,  "SERVER"},
    {PeerType::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, PeerType> Maps::stringToPeerType
{
    {"CLIENT",  PeerType::CLIENT},
    {"SERVER",  PeerType::SERVER},
    {"INVALID", PeerType::INVALID}
};

const boost::unordered_map<ConnectionType, std::string> Maps::connectionTypeToString
{
    {ConnectionType::COMMAND,           "COMMAND"},
    {ConnectionType::DATA,              "DATA"},
    {ConnectionType::INIT,              "INIT"},
    {ConnectionType::INVALID,           "INVALID"}
};

const boost::unordered_map<std::string, ConnectionType> Maps::stringToConnectionType
{
    {"COMMAND",             ConnectionType::COMMAND},
    {"DATA",                ConnectionType::DATA},
    {"INIT",                ConnectionType::INIT},
    {"INVALID",             ConnectionType::INVALID}
};

const boost::unordered_map<ConnectionState, std::string> Maps::connectionStateToString
{
    {ConnectionState::CLOSED,       "CLOSED"},
    {ConnectionState::ESTABLISHED,  "ESTABLISHED"},
    {ConnectionState::INVALID,      "INVALID"}
};

const boost::unordered_map<std::string, ConnectionState> Maps::stringToConnectionState
{
    {"CLOSED",      ConnectionState::CLOSED},
    {"ESTABLISHED", ConnectionState::ESTABLISHED},
    {"INVALID",     ConnectionState::INVALID}
};

const boost::unordered_map<ConnectionSubstate, std::string> Maps::connectionSubstateToString
{
    {ConnectionSubstate::DROPPED,   "DROPPED"},
    {ConnectionSubstate::NONE,      "NONE"},
    {ConnectionSubstate::READING,   "READING"},
    {ConnectionSubstate::FAILED,    "FAILED"},
    {ConnectionSubstate::WAITING,   "WAITING"},
    {ConnectionSubstate::WRITING,   "WRITING"}
};

const boost::unordered_map<std::string, ConnectionSubstate> Maps::stringToConnectionSubstate
{
    {"DROPPED", ConnectionSubstate::DROPPED},
    {"NONE",    ConnectionSubstate::NONE},
    {"READING", ConnectionSubstate::READING},
    {"FAILED",  ConnectionSubstate::FAILED},
    {"WAITING", ConnectionSubstate::WAITING},
    {"WRITING", ConnectionSubstate::WRITING}
};


const boost::unordered_map<ConnectionInitiation, std::string> Maps::connectionInitiationToString
{
    {ConnectionInitiation::LOCAL,   "LOCAL"},
    {ConnectionInitiation::REMOTE,  "REMOTE"},
    {ConnectionInitiation::INVALID, "INVALID"}
};

const boost::unordered_map<std::string, ConnectionInitiation> Maps::stringToConnectionInitiation
{
    {"LOCAL",   ConnectionInitiation::LOCAL},
    {"REMOTE",  ConnectionInitiation::REMOTE},
    {"INVALID", ConnectionInitiation::INVALID}
};

const boost::unordered_map<ConnectionSetupState, std::string> Maps::connectionSetupStateToString
{
    {ConnectionSetupState::COMPLETED,                       "COMPLETED"},
    {ConnectionSetupState::CONNECTION_REQUEST_RECEIVED,     "COMPLETED"},
    {ConnectionSetupState::CONNECTION_REQUEST_SENT,         "COMPLETED"},
    {ConnectionSetupState::CONNECTION_RESPONSE_RECEIVED,    "COMPLETED"},
    {ConnectionSetupState::CONNECTION_RESPONSE_SENT,        "COMPLETED"},
    {ConnectionSetupState::FAILED,                          "COMPLETED"},
    {ConnectionSetupState::INITIATED,                       "COMPLETED"},
    {ConnectionSetupState::INVALID,                         "INVALID"}
};

const boost::unordered_map<std::string, ConnectionSetupState> Maps::stringToConnectionSetupState
{
    {"COMPLETED",                       ConnectionSetupState::COMPLETED},
    {"CONNECTION_REQUEST_RECEIVED",     ConnectionSetupState::CONNECTION_REQUEST_RECEIVED},
    {"CONNECTION_REQUEST_SENT",         ConnectionSetupState::CONNECTION_REQUEST_SENT},
    {"CONNECTION_RESPONSE_RECEIVED",    ConnectionSetupState::CONNECTION_RESPONSE_RECEIVED},
    {"CONNECTION_RESPONSE_SENT",        ConnectionSetupState::CONNECTION_RESPONSE_SENT},
    {"FAILED",                          ConnectionSetupState::FAILED},
    {"INITIATED",                       ConnectionSetupState::INITIATED},
    {"INVALID",                         ConnectionSetupState::INVALID}
};

std::string Utilities::Strings::toString(PeerType var)
{
    if(Maps::peerTypeToString.find(var) != Maps::peerTypeToString.end())
        return Maps::peerTypeToString.at(var);
    else
        return "INVALID";
}

PeerType Utilities::Strings::toPeerType(std::string var)
{
    if(Maps::stringToPeerType.find(var) != Maps::stringToPeerType.end())
        return Maps::stringToPeerType.at(var);
    else
        return PeerType::INVALID;
}

std::string Utilities::Strings::toString(ConnectionType var)
{
    if(Maps::connectionTypeToString.find(var) != Maps::connectionTypeToString.end())
        return Maps::connectionTypeToString.at(var);
    else
        return "INVALID";
}

ConnectionType Utilities::Strings::toConnectionType(std::string var)
{
    if(Maps::stringToConnectionType.find(var) != Maps::stringToConnectionType.end())
        return Maps::stringToConnectionType.at(var);
    else
        return ConnectionType::INVALID;
}

std::string Utilities::Strings::toString(ConnectionState var)
{
    if(Maps::connectionStateToString.find(var) != Maps::connectionStateToString.end())
        return Maps::connectionStateToString.at(var);
    else
        return "INVALID";
}

ConnectionState Utilities::Strings::toConnectionState(std::string var)
{
    if(Maps::stringToConnectionState.find(var) != Maps::stringToConnectionState.end())
        return Maps::stringToConnectionState.at(var);
    else
        return ConnectionState::INVALID;
}

std::string Utilities::Strings::toString(ConnectionSubstate var)
{
    if(Maps::connectionSubstateToString.find(var) != Maps::connectionSubstateToString.end())
        return Maps::connectionSubstateToString.at(var);
    else
        return "NONE";
}

ConnectionSubstate Utilities::Strings::toConnectionSubstate(std::string var)
{
    if(Maps::stringToConnectionSubstate.find(var) != Maps::stringToConnectionSubstate.end())
        return Maps::stringToConnectionSubstate.at(var);
    else
        return ConnectionSubstate::NONE;
}

std::string Utilities::Strings::toString(ConnectionInitiation var)
{
    if(Maps::connectionInitiationToString.find(var) != Maps::connectionInitiationToString.end())
        return Maps::connectionInitiationToString.at(var);
    else
        return "INVALID";
}

ConnectionInitiation Utilities::Strings::toConnectionInitiation(std::string var)
{
    if(Maps::stringToConnectionInitiation.find(var) != Maps::stringToConnectionInitiation.end())
        return Maps::stringToConnectionInitiation.at(var);
    else
        return ConnectionInitiation::INVALID;
}

std::string Utilities::Strings::toString(ConnectionSetupState var)
{
    if(Maps::connectionSetupStateToString.find(var) != Maps::connectionSetupStateToString.end())
        return Maps::connectionSetupStateToString.at(var);
    else
        return "INVALID";
}

ConnectionSetupState Utilities::Strings::toConnectionSetupState(std::string var)
{
    if(Maps::stringToConnectionSetupState.find(var) != Maps::stringToConnectionSetupState.end())
        return Maps::stringToConnectionSetupState.at(var);
    else
        return ConnectionSetupState::INVALID;
}