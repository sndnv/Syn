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

#ifndef TESTDAL_H
#define TESTDAL_H

#include <atomic>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/uuid/random_generator.hpp>
#include "../../main/Utilities/FileLogger.h"
#include "../../main/Utilities/Tools.h"
#include "../../main/DatabaseManagement/Interfaces/DatabaseAbstractionLayer.h"
#include "../../main/DatabaseManagement/Interfaces/DatabaseSettingsContainer.h"
#include "../../main/DatabaseManagement/Interfaces/DatabaseInformationContainer.h"
#include "../../main/DatabaseManagement/Containers/VectorDataContainer.h"
#include "../../main/DatabaseManagement/Containers/UserDataContainer.h"

using DatabaseManagement_Interfaces::DatabaseAbstractionLayer;
using DatabaseManagement_Interfaces::DatabaseInformationContainer;
using DatabaseManagement_Interfaces::DatabaseSettingsContainer;

using DatabaseManagement_Containers::UserDataContainer;
using DatabaseManagement_Containers::UserDataContainerPtr;
using DatabaseManagement_Containers::VectorDataContainer;
using DatabaseManagement_Containers::VectorDataContainerPtr;

namespace Testing
{
    class TestDAL : public DatabaseAbstractionLayer
    {
        public:
            class TestDALInformationContainer : public DatabaseInformationContainer
            {
                public:
                    ~TestDALInformationContainer() {}

                    virtual std::string getDatabaseName() const { return "TestDAL"; }
                    virtual long getDatabaseSize() const { return 42; }
            };
            
            TestDAL(bool getObjectResponse, bool putObjectResponse,
                    bool updateObjectResponse, bool removeObjectResponse,
                    DatabaseObjectType type, bool enableLogger);
            
            ~TestDAL() override;
            
            bool getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue) override;
            bool putObject(DatabaseRequestID requestID, const DataContainerPtr inputData) override;
            bool updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData) override;
            bool removeObject(DatabaseRequestID requestID, DBObjectID id) override;
            
            bool changeDatabaseSettings(const DatabaseSettingsContainer settings) override
            {
                ++changeDatabaseSettings_calls;
                return true;
            }
            
            bool buildDatabase() override
            {
                ++buildDatabase_calls;
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                data.clear();
                return true;
            }
            
            bool rebuildDatabase() override
            {
                ++rebuildDatabase_calls;
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                data.clear();
                return true;
            }
            
            bool clearDatabase() override
            {
                ++clearDatabase_calls;
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                data.clear();
                return true;
            }
            
            bool connect() override
            {
                ++connect_calls;
                return true;
            }
            
            bool disconnect() override
            {
                ++disconnect_calls;
                return true;
            }
            
            const DatabaseInformationContainer* getDatabaseInfo() const override
            {
                ++getDatabaseInfo_calls;
                return new TestDALInformationContainer{};
            }
            
            DatabaseObjectType getType() const override
            {
                ++getType_calls;
                return dalType;
            }
            
            void setID(DatabaseAbstractionLayerID id) override
            {
                ++setID_calls;
                dalID = id;
            }
            
            DatabaseAbstractionLayerID getID() const override
            {
                ++getID_calls;
                return dalID;
            }
            
            bool getObject_expectedResponse;
            std::atomic<unsigned int> getObject_received;
            std::atomic<unsigned int> getObject_completed;
            std::atomic<unsigned int> getObject_failed;
            
            bool putObject_expectedResponse;
            std::atomic<unsigned int> putObject_received;
            std::atomic<unsigned int> putObject_completed;
            std::atomic<unsigned int> putObject_failed;
            
            bool updateObject_expectedResponse;
            std::atomic<unsigned int> updateObject_received;
            std::atomic<unsigned int> updateObject_completed;
            std::atomic<unsigned int> updateObject_failed;
            
            bool removeObject_expectedResponse;
            std::atomic<unsigned int> removeObject_received;
            std::atomic<unsigned int> removeObject_completed;
            std::atomic<unsigned int> removeObject_failed;
            
            std::atomic<unsigned int> changeDatabaseSettings_calls;
            std::atomic<unsigned int> buildDatabase_calls;
            std::atomic<unsigned int> rebuildDatabase_calls;
            std::atomic<unsigned int> clearDatabase_calls;
            std::atomic<unsigned int> connect_calls;
            std::atomic<unsigned int> disconnect_calls;
            mutable std::atomic<unsigned int> getDatabaseInfo_calls;
            mutable std::atomic<unsigned int> getType_calls;
            std::atomic<unsigned int> setID_calls;
            mutable std::atomic<unsigned int> getID_calls;
            
        private:
            DatabaseAbstractionLayerID dalID;
            boost::uuids::uuid dalUUID;
            DatabaseObjectType dalType;
            
            boost::mutex dataMutex;
            boost::unordered_map<DBObjectID, DataContainerPtr> data;
            
            Utilities::FileLoggerPtr debugLogger;
            void logMessage(LogSeverity severity, const std::string & message) const
            {
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "TestDAL " + message);
            }
    };
}

#endif /* TESTDAL_H */
