/**
 * Copyright (C) 2016 https://github.com/sndnv
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

#include "TestDAL.h"

namespace Convert = Utilities::Strings;

Testing::TestDAL::TestDAL(
        bool getObjectResponse, bool putObjectResponse, bool updateObjectResponse, 
        bool removeObjectResponse, DatabaseObjectType type, bool enableLogger)
        : getObject_expectedResponse(getObjectResponse),
        getObject_received(0), getObject_completed(0), getObject_failed(0),
        putObject_expectedResponse(putObjectResponse),
        putObject_received(0), putObject_completed(0), putObject_failed(0),
        updateObject_expectedResponse(updateObjectResponse),
        updateObject_received(0), updateObject_completed(0), updateObject_failed(0),
        removeObject_expectedResponse(removeObjectResponse),
        removeObject_received(0), removeObject_completed(0), removeObject_failed(0),
        changeDatabaseSettings_calls(0), buildDatabase_calls(0), rebuildDatabase_calls(0),
        clearDatabase_calls(0), connect_calls(0), disconnect_calls(0),
        getDatabaseInfo_calls(0), getType_calls(0), setID_calls(0), getID_calls(0),
        dalID(DatabaseManagement_Types::INVALID_DAL_ID),
        dalUUID(boost::uuids::random_generator()()), dalType(type)
{
    if(enableLogger)
    {
        Utilities::FileLoggerParameters loggerParams
        {
            "./TestDAL_" + Convert::toString(dalType) + "_" + Convert::toString(dalUUID) + ".log",
            32*1024*1024,
            Utilities::FileLogSeverity::Debug
        };

        debugLogger = Utilities::FileLoggerPtr(new Utilities::FileLogger(loggerParams));
    }
}

Testing::TestDAL::~TestDAL()
{
    boost::lock_guard<boost::mutex> dataLock(dataMutex);
    if(!data.empty())
    {
        logMessage(LogSeverity::Warning, "(~) > Data map is not empty.");
        data.clear();
    }

    if((getObject_expectedResponse && getObject_received != getObject_completed)
       || (!getObject_expectedResponse && getObject_received != getObject_failed))
    {
        logMessage(LogSeverity::Warning, "(~) > getObject response mismatch!");
    }

    if((putObject_expectedResponse && putObject_received != putObject_completed)
       || (!putObject_expectedResponse && putObject_received != putObject_failed))
    {
        logMessage(LogSeverity::Warning, "(~) > putObject response mismatch!");
    }

    if((updateObject_expectedResponse && updateObject_received != updateObject_completed)
       || (!updateObject_expectedResponse && updateObject_received != updateObject_failed))
    {
        logMessage(LogSeverity::Warning, "(~) > updateObject response mismatch!");
    }

    if((updateObject_expectedResponse && updateObject_received != updateObject_completed)
       || (!updateObject_expectedResponse && updateObject_received != updateObject_failed))
    {
        logMessage(LogSeverity::Warning, "(~) > updateObject response mismatch!");
    }

    logMessage(LogSeverity::Debug, "(~) > --- Config ---");
    logMessage(LogSeverity::Debug, "(~) > getObject_expectedResponse \t\t\t " + Convert::toString(getObject_expectedResponse));
    logMessage(LogSeverity::Debug, "(~) > putObject_expectedResponse \t\t\t " + Convert::toString(putObject_expectedResponse));
    logMessage(LogSeverity::Debug, "(~) > updateObject_expectedResponse \t\t " + Convert::toString(updateObject_expectedResponse));
    logMessage(LogSeverity::Debug, "(~) > removeObject_expectedResponse \t\t " + Convert::toString(removeObject_expectedResponse));
    logMessage(LogSeverity::Debug, "(~) > dalID \t\t\t\t\t " + Convert::toString(dalID));
    logMessage(LogSeverity::Debug, "(~) > dalType \t\t\t\t\t " + Convert::toString(dalType));
    logMessage(LogSeverity::Debug, "(~) > --- End of Config ---");
    logMessage(LogSeverity::Debug, "(~) > --- Stats ---");
    logMessage(LogSeverity::Debug, "(~) > getObject_received \t\t\t\t " + Convert::toString(getObject_received));
    logMessage(LogSeverity::Debug, "(~) > getObject_completed \t\t\t " + Convert::toString(getObject_completed));
    logMessage(LogSeverity::Debug, "(~) > getObject_failed \t\t\t\t " + Convert::toString(getObject_failed));
    logMessage(LogSeverity::Debug, "(~) > putObject_received \t\t\t\t " + Convert::toString(putObject_received));
    logMessage(LogSeverity::Debug, "(~) > putObject_completed \t\t\t " + Convert::toString(putObject_completed));
    logMessage(LogSeverity::Debug, "(~) > putObject_failed \t\t\t\t " + Convert::toString(putObject_failed));
    logMessage(LogSeverity::Debug, "(~) > updateObject_received \t\t\t " + Convert::toString(updateObject_received));
    logMessage(LogSeverity::Debug, "(~) > updateObject_completed \t\t\t " + Convert::toString(updateObject_completed));
    logMessage(LogSeverity::Debug, "(~) > updateObject_failed \t\t\t " + Convert::toString(updateObject_failed));
    logMessage(LogSeverity::Debug, "(~) > removeObject_received \t\t\t " + Convert::toString(removeObject_received));
    logMessage(LogSeverity::Debug, "(~) > removeObject_completed \t\t\t " + Convert::toString(removeObject_completed));
    logMessage(LogSeverity::Debug, "(~) > removeObject_failed \t\t\t " + Convert::toString(removeObject_failed));
    logMessage(LogSeverity::Debug, "(~) > changeDatabaseSettings_calls \t\t " + Convert::toString(changeDatabaseSettings_calls));
    logMessage(LogSeverity::Debug, "(~) > buildDatabase_calls \t\t\t " + Convert::toString(buildDatabase_calls));
    logMessage(LogSeverity::Debug, "(~) > rebuildDatabase_calls \t\t\t " + Convert::toString(rebuildDatabase_calls));
    logMessage(LogSeverity::Debug, "(~) > clearDatabase_calls \t\t\t " + Convert::toString(clearDatabase_calls));
    logMessage(LogSeverity::Debug, "(~) > connect_calls \t\t\t\t " + Convert::toString(connect_calls));
    logMessage(LogSeverity::Debug, "(~) > disconnect_calls \t\t\t\t " + Convert::toString(disconnect_calls));
    logMessage(LogSeverity::Debug, "(~) > getDatabaseInfo_calls \t\t\t " + Convert::toString(getDatabaseInfo_calls));
    logMessage(LogSeverity::Debug, "(~) > getType_calls \t\t\t\t " + Convert::toString(getType_calls));
    logMessage(LogSeverity::Debug, "(~) > setID_calls \t\t\t\t " + Convert::toString(setID_calls));
    logMessage(LogSeverity::Debug, "(~) > getID_calls \t\t\t\t " + Convert::toString(getID_calls));
    logMessage(LogSeverity::Debug, "(~) > --- End of Stats ---");
}

bool Testing::TestDAL::getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue)
{
    ++getObject_received;
    logMessage(LogSeverity::Debug, "(getObject) > Request [" + Convert::toString(requestID) + "].");

    if(getObject_expectedResponse)
    {
        DBObjectID objectID = Utilities::Tools::getIDFromConstraint(dalType, constraintType, constraintValue);

        if(dalType == DatabaseObjectType::USER 
           && boost::any_cast<DatabaseSelectConstraints::USERS>(constraintType) == DatabaseSelectConstraints::USERS::LIMIT_BY_NAME)
        {
            bool done = false;
            std::string constraint = boost::any_cast<std::string>(constraintValue);

            boost::unique_lock<boost::mutex> dataLock(dataMutex);
            for(auto it = data.begin(); it != data.end(); ++it)
            {
                if(boost::dynamic_pointer_cast<UserDataContainer>(it->second)->getUsername().compare(constraint) == 0)
                {
                    dataLock.unlock();
                    onSuccess(dalID, requestID, it->second);
                    ++getObject_completed;
                    done = true;
                    break;
                }
            }

            if(!done)
            {
                dataLock.unlock();
                onFailure(dalID, requestID, Common_Types::INVALID_OBJECT_ID);
                ++getObject_failed;
            }
        }
        else
        {
            if(objectID != Common_Types::INVALID_OBJECT_ID)
            {
                boost::unique_lock<boost::mutex> dataLock(dataMutex);
                auto result = data[objectID];
                dataLock.unlock();
                onSuccess(dalID, requestID, result);
                ++getObject_completed;
            }
            else
            {
                VectorDataContainerPtr vect(new DatabaseManagement_Containers::VectorDataContainer());

                {
                    boost::lock_guard<boost::mutex> dataLock(dataMutex);
                    for(auto currentEntry : data)
                        vect->addDataContainer(currentEntry.second);
                }

                if(!vect->isEmpty())
                {
                    onSuccess(dalID, requestID, vect);
                }
                else
                {
                    onFailure(dalID, requestID, Common_Types::INVALID_OBJECT_ID);
                }
            }
        }
    }
    else
    {
        onFailure(dalID, requestID, Common_Types::INVALID_OBJECT_ID);
        ++getObject_failed;
    }

    return getObject_expectedResponse;
}

bool Testing::TestDAL::putObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    ++putObject_received;
    logMessage(LogSeverity::Debug, "(putObject) > Request [" + Convert::toString(requestID)
            + "] for container [" + Convert::toString(inputData->getContainerID()) + "].");

    boost::unique_lock<boost::mutex> dataLock(dataMutex);
    if(putObject_expectedResponse && data.find(inputData->getContainerID()) == data.end())
    {
        data.insert(std::pair<DBObjectID, DataContainerPtr>(inputData->getContainerID(), inputData));
        dataLock.unlock();
        onSuccess(dalID, requestID, inputData);
        ++putObject_completed;
    }
    else
    {
        dataLock.unlock();
        onFailure(dalID, requestID, inputData->getContainerID());
        ++putObject_failed;
    }

    return putObject_expectedResponse;
}

bool Testing::TestDAL::updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    ++updateObject_received;
    logMessage(LogSeverity::Debug, "(updateObject) > Request [" + Convert::toString(requestID)
            + "] for container [" + Convert::toString(inputData->getContainerID()) + "].");

    boost::unique_lock<boost::mutex> dataLock(dataMutex);
    if(updateObject_expectedResponse && data.find(inputData->getContainerID()) != data.end())
    {
        data[inputData->getContainerID()] = inputData;
        dataLock.unlock();
        onSuccess(dalID, requestID, inputData);
        ++updateObject_completed;
    }
    else
    {
        dataLock.unlock();
        onFailure(dalID, requestID, inputData->getContainerID());
        ++updateObject_failed;
    }

    return updateObject_expectedResponse;
}

bool Testing::TestDAL::removeObject(DatabaseRequestID requestID, DBObjectID id)
{
    ++removeObject_received;
    logMessage(LogSeverity::Debug, "(removeObject) > Request [" + Convert::toString(requestID)
            + "] for container [" + Convert::toString(id) + "].");

    boost::unique_lock<boost::mutex> dataLock(dataMutex);
    if(removeObject_expectedResponse && data.find(id) != data.end())
    {
        auto result = data[id];
        data.erase(id);
        dataLock.unlock();
        onSuccess(dalID, requestID, result);
        ++removeObject_completed;
    }
    else
    {
        dataLock.unlock();
        onFailure(dalID, requestID, id);
        ++removeObject_failed;
    }

    return removeObject_expectedResponse;
}
