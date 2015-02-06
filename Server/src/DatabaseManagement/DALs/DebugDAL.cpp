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

#include "DebugDAL.h"

DatabaseManagement_DALs::DebugDAL::DebugDAL(std::string logPath, std::string dataPath, DatabaseObjectType dbType)
    : logger(logPath, 8*1024*1024, Utilities::FileLogSeverity::Debug), dataFilePath(dataPath), dalType(dbType), nextIntID(0)
{
    info = new DebugDALInformationContainer();
    
    mainThreadObject = new boost::thread(&DatabaseManagement_DALs::DebugDAL::mainThread, this);
}

DatabaseManagement_DALs::DebugDAL::~DebugDAL()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (~) > Destruction initiated.");
    
    {
        boost::lock_guard<boost::mutex> dataLock(mainThreadMutex);
        stopDebugger = true;
        
        mainThreadLockCondition.notify_all();
    }
    
    mainThreadObject->join();
    delete mainThreadObject;
    
    delete info;
}

bool DatabaseManagement_DALs::DebugDAL::getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue)
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Get Object) > <" + Convert::toString(requestID) + ">.");
    addRequest(requestID, RequestType::SELECT, constraintType, constraintValue);
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::putObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Insert Object) > <" + Convert::toString(requestID) + ">.");
    addRequest(requestID, RequestType::INSERT, inputData, 0);
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData)
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Update Object) > <" + Convert::toString(requestID) + ">.");
    addRequest(requestID, RequestType::UPDATE, inputData, 0);
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::removeObject(DatabaseRequestID requestID, DBObjectID id)
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Remove Object) > <" + Convert::toString(requestID) + ">.");
    addRequest(requestID, RequestType::REMOVE, id, 0);
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::changeDatabaseSettings(const DatabaseSettingsContainer settings)
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Change DB Settings) > Database settings update requested.");
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::buildDatabase()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Build DB) > Database build requested.");
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::rebuildDatabase()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Rebuild DB) > Database rebuild requested.");
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::clearDatabase()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Clear DB) > Database clearing requested.");
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Clear DB) > Entering critical section.");
    boost::lock_guard<boost::mutex> requestsLock(mainThreadMutex);
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Clear DB) > Critical section entered.");

    data.clear();
    saveDataFile();

    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Clear DB) > Exiting critical section.");
    
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::connect()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Connect) > Database connect requested.");
    
    if(isConnected)
        return false;
    
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Connect) > Entering critical section.");
    boost::lock_guard<boost::mutex> requestsLock(mainThreadMutex);
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Connect) > Critical section entered.");

    loadDataFile();
    isConnected = true;

    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Connect) > Exiting critical section.");
    
    return true;
}

bool DatabaseManagement_DALs::DebugDAL::disconnect()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Disconnect) > Database disconnect requested.");
    
    if(!isConnected)
        return false;
    
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Disconnect) > Entering critical section.");
    boost::lock_guard<boost::mutex> requestsLock(mainThreadMutex);
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Disconnect) > Critical section entered.");

    saveDataFile();
    data.clear();
    isConnected = false;

    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Disconnect) > Exiting critical section.");
    
    return true;
}

const DatabaseInformationContainer* DatabaseManagement_DALs::DebugDAL::getDatabaseInfo() const
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Get DB Info) > Database info requested.");
    return info;
}

DatabaseObjectType DatabaseManagement_DALs::DebugDAL::getType() const
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Get Type) > Type requested.");
    return dalType;
}

void DatabaseManagement_DALs::DebugDAL::setID(DatabaseAbstractionLayerID id)
{
    dalID = id;
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Set ID) > DAL ID set to <" + Convert::toString(id) + ">");
}

DatabaseAbstractionLayerID DatabaseManagement_DALs::DebugDAL::getID() const
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Get ID) > DAL ID returned <" + Convert::toString(dalID) + ">");
    return dalID;
}

