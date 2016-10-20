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

#include "Tools.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

unsigned long Utilities::Tools::powerof(unsigned long base, unsigned long exponent)
{
    if(exponent != 0)
    {
        unsigned long intermediateResult = powerof(base, (exponent - 1));
        unsigned long result = base*intermediateResult;

        if(result/intermediateResult != base)
            throw std::overflow_error("Tools::powerof() > Overflow encountered.");
        else
            return result;
    }
    else
        return 1;
}

Common_Types::DBObjectID Utilities::Tools::getIDFromString(std::string var)
{
    try { return boost::lexical_cast<boost::uuids::uuid>(var); }
    catch(boost::bad_lexical_cast) { return Common_Types::INVALID_OBJECT_ID;}
}

Common_Types::DBObjectID Utilities::Tools::getIDFromConstraint(DatabaseObjectType objectType, boost::any constraintType, boost::any constraintValue)
{
    Common_Types::DBObjectID objectID = Common_Types::INVALID_OBJECT_ID;

    switch(objectType)
    {
        case DatabaseObjectType::DEVICE:
        {
            if(boost::any_cast<DatabaseSelectConstraints::DEVICES>(constraintType) == DatabaseSelectConstraints::DEVICES::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::DeviceID>(constraintValue);
        } break;

        case DatabaseObjectType::LOG:
        {
            if(boost::any_cast<DatabaseSelectConstraints::LOGS>(constraintType) == DatabaseSelectConstraints::LOGS::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::LogID>(constraintValue);
        } break;

        case DatabaseObjectType::SCHEDULE:
        {
            if(boost::any_cast<DatabaseSelectConstraints::SCHEDULES>(constraintType) == DatabaseSelectConstraints::SCHEDULES::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::ScheduleID>(constraintValue);
        } break;

        case DatabaseObjectType::SESSION:
        {
            if(boost::any_cast<DatabaseSelectConstraints::SESSIONS>(constraintType) == DatabaseSelectConstraints::SESSIONS::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::SessionID>(constraintValue);
        } break;

        case DatabaseObjectType::STATISTICS:
        {
            if(boost::any_cast<DatabaseSelectConstraints::STATISTCS>(constraintType) == DatabaseSelectConstraints::STATISTCS::LIMIT_BY_TYPE)
                objectID = boost::any_cast<boost::uuids::uuid>(constraintValue);
        } break;

        case DatabaseObjectType::SYNC_FILE:
        {
            if(boost::any_cast<DatabaseSelectConstraints::SYNC>(constraintType) == DatabaseSelectConstraints::SYNC::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::SyncID>(constraintValue);
        } break;

        case DatabaseObjectType::SYSTEM_SETTINGS:
        {
            if(boost::any_cast<DatabaseSelectConstraints::SYSTEM>(constraintType) == DatabaseSelectConstraints::SYSTEM::LIMIT_BY_TYPE)
                objectID = boost::any_cast<boost::uuids::uuid>(constraintValue);
        } break;

        case DatabaseObjectType::USER:
        {
            if(boost::any_cast<DatabaseSelectConstraints::USERS>(constraintType) == DatabaseSelectConstraints::USERS::LIMIT_BY_ID)
                objectID = boost::any_cast<Common_Types::UserID>(constraintValue);
        } break;

        default: ; break; //ignore
    }

    return objectID;
}
