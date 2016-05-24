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

#ifndef TESTINSTRUCTIONSET_H
#define	TESTINSTRUCTIONSET_H

#include <string>
#include "../../main/InstructionManagement/Sets/InstructionSet.h"
#include "../../main/InstructionManagement/Types/Types.h"

using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::TestInstructionType;
using InstructionManagement_Types::InstructionSetType;

namespace Testing
{
    namespace InstructionManagement_Sets
    {
        namespace TestInstructions
        {
            struct DoTestOne : public Instruction<TestInstructionType>
            {
                DoTestOne()
                : Instruction(InstructionSetType::TEST, TestInstructionType::DO_TEST_1)
                {}

                bool isValid() override
                {
                    return true;
                }
            };

            struct DoTestTwo : public Instruction<TestInstructionType>
            {
                explicit DoTestTwo(std::string input)
                : Instruction(InstructionSetType::TEST, TestInstructionType::DO_TEST_2), data(input)
                {}

                bool isValid() override
                {
                    return !data.empty();
                }

                std::string data;
            };

            struct DoTestThree : public Instruction<TestInstructionType>
            {
                explicit DoTestThree(unsigned int input)
                : Instruction(InstructionSetType::TEST, TestInstructionType::DO_TEST_3), data(input)
                {}

                bool isValid() override
                {
                    return (data > 0);
                }

                unsigned int data;
            };

            namespace Results
            {
                struct DoTestOne : public InstructionResult<TestInstructionType>
                {
                    explicit DoTestOne(bool input)
                    : InstructionResult(TestInstructionType::DO_TEST_1), result(input)
                    {}

                    bool result;
                };

                struct DoTestTwo : public InstructionResult<TestInstructionType>
                {
                    explicit DoTestTwo(bool input)
                    : InstructionResult(TestInstructionType::DO_TEST_2), result(input)
                    {}

                    bool result;
                };

                struct DoTestThree : public InstructionResult<TestInstructionType>
                {
                    explicit DoTestThree(bool input)
                    : InstructionResult(TestInstructionType::DO_TEST_3), result(input)
                    {}

                    bool result;
                };
            }
        }
    }
}

#endif	/* TESTINSTRUCTIONSET_H */