void DatabaseManagement_DALs::DebugDAL::loadDataFile()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Load Data) > Data load requested.");
    
    std::ifstream dataFile(dataFilePath);
    
    if(dataFile.is_open())
    {
        std::string entry;
        dataFile.seekg(0);
        
        getline(dataFile, entry);
        
        nextIntID = boost::lexical_cast<unsigned long>(entry);
        
        unsigned int i = 1;
        while(getline(dataFile, entry))
        {
            boost::smatch currentMatch;
            if(!boost::regex_search(entry, currentMatch, boost::regex("^.,(.*);(.*)")))
            {
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Load Data) > Malformed entry found <" + Convert::toString(i) + ">"); break;
                i++;
                continue;
            }
            
            DBObjectID currentID;
            std::string currentStringID = currentMatch[1];
            
            switch(entry.at(0))
            {
                case 'U': currentID = boost::lexical_cast<boost::uuids::uuid>(currentStringID); break;
                default: logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Load Data) > Invalid entry found <" + Convert::toString(i) + ">"); break;
            }
            
            data.insert(std::pair<DBObjectID,std::string>(currentID, currentMatch[2]));
            
            i++;
        }
    }
    else
        logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Load Data) > Data file is not open <" + dataFilePath + ">.");
    
    dataFile.close();
}

void DatabaseManagement_DALs::DebugDAL::saveDataFile()
{
    std::ofstream dataFile(dataFilePath, std::ofstream::trunc);
    
    dataFile << Convert::toString(nextIntID) << std::endl;
    
    for(std::pair<DBObjectID, std::string> currentEntry : data)
    {
        std::string entryString = "U," + Convert::toString(currentEntry.first) + ";" + currentEntry.second;
        
        dataFile << entryString << std::endl;
    }
    
    dataFile.close();
}

