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

#include "../../BasicSpec.h"
#include "../../Fixtures.h"
#include "../../../main/StorageManagement/Types/Types.h"
#include "../../../main/StorageManagement/Pools/MemoryDataPool.h"

SCENARIO("Memory data pools can store, retrieve and manage data",
         "[MemoryDataPool][DataPools][StorageManagement][Core]")
{
    StorageManagement_Types::DataSize poolSize = 20*1024*1024;
    
    GIVEN("a new, read/write, MemoryDataPool")
    {
        StorageManagement_Pools::MemoryDataPool::MemoryDataPoolParameters initParams
        {
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            poolSize    //size
        };
        
        StorageManagement_Pools::MemoryDataPool testPool(initParams);
        
        CHECK(testPool.getStoredEntitiesNumber() == 0);
        CHECK(testPool.getFreeSpace() == poolSize);
        
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
                    (poolSize - (testData_1->size() + testData_2->size() + testData_3->size() + testData_4->size())));
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
                CHECK(testPool.getFreeSpace() == (poolSize - testData_4->size()));
            }
        }
    }
    
    GIVEN("a new, read-only, MemoryDataPool")
    {
        StorageManagement_Pools::MemoryDataPool::MemoryDataPoolParameters initParams
        {
            StorageManagement_Types::PoolMode::READ_ONLY,  //mode
            poolSize    //size
        };
        
        StorageManagement_Pools::MemoryDataPool testPool(initParams);
        
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