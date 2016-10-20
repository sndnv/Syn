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

#include "../BasicSpec.h"
#include "../../main/Utilities/ThreadPool.h"
#include <atomic>

SCENARIO("Thread pools are created, managed and can process tasks", "[ThreadPool][Utilities]")
{
    GIVEN("ThreadPools with varying number of threads")
    {
        Utilities::ThreadPool smallPool(2), mediumPool(6), largePool(16);
        
        CHECK(smallPool.getPoolSize() == 2);
        CHECK(mediumPool.getPoolSize() == 6);
        CHECK(largePool.getPoolSize() == 16);
        
        WHEN("their thread count is increased")
        {
            smallPool.addThreads(1);
            mediumPool.addThreads(2);
            largePool.addThreads(4);
            
            THEN("their thread count goes up")
            {
                CHECK(smallPool.getPoolSize() == 3);
                CHECK(mediumPool.getPoolSize() == 8);
                CHECK(largePool.getPoolSize() == 20);
            }
        }
        
        WHEN("their thread count is decreased")
        {
            smallPool.removeThreads(1);
            mediumPool.removeThreads(2);
            largePool.removeThreads(4);
            
            waitFor(1);
            
            THEN("their thread count goes down")
            {
                CHECK(smallPool.getPoolSize() == 1);
                CHECK(mediumPool.getPoolSize() == 4);
                CHECK(largePool.getPoolSize() == 12);
            }
        }
        
        WHEN("all threads are stopped")
        {
            smallPool.stopAllThreads();
            mediumPool.stopAllThreads();
            largePool.stopAllThreads();
            
            waitFor(1);
            
            THEN("there are no threads left")
            {
                CHECK(smallPool.getPoolSize() == 0);
                CHECK(mediumPool.getPoolSize() == 0);
                CHECK(largePool.getPoolSize() == 0);
            }
        }
        
        WHEN("they are stopped")
        {
            smallPool.stopThreadPool();
            mediumPool.stopThreadPool();
            largePool.stopThreadPool();
            
            waitFor(1);
            
            THEN("no further management can be performed")
            {
                smallPool.addThreads(2);
                mediumPool.removeThreads(2);
                largePool.stopAllThreads();
                        
                CHECK(smallPool.getPoolSize() == 2);
                CHECK(mediumPool.getPoolSize() == 6);
                CHECK(largePool.getPoolSize() == 16);
            }
        }
    }
    
    GIVEN("A ThreadPool with 4 threads")
    {
        Utilities::FileLoggerParameters loggerParams{"test_data/ThreadPool.log", 32*1024*1024, Utilities::FileLogSeverity::Debug};
        Utilities::FileLoggerPtr logger(new Utilities::FileLogger(loggerParams));
        
        Utilities::ThreadPool testPool(4, logger);

        CHECK(testPool.getPoolSize() == 4);

        WHEN("new tasks are assigned")
        {
            std::atomic<unsigned int> taskCounter(0);
            std::atomic<unsigned int> tasksToStart(8);

            auto testTask = [&]()
            {
                ++taskCounter;
            };

            for(unsigned int i = 0; i < tasksToStart; i++)
                testPool.assignTask(testTask);

            waitFor(1);

            THEN("the tasks are executed and their results stored")
            {
                CHECK(taskCounter == tasksToStart);
            }
        }

        WHEN("new timed tasks are assigned")
        {
            std::atomic<unsigned int> taskCounter(0);
            std::atomic<unsigned int> tasksToStart(8);
            std::atomic<unsigned long> secondsToWait(5);

            auto testTask = [&]()
            {
                ++taskCounter;
            };

            for(unsigned int i = 0; i < tasksToStart; i++)
                testPool.assignTimedTask(testTask, secondsToWait);

            waitFor(secondsToWait - 1);

            THEN("the tasks are executed after the specified time")
            {
                CHECK(taskCounter == 0);
                waitFor(2);
                CHECK(taskCounter == tasksToStart);
            }
        }

        WHEN("exceptions are encountered during task processing")
        {
            std::atomic<unsigned int> taskCounter(0);
            std::atomic<unsigned int> tasksToStart(8);

            auto testTask = [&]()
            {
                ++taskCounter;
                throw std::runtime_error("Test Exception");
            };

            for(unsigned int i = 0; i < tasksToStart; i++)
                testPool.assignTask(testTask);

            waitFor(1);

            THEN("the tasks are executed and the pool does not stop its threads")
            {
                CHECK(taskCounter == tasksToStart);
                CHECK(testPool.getPoolSize() == 4);
            }
        }
    }
}
