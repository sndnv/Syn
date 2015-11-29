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

#ifndef COMMANDHANDLER_H
#define	COMMANDHANDLER_H

#include <string>

#include <boost/unordered_map.hpp>

#include "../Utilities/Strings/Common.h"
#include "../Utilities/Strings/Instructions.h"
#include "../Utilities/Strings/Security.h"
#include "../Common/Types.h"
#include "../InstructionManagement/Types/Types.h"
#include "../InstructionManagement/Sets/InstructionSet.h"
#include "../InstructionManagement/Sets/NetworkManagerInstructionSet.h"

//Protocols
#include "../../../external/protobuf/BaseComm.pb.h"
#include "../../../external/protobuf/Commands.pb.h"

using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;

using InstructionManagement_Sets::InstructionBasePtr;
using InstructionManagement_Sets::InstructionResultBasePtr;
using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionPtr;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::NetworkManagerConnectionLifeCycleInstructionType;
using InstructionManagement_Sets::NetworkManagerInstructions::LifeCycleOpenDataConnection;

using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::KeyData;
using SecurityManagement_Types::IVData;

using NetworkManagement_Types::CommandID;
using NetworkManagement_Types::INVALID_CONNECTION_MANAGER_ID;

using NetworkManagement_Protocols::Command;
using NetworkManagement_Protocols::Response;
using NetworkManagement_Protocols::Response_Status;
using NetworkManagement_Protocols::LifeCycleCommand_OpenDataConnection;

namespace Convert = Utilities::Strings;
namespace Results = InstructionManagement_Sets::NetworkManagerInstructions::Results;

namespace NetworkManagement_Handlers
{
    /**
     * Class for managing "network command <-> instruction"
     * and "network command response <-> instruction result" conversions.
     */
    class CommandConverter
    {
        public:
            /** Structure for holding command data. */
            struct CommandData
            {
                /** Instruction pointer (if set). */
                InstructionBasePtr instruction;
                /** Serialized data (if set). */
                PlaintextData serializedData;
                /** Denotes whether a response is expected from the remote peer. */
                bool sendResponse;
                /** ID associated with the command. */
                CommandID commandID;
            };
            
            /**
             * Creates a new command converter and initiates all available parsers/serializers.
             */
            CommandConverter();
            
            /**
             * Clears all parsers/serializers.
             */
            ~CommandConverter();

            CommandConverter(const CommandConverter&) = delete;
            CommandConverter& operator=(const CommandConverter&) = delete;
            
            /**
             * Attempts to serialize the supplied instruction for network transmission.
             * 
             * @param instruction the data to be serialized
             * @param deviceID the ID of the device associated with the instruction/command
             * @param commandID the ID of the command
             * @throw runtime_error if no serializer could be found for the supplied instruction type
             * @return the serialized data
             */
            const CommandData serializeCommand(
                const InstructionBasePtr instruction,
                const DeviceID deviceID,
                const CommandID commandID);
            
            /**
             * Attempts to parse the supplied command data.
             * 
             * @param rawCommand the raw command data
             * @param deviceID the ID of the device associated with the instruction/command
             * @param setResponseSerializationHandlerFunction function for setting a response serialization handler
             * @throw runtime_error if the supplied data is invalid
             *                      or if an appropriate parser could not be found
             * @return the parsed data
             */
            const CommandData parseCommand(
                const PlaintextData & rawCommand,
                const DeviceID deviceID,
                std::function<void (std::function<const PlaintextData (void)>)> setResponseSerializationHandlerFunction);
            
            /**
             * Attempts to serialize the supplied result data for network transmission.
             * 
             * @param instructionResult the instruction result to be serialized
             * @param deviceID the ID of the device associated with the instruction/command
             * @param commandID the ID of the command associated with the result
             * @throw runtime_error if no serializer could be found for the supplied instruction type
             * @return the serialized data
             */
            const PlaintextData serializeResponse(
                const InstructionResultBasePtr instructionResult,
                const DeviceID deviceID,
                const CommandID commandID);
            
            /**
             * Attempts to parse the supplied response data.
             * 
             * Note: Retrieves the instruction associated with the response and
             * directly sets the promise.
             * 
             * @param rawResponse the raw response data
             * @param deviceID the ID of the device associated with the instruction/command
             * @param instructionRetrievalFunction function for retrieving pending instructions
             * @throw runtime_error if the supplied data is invalid
             *                      or if an appropriate parser could not be found
             */
            void parseResponse(
                const PlaintextData & rawResponse,
                const DeviceID deviceID,
                std::function<InstructionBasePtr (const CommandID)> instructionRetrievalFunction);
            
        private:
            //<editor-fold defaultstate="collapsed" desc="Types">
            typedef std::function<
                const CommandData //return type
                (//parameters
                    const InstructionBasePtr,
                    const DeviceID,
                    const CommandID
                )
            >
            CommandSerializationFunction;
            
            typedef std::function<
                const CommandData //return type
                (//parameters
                    const Command &,
                    const DeviceID,
                    std::function<void (std::function<const PlaintextData (void)>)>
                )
            >
            CommandParsingFunction;
            
            typedef std::pair<std::string, CommandSerializationFunction> CommandSerializerPairType;
            typedef std::pair<std::string, CommandParsingFunction> CommandParserPairType;
            
            typedef std::function<
                const PlaintextData //return type
                (//parameters
                    const InstructionResultBasePtr,
                    const DeviceID,
                    const CommandID
                )
            >
            ResponseSerializationFunction;
            
            typedef std::function<
                void //return type
                (//parameters
                    const Response &,
                    const DeviceID,
                    std::function<InstructionBasePtr (const CommandID)>
                )
            >
            ResponseParsingFunction;
            
            typedef std::pair<std::string, ResponseSerializationFunction> ResponseSerializerPairType;
            typedef std::pair<std::string, ResponseParsingFunction> ResponseParserPairType;
            //</editor-fold>
            
            //command parsers/serializers
            boost::unordered_map<std::string, CommandSerializationFunction> commandSerializers;
            boost::unordered_map<std::string, CommandParsingFunction> commandParsers;
            
            //response parsers/serializers
            boost::unordered_map<std::string, ResponseSerializationFunction> responseSerializers;
            boost::unordered_map<std::string, ResponseParsingFunction> responseParsers;
            
            const CommandData serializeCommand_LifeCycleOpenDataConnection(
                const InstructionBasePtr instruction,
                const DeviceID deviceID,
                const CommandID commandID);
            
            const CommandData parseCommand_LifeCycleOpenDataConnection(
                const Command & commandObject,
                const DeviceID deviceID,
                std::function<void (std::function<const PlaintextData (void)>)> setResponseSerializationHandlerFunction);
            
            const PlaintextData serializeResponse_LifeCycleOpenDataConnection(
                const InstructionResultBasePtr instructionResult,
                const DeviceID deviceID,
                const CommandID commandID);
            
            void parseResponse_LifeCycleOpenDataConnection(
                const Response & responseObject,
                const DeviceID deviceID,
                std::function<InstructionBasePtr (const CommandID)> instructionRetrievalFunction);
    };
}

#endif	/* COMMANDHANDLER_H */

