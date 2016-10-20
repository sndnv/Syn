/* 
 * Copyright (C) 2014 https://github.com/sndnv
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

#include "DiskDataPool.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread/lock_guard.hpp>
#include <winsock2.h> //TODO - remove/impl htonl()
#include <iostream>

const std::string StorageManagement_Pools::DiskDataPool::FILE_SIGNATURE = "DDP";

const DataSize StorageManagement_Pools::DiskDataPool::OVERHEAD_POOL_MANAGEMENT =
    (StorageManagement_Pools::DiskDataPool::FILE_SIGNATURE.size()
        + sizeof(StorageManagement_Pools::DiskDataPool::CURRENT_VERSION)
        + StorageManagement_Pools::DiskDataPool::UUID_BYTE_LENGTH
        + StorageManagement_Pools::DiskDataPool::PoolHeader::BYTE_LENGTH
        + StorageManagement_Pools::DiskDataPool::PoolFooter::BYTE_LENGTH);

const char StorageManagement_Pools::DiskDataPool::CURRENT_VERSION = '1';

ByteVector StorageManagement_Pools::DiskDataPool::PoolHeader::toBytes()
{
    auto nFooterLocation = htonl(footer);

    if(sizeof nFooterLocation != PoolHeader::BYTE_LENGTH)
        throw std::invalid_argument("PoolHeader::toBytes() > The converted pool header does not have the expected byte length.");

    ByteVector result(PoolHeader::BYTE_LENGTH);
    Byte* rawResult = static_cast<Byte*>(static_cast<void*>(&nFooterLocation));

    for(std::size_t i = 0; i < (sizeof nFooterLocation); i++)
        result[i] = rawResult[i];

    return result;
}

StorageManagement_Pools::DiskDataPool::PoolHeader StorageManagement_Pools::DiskDataPool::PoolHeader::fromBytes(const ByteVector & data)
{
    if(PoolHeader::BYTE_LENGTH != data.size())
        throw std::invalid_argument("PoolHeader::fromBytes() > Unexpected data length encountered.");

    PoolHeader result;

    Byte nFooterLocation[data.size()];

    for(std::size_t i = 0; i < data.size(); i++)
        nFooterLocation[i] = data[i];

    result.footer = ntohl(*static_cast<DiskDataAddress*>(static_cast<void*>(nFooterLocation)));

    return result;
}

ByteVector StorageManagement_Pools::DiskDataPool::PoolFooter::toBytes()
{
    auto nEntitiesNumber = htonl(entitiesNumber);
    auto nFirstHeaderAddress = htonl(firstHeader);
    auto nLastDataID = htonl(lastDataID);

    if((sizeof nEntitiesNumber + sizeof nFirstHeaderAddress + sizeof nLastDataID) != PoolFooter::BYTE_LENGTH)
        throw std::invalid_argument("PoolFooter::toBytes() > The converted pool footer does not have the expected byte length.");

    ByteVector result(PoolFooter::BYTE_LENGTH);

    Byte* rawEntitiesNumber = static_cast<Byte*>(static_cast<void*>(&nEntitiesNumber));
    Byte* rawFirstHeaderAddress = static_cast<Byte*>(static_cast<void*>(&nFirstHeaderAddress));
    Byte* rawLastDataID = static_cast<Byte*>(static_cast<void*>(&nLastDataID));

    for(std::size_t i = 0; i < (sizeof nEntitiesNumber); i++)
        result[i] = rawEntitiesNumber[i];

    for(std::size_t i = (sizeof nEntitiesNumber); i < (sizeof nEntitiesNumber + sizeof nFirstHeaderAddress); i++)
        result[i] = rawFirstHeaderAddress[i - (sizeof nEntitiesNumber)];
    
    for(std::size_t i = (sizeof nEntitiesNumber + sizeof nFirstHeaderAddress); i < (sizeof nEntitiesNumber + sizeof nFirstHeaderAddress + sizeof nLastDataID); i++)
        result[i] = rawLastDataID[i - (sizeof nEntitiesNumber + sizeof nFirstHeaderAddress)];

    return result;
}

StorageManagement_Pools::DiskDataPool::PoolFooter StorageManagement_Pools::DiskDataPool::PoolFooter::fromBytes(const ByteVector & data)
{
    if(PoolFooter::BYTE_LENGTH != data.size())
        throw std::invalid_argument("PoolFooter::fromBytes() > Unexpected data length encountered.");

    PoolFooter result;

    Byte nEntitiesNumber[sizeof(EntitiesCountType)];
    Byte nFirstHeaderAddress[sizeof(DiskDataAddress)];
    Byte nLastDataID[sizeof(StoredDataID)];

    for(std::size_t i = 0; i < sizeof(EntitiesCountType); i++)
        nEntitiesNumber[i] = data[i];

    for(std::size_t i = sizeof(EntitiesCountType); i < (sizeof(EntitiesCountType) + sizeof(DiskDataAddress)); i++)
        nFirstHeaderAddress[i - (sizeof(EntitiesCountType))] = data[i];
    
    for(std::size_t i = (sizeof(EntitiesCountType) + sizeof(DiskDataAddress)); i < (sizeof(EntitiesCountType) + sizeof(DiskDataAddress) + sizeof(StoredDataID)); i++)
        nLastDataID[i - (sizeof(EntitiesCountType) + sizeof(DiskDataAddress))] = data[i];

    result.entitiesNumber = ntohl(*static_cast<EntitiesCountType*>(static_cast<void*>(nEntitiesNumber)));
    result.firstHeader = ntohl(*static_cast<DiskDataAddress*>(static_cast<void*>(nFirstHeaderAddress)));
    result.lastDataID = ntohl(*static_cast<StoredDataID*>(static_cast<void*>(nLastDataID)));

    return result;
}

ByteVector StorageManagement_Pools::DiskDataPool::EntityHeader::toBytes()
{
    auto nEntityID = htonl(id);
    auto nEntitySize = htonl(size);
    auto nNextHeader = htonl(nextHeader);

    if((sizeof nEntityID + sizeof nEntitySize + sizeof nNextHeader) != EntityHeader::BYTE_LENGTH)
        throw std::invalid_argument("EntityHeader::toBytes() > The converted entity header does not have the expected byte length.");

    ByteVector result(EntityHeader::BYTE_LENGTH);

    Byte* rawEntityID = static_cast<Byte*>(static_cast<void*>(&nEntityID));
    Byte* rawEntitySize = static_cast<Byte*>(static_cast<void*>(&nEntitySize));
    Byte* rawNextHeader= static_cast<Byte*>(static_cast<void*>(&nNextHeader));


    for(std::size_t i = 0; i < (sizeof nEntityID); i++)
        result[i] = rawEntityID[i];

    for(std::size_t i = (sizeof nEntityID); i < (sizeof nEntityID + sizeof nEntitySize); i++)
        result[i] = rawEntitySize[i - (sizeof nEntityID)];

    for(std::size_t i = (sizeof nEntityID + sizeof nEntitySize); i < (sizeof nEntityID + sizeof nEntitySize + sizeof nNextHeader); i++)
        result[i] = rawNextHeader[i - (sizeof nEntityID + sizeof nEntitySize)];

    return result;
}

StorageManagement_Pools::DiskDataPool::EntityHeader StorageManagement_Pools::DiskDataPool::EntityHeader::fromBytes(const ByteVector & data)
{
    if(EntityHeader::BYTE_LENGTH != data.size())
        throw std::invalid_argument("EntityHeader::fromBytes() > Unexpected data length encountered.");

    EntityHeader result;

    Byte nEntityID[sizeof(StoredDataID)];
    Byte nEntitySize[sizeof(DataSize)];
    Byte nNextHeader[sizeof(DiskDataAddress)];

    for(std::size_t i = 0; i < sizeof(StoredDataID); i++)
        nEntityID[i] = data[i];

    for(std::size_t i = sizeof(StoredDataID); i < (sizeof(StoredDataID) + sizeof(DataSize)); i++)
        nEntitySize[i - (sizeof(StoredDataID))] = data[i];

    for(std::size_t i = (sizeof(StoredDataID) + sizeof(DataSize)); i < (sizeof(StoredDataID) + sizeof(DataSize) + sizeof(DiskDataAddress)); i++)
        nNextHeader[i - (sizeof(StoredDataID) + sizeof(DataSize))] = data[i];

    result.id = ntohl(*static_cast<StoredDataID*>(static_cast<void*>(nEntityID)));
    result.size = ntohl(*static_cast<DataSize*>(static_cast<void*>(nEntitySize)));
    result.nextHeader = ntohl(*static_cast<DiskDataAddress*>(static_cast<void*>(nNextHeader)));

    return result;
}

StorageManagement_Pools::DiskDataPool::DiskDataPool(DiskDataPoolInitParameters parameters)
: poolPath(parameters.poolFilePath),
  eraseDataOnFailure(parameters.eraseDataOnFailure),
  header({parameters.poolSize - PoolFooter::BYTE_LENGTH}),
  footer({0, 0, 0})
{
    if(parameters.poolSize <= OVERHEAD_POOL_MANAGEMENT)
        throw std::invalid_argument("DiskDataPool::() > The requested pool size is too low.");

    if(!boost::filesystem::exists(parameters.poolFilePath))
    {
        fileStream.open(parameters.poolFilePath, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);

        if(fileStream.fail() || !fileStream.is_open())
            throw std::runtime_error("DiskDataPool::() > Failed to open pool file; <" + getErrnoMessage() + ">.");

        uuid = boost::uuids::random_generator()();
        
        //writes the header & footer
        flushCompleteHeader();
        flushFooter();

        if(fileStream.fail())
            throw std::runtime_error("DiskDataPool::() > Failed to initialize pool; <" + getErrnoMessage() + ">.");

        //pool initialization parameters
        state = PoolState::OPEN;
        mode = PoolMode::READ_WRITE;
        bytesRead = 0;
        bytesWritten = 0;

        size = parameters.poolSize;
        totalFreeSpace = (parameters.poolSize - OVERHEAD_POOL_MANAGEMENT);
        std::deque<DiskDataAddress> initialFreeChunk;
        initialFreeChunk.push_back(FILE_SIGNATURE.size() + sizeof(CURRENT_VERSION) + UUID_BYTE_LENGTH + PoolHeader::BYTE_LENGTH);
        freeChunks.insert({totalFreeSpace, initialFreeChunk});
        freeSpace.insert({initialFreeChunk.front(), totalFreeSpace});
        lastEntityInChain = INVALID_STORED_DATA_ID;
    }
    else
        throw std::invalid_argument("DiskDataPool::() > The specified disk pool path already exists.");
}

StorageManagement_Pools::DiskDataPool::DiskDataPool(DiskDataPoolLoadParameters parameters)
: poolPath(parameters.poolFilePath),
  eraseDataOnFailure(parameters.eraseDataOnFailure)
{
    //checks if the path exists & if it is a file
    size = boost::filesystem::file_size(poolPath);
    
    fileStream.open(parameters.poolFilePath, std::ios::out | std::ios::in | std::ios::binary);

    if(fileStream.fail() || !fileStream.is_open())
        throw std::runtime_error("DiskDataPool::() > Failed to open pool file; <" + getErrnoMessage() + ">.");

    //reads the file signature
    char signature[FILE_SIGNATURE.size()+1];
    fileStream.read(signature, FILE_SIGNATURE.size());
    signature[FILE_SIGNATURE.size()] = '\0';

    //checks the file signature
    if(FILE_SIGNATURE.compare(signature) != 0)
        throw std::runtime_error("DiskDataPool::() > The supplied file path does not point to a disk pool with a valid signature.");

    //reads the pool version
    char version;
    fileStream.read(&version, 1);

    //checks the pool version
    //FUTURE - (partial) support for older versions will be implemented with respect to each new version
    if(version != CURRENT_VERSION)
        throw std::runtime_error("DiskDataPool::() > The supplied file path does not point to a disk pool with a valid version.");

    //reads the pool UUID
    char rawUUID[UUID_BYTE_LENGTH + 1];
    fileStream.read(rawUUID, UUID_BYTE_LENGTH);
    rawUUID[UUID_BYTE_LENGTH] = '\0';
    uuid = boost::uuids::string_generator()(rawUUID);
    
    //retrieves the pool header
    ByteVector rawHeader(PoolHeader::BYTE_LENGTH);
    fileStream.read((char*)&rawHeader[0], PoolHeader::BYTE_LENGTH);
    header = PoolHeader::fromBytes(rawHeader);

    //retrieves the pool footer
    fileStream.seekg(header.footer);
    ByteVector rawFooter(PoolFooter::BYTE_LENGTH);
    fileStream.read((char*)&rawFooter[0], PoolFooter::BYTE_LENGTH);
    footer = PoolFooter::fromBytes(rawFooter);

    //initial pool configuration
    totalFreeSpace = (size - OVERHEAD_POOL_MANAGEMENT);
    std::deque<DiskDataAddress> initialFreeChunk;
    initialFreeChunk.push_back(FILE_SIGNATURE.size() + sizeof(CURRENT_VERSION) + UUID_BYTE_LENGTH + PoolHeader::BYTE_LENGTH);
    freeSpace.insert({initialFreeChunk.front(), totalFreeSpace});
    lastEntityInChain = INVALID_STORED_DATA_ID;

    if(footer.entitiesNumber > 0)
    {//attempts to load all entities
        if(footer.firstHeader == INVALID_DISK_DATA_ADDRESS)
            throw std::runtime_error("DiskDataPool::() > Corrupt data pool encountered; entity header not found.");

        EntitiesCountType remainingEntities = footer.entitiesNumber;
        DiskDataAddress currentEntityAddress = footer.firstHeader;
        ByteVector currentRawEntity(EntityHeader::BYTE_LENGTH);

        //goes through all entities
        while(remainingEntities > 0)
        {
            if(currentEntityAddress == INVALID_DISK_DATA_ADDRESS)
                throw std::runtime_error("DiskDataPool::() > Corrupt data pool encountered; invalid entity header found");

            fileStream.seekg(currentEntityAddress);
            fileStream.read((char*)&currentRawEntity[0], EntityHeader::BYTE_LENGTH);

            if(fileStream.fail())
                throw std::runtime_error("DiskDataPool::() > Failed to load entity header; <" + getErrnoMessage() + ">.");

            EntityDescriptor currentDescriptor{currentEntityAddress,
                                               EntityHeader::fromBytes(currentRawEntity),
                                               (lastEntityInChain != INVALID_STORED_DATA_ID) ? &entities[lastEntityInChain] : nullptr,
                                               nullptr, 0, false};

            entities.insert({currentDescriptor.rawHeader.id, currentDescriptor});

            if(lastEntityInChain != INVALID_STORED_DATA_ID)
                entities[lastEntityInChain].nextEntity = &entities[currentDescriptor.rawHeader.id];
            lastEntityInChain = currentDescriptor.rawHeader.id;

            totalFreeSpace -= (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH);

            if(parameters.mode == PoolMode::READ_WRITE)
            {//builds the free space management data structures fot the current entity
                auto nextFreeChunk = freeSpace.upper_bound(currentEntityAddress);
                auto previousFreeChunk = std::prev(nextFreeChunk);

                if(previousFreeChunk->first == currentEntityAddress)
                {//entity is at the beginning of the chunk
                    std::pair<DiskDataAddress, DataSize> newChunk{previousFreeChunk->first + (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH),
                                                                      previousFreeChunk->second - (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH)};
                    freeSpace.erase(previousFreeChunk);
                    freeSpace.insert(newChunk);
                }
                else if(previousFreeChunk->first + previousFreeChunk->second == currentEntityAddress + currentDescriptor.rawHeader.size)
                {//entity is at the end of the chunk
                    previousFreeChunk->second -= (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH);
                }
                else
                {//entity is inside the current chunk
                    DiskDataAddress newBackChunkAddress = currentEntityAddress + (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH);
                    DataSize newBackChunkSize = previousFreeChunk->first + previousFreeChunk->second - newBackChunkAddress;

                    DiskDataAddress newFrontChunkAddress = previousFreeChunk->first;
                    DataSize newFrontChunkSize = previousFreeChunk->second - (currentDescriptor.rawHeader.size + EntityHeader::BYTE_LENGTH) - newBackChunkSize;

                    freeSpace.erase(previousFreeChunk);
                    freeSpace.insert({newFrontChunkAddress, newFrontChunkSize});
                    freeSpace.insert({newBackChunkAddress, newBackChunkSize});
                }
            }

            currentEntityAddress = currentDescriptor.rawHeader.nextHeader;
            --remainingEntities;
        }

        //free space management is used only when the pool is in read/write mode
        if(parameters.mode == PoolMode::READ_WRITE)
        {
            for(auto currentChunk : freeSpace)
            {
                if(freeChunks.find(currentChunk.second) != freeChunks.end())
                    freeChunks[currentChunk.second].push_back(currentChunk.first);
                else
                {
                    std::deque<DiskDataAddress> newChunkQueue;
                    newChunkQueue.push_back(currentChunk.first);
                    freeChunks.insert({currentChunk.second, newChunkQueue});
                }
            }
        }
    }
    else
    {
        if(parameters.mode == PoolMode::READ_WRITE)
            freeChunks.insert({totalFreeSpace, initialFreeChunk});
    }

    state = PoolState::OPEN;
    mode = parameters.mode;
    bytesRead = parameters.bytesRead;
    bytesWritten = parameters.bytesWritten;
}

StorageManagement_Pools::DiskDataPool::~DiskDataPool()
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);
    state = PoolState::CLOSED;

    fileStream.close();
    entities.clear();
    freeChunks.clear();
    freeSpace.clear();
}

ByteVectorPtr StorageManagement_Pools::DiskDataPool::retrieveData(StoredDataID id)
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);
    
    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::retrieveData() > Failed to retrieve data; the pool is not in an open state.");
    
    auto requestedEntity = entities.find(id);
    if(requestedEntity != entities.end())
    {
        if(requestedEntity->second.streamWriteLocked)
            throw std::runtime_error("DiskDataPool::retrieveData() > Failed to retrieve data; there is a pending write operation for it.");
        
        fileStream.seekg(requestedEntity->second.entityAddress + EntityHeader::BYTE_LENGTH);
        ByteVector result(requestedEntity->second.rawHeader.size);

        fileStream.read((char*)&result[0], requestedEntity->second.rawHeader.size);

        if(fileStream.fail())
        {
            state = PoolState::FAILED;
            throw std::runtime_error("DiskDataPool::retrieveData() > Failed to retrieved the requested data; <" + getErrnoMessage() + ">.");
        }

        bytesRead += result.size();
        return ByteVectorPtr(new ByteVector(result));
    }
    else
        throw std::runtime_error("DiskDataPool::retrieveData() > Failed to retrieve the requested data; id not found.");
}

StoredDataID StorageManagement_Pools::DiskDataPool::storeData(const ByteVectorPtr data)
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);
    
    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::storeData() > Failed to store data; the pool is not in an open state.");
    
    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("DiskDataPool::storeData() > Failed to store data; the pool is not in read/write mode.");
    
    if(data->size() > 0)
    {
        if(data->size() > totalFreeSpace)
            throw std::runtime_error("DiskDataPool::storeData() > Failed to store data; the pool has insufficient free space.");

        StoredDataID newEntityID = ++footer.lastDataID;
        DiskDataAddress newEntityAddress = allocateEntityChunk(data->size() + EntityHeader::BYTE_LENGTH);
        
        if(newEntityAddress == INVALID_DISK_DATA_ADDRESS)
            throw std::runtime_error("DiskDataPool::storeData() > Failed to store data; no suitable storage location was found.");

        EntityDescriptor newEntityDescriptor
        {
            newEntityAddress,
            {newEntityID, data->size(), INVALID_DISK_DATA_ADDRESS},
            (lastEntityInChain != INVALID_STORED_DATA_ID) ? &entities[lastEntityInChain] : nullptr,
            nullptr,
            0,
            false
        };

        entities.insert({newEntityID, newEntityDescriptor});

        if(footer.firstHeader == INVALID_DISK_DATA_ADDRESS)
            footer.firstHeader = newEntityAddress;
        ++footer.entitiesNumber;

        if(lastEntityInChain != INVALID_STORED_DATA_ID)
        {
            EntityDescriptor & lastEntity = entities[lastEntityInChain];
            lastEntity.rawHeader.nextHeader = newEntityAddress;
            lastEntity.nextEntity = &entities[newEntityID];

            fileStream.seekp(lastEntity.entityAddress);
            fileStream.write((const char*)&lastEntity.rawHeader.toBytes()[0], EntityHeader::BYTE_LENGTH);
        }

        lastEntityInChain = newEntityID;

        fileStream.seekp(newEntityAddress);
        fileStream.write((const char*)&newEntityDescriptor.rawHeader.toBytes()[0], EntityHeader::BYTE_LENGTH);
        fileStream.write((const char*)&(*data)[0], data->size());

        flushFooter();
        
        if(fileStream.fail())
        {
            state = PoolState::FAILED;
            discardDataWithoutLock(newEntityID, eraseDataOnFailure);
            throw std::runtime_error("DiskDataPool::storeData() > Failed to store data; <" + getErrnoMessage() + ">.");
        }

        bytesWritten += data->size();
        return newEntityID;
    }
    else
        throw std::invalid_argument("DiskDataPool::storeData() > Failed to store data; no data supplied.");
}

void StorageManagement_Pools::DiskDataPool::discardDataWithoutLock(StoredDataID id, bool erase)
{
    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::discardData() > Failed to discard data; the pool is not in an open state.");
    
    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("DiskDataPool::discardData() > Failed to discard data; the pool is not in read/write mode.");
    
    auto requestedEntity = entities.find(id);
    if(requestedEntity != entities.end())
    {
        if(requestedEntity->second.streamWriteLocked || requestedEntity->second.streamReadLocks > 0)
            throw std::runtime_error("DiskDataPool::discardData() > Failed to discard data; there is a pending read or write operation for it.");
        
        //retrieves the adjacent entities (if any)
        EntityDescriptor * nextEntity = requestedEntity->second.nextEntity;
        EntityDescriptor * previousEntity = requestedEntity->second.previousEntity;

        if(previousEntity != nullptr)
        {
            //updates the previous entity header/descriptor to skip the entity to be discarded
            previousEntity->rawHeader.nextHeader = (nextEntity != nullptr) ? nextEntity->entityAddress : INVALID_DISK_DATA_ADDRESS;
            previousEntity->nextEntity = nextEntity;

            fileStream.seekp(previousEntity->entityAddress);
            fileStream.write((const char*)&previousEntity->rawHeader.toBytes()[0], EntityHeader::BYTE_LENGTH);
        }

        if(nextEntity != nullptr)
            nextEntity->previousEntity = previousEntity;

        if(footer.firstHeader == requestedEntity->second.entityAddress)
            footer.firstHeader = (nextEntity != nullptr) ? nextEntity->entityAddress : INVALID_DISK_DATA_ADDRESS;
        
        if(lastEntityInChain == id)
            lastEntityInChain = (previousEntity != nullptr) ? previousEntity->rawHeader.id : INVALID_STORED_DATA_ID;
        
        if(erase)
        {
            //TODO - more efficient approach (?)
            //overwrites the existing data
            fileStream.seekp(requestedEntity->second.entityAddress);
            const char zeroVal = '\0';
            for(DataSize i = 0; i < (requestedEntity->second.rawHeader.size + EntityHeader::BYTE_LENGTH); i++)
                fileStream.write(&zeroVal, 1);
            fileStream.flush();
        }
        
        //marks the data occupied by the entity as free & removes it from the table
        freeEntityChunk(requestedEntity->second.entityAddress, (requestedEntity->second.rawHeader.size + EntityHeader::BYTE_LENGTH));
        entities.erase(requestedEntity);
        --footer.entitiesNumber;

        flushFooter();
        
        if(fileStream.fail())
        {
            state = PoolState::FAILED;
            throw std::runtime_error("DiskDataPool::discardData() > Failed to discard data; <" + getErrnoMessage() + ">.");
        }
    }
    else
        throw std::runtime_error("DiskDataPool::discardData() > Failed to discard the requested data; id not found.");
}

void StorageManagement_Pools::DiskDataPool::discardData(StoredDataID id, bool erase)
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);
    discardDataWithoutLock(id, erase);
}

void StorageManagement_Pools::DiskDataPool::clearPool()
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::clearPool() > Failed to clear pool; the pool is not in an open state.");

    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("DiskDataPool::clearPool() > Failed to clear pool; the pool is not in read/write mode.");

    entities.clear();
    freeChunks.clear();
    freeSpace.clear();

    footer.entitiesNumber = 0;
    footer.firstHeader = INVALID_DISK_DATA_ADDRESS;

    totalFreeSpace = (size - OVERHEAD_POOL_MANAGEMENT);
    std::deque<DiskDataAddress> initialFreeChunk;
    initialFreeChunk.push_back(FILE_SIGNATURE.size() + sizeof(CURRENT_VERSION) + UUID_BYTE_LENGTH + PoolHeader::BYTE_LENGTH);
    freeSpace.insert({initialFreeChunk.front(), totalFreeSpace});
    freeChunks.insert({totalFreeSpace, initialFreeChunk});
    lastEntityInChain = INVALID_STORED_DATA_ID;

    flushFooter();

    if(fileStream.fail())
    {
        state = PoolState::FAILED;
        throw std::runtime_error("DiskDataPool::clearPool() > Failed to clear pool; <" + getErrnoMessage() + ">.");
    }
}

PoolInputStreamPtr StorageManagement_Pools::DiskDataPool::getInputStream(StoredDataID dataID)
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::getInputStream() > Failed to retrieve input stream; the pool is not in an open state.");

    auto requestedEntity = entities.find(dataID);
    if(requestedEntity != entities.end())
    {
        ++(requestedEntity->second.streamReadLocks);
        bytesRead += requestedEntity->second.rawHeader.size;
        
        PoolInputStreamPtr result(new DiskPoolInputStream(dataID, 
                                                          requestedEntity->second.rawHeader.size,
                                                          (requestedEntity->second.entityAddress + EntityHeader::BYTE_LENGTH),
                                                          fileMutex,
                                                          fileStream,
                                                          requestedEntity->second.streamReadLocks
                                                         )
                                 );
        
        return result;
    }
    else
        throw std::runtime_error("DiskDataPool::getInputStream() > Failed to retrieve input stream; ID not found.");
}

PoolOutputStreamPtr StorageManagement_Pools::DiskDataPool::getOutputStream(DataSize dataSize)
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);

    if(state != PoolState::OPEN)
        throw std::runtime_error("DiskDataPool::getOutputStream() > Failed to retrieve output stream; the pool is not in an open state.");

    if(mode != PoolMode::READ_WRITE)
        throw std::runtime_error("DiskDataPool::getOutputStream() > Failed to retrieve output stream; the pool is not in read/write mode.");

    if(dataSize > 0)
    {
        if(dataSize > totalFreeSpace)
            throw std::runtime_error("DiskDataPool::getOutputStream() > Failed to retrieve output stream; the pool has insufficient free space.");

        StoredDataID newEntityID = ++footer.lastDataID;
        DiskDataAddress newEntityAddress = allocateEntityChunk(dataSize + EntityHeader::BYTE_LENGTH);

        if(newEntityAddress == INVALID_DISK_DATA_ADDRESS)
            throw std::runtime_error("DiskDataPool::getOutputStream() > Failed to retrieve output stream; no suitable storage location was found.");

        EntityDescriptor newEntityDescriptor
        {
            newEntityAddress,
            {newEntityID, dataSize, INVALID_DISK_DATA_ADDRESS},
            (lastEntityInChain != INVALID_STORED_DATA_ID) ? &entities[lastEntityInChain] : nullptr,
            nullptr,
            0,
            true
        };

        entities.insert({newEntityID, newEntityDescriptor});

        if(footer.firstHeader == INVALID_DISK_DATA_ADDRESS)
            footer.firstHeader = newEntityAddress;
        ++footer.entitiesNumber;

        if(lastEntityInChain != INVALID_STORED_DATA_ID)
        {
            EntityDescriptor & lastEntity = entities[lastEntityInChain];
            lastEntity.rawHeader.nextHeader = newEntityAddress;
            lastEntity.nextEntity = &entities[newEntityID];

            fileStream.seekp(lastEntity.entityAddress);
            fileStream.write((const char*)&lastEntity.rawHeader.toBytes()[0], EntityHeader::BYTE_LENGTH);
        }

        lastEntityInChain = newEntityID;
        fileStream.seekp(newEntityAddress);
        fileStream.write((const char*)&newEntityDescriptor.rawHeader.toBytes()[0], EntityHeader::BYTE_LENGTH);

        flushFooter();

        if(fileStream.fail())
        {
            state = PoolState::FAILED;
            discardDataWithoutLock(newEntityID, eraseDataOnFailure);
            throw std::runtime_error("DiskDataPool::getOutputStream() > Failed to retrieve output stream;; <" + getErrnoMessage() + ">.");
        }
        
        bytesWritten += dataSize;
        PoolOutputStreamPtr result(new DiskPoolOutputStream(newEntityID,
                                                            dataSize,
                                                            (newEntityAddress + EntityHeader::BYTE_LENGTH),
                                                            fileMutex,
                                                            fileStream,
                                                            entities[newEntityID].streamWriteLocked
                                                           )
                                  );
        
        return result;
    }
    else
        throw std::invalid_argument("DiskDataPool::getOutputStream() > Failed to retrieve output stream; no data supplied.");
}

DataSize StorageManagement_Pools::DiskDataPool::getEntitySize(StoredDataID id) const
{
    boost::lock_guard<boost::mutex> fileLock(fileMutex);
    auto requestedEntity = entities.find(id);
    if(requestedEntity != entities.end())
        return requestedEntity->second.rawHeader.size;
    else
        return INVALID_DATA_SIZE;
}

DiskDataAddress StorageManagement_Pools::DiskDataPool::allocateEntityChunk(DataSize entitySize)
{
    auto availableChunks = freeChunks.lower_bound(entitySize);
    
    if(availableChunks != freeChunks.end())
    {//suitable free chunk was found
        DataSize chunkSize = availableChunks->first;
        std::deque<DiskDataAddress> & chunkQueue = availableChunks->second;
        DiskDataAddress chunkAddress = chunkQueue.front();
        
        if(chunkSize != entitySize)
        {//the chunk needs to be resized
            DataSize remainingChunkSize = chunkSize - entitySize;
            DiskDataAddress newChunkAddress = chunkAddress + entitySize;

            //adds the resized chunk to the appropriate place in the table
            auto newChunkLocation = freeChunks.find(remainingChunkSize);
            if(newChunkLocation != freeChunks.end())
                newChunkLocation->second.push_back(newChunkAddress);
            else
            {//adds a new entry
                std::deque<DiskDataAddress> newChunksQueue;
                newChunksQueue.push_back(newChunkAddress);
                freeChunks.insert({remainingChunkSize, newChunksQueue});
            }

            freeSpace.insert({newChunkAddress, remainingChunkSize});
        }

        //removes the old chunk data
        chunkQueue.pop_front();
        if(chunkQueue.size() == 0)
            freeChunks.erase(chunkSize);
        freeSpace.erase(chunkAddress);

        totalFreeSpace -= entitySize;
        return chunkAddress;
    }
    else //no suitable free chunk was found
        return INVALID_DISK_DATA_ADDRESS;
}

void StorageManagement_Pools::DiskDataPool::freeEntityChunk(DiskDataAddress entityAddress, DataSize entitySize)
{
    totalFreeSpace += entitySize;
    
    if(freeSpace.size() == 0)
    {
        assert(freeChunks.size() == 0);
        
        freeSpace.insert({entityAddress, entitySize});
        
        std::deque<DiskDataAddress> newChunksQueue;
        newChunksQueue.push_back(entityAddress);
        freeChunks.insert({entitySize, newChunksQueue});
        
        return;
    }
    
    auto nextFreeChunk = freeSpace.upper_bound(entityAddress);
    auto prevFreeChunk = (nextFreeChunk != freeSpace.begin()) ? std::prev(nextFreeChunk) : freeSpace.end();
    
    bool deleteNextChunk = false;
    bool deletePrevChunk = false;
    DiskDataAddress newChunkAddress = entityAddress;
    DataSize newChunkSize = entitySize;
    
    if(nextFreeChunk != freeSpace.end())
    {
        //checks if the current free chunk is to be merged with the next one
        if((entityAddress + entitySize) == nextFreeChunk->first)
        {
            deleteNextChunk = true;
            newChunkSize += nextFreeChunk->second;
        }
    }
    
    if(prevFreeChunk != freeSpace.end())
    {
        //checks if the current free chunk is to be merged with the previous one
        if((prevFreeChunk->first + prevFreeChunk->second) == entityAddress)
        {
            deletePrevChunk = true;
            newChunkAddress = prevFreeChunk->first;
            newChunkSize += prevFreeChunk->second;
        }
    }
    
    if(deleteNextChunk)
    {
        std::deque<DiskDataAddress> & nextChunkQueue = freeChunks[nextFreeChunk->second];
        
        //removes the next chunk from its queue
        nextChunkQueue.erase(std::find(nextChunkQueue.begin(),
                                       nextChunkQueue.end(),
                                       nextFreeChunk->first));
        
        if(nextChunkQueue.size() == 0)
            freeChunks.erase(nextFreeChunk->second);
        
        freeSpace.erase(nextFreeChunk);
    }
    
    if(deletePrevChunk)
    {
        std::deque<DiskDataAddress> & previousChunkSizeQueue = freeChunks[prevFreeChunk->second];
        
        //removes the previous chunk from its queue
        previousChunkSizeQueue.erase(std::find(previousChunkSizeQueue.begin(),
                                               previousChunkSizeQueue.end(),
                                               prevFreeChunk->first));
        
        if(previousChunkSizeQueue.size() == 0)
            freeChunks.erase(prevFreeChunk->second);
        
        freeSpace.erase(prevFreeChunk);
    }
    
    freeSpace.insert({newChunkAddress, newChunkSize});
    
    //adds new chunk to the appropriate place in the table
    if(freeChunks.find(newChunkSize) != freeChunks.end())
        freeChunks[newChunkSize].push_back(newChunkAddress);
    else
    {
        std::deque<DiskDataAddress> newChunksQueue;
        newChunksQueue.push_back(newChunkAddress);
        freeChunks.insert({newChunkSize, newChunksQueue});
    }
}