void DatabaseManagement_DALs::DebugDAL::mainThread()
{
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Started.");
    mainThreadRunnig = true;

    while(!stopDebugger)
    {
        logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Acquiring data lock.");
        boost::unique_lock<boost::mutex> dataLock(mainThreadMutex);
        logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Data lock acquired.");
        
        if(pendingRequests.size() == 0)
        {
            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Waiting on data lock.");
            mainThreadLockCondition.wait(dataLock);
            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Data lock re-acquired after wait.");
        }
        else
        {
            unsigned int numberOfNewRequests = pendingRequests.size();
            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Starting work on <" + Convert::toString(numberOfNewRequests) + "> new request(s).");
            for(unsigned int i = 0; i < numberOfNewRequests; i++)
            {
                DatabaseRequestID currentRequest = pendingRequests.front();
                tuple<RequestType, boost::any, boost::any> * currentRequestData = requestsData[currentRequest];
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Working with request <#" + Convert::toString(i) + "/" + Convert::toString(currentRequest) + ">.");
                
                switch(currentRequestData->get<0>())
                {
                    case RequestType::SELECT:
                    {
                        DBObjectID objectID = Utilities::Tools::getIDFromConstraint(dalType, currentRequestData->get<1>(), currentRequestData->get<2>());
                        
                        if(dalType == DatabaseObjectType::USER 
                                && boost::any_cast<DatabaseSelectConstraints::USERS>(currentRequestData->get<1>()) == DatabaseSelectConstraints::USERS::LIMIT_BY_NAME)
                        {
                            bool done = false;
                            
                            for(const std::pair<DBObjectID, std::string> & currentPair : data)
                            {
                                if(currentPair.second.find(boost::any_cast<std::string>(currentRequestData->get<2>())) != std::string::npos)
                                {
                                    dataLock.unlock();
                                    onSuccess(dalID, currentRequest, Stringifier::toUser(currentPair.second, currentPair.first));
                                    dataLock.lock();
                                    done = true;
                                    break;
                                }
                            }
                            
                            if(!done)
                            {
                                dataLock.unlock();
                                onFailure(dalID, currentRequest, objectID);
                                dataLock.lock();
                            }
                            
                            break;
                        }
                        
                        if(objectID != Common_Types::INVALID_OBJECT_ID)
                        {
                            if(data.find(objectID) != data.end())
                            {
                                dataLock.unlock();
                                onSuccess(dalID, currentRequest, Stringifier::toContainer(data[objectID], dalType, objectID));
                                dataLock.lock();
                            }
                            else
                            {
                                dataLock.unlock();
                                onFailure(dalID, currentRequest, objectID);
                                dataLock.lock();
                            }
                        }
                        else //for debugging purposes, always return all objects (no constraint check)
                        {
                            VectorDataContainerPtr vect(new DatabaseManagement_Containers::VectorDataContainer());
                            for(std::pair<DBObjectID, std::string> currentEntry : data)
                                vect->addDataContainer(Stringifier::toContainer(currentEntry.second, dalType, currentEntry.first));
                            
                            if(!vect->isEmpty())
                            {
                                dataLock.unlock();
                                onSuccess(dalID, currentRequest, vect);
                                dataLock.lock();
                            }
                            else
                            {
                                dataLock.unlock();
                                onFailure(dalID, currentRequest, objectID);
                                dataLock.lock();
                            }
                        }
                        
                    } break;
                    
                    case RequestType::INSERT:
                    {
                        bool successful = true;
                        
                        DataContainerPtr container = boost::any_cast<DataContainerPtr>(currentRequestData->get<1>());
                        
                        if(successful && data.find(container->getContainerID()) == data.end())
                        {
                            data.insert(std::pair<DBObjectID, std::string>(container->getContainerID(), Stringifier::toString(container)));
                        }
                        else
                            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > INSERT failed for request; object exists <#" + Convert::toString(i) + "/" + Convert::toString(currentRequest) + ">.");
                        
                        if(successful)
                        {
                            dataLock.unlock();
                            onSuccess(dalID, currentRequest, container);
                            dataLock.lock();
                        }
                        else
                        {
                            dataLock.unlock();
                            onFailure(dalID, currentRequest, container->getContainerID());
                            dataLock.lock();
                        }
                    } break;
                        
                    case RequestType::UPDATE:
                    {
                        DataContainerPtr container = boost::any_cast<DataContainerPtr>(currentRequestData->get<1>());
                        
                        if(data.find(container->getContainerID()) != data.end())
                        {
                            data[container->getContainerID()] = Stringifier::toString(container);
                            dataLock.unlock();
                            onSuccess(dalID, currentRequest, container);
                            dataLock.lock();
                        }
                        else
                        {
                            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > UPDATE failed for request <#" + Convert::toString(i) + "/" + Convert::toString(currentRequest) + ">.");
                            dataLock.unlock();
                            onFailure(dalID, currentRequest, container->getContainerID());
                            dataLock.lock();
                        }
                    } break;
                    
                    case RequestType::REMOVE:
                    {
                        DBObjectID id = boost::any_cast<DBObjectID>(currentRequestData->get<1>());
                        
                        auto containerIterator = data.find(id);
                        
                        if(containerIterator != data.end())
                        {
                            DataContainerPtr container = Stringifier::toContainer((*containerIterator).second, dalType, id);
                            data.erase(containerIterator);
                            dataLock.unlock();
                            onSuccess(dalID, currentRequest, container);
                            dataLock.lock();
                        }
                        else
                        {
                            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > REMOVE failed for request <#" + Convert::toString(i) + "/" + Convert::toString(currentRequest) + "/" + Convert::toString(id) + ">.");
                            dataLock.unlock();
                            onFailure(dalID, currentRequest, id);
                            dataLock.lock();
                        }
                    } break;
                    
                    default:
                    {
                        logger.logMessage(Utilities::FileLogSeverity::Error, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Unexpected request type encountered for new request <" + Convert::toString(currentRequest) + ">.");
                        dataLock.unlock();
                        onFailure(dalID, currentRequest, DBObjectID());
                        dataLock.lock();
                    } break;
                }
                
                pendingRequests.pop();
                requestsData.erase(currentRequest);
                delete currentRequestData;
                
                logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Done with request <#" + Convert::toString(i) + "/" + Convert::toString(currentRequest) + ">.");
            }
            
            logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Work on new requests finished.");
        }
        
        logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Data lock released.");
    }
    
    mainThreadRunnig = false;
    logger.logMessage(Utilities::FileLogSeverity::Debug, "DebugDAL / " + Convert::toString(dalType) + " (Main Thread) > Stopped.");
    return;
}

