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

#ifndef NETWORKMANAGERINSTRUCTIONSET_H
#define	NETWORKMANAGERINSTRUCTIONSET_H

#include <vector>
#include <string>

#include "../../Utilities/Strings/Instructions.h"
#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"
#include "../../NetworkManagement/Types/Types.h"
#include "../../SecurityManagement/Types/Types.h"
#include "../../SecurityManagement/Crypto/Handlers.h"

using InstructionManagement_Types::NetworkManagerAdminInstructionType;
using InstructionManagement_Types::NetworkManagerStateInstructionType;
using InstructionManagement_Types::NetworkManagerConnectionLifeCycleInstructionType;
using InstructionManagement_Types::NetworkManagerConnectionBridgingInstructionType;

using Common_Types::IPAddress;
using Common_Types::INVALID_IP_ADDRESS;
using Common_Types::IPPort;
using Common_Types::INVALID_IP_PORT;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;
using NetworkManagement_Types::ConnectionManagerID;
using NetworkManagement_Types::INVALID_CONNECTION_MANAGER_ID;
using NetworkManagement_Types::PeerType;
using NetworkManagement_Types::TransientConnectionID;
using NetworkManagement_Types::INVALID_TRANSIENT_CONNECTION_ID;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;

namespace InstructionManagement_Sets
{
    namespace NetworkManagerInstructions
    {
        struct LifeCycleOpenInitConnection : public Instruction<NetworkManagerConnectionLifeCycleInstructionType>
        {
            LifeCycleOpenInitConnection(
                const ConnectionManagerID managerID, const IPAddress initAddress, const IPPort initPort,
                const std::string sharedPassword, const PeerType remotePeerType, const DeviceID remotePeerID,
                const TransientConnectionID transientID)
            : Instruction(InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE, NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION),
              managerID(managerID), initAddress(initAddress), initPort(initPort), sharedPassword(sharedPassword),
              remotePeerType(remotePeerType), remotePeerID(remotePeerID), transientID(transientID)
            {}
            
            LifeCycleOpenInitConnection(
                const std::string sharedPassword, const PeerType remotePeerType, const DeviceID remotePeerID,
                const TransientConnectionID transientID)
            : Instruction(InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE, NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION),
              managerID(INVALID_CONNECTION_MANAGER_ID), initAddress(INVALID_IP_ADDRESS), initPort(INVALID_IP_PORT), sharedPassword(sharedPassword),
              remotePeerType(remotePeerType), remotePeerID(remotePeerID), transientID(transientID)
            {}
            
            bool isValid() override
            {
                return (!sharedPassword.empty()
                        && remotePeerType != PeerType::INVALID
                        && remotePeerID != INVALID_DEVICE_ID
                        && transientID != INVALID_TRANSIENT_CONNECTION_ID);
            }
            
            const ConnectionManagerID managerID;
            const IPAddress initAddress;
            const IPPort initPort;
            const std::string sharedPassword;
            const PeerType remotePeerType;
            const DeviceID remotePeerID;
            const TransientConnectionID transientID;
        };
        
        struct LifeCycleOpenDataConnection : public Instruction<NetworkManagerConnectionLifeCycleInstructionType>
        {
            LifeCycleOpenDataConnection(
                const ConnectionManagerID manager, const TransientConnectionID transientConnection,
                const DeviceID device, const KeyData keyData, const IVData ivData, SymmetricCipherType cipher,
                AuthenticatedSymmetricCipherModeType mode, bool shouldEncrypt, bool shouldCompress)
            : Instruction(InstructionSetType::NETWORK_MANAGER_CONNECTION_LIFE_CYCLE, NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION),
              managerID(manager), transientID(transientConnection), deviceID(device), key(keyData), iv(ivData), cipherType(cipher),
              cipherMode(mode), encrypt(shouldEncrypt), compress(shouldCompress)
            {}
            
            bool isValid() override
            {
                return (transientID != INVALID_TRANSIENT_CONNECTION_ID
                        && key.SizeInBytes() > 0 && iv.SizeInBytes() > 0
                        && cipherType != SymmetricCipherType::INVALID
                        && cipherMode != AuthenticatedSymmetricCipherModeType::INVALID);
            }
            
            const ConnectionManagerID managerID;
            const TransientConnectionID transientID;
            const DeviceID deviceID;
            const KeyData key;
            const IVData iv;
            const SymmetricCipherType cipherType;
            const AuthenticatedSymmetricCipherModeType cipherMode;
            const bool encrypt;
            const bool compress;
        };
        
        namespace Results
        {
            struct LifeCycleOpenInitConnection : public InstructionResult<NetworkManagerConnectionLifeCycleInstructionType>
            {
                explicit LifeCycleOpenInitConnection(bool input)
                : InstructionResult(NetworkManagerConnectionLifeCycleInstructionType::OPEN_INIT_CONNECTION), result(input)
                {}
                bool result;
            };
            
            struct LifeCycleOpenDataConnection : public InstructionResult<NetworkManagerConnectionLifeCycleInstructionType>
            {
                explicit LifeCycleOpenDataConnection(bool input)
                : InstructionResult(NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION), result(input) {}
                bool result;
            };
        }
    }
}

#endif	/* NETWORKMANAGERINSTRUCTIONSET_H */

