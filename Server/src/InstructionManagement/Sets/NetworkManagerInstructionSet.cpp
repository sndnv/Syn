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

#include "NetworkManagerInstructionSet.h"

namespace InstructionManagement_Sets
{
    template <>
    void InstructionSet<NetworkManagerConnectionLifeCycleInstructionType>::buildTable()
    {
        instructionHandlers.insert({NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION, &InstructionManagement_Sets::InstructionSet<NetworkManagerConnectionLifeCycleInstructionType>::instructionNotSet});
        instructionHandlers.insert({NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION, &InstructionManagement_Sets::InstructionSet<NetworkManagerConnectionLifeCycleInstructionType>::instructionNotSet});
    }
}