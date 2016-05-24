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

#ifndef TESTINSTRUCTIONTARGET_H
#define	TESTINSTRUCTIONTARGET_H

#include "../../main/InstructionManagement/Interfaces/InstructionTarget.h"

namespace Testing
{
    class TestInstructionTarget : public InstructionManagement_Interfaces::InstructionTarget<InstructionManagement_Types::TestInstructionType>
    {
        public:
            virtual ~TestInstructionTarget() {}

            InstructionManagement_Types::InstructionSetType getType() const override
            {
                return InstructionManagement_Types::InstructionSetType::TEST;
            }

            bool registerInstructionSet(InstructionSetPtr<InstructionManagement_Types::TestInstructionType> set) const override
            {
                if(set)
                {
                    set->setMinimumAccessLevel(UserAccessLevel::ADMIN);

                    try
                    {
                        set->bindInstructionHandler(
                            InstructionManagement_Types::TestInstructionType::DO_TEST_1,
                            testOneHandlerBind);

                        set->bindInstructionHandler(
                            InstructionManagement_Types::TestInstructionType::DO_TEST_2,
                            testTwoHandlerBind);

                        set->bindInstructionHandler(
                            InstructionManagement_Types::TestInstructionType::DO_TEST_3,
                            testThreeHandlerBind);
                    }
                    catch(const std::invalid_argument & ex)
                    {
                        FAIL("(registerInstructionSet) > Exception encountered: <" + std::string(ex.what()) + ">");
                        return false;
                    }

                    return true;
                }
                else
                {
                    FAIL("(registerInstructionSet) > The supplied set is not initialised.");
                    return false;
                }
            }

            unsigned int instructionCounter1 = 0;
            unsigned int instructionCounter2 = 0;
            unsigned int instructionCounter3 = 0;

        private:
            boost::mutex testMutex;

            void testOneHandler(InstructionPtr<InstructionManagement_Types::TestInstructionType> instruction)
            {
                INFO("TEST_ONE_HANDLER");
                boost::lock_guard<boost::mutex> testLock(testMutex);
                ++instructionCounter1;
            }

            void testTwoHandler(InstructionPtr<InstructionManagement_Types::TestInstructionType> instruction)
            {
                INFO("TEST_TWO_HANDLER");
                boost::lock_guard<boost::mutex> testLock(testMutex);
                ++instructionCounter2;
            }

            void testThreeHandler(InstructionPtr<InstructionManagement_Types::TestInstructionType> instruction)
            {
                INFO("TEST_THREE_HANDLER");
                boost::lock_guard<boost::mutex> testLock(testMutex);
                ++instructionCounter3;
            }

            std::function<void(InstructionPtr<InstructionManagement_Types::TestInstructionType>)>
            testOneHandlerBind = boost::bind(&TestInstructionTarget::testOneHandler, this, _1);

            std::function<void(InstructionPtr<InstructionManagement_Types::TestInstructionType>)>
            testTwoHandlerBind = boost::bind(&TestInstructionTarget::testTwoHandler, this, _1);

            std::function<void(InstructionPtr<InstructionManagement_Types::TestInstructionType>)>
            testThreeHandlerBind = boost::bind(&TestInstructionTarget::testThreeHandler, this, _1);
    };
}

#endif	/* TESTINSTRUCTIONTARGET_H */
