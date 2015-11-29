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

#include "CommandConverter.h"

NetworkManagement_Handlers::CommandConverter::CommandConverter()
{
    commandSerializers.insert(
            CommandSerializerPairType(
                Convert::toString(NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION),
                boost::bind(&CommandConverter::serializeCommand_LifeCycleOpenDataConnection, this, _1, _2, _3)));

    commandParsers.insert(
            CommandParserPairType(
                Convert::toString(NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION),
                boost::bind(&CommandConverter::parseCommand_LifeCycleOpenDataConnection, this, _1, _2, _3)));

    responseSerializers.insert(
            ResponseSerializerPairType(
                Convert::toString(NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION),
                boost::bind(&CommandConverter::serializeResponse_LifeCycleOpenDataConnection, this, _1, _2, _3)));

    responseParsers.insert(
            ResponseParserPairType(
                Convert::toString(NetworkManagerConnectionLifeCycleInstructionType::OPEN_DATA_CONNECTION),
                boost::bind(&CommandConverter::parseResponse_LifeCycleOpenDataConnection, this, _1, _2, _3)));
}

NetworkManagement_Handlers::CommandConverter::~CommandConverter()
{
    commandSerializers.clear();
    commandParsers.clear();
    responseSerializers.clear();
    responseParsers.clear();
}

const NetworkManagement_Handlers::CommandConverter::CommandData
NetworkManagement_Handlers::CommandConverter::serializeCommand(
    const InstructionBasePtr instruction, const DeviceID deviceID, const CommandID commandID)
{
    auto commandSerializer = commandSerializers.find(instruction->getInstructionTypeName());
    if(commandSerializer != commandSerializers.end())
    {
        return commandSerializer->second(instruction, deviceID, commandID);
    }
    else
    {
        throw std::runtime_error("CommandHandler::serializeCommand() > No serializer found for instruction of type ["
                + instruction->getInstructionTypeName() + "].");
    }
}

const NetworkManagement_Handlers::CommandConverter::CommandData
NetworkManagement_Handlers::CommandConverter::parseCommand(
    const PlaintextData & rawCommand,
    const DeviceID deviceID,
    std::function<void (std::function<const PlaintextData (void)>)> setResponseSerializationHandlerFunction)
{
    Command commandObject;
    commandObject.ParseFromString(rawCommand);

    if(!commandObject.IsInitialized())
    {
        throw std::runtime_error("CommandHandler::parseCommand() > Failed to parse command object.");
    }

    auto commandParser = commandParsers.find(commandObject.command());
    if(commandParser != commandParsers.end())
    {
        return commandParser->second(commandObject, deviceID, setResponseSerializationHandlerFunction);
    }
    else
    {
        throw std::runtime_error("CommandHandler::parseCommand() > Parsing not supported"
                " for instruction of type [" + commandObject.command() + "].");
    }
}

const PlaintextData NetworkManagement_Handlers::CommandConverter::serializeResponse(
    const InstructionResultBasePtr instructionResult, const DeviceID deviceID, const CommandID commandID)
{
    auto responseSerializer = responseSerializers.find(instructionResult->getInstructionResultTypeName());
    if(responseSerializer != responseSerializers.end())
    {
        return responseSerializer->second(instructionResult, deviceID, commandID);
    }
    else
    {
        throw std::runtime_error("CommandHandler::serializeResponse() > No serializer"
                " found for instruction of type ["
                + instructionResult->getInstructionResultTypeName() + "].");
    }
}

void NetworkManagement_Handlers::CommandConverter::parseResponse(
    const PlaintextData & rawResponse,
    const DeviceID deviceID,
    std::function<InstructionBasePtr (const CommandID)> instructionRetrievalFunction)
{
    Response responseObject;
    responseObject.ParseFromString(rawResponse);

    if(!responseObject.IsInitialized())
    {
        throw std::runtime_error("CommandHandler::parseResponse() > Failed to parse response object.");
    }

    auto responseParser = responseParsers.find(responseObject.command());
    if(responseParser != responseParsers.end())
    {
        responseParser->second(responseObject, deviceID, instructionRetrievalFunction);
    }
    else
    {
        throw std::runtime_error("CommandHandler::parseResponse() > Parsing not"
                " supported for instruction result of type [" + responseObject.command() + "].");
    }
}

