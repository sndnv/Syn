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

#include <stdio.h>
#include "../../BasicSpec.h"
#include "../../Fixtures.h"
#include "../../../main/StorageManagement/Types/Types.h"
#include "../../../main/StorageManagement/Pools/DiskDataPool.h"

SCENARIO("Disk data pools can store, retrieve and manage data",
         "[DiskDataPool][DataPools][StorageManagement][Core]")
{
    StorageManagement_Types::DataSize poolSize = 20*1024*1024;
    std::string poolPath = "test_data/test_pool_1";
    
    GIVEN("a new DiskDataPool")
    {
        remove(poolPath.c_str());
    
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams
        {
            poolPath,   //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool testPool(initParams);
        
        CHECK(testPool.getStoredEntitiesNumber() == 0);
        CHECK(testPool.getFreeSpace() == (poolSize - testPool.getPoolManagementStorageOverhead()));
        
        WHEN("data is stored")
        {
            auto testData_1 = getByteVectorPtrFromString("test data __ 1");
            auto testData_2 = getByteVectorPtrFromString("test data __ 2");
            auto testData_3 = getByteVectorPtrFromString("test data __ 3");
            auto testData_4 = getByteVectorPtrFromString("test data __ 4");
            
            auto dataID_1 = testPool.storeData(testData_1);
            auto dataID_2 = testPool.storeData(testData_2);
            auto dataID_3 = testPool.storeData(testData_3);
            auto dataID_4 = testPool.storeData(testData_4);
            
            CHECK(testPool.getStoredEntitiesNumber() == 4);
            CHECK(testPool.getPoolState() == StorageManagement_Types::PoolState::OPEN);
            CHECK(testPool.getFreeSpace() ==
                    (poolSize
                        - testPool.getPoolManagementStorageOverhead()
                        - (4 * testPool.getEntityManagementStorageOverhead())
                        - (testData_1->size() + testData_2->size() + testData_3->size() + testData_4->size())));
            CHECK(testPool.getEntitySize(dataID_1) == testData_1->size());
            CHECK(testPool.getEntitySize(dataID_2) == testData_2->size());
            CHECK(testPool.getEntitySize(dataID_3) == testData_3->size());
            CHECK(testPool.getEntitySize(dataID_4) == testData_4->size());
            
            THEN("it can be retrieved and discarded")
            {
                CHECK(equal(testPool.retrieveData(dataID_1), testData_1));
                CHECK(equal(testPool.retrieveData(dataID_2), testData_2));
                CHECK(equal(testPool.retrieveData(dataID_3), testData_3));
                CHECK(equal(testPool.retrieveData(dataID_4), testData_4));
                
                CHECK_NOTHROW(testPool.discardData(dataID_1, true));
                CHECK_NOTHROW(testPool.discardData(dataID_2, false));
                CHECK_NOTHROW(testPool.discardData(dataID_3, true));
                
                CHECK(testPool.getStoredEntitiesNumber() == 1);
                CHECK(testPool.getFreeSpace() ==
                        (poolSize
                            - testPool.getPoolManagementStorageOverhead()
                            - testPool.getEntityManagementStorageOverhead()
                            - testData_4->size()));
            }
        }
    }
    
    GIVEN("an existing, read/write, DiskDataPool")
    {
        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams
        {
            poolPath,   //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };
        
        StorageManagement_Pools::DiskDataPool testPool(loadParams);
        
        WHEN("data is stored")
        {
            CHECK(testPool.getStoredEntitiesNumber() == 1);
            
            auto testData_1 = getByteVectorPtrFromString("test data __ 1");
            auto testData_2 = getByteVectorPtrFromString("test data __ 2");
            auto testData_3 = getByteVectorPtrFromString("test data __ 3");
            auto testData_4 = getByteVectorPtrFromString("test data __ 4");
            
            auto dataID_1 = testPool.storeData(testData_1);
            auto dataID_2 = testPool.storeData(testData_2);
            auto dataID_3 = testPool.storeData(testData_3);
            auto dataID_4 = testPool.storeData(testData_4);
            
            CHECK(testPool.getStoredEntitiesNumber() == 5);
            CHECK(testPool.getPoolState() == StorageManagement_Types::PoolState::OPEN);
            CHECK(testPool.getEntitySize(dataID_1) == testData_1->size());
            CHECK(testPool.getEntitySize(dataID_2) == testData_2->size());
            CHECK(testPool.getEntitySize(dataID_3) == testData_3->size());
            CHECK(testPool.getEntitySize(dataID_4) == testData_4->size());
            
            THEN("it can be retrieved")
            {
                CHECK(equal(testPool.retrieveData(dataID_1), testData_1));
                CHECK(equal(testPool.retrieveData(dataID_2), testData_2));
                CHECK(equal(testPool.retrieveData(dataID_3), testData_3));
                CHECK(equal(testPool.retrieveData(dataID_4), testData_4));
                
                CHECK_NOTHROW(testPool.discardData(dataID_1, false));
                CHECK_NOTHROW(testPool.discardData(dataID_2, true));
                CHECK_NOTHROW(testPool.discardData(dataID_3, false));
                
                CHECK(testPool.getStoredEntitiesNumber() == 2);
            }
        }
        
        AND_WHEN("the pool is cleared")
        {
            CHECK_NOTHROW(testPool.clearPool());
            
            THEN("it becomes empty")
            {
                CHECK(testPool.getStoredEntitiesNumber() == 0);
                CHECK(testPool.getFreeSpace() == (poolSize - testPool.getPoolManagementStorageOverhead()));
            }
        }
    }
    
    GIVEN("an existing, read-only, DiskDataPool")
    {
        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams
        {
            poolPath,   //poolFilePath
            StorageManagement_Types::PoolMode::READ_ONLY,   //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };
        
        StorageManagement_Pools::DiskDataPool testPool(loadParams);
        
        WHEN("data modification attempts are made")
        {
            THEN("they are rejected")
            {
                auto testData_1 = getByteVectorPtrFromString("test data __ 1");
                
                CHECK_THROWS_AS(testPool.storeData(testData_1), std::runtime_error);
                CHECK_THROWS_AS(testPool.discardData(1), std::runtime_error);
                CHECK_THROWS_AS(testPool.clearPool(), std::runtime_error);
            }
        }
    }
}

SCENARIO("Disk data pools fail to initialize with invalid parameters",
         "[DiskDataPool][DataPools][StorageManagement][Core]")
{
    GIVEN("invalid parameters for a new DiskDataPool")
    {
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_1
        {
            "test_data/test_pool_1",   //poolFilePath
            StorageManagement_Pools::DiskDataPool::OVERHEAD_POOL_MANAGEMENT, //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_2
        {
            "test_data/test_pool_1",   //poolFilePath
            (StorageManagement_Pools::DiskDataPool::OVERHEAD_POOL_MANAGEMENT - 1), //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_3
        {
            "",     //poolFilePath
            1024,   //poolSize
            false   //eraseDataOnFailure
        };
        
        THEN("a DiskDataPool fails to initialize")
        {
            CHECK_THROWS_AS(new StorageManagement_Pools::DiskDataPool(initParams_1), std::invalid_argument);
            CHECK_THROWS_AS(new StorageManagement_Pools::DiskDataPool(initParams_2), std::invalid_argument);
            CHECK_THROWS_AS(new StorageManagement_Pools::DiskDataPool(initParams_3), std::runtime_error);
        }
    }
}
