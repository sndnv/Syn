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

#ifndef TESTINSTRUCTIONSOURCE_H
#define	TESTINSTRUCTIONSOURCE_H

#include "TestInstructionSet.h"
#include "../../main/InstructionManagement/Types/Types.h"
#include "../../main/InstructionManagement/Sets/InstructionSet.h"
#include "../../main/InstructionManagement/Interfaces/InstructionSource.h"
#include "../../main/SecurityManagement/Types/SecurityTokens.h"

using InstructionManagement_Sets::InstructionPtr;
using InstructionManagement_Sets::InstructionBasePtr; 

namespace Testing
{
    class TestInstructionSource : public InstructionManagement_Interfaces::InstructionSource
    {
        public:
            explicit TestInstructionSource(bool validRegistration)
            : allowRegistration(validRegistration)
            {
                requiredSets.push_back(InstructionManagement_Types::InstructionSetType::TEST);
            }
            
            TestInstructionSource(bool validRegistration, std::vector<InstructionManagement_Types::InstructionSetType> sets)
            : allowRegistration(validRegistration)
            {
                requiredSets.swap(sets);
            }

            void doTestInstructionOne()
            {
                boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestOne> testInstruction(
                    new Testing::InstructionManagement_Sets::TestInstructions::DoTestOne());

                hndlr(testInstruction, nullptr);
            }

            void doTestInstructionTwo(std::string input)
            {
                boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo> testInstruction(
                    new Testing::InstructionManagement_Sets::TestInstructions::DoTestTwo(input));

                hndlr(testInstruction, nullptr);
            }

            void doTestInstructionThree(unsigned int input)
            {
                boost::shared_ptr<Testing::InstructionManagement_Sets::TestInstructions::DoTestThree> testInstruction(
                    new Testing::InstructionManagement_Sets::TestInstructions::DoTestThree(input));

                hndlr(testInstruction, nullptr);
            }
            
            void runInstruction(
                InstructionPtr<InstructionManagement_Types::NetworkManagerConnectionLifeCycleInstructionType> instruction,
                SecurityManagement_Types::AuthorizationTokenPromisePtr & authorization)
            {
                AuthorizationTokenPtr token = authorization->get_future().get();
                hndlr(instruction, token);
            }

            bool registerInstructionHandler(
                    const std::function<void(InstructionBasePtr,
                    SecurityManagement_Types::AuthorizationTokenPtr)> handler) override
            {
                hndlr = handler;
                return allowRegistration;
            }

            std::vector<InstructionManagement_Types::InstructionSetType> getRequiredInstructionSetTypes() override
            {
                return requiredSets;
            }

        private:
            std::function<void(InstructionBasePtr, SecurityManagement_Types::AuthorizationTokenPtr)> hndlr;
            bool allowRegistration;
            std::vector<InstructionManagement_Types::InstructionSetType> requiredSets;
    };
}

#endif	/* TESTINSTRUCTIONSOURCE_H */