const NetworkManagement_Handlers::CommandConverter::CommandData
NetworkManagement_Handlers::CommandConverter::serializeCommand_LifeCycleOpenDataConnection(
    const InstructionBasePtr instruction, const DeviceID deviceID, const CommandID commandID)
{
    boost::shared_ptr<LifeCycleOpenDataConnection> actualInstruction =
                    boost::dynamic_pointer_cast<LifeCycleOpenDataConnection>(instruction);

    if(actualInstruction)
    {
        LifeCycleCommand_OpenDataConnection commandDataObject;
        commandDataObject.set_transient_id(actualInstruction->transientID);
        commandDataObject.set_content_encryption_key_data(actualInstruction->key.BytePtr(), actualInstruction->key.SizeInBytes());
        commandDataObject.set_content_encryption_key_iv(actualInstruction->iv.BytePtr(), actualInstruction->iv.SizeInBytes());
        commandDataObject.set_sym_cipher(Convert::toString(actualInstruction->cipherType));
        commandDataObject.set_sym_mode(Convert::toString(actualInstruction->cipherMode));
        commandDataObject.set_encrypt(actualInstruction->encrypt);
        commandDataObject.set_compress(actualInstruction->compress);

        PlaintextData plaintextCommandData;
        if(!commandDataObject.SerializeToString(&plaintextCommandData))
        {
            throw std::runtime_error("CommandHandler::serializeCommand_OpenDataConnection() >"
                    " Failed to serialize command data object.");
        }

        Command commandObject;
        commandObject.set_command(actualInstruction->getInstructionTypeName());
        commandObject.set_command_id(commandID);
        commandObject.set_data(plaintextCommandData);
        commandObject.set_send_response(true);

        PlaintextData plaintextCommand;
        if(!commandObject.SerializeToString(&plaintextCommand))
        {
            throw std::runtime_error("CommandHandler::serializeCommand_OpenDataConnection() >"
                    " Failed to serialize command object.");
        }

        return CommandData{nullptr, plaintextCommand, commandObject.send_response(), commandID};
    }
    else
    {
        throw std::logic_error("CommandHandler::serializeCommand_OpenDataConnection() >"
                " Supplied instruction is not of the expected type.");
    }
}

const NetworkManagement_Handlers::CommandConverter::CommandData
NetworkManagement_Handlers::CommandConverter::parseCommand_LifeCycleOpenDataConnection(
    const Command & commandObject,
    const DeviceID deviceID,
    std::function<void (std::function<const PlaintextData (void)>)> setResponseSerializationHandlerFunction)
{
    LifeCycleCommand_OpenDataConnection commandDataObject;
    commandDataObject.ParseFromString(commandObject.data());

    if(!commandDataObject.IsInitialized())
    {
        throw std::runtime_error("CommandHandler::parseCommand_OpenDataConnection() >"
                " Failed to parse command data object.");
    }

    //builds the content encryption key (CEK)
    KeyData cek(reinterpret_cast<const unsigned char*>(
        commandDataObject.content_encryption_key_data().data()),
        commandDataObject.content_encryption_key_data().size());
    
    IVData cekIV(reinterpret_cast<const unsigned char*>(
        commandDataObject.content_encryption_key_iv().data()),
        commandDataObject.content_encryption_key_iv().size());

    boost::shared_ptr<LifeCycleOpenDataConnection> instruction(
        new LifeCycleOpenDataConnection(
            INVALID_CONNECTION_MANAGER_ID,
            commandDataObject.transient_id(),
            deviceID,
            cek,
            cekIV,
            Convert::toSymmetricCipherType(commandDataObject.sym_cipher()),
            Convert::toAuthenticatedSymmetricCipherModeType(commandDataObject.sym_mode()),
            commandDataObject.encrypt(),
            commandDataObject.compress()));
    
    if(commandObject.send_response())
    {
        auto responseSerializationHandler = [this, instruction, deviceID, commandID = commandObject.command_id()] () -> const PlaintextData
        {
            auto result = instruction->getFuture().get();
            return serializeResponse_LifeCycleOpenDataConnection(result, deviceID, commandID);
        };

        setResponseSerializationHandlerFunction(responseSerializationHandler);
    }

    return CommandData{instruction, "", commandObject.send_response(), commandObject.command_id()};
}

const PlaintextData
NetworkManagement_Handlers::CommandConverter::serializeResponse_LifeCycleOpenDataConnection(
    const InstructionResultBasePtr instructionResult, const DeviceID deviceID, const CommandID commandID)
{
    boost::shared_ptr<Results::LifeCycleOpenDataConnection> actualResult =
                    boost::dynamic_pointer_cast<Results::LifeCycleOpenDataConnection>(instructionResult);

    if(actualResult)
    {
        Response responseObject;
        responseObject.set_command(actualResult->getInstructionResultTypeName());
        responseObject.set_command_id(commandID);
        responseObject.set_status(actualResult->result
                                  ? Response_Status::Response_Status_OK
                                  : Response_Status::Response_Status_FAILED);

        PlaintextData plaintextResponse;
        if(!responseObject.SerializeToString(&plaintextResponse))
        {
            throw std::runtime_error("CommandHandler::serializeResponse_OpenDataConnection() >"
                    " Failed to serialize response object.");
        }

        return plaintextResponse;
    }
    else
    {
        throw std::logic_error("CommandHandler::serializeResponse_OpenDataConnection() >"
                " Supplied instruction result is not of the expected type.");
    }
}

void NetworkManagement_Handlers::CommandConverter::parseResponse_LifeCycleOpenDataConnection(
    const Response & responseObject,
    const DeviceID deviceID,
    std::function<InstructionBasePtr (const CommandID)> instructionRetrievalFunction)
{
    auto result = boost::shared_ptr<Results::LifeCycleOpenDataConnection>(
        new Results::LifeCycleOpenDataConnection(responseObject.status() == Response_Status::Response_Status_OK));
    
    auto instruction = instructionRetrievalFunction(responseObject.command_id());
    
    boost::shared_ptr<LifeCycleOpenDataConnection> actualInstruction =
                    boost::dynamic_pointer_cast<LifeCycleOpenDataConnection>(instruction);

    if(actualInstruction)
    {
        actualInstruction->getPromise().set_value(result);
    }
    else
    {
        throw std::logic_error("CommandHandler::parseResponse_LifeCycleOpenDataConnection() >"
                " Supplied instruction is not of the expected type.");
    }
}
