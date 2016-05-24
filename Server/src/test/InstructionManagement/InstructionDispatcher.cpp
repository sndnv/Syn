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

#include <vector>
#include <string>
#include <boost/thread/mutex.hpp>

#include "../../main/InstructionManagement/Interfaces/InstructionSource.h"
#include "../../main/InstructionManagement/Interfaces/InstructionTarget.h"
#include "../../main/InstructionManagement/InstructionDispatcher.h"
#include "TestInstructionSet.h"
#include "TestInstructionSource.h"
#include "TestInstructionTarget.h"

SCENARIO("An instruction dispatcher is created and registers sources & targets",
         "[InstructionDispatcher][InstructionManagement][Managers][Core]")
{
    GIVEN("An InstructionDispatcher without sources or targets")
    {
        std::vector<InstructionSetType> expectedSets;
        expectedSets.push_back(InstructionSetType::TEST);
        SyncServer_Core::InstructionDispatcher testDispatcher(
            SyncServer_Core::InstructionDispatcher::InstructionDispatcherParameters{expectedSets});
        
        WHEN("no instruction sources or targets are registered")
        {
            THEN("no instruction sources/targets are available")
            {
                CHECK(testDispatcher.getMinimumAccessLevelForSet(InstructionSetType::TEST) == Common_Types::UserAccessLevel::INVALID);
                CHECK(testDispatcher.getSourcesCount() == 0);
                CHECK(testDispatcher.getTargetSetsCount() == 0);
            }
        }
        
        WHEN("instruction sources and targets are added")
        {
            Testing::TestInstructionSource testSource(true);
            Testing::TestInstructionTarget testTarget;
            testDispatcher.registerInstructionSource(testSource);
            testDispatcher.registerInstructionTarget<InstructionManagement_Types::TestInstructionType>(testTarget);

            THEN("they become available")
            {
                CHECK(testDispatcher.getMinimumAccessLevelForSet(InstructionSetType::TEST) == Common_Types::UserAccessLevel::ADMIN);
                CHECK(testDispatcher.getSourcesCount() == 1);
                CHECK(testDispatcher.getTargetSetsCount() == 1);
            }
        }
    }
}

SCENARIO("An instruction dispatcher processes valid instructions",
         "[InstructionDispatcher][InstructionManagement][Managers][Core]")
{
    GIVEN("An InstructionDispatcher with a test source and target")
    {
        std::vector<InstructionSetType> expectedSets;
        expectedSets.push_back(InstructionSetType::TEST);
        SyncServer_Core::InstructionDispatcher testDispatcher(
            SyncServer_Core::InstructionDispatcher::InstructionDispatcherParameters{expectedSets});
        
        Testing::TestInstructionSource testSource(true);
        Testing::TestInstructionTarget testTarget;
        testDispatcher.registerInstructionSource(testSource);
        testDispatcher.registerInstructionTarget<InstructionManagement_Types::TestInstructionType>(testTarget);
        
        WHEN("several valid instructions are sent by the source")
        {
            testSource.doTestInstructionOne();
            testSource.doTestInstructionOne();
            testSource.doTestInstructionTwo("test1");
            testSource.doTestInstructionTwo("test2");
            testSource.doTestInstructionTwo("test3");
            testSource.doTestInstructionThree(1);
            testSource.doTestInstructionThree(1);
            testSource.doTestInstructionThree(1);
            testSource.doTestInstructionThree(100);
            testSource.doTestInstructionThree(999999);
            
            THEN("they are received by the target")
            {
                CHECK(testTarget.instructionCounter1 == 2);
                CHECK(testTarget.instructionCounter2 == 3);
                CHECK(testTarget.instructionCounter3 == 5);
            }
        }
    }
}

SCENARIO("An instruction dispatcher fails to process invalid instructions",
         "[InstructionDispatcher][InstructionManagement][Managers][Core]") 
{
    GIVEN("An InstructionDispatcher with a test source and target")
    {
        std::vector<InstructionSetType> expectedSets;
        expectedSets.push_back(InstructionSetType::TEST);
        SyncServer_Core::InstructionDispatcher testDispatcher(
            SyncServer_Core::InstructionDispatcher::InstructionDispatcherParameters{expectedSets});
        
        Testing::TestInstructionSource registeredTestSource(true);
        Testing::TestInstructionSource unregisteredTestSource(false);
        Testing::TestInstructionTarget testTarget;
        testDispatcher.registerInstructionSource(registeredTestSource);
        testDispatcher.registerInstructionSource(unregisteredTestSource);
        testDispatcher.registerInstructionTarget<InstructionManagement_Types::TestInstructionType>(testTarget);
        
        WHEN("several invalid instructions are sent by a registered source")
        {
            registeredTestSource.doTestInstructionTwo("");
            registeredTestSource.doTestInstructionTwo("");
            registeredTestSource.doTestInstructionThree(0);
            registeredTestSource.doTestInstructionThree(0);
            registeredTestSource.doTestInstructionThree(0);
            
            THEN("they are never received by the target")
            {
                CHECK(testTarget.instructionCounter1 == 0);
                CHECK(testTarget.instructionCounter2 == 0);
                CHECK(testTarget.instructionCounter3 == 0);
            }
        }
        
        WHEN("several valid and invalid instructions are sent by a unregistered source")
        {
            unregisteredTestSource.doTestInstructionOne();
            unregisteredTestSource.doTestInstructionTwo("test1");
            unregisteredTestSource.doTestInstructionThree(1);
            unregisteredTestSource.doTestInstructionTwo("");
            unregisteredTestSource.doTestInstructionTwo("");
            unregisteredTestSource.doTestInstructionThree(0);
            unregisteredTestSource.doTestInstructionThree(0);
            unregisteredTestSource.doTestInstructionThree(0);
            
            THEN("they are never received by the target")
            {
                CHECK(testTarget.instructionCounter1 == 0);
                CHECK(testTarget.instructionCounter2 == 0);
                CHECK(testTarget.instructionCounter3 == 0);
            }
        }
    }
}