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
#include "../../../main/StorageManagement/Pools/PoolAggregator.h"

SCENARIO("Pool aggregators can be initialized and managed",
         "[PoolAggregator][DataPools][StorageManagement][Core]")
{
    StorageManagement_Types::DataSize poolSize = 20*1024*1024;
    
    GIVEN("a new PoolAggregator and a set of DataPools")
    {
        StorageManagement_Pools::PoolAggregator::PoolAggregatorInitParameters params
        {
            2,      //threadPoolSize
            false,  //completeRetrieve
            false,  //completeDiscard
            false,  //completePendingStore
            true,   //eraseOnDiscard
            false,  //cancelActionsOnShutdown
            0,      //maxNonStreamableData
            StorageManagement_Types::PoolMode::READ_WRITE,
            nullptr //streamingPool
        };
        
        StorageManagement_Pools::PoolAggregator testAggregator(params);
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_1
        {
            "./test_pool_1", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_2
        {
            "./test_pool_2", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_3
        {
            "./test_pool_3", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_4
        {
            "./test_pool_4", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_5
        {
            "./test_pool_5", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_6
        {
            "./test_pool_6", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_7
        {
            "./test_pool_7", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_8
        {
            "./test_pool_8", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_9
        {
            "./test_pool_9", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_10
        {
            "./test_pool_10", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };
        
        StorageManagement_Pools::DiskDataPool testDiskPool_1(initParams_1);
        StorageManagement_Pools::DiskDataPool testDiskPool_2(initParams_2);
        StorageManagement_Pools::DiskDataPool testDiskPool_3(initParams_3);
        StorageManagement_Pools::DiskDataPool testDiskPool_4(initParams_4);
        StorageManagement_Pools::DiskDataPool testDiskPool_5(initParams_5);
        StorageManagement_Pools::DiskDataPool testDiskPool_6(initParams_6);
        StorageManagement_Pools::DiskDataPool testDiskPool_7(initParams_7);
        StorageManagement_Pools::DiskDataPool testDiskPool_8(initParams_8);
        StorageManagement_Pools::DiskDataPool testDiskPool_9(initParams_9);
        StorageManagement_Pools::DiskDataPool testDiskPool_10(initParams_10);
        
        CHECK(testDiskPool_1.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_1.getFreeSpace() == (poolSize - testDiskPool_1.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_2.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_2.getFreeSpace() == (poolSize - testDiskPool_2.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_3.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_3.getFreeSpace() == (poolSize - testDiskPool_3.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_4.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_4.getFreeSpace() == (poolSize - testDiskPool_4.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_5.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_5.getFreeSpace() == (poolSize - testDiskPool_5.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_6.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_6.getFreeSpace() == (poolSize - testDiskPool_6.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_7.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_7.getFreeSpace() == (poolSize - testDiskPool_7.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_8.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_8.getFreeSpace() == (poolSize - testDiskPool_8.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_9.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_9.getFreeSpace() == (poolSize - testDiskPool_9.getPoolManagementStorageOverhead()));
        CHECK(testDiskPool_10.getStoredEntitiesNumber() == 0);
        CHECK(testDiskPool_10.getFreeSpace() == (poolSize - testDiskPool_10.getPoolManagementStorageOverhead()));
        
        WHEN("they are added to the aggregator")
        {
            auto poolID_1 = testAggregator.addPool(&testDiskPool_1);
            auto poolID_2 = testAggregator.addPool(&testDiskPool_2);
            auto poolID_3 = testAggregator.addPool(&testDiskPool_3);
            auto poolID_4 = testAggregator.addPool(&testDiskPool_4);
            auto poolID_5 = testAggregator.addPool(&testDiskPool_5);
            auto poolID_6 = testAggregator.addPool(&testDiskPool_6);
            auto poolID_7 = testAggregator.addPool(&testDiskPool_7);
            auto poolID_8 = testAggregator.addPool(&testDiskPool_8);
            auto poolID_9 = testAggregator.addPool(&testDiskPool_9);
            auto poolID_10 = testAggregator.addPool(&testDiskPool_10);
            
            THEN("they can be linked, unlinked, have their data exported/imported and removed")
            {
                StorageManagement_Pools::PoolAggregator::LinkParameters link_0_1
                {
                    poolID_1,                                               //targetPool
                    StorageManagement_Types::LinkActionType::COPY,          //action
                    StorageManagement_Types::LinkActionConditionType::NONE, //condition
                    0                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_0_2
                {
                    poolID_2,                                               //targetPool
                    StorageManagement_Types::LinkActionType::COPY,          //action
                    StorageManagement_Types::LinkActionConditionType::NONE, //condition
                    0                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_1_3
                {
                    poolID_3,                                               //targetPool
                    StorageManagement_Types::LinkActionType::COPY,          //action
                    StorageManagement_Types::LinkActionConditionType::NONE, //condition
                    0                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_2_4
                {
                    poolID_4,                                               //targetPool
                    StorageManagement_Types::LinkActionType::COPY,          //action
                    StorageManagement_Types::LinkActionConditionType::NONE, //condition
                    0                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_3_5
                {
                    poolID_5,                                               //targetPool
                    StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
                    StorageManagement_Types::LinkActionConditionType::TIMED,//condition
                    2                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_3_6
                {
                    poolID_6,                                               //targetPool
                    StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
                    StorageManagement_Types::LinkActionConditionType::TIMED,//condition
                    2                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_3_7
                {
                    poolID_7,                                               //targetPool
                    StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
                    StorageManagement_Types::LinkActionConditionType::TIMED,//condition
                    2                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_4_8
                {
                    poolID_8,                                               //targetPool
                    StorageManagement_Types::LinkActionType::MOVE,          //action
                    StorageManagement_Types::LinkActionConditionType::DATA_MIN_SIZE, //condition
                    10                                                      //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_5_9
                {
                    poolID_9,                                               //targetPool
                    StorageManagement_Types::LinkActionType::MOVE,          //action
                    StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
                    3                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_6_9
                {
                    poolID_9,                                               //targetPool
                    StorageManagement_Types::LinkActionType::MOVE,          //action
                    StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
                    3                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_7_9
                {
                    poolID_9,                                               //targetPool
                    StorageManagement_Types::LinkActionType::MOVE,          //action
                    StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
                    3                                                       //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_8_0
                {
                    INVALID_POOL_ID,                                            //targetPool
                    StorageManagement_Types::LinkActionType::DISCARD,           //action
                    StorageManagement_Types::LinkActionConditionType::TIMED,    //condition
                    2                                                           //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_9_10
                {
                    poolID_10,                                              //targetPool
                    StorageManagement_Types::LinkActionType::COPY,          //action
                    StorageManagement_Types::LinkActionConditionType::TARGET_MAX_FULL, //condition
                    50                                                      //conditionValue
                };
                
                StorageManagement_Pools::PoolAggregator::LinkParameters link_10_0
                {
                    INVALID_POOL_ID,                                            //targetPool
                    StorageManagement_Types::LinkActionType::DISCARD,           //action
                    StorageManagement_Types::LinkActionConditionType::TIMED,    //condition
                    2                                                           //conditionValue
                };
                
                testAggregator.addPoolLink(testAggregator.getAggregatorID(), link_0_1);
                testAggregator.addPoolLink(testAggregator.getAggregatorID(), link_0_2);
                testAggregator.addPoolLink(poolID_1, link_1_3);
                testAggregator.addPoolLink(poolID_2, link_2_4);
                testAggregator.addPoolLink(poolID_3, link_3_5);
                testAggregator.addPoolLink(poolID_3, link_3_6);
                testAggregator.addPoolLink(poolID_3, link_3_7);
                testAggregator.addPoolLink(poolID_4, link_4_8);
                testAggregator.addPoolLink(poolID_5, link_5_9);
                testAggregator.addPoolLink(poolID_6, link_6_9);
                testAggregator.addPoolLink(poolID_7, link_7_9);
                testAggregator.addPoolLink(poolID_8, link_8_0);
                testAggregator.addPoolLink(poolID_9, link_9_10);
                testAggregator.addPoolLink(poolID_10, link_10_0);
                
                auto exportedConfig_1 = testAggregator.exportConfiguration();
                auto exportedIDData_1 = testAggregator.exportIDData();
                CHECK(exportedIDData_1.size() == 0);
                
                testAggregator.removePoolLink(poolID_10, INVALID_POOL_ID);
                testAggregator.removePoolLink(poolID_9, poolID_10);
                testAggregator.removePoolLink(poolID_8, INVALID_POOL_ID);
                testAggregator.removePoolLink(poolID_7, poolID_9);
                testAggregator.removePoolLink(poolID_6, poolID_9);
                testAggregator.removePoolLink(poolID_5, poolID_9);
                testAggregator.removePoolLink(poolID_4, poolID_8);
                testAggregator.removePoolLink(poolID_3, poolID_7);
                testAggregator.removePoolLink(poolID_3, poolID_6);
                testAggregator.removePoolLink(poolID_3, poolID_5);
                testAggregator.removePoolLink(poolID_2, poolID_4);
                testAggregator.removePoolLink(poolID_1, poolID_3);
                testAggregator.removePoolLink(testAggregator.getAggregatorID(), poolID_2);
                testAggregator.removePoolLink(testAggregator.getAggregatorID(), poolID_1);
                
                testAggregator.removePool(poolID_10);
                testAggregator.removePool(poolID_9);
                testAggregator.removePool(poolID_8);
                testAggregator.removePool(poolID_7);
                testAggregator.removePool(poolID_6);
                testAggregator.removePool(poolID_5);
                testAggregator.removePool(poolID_4);
                testAggregator.removePool(poolID_3);
                testAggregator.removePool(poolID_2);
                testAggregator.removePool(poolID_1);
                
                {
                    exportedConfig_1.pools[testDiskPool_1.getPoolUUID()] = &testDiskPool_1;
                    exportedConfig_1.pools[testDiskPool_2.getPoolUUID()] = &testDiskPool_2;
                    exportedConfig_1.pools[testDiskPool_3.getPoolUUID()] = &testDiskPool_3;
                    exportedConfig_1.pools[testDiskPool_4.getPoolUUID()] = &testDiskPool_4;
                    exportedConfig_1.pools[testDiskPool_5.getPoolUUID()] = &testDiskPool_5;
                    exportedConfig_1.pools[testDiskPool_6.getPoolUUID()] = &testDiskPool_6;
                    exportedConfig_1.pools[testDiskPool_7.getPoolUUID()] = &testDiskPool_7;
                    exportedConfig_1.pools[testDiskPool_8.getPoolUUID()] = &testDiskPool_8;
                    exportedConfig_1.pools[testDiskPool_9.getPoolUUID()] = &testDiskPool_9;
                    exportedConfig_1.pools[testDiskPool_10.getPoolUUID()] = &testDiskPool_10;
                    
                    StorageManagement_Pools::PoolAggregator rebuiltAggregator(exportedConfig_1);

                    auto exportedConfig_2 = rebuiltAggregator.exportConfiguration();
                    auto poolIDs = rebuiltAggregator.getPoolIDsMap();
                    auto poolID_11 = poolIDs[testDiskPool_1.getPoolUUID()];
                    auto poolID_12 = poolIDs[testDiskPool_2.getPoolUUID()];
                    auto poolID_13 = poolIDs[testDiskPool_3.getPoolUUID()];
                    auto poolID_14 = poolIDs[testDiskPool_4.getPoolUUID()];
                    auto poolID_15 = poolIDs[testDiskPool_5.getPoolUUID()];
                    auto poolID_16 = poolIDs[testDiskPool_6.getPoolUUID()];
                    auto poolID_17 = poolIDs[testDiskPool_7.getPoolUUID()];
                    auto poolID_18 = poolIDs[testDiskPool_8.getPoolUUID()];
                    auto poolID_19 = poolIDs[testDiskPool_9.getPoolUUID()];
                    auto poolID_20 = poolIDs[testDiskPool_10.getPoolUUID()];
                    
                    CHECK(exportedConfig_1.bytesRead == exportedConfig_2.bytesRead);
                    CHECK(exportedConfig_1.bytesWritten == exportedConfig_2.bytesWritten);
                    CHECK(exportedConfig_1.cancelActionsOnShutdown == exportedConfig_2.cancelActionsOnShutdown);
                    CHECK(exportedConfig_1.completeDiscard == exportedConfig_2.completeDiscard);
                    CHECK(exportedConfig_1.completePendingStore == exportedConfig_2.completePendingStore);
                    CHECK(exportedConfig_1.completeRetrieve == exportedConfig_2.completeRetrieve);
                    CHECK(exportedConfig_1.eraseOnDiscard == exportedConfig_2.eraseOnDiscard);
                    CHECK(exportedConfig_1.lastEntityID == exportedConfig_2.lastEntityID);
                    CHECK(exportedConfig_1.maxNonStreamableData == exportedConfig_2.maxNonStreamableData);
                    CHECK(exportedConfig_1.mode == exportedConfig_2.mode);
                    CHECK(exportedConfig_1.streamingPoolUUID == exportedConfig_2.streamingPoolUUID);
                    CHECK(exportedConfig_1.threadPoolSize == exportedConfig_2.threadPoolSize);
                    CHECK(exportedConfig_1.uuid == exportedConfig_2.uuid);
                    CHECK(exportedConfig_1.links.size() == exportedConfig_2.links.size());
                    CHECK(exportedConfig_1.pools.size() == exportedConfig_2.pools.size());
                    
                    for(auto currentLink : exportedConfig_1.links)
                        CHECK(currentLink.second == exportedConfig_2.links[currentLink.first]);
                    
                    for(auto currentPool : exportedConfig_1.pools)
                        CHECK(currentPool.second == exportedConfig_2.pools[currentPool.first]);
                    
                    rebuiltAggregator.removePoolLink(poolID_20, INVALID_POOL_ID);
                    rebuiltAggregator.removePoolLink(poolID_19, poolID_20);
                    rebuiltAggregator.removePoolLink(poolID_18, INVALID_POOL_ID);
                    rebuiltAggregator.removePoolLink(poolID_17, poolID_19);
                    rebuiltAggregator.removePoolLink(poolID_16, poolID_19);
                    rebuiltAggregator.removePoolLink(poolID_15, poolID_19);
                    rebuiltAggregator.removePoolLink(poolID_14, poolID_18);
                    rebuiltAggregator.removePoolLink(poolID_13, poolID_17);
                    rebuiltAggregator.removePoolLink(poolID_13, poolID_16);
                    rebuiltAggregator.removePoolLink(poolID_13, poolID_15);
                    rebuiltAggregator.removePoolLink(poolID_12, poolID_14);
                    rebuiltAggregator.removePoolLink(poolID_11, poolID_13);
                    rebuiltAggregator.removePoolLink(rebuiltAggregator.getAggregatorID(), poolID_12);
                    rebuiltAggregator.removePoolLink(rebuiltAggregator.getAggregatorID(), poolID_11);

                    rebuiltAggregator.removePool(poolID_20);
                    rebuiltAggregator.removePool(poolID_19);
                    rebuiltAggregator.removePool(poolID_18);
                    rebuiltAggregator.removePool(poolID_17);
                    rebuiltAggregator.removePool(poolID_16);
                    rebuiltAggregator.removePool(poolID_15);
                    rebuiltAggregator.removePool(poolID_14);
                    rebuiltAggregator.removePool(poolID_13);
                    rebuiltAggregator.removePool(poolID_12);
                    rebuiltAggregator.removePool(poolID_11);
                }
            }
        }
    }
}

StorageManagement_Pools::PoolAggregator::PoolAggregatorLoadParameters exportedConfig;
boost::unordered_map<PoolUUID, std::deque<StorageManagement_Pools::PoolAggregator::EntityIDData>> exportedIDData;

SCENARIO("Pool aggregators can store, retrieve and manage data",
         "[PoolAggregator][DataPools][StorageManagement][Core]")
{
    StorageManagement_Types::DataSize poolSize = 20*1024*1024;
    
    GIVEN("a new PoolAggregator")
    {
        StorageManagement_Pools::PoolAggregator::PoolAggregatorInitParameters params
        {
            2,      //threadPoolSize
            false,  //completeRetrieve
            false,  //completeDiscard
            false,  //completePendingStore
            true,   //eraseOnDiscard
            false,  //cancelActionsOnShutdown
            0,      //maxNonStreamableData
            StorageManagement_Types::PoolMode::READ_WRITE,
            nullptr //streamingPool
        };
        
        StorageManagement_Pools::PoolAggregator testAggregator(params);
        
        remove("./test_pool_1");
        remove("./test_pool_2");
        remove("./test_pool_3");
        remove("./test_pool_4");
        remove("./test_pool_5");
        remove("./test_pool_6");
        remove("./test_pool_7");
        remove("./test_pool_8");
        remove("./test_pool_9");
        remove("./test_pool_10");

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_1
        {
            "./test_pool_1", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_2
        {
            "./test_pool_2", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_3
        {
            "./test_pool_3", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_4
        {
            "./test_pool_4", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_5
        {
            "./test_pool_5", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_6
        {
            "./test_pool_6", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_7
        {
            "./test_pool_7", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_8
        {
            "./test_pool_8", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_9
        {
            "./test_pool_9", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolInitParameters initParams_10
        {
            "./test_pool_10", //poolFilePath
            poolSize,   //poolSize
            false       //eraseDataOnFailure
        };

        StorageManagement_Pools::DiskDataPool testDiskPool_1(initParams_1);
        StorageManagement_Pools::DiskDataPool testDiskPool_2(initParams_2);
        StorageManagement_Pools::DiskDataPool testDiskPool_3(initParams_3);
        StorageManagement_Pools::DiskDataPool testDiskPool_4(initParams_4);
        StorageManagement_Pools::DiskDataPool testDiskPool_5(initParams_5);
        StorageManagement_Pools::DiskDataPool testDiskPool_6(initParams_6);
        StorageManagement_Pools::DiskDataPool testDiskPool_7(initParams_7);
        StorageManagement_Pools::DiskDataPool testDiskPool_8(initParams_8);
        StorageManagement_Pools::DiskDataPool testDiskPool_9(initParams_9);
        StorageManagement_Pools::DiskDataPool testDiskPool_10(initParams_10);
        
        auto poolID_1 = testAggregator.addPool(&testDiskPool_1);
        auto poolID_2 = testAggregator.addPool(&testDiskPool_2);
        auto poolID_3 = testAggregator.addPool(&testDiskPool_3);
        auto poolID_4 = testAggregator.addPool(&testDiskPool_4);
        auto poolID_5 = testAggregator.addPool(&testDiskPool_5);
        auto poolID_6 = testAggregator.addPool(&testDiskPool_6);
        auto poolID_7 = testAggregator.addPool(&testDiskPool_7);
        auto poolID_8 = testAggregator.addPool(&testDiskPool_8);
        auto poolID_9 = testAggregator.addPool(&testDiskPool_9);
        auto poolID_10 = testAggregator.addPool(&testDiskPool_10);
        
        StorageManagement_Pools::PoolAggregator::LinkParameters link_0_1
        {
            poolID_1,                                               //targetPool
            StorageManagement_Types::LinkActionType::COPY,          //action
            StorageManagement_Types::LinkActionConditionType::NONE, //condition
            0                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_0_2
        {
            poolID_2,                                               //targetPool
            StorageManagement_Types::LinkActionType::COPY,          //action
            StorageManagement_Types::LinkActionConditionType::NONE, //condition
            0                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_1_3
        {
            poolID_3,                                               //targetPool
            StorageManagement_Types::LinkActionType::COPY,          //action
            StorageManagement_Types::LinkActionConditionType::NONE, //condition
            0                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_2_4
        {
            poolID_4,                                               //targetPool
            StorageManagement_Types::LinkActionType::COPY,          //action
            StorageManagement_Types::LinkActionConditionType::NONE, //condition
            0                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_3_5
        {
            poolID_5,                                               //targetPool
            StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
            StorageManagement_Types::LinkActionConditionType::TIMED,//condition
            2                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_3_6
        {
            poolID_6,                                               //targetPool
            StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
            StorageManagement_Types::LinkActionConditionType::TIMED,//condition
            2                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_3_7
        {
            poolID_7,                                               //targetPool
            StorageManagement_Types::LinkActionType::DISTRIBUTE,    //action
            StorageManagement_Types::LinkActionConditionType::TIMED,//condition
            2                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_4_8
        {
            poolID_8,                                               //targetPool
            StorageManagement_Types::LinkActionType::MOVE,          //action
            StorageManagement_Types::LinkActionConditionType::DATA_MIN_SIZE, //condition
            10                                                      //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_5_9
        {
            poolID_9,                                               //targetPool
            StorageManagement_Types::LinkActionType::MOVE,          //action
            StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
            3                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_6_9
        {
            poolID_9,                                               //targetPool
            StorageManagement_Types::LinkActionType::MOVE,          //action
            StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
            3                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_7_9
        {
            poolID_9,                                               //targetPool
            StorageManagement_Types::LinkActionType::MOVE,          //action
            StorageManagement_Types::LinkActionConditionType::SOURCE_MIN_ENTITIES, //condition
            3                                                       //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_8_0
        {
            INVALID_POOL_ID,                                            //targetPool
            StorageManagement_Types::LinkActionType::DISCARD,           //action
            StorageManagement_Types::LinkActionConditionType::TIMED,    //condition
            2                                                           //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_9_10
        {
            poolID_10,                                              //targetPool
            StorageManagement_Types::LinkActionType::COPY,          //action
            StorageManagement_Types::LinkActionConditionType::TARGET_MAX_FULL, //condition
            50                                                      //conditionValue
        };

        StorageManagement_Pools::PoolAggregator::LinkParameters link_10_0
        {
            INVALID_POOL_ID,                                            //targetPool
            StorageManagement_Types::LinkActionType::DISCARD,           //action
            StorageManagement_Types::LinkActionConditionType::TIMED,    //condition
            2                                                           //conditionValue
        };

        testAggregator.addPoolLink(testAggregator.getAggregatorID(), link_0_1);
        testAggregator.addPoolLink(testAggregator.getAggregatorID(), link_0_2);
        testAggregator.addPoolLink(poolID_1, link_1_3);
        testAggregator.addPoolLink(poolID_2, link_2_4);
        testAggregator.addPoolLink(poolID_3, link_3_5);
        testAggregator.addPoolLink(poolID_3, link_3_6);
        testAggregator.addPoolLink(poolID_3, link_3_7);
        testAggregator.addPoolLink(poolID_4, link_4_8);
        testAggregator.addPoolLink(poolID_5, link_5_9);
        testAggregator.addPoolLink(poolID_6, link_6_9);
        testAggregator.addPoolLink(poolID_7, link_7_9);
        testAggregator.addPoolLink(poolID_8, link_8_0);
        testAggregator.addPoolLink(poolID_9, link_9_10);
        testAggregator.addPoolLink(poolID_10, link_10_0);
        
        WHEN("data is stored")
        {
            auto testData_1 = getByteVectorPtrFromString("test data __ 1");
            auto testData_2 = getByteVectorPtrFromString("test data __ 2");
            auto testData_3 = getByteVectorPtrFromString("test data __ 3");
            auto testData_4 = getByteVectorPtrFromString("test data __ 4");
            
            auto dataID_1 = testAggregator.storeData(testData_1);
            auto dataID_2 = testAggregator.storeData(testData_2);
            auto dataID_3 = testAggregator.storeData(testData_3);
            auto dataID_4 = testAggregator.storeData(testData_4);
            
            waitFor(1);
            
            CHECK(testAggregator.getStoredEntitiesNumber() == 4);
            
            CHECK(testAggregator.getPoolState() == StorageManagement_Types::PoolState::OPEN);
            CHECK(testAggregator.getEntitySize(dataID_1) == testData_1->size());
            CHECK(testAggregator.getEntitySize(dataID_2) == testData_2->size());
            CHECK(testAggregator.getEntitySize(dataID_3) == testData_3->size());
            CHECK(testAggregator.getEntitySize(dataID_4) == testData_4->size());
            
            THEN("it can be retrieved and discarded")
            {
                CHECK(equal(testAggregator.retrieveData(dataID_1), testData_1));
                CHECK(equal(testAggregator.retrieveData(dataID_2), testData_2));
                CHECK(equal(testAggregator.retrieveData(dataID_3), testData_3));
                CHECK(equal(testAggregator.retrieveData(dataID_4), testData_4));
                
                CHECK_NOTHROW(testAggregator.discardData(dataID_1, true));
                CHECK_NOTHROW(testAggregator.discardData(dataID_2, false));
                CHECK_NOTHROW(testAggregator.discardData(dataID_3, true));
                
                CHECK(testAggregator.getStoredEntitiesNumber() == 1);
            }
        }
        
        exportedConfig = testAggregator.exportConfiguration();
        exportedIDData = testAggregator.exportIDData();
    }
    
    GIVEN("an existing, read/write, PoolAggregator")
    {
        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_1
        {
            "./test_pool_1", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_2
        {
            "./test_pool_2", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_3
        {
            "./test_pool_3", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_4
        {
            "./test_pool_4", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_5
        {
            "./test_pool_5", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_6
        {
            "./test_pool_6", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_7
        {
            "./test_pool_7", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_8
        {
            "./test_pool_8", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_9
        {
            "./test_pool_9", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool::DiskDataPoolLoadParameters loadParams_10
        {
            "./test_pool_10", //poolFilePath
            StorageManagement_Types::PoolMode::READ_WRITE,  //mode
            false,      //eraseDataOnFailure
            0,          //bytesRead
            0           //bytesWritten
        };

        StorageManagement_Pools::DiskDataPool testDiskPool_1(loadParams_1);
        StorageManagement_Pools::DiskDataPool testDiskPool_2(loadParams_2);
        StorageManagement_Pools::DiskDataPool testDiskPool_3(loadParams_3);
        StorageManagement_Pools::DiskDataPool testDiskPool_4(loadParams_4);
        StorageManagement_Pools::DiskDataPool testDiskPool_5(loadParams_5);
        StorageManagement_Pools::DiskDataPool testDiskPool_6(loadParams_6);
        StorageManagement_Pools::DiskDataPool testDiskPool_7(loadParams_7);
        StorageManagement_Pools::DiskDataPool testDiskPool_8(loadParams_8);
        StorageManagement_Pools::DiskDataPool testDiskPool_9(loadParams_9);
        StorageManagement_Pools::DiskDataPool testDiskPool_10(loadParams_10);
        
        exportedConfig.pools[testDiskPool_1.getPoolUUID()] = &testDiskPool_1;
        exportedConfig.pools[testDiskPool_2.getPoolUUID()] = &testDiskPool_2;
        exportedConfig.pools[testDiskPool_3.getPoolUUID()] = &testDiskPool_3;
        exportedConfig.pools[testDiskPool_4.getPoolUUID()] = &testDiskPool_4;
        exportedConfig.pools[testDiskPool_5.getPoolUUID()] = &testDiskPool_5;
        exportedConfig.pools[testDiskPool_6.getPoolUUID()] = &testDiskPool_6;
        exportedConfig.pools[testDiskPool_7.getPoolUUID()] = &testDiskPool_7;
        exportedConfig.pools[testDiskPool_8.getPoolUUID()] = &testDiskPool_8;
        exportedConfig.pools[testDiskPool_9.getPoolUUID()] = &testDiskPool_9;
        exportedConfig.pools[testDiskPool_10.getPoolUUID()] = &testDiskPool_10;
        
        StorageManagement_Pools::PoolAggregator rebuiltAggregator(exportedConfig);
        rebuiltAggregator.importIDData(exportedIDData, true);
        
        CHECK(rebuiltAggregator.getStoredEntitiesNumber() == 1);
        
        WHEN("data is stored")
        {
            auto testData_1 = getByteVectorPtrFromString("test data __ 1");
            auto testData_2 = getByteVectorPtrFromString("test data __ 2");
            auto testData_3 = getByteVectorPtrFromString("test data __ 3");
            auto testData_4 = getByteVectorPtrFromString("test data __ 4");
            
            auto dataID_1 = rebuiltAggregator.storeData(testData_1);
            auto dataID_2 = rebuiltAggregator.storeData(testData_2);
            auto dataID_3 = rebuiltAggregator.storeData(testData_3);
            auto dataID_4 = rebuiltAggregator.storeData(testData_4);
            
            waitFor(1);
            
            CHECK(rebuiltAggregator.getStoredEntitiesNumber() == 5);
            CHECK(rebuiltAggregator.getPoolState() == StorageManagement_Types::PoolState::OPEN);
            CHECK(rebuiltAggregator.getEntitySize(dataID_1) == testData_1->size());
            CHECK(rebuiltAggregator.getEntitySize(dataID_2) == testData_2->size());
            CHECK(rebuiltAggregator.getEntitySize(dataID_3) == testData_3->size());
            CHECK(rebuiltAggregator.getEntitySize(dataID_4) == testData_4->size());
            
            THEN("it can be retrieved")
            {
                CHECK(equal(rebuiltAggregator.retrieveData(dataID_1), testData_1));
                CHECK(equal(rebuiltAggregator.retrieveData(dataID_2), testData_2));
                CHECK(equal(rebuiltAggregator.retrieveData(dataID_3), testData_3));
                CHECK(equal(rebuiltAggregator.retrieveData(dataID_4), testData_4));
                
                CHECK_NOTHROW(rebuiltAggregator.discardData(dataID_1, false));
                CHECK_NOTHROW(rebuiltAggregator.discardData(dataID_2, true));
                CHECK_NOTHROW(rebuiltAggregator.discardData(dataID_3, false));
                
                CHECK(rebuiltAggregator.getStoredEntitiesNumber() == 2);
            }
        }
        
        AND_WHEN("the pool is cleared")
        {
            CHECK_NOTHROW(rebuiltAggregator.clearPool());
            
            THEN("it becomes empty")
            {
                CHECK(rebuiltAggregator.getStoredEntitiesNumber() == 0);
            }
        }
    }
}
