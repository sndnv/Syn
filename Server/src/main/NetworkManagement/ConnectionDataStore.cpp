/* 
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

#include "ConnectionDataStore.h"
#include <boost/thread/lock_guard.hpp>
#include "../Utilities/Strings/Common.h"

NetworkManagement_Handlers::ConnectionDataStore::~ConnectionDataStore()
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    if(!pendingCommandConnections.empty())
    {
        pendingCommandConnections.clear();
    }

    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    if(!pendingDataConnections.empty())
    {
        pendingDataConnections.clear();
    }

    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    if(!pendingInitConnections.empty())
    {
        pendingInitConnections.clear();
    }
}

void NetworkManagement_Handlers::ConnectionDataStore::addCommandConnectionData
(DeviceDataContainerPtr data)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    pendingCommandConnections.insert(
        CommandConnectionEntry
        {
            data->getDeviceID(),
            data->getDeviceCommandAddress(),
            data->getDeviceCommandPort(),
            data
        });
}

void NetworkManagement_Handlers::ConnectionDataStore::addDataConnectionData
(PendingDataConnectionConfigPtr data)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    pendingDataConnections.insert(
        DataConnectionEntry
        {
            data->data->getDeviceID(),
            data->transientID,
            data->data->getDeviceDataAddress(),
            data->data->getDeviceDataPort(),
            data
        });
}

void NetworkManagement_Handlers::ConnectionDataStore::addInitConnectionData
(const IPAddress initAddress, const IPPort initPort, PendingInitConnectionConfigPtr data)
{
    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    pendingInitConnections.insert(
        InitConnectionEntry
        {
            data->transientID,
            initAddress,
            initPort,
            data
        });
}

DeviceDataContainerPtr NetworkManagement_Handlers::ConnectionDataStore::getCommandConnectionData
(const DeviceID deviceID)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    auto resultIterator = pendingCommandConnections.get<CommandConnectionEntry::byID>().find(deviceID);
    if(resultIterator != pendingCommandConnections.get<CommandConnectionEntry::byID>().end())
    {
        return resultIterator->data;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getCommandConnectionData(DeviceID) > No data found for device ["
                + Utilities::Strings::toString(deviceID) + "].");
    }
}

DeviceDataContainerPtr NetworkManagement_Handlers::ConnectionDataStore::getCommandConnectionData
(const IPAddress address, const IPPort port)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    auto resultIterator = pendingCommandConnections.get<CommandConnectionEntry::byIP>().find(boost::make_tuple(address, port));
    if(resultIterator != pendingCommandConnections.get<CommandConnectionEntry::byIP>().end())
    {
        return resultIterator->data;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getCommandConnectionData(IPAddress, IPPort) >"
                " No data found for address/port ["
                + address + " / " + Utilities::Strings::toString(port) + "].");
    }
}

PendingDataConnectionConfigPtr NetworkManagement_Handlers::ConnectionDataStore::getDataConnectionData
(const DeviceID deviceID, const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    auto resultIterator = pendingDataConnections.get<DataConnectionEntry::byID>().find(boost::make_tuple(deviceID, transientID));
    if(resultIterator != pendingDataConnections.get<DataConnectionEntry::byID>().end())
    {
        PendingDataConnectionConfigPtr result = resultIterator->data;
        pendingDataConnections.get<DataConnectionEntry::byID>().erase(resultIterator);
        return result;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getDataConnectionData(DeviceID, TransientConnectionID) >"
                " No data found for device [" + Utilities::Strings::toString(deviceID) +
                " and connection [" + Utilities::Strings::toString(transientID) + "].");
    }
}

PendingDataConnectionConfigPtr NetworkManagement_Handlers::ConnectionDataStore::getDataConnectionData
(const IPAddress address, const IPPort port)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    auto resultIterator = pendingDataConnections.get<DataConnectionEntry::byIP>().find(boost::make_tuple(address, port));
    if(resultIterator != pendingDataConnections.get<DataConnectionEntry::byIP>().end())
    {
        PendingDataConnectionConfigPtr result = resultIterator->data;
        pendingDataConnections.get<DataConnectionEntry::byIP>().erase(resultIterator);
        return result;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getDataConnectionData(IPAddress, IPPort) >"
                " No data found for address/port [" + address + " / " + Utilities::Strings::toString(port) + "].");
    }
}

PendingInitConnectionConfigPtr NetworkManagement_Handlers::ConnectionDataStore::getInitConnectionData
(const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    auto resultIterator = pendingInitConnections.get<InitConnectionEntry::byID>().find(transientID);
    if(resultIterator != pendingInitConnections.get<InitConnectionEntry::byID>().end())
    {
        return resultIterator->data;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getInitConnectionData(TransientConnectionID) >"
                " No data found for connection [" + Utilities::Strings::toString(transientID) + "].");
    }
}

PendingInitConnectionConfigPtr NetworkManagement_Handlers::ConnectionDataStore::getInitConnectionData
(const IPAddress address, const IPPort port)
{
    if(address == INVALID_IP_ADDRESS || port == INVALID_IP_PORT)
    {
        throw std::logic_error("ConnectionDataStore::getInitConnectionData(IPAddress, IPPort) >"
                " Cannot access data with invalid address/port .");
    }

    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    auto resultIterator = pendingInitConnections.get<InitConnectionEntry::byIP>().find(boost::make_tuple(address, port));
    if(resultIterator != pendingInitConnections.get<InitConnectionEntry::byIP>().end())
    {
        return resultIterator->data;
    }
    else
    {
        throw std::runtime_error("ConnectionDataStore::getInitConnectionData(IPAddress, IPPort) >"
                " No data found for address/port [" + address + " / " + Utilities::Strings::toString(port) + "].");
    }
}

bool NetworkManagement_Handlers::ConnectionDataStore::discardCommandConnectionData
(const DeviceID deviceID)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    auto resultIterator = pendingCommandConnections.get<CommandConnectionEntry::byID>().find(deviceID);
    if(resultIterator != pendingCommandConnections.get<CommandConnectionEntry::byID>().end())
    {
        pendingCommandConnections.get<CommandConnectionEntry::byID>().erase(resultIterator);
        return true;
    }
    else
    {
        return false;
    }
}

bool NetworkManagement_Handlers::ConnectionDataStore::discardCommandConnectionData
(const IPAddress address, const IPPort port)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    auto resultIterator = pendingCommandConnections.get<CommandConnectionEntry::byIP>().find(boost::make_tuple(address, port));
    if(resultIterator != pendingCommandConnections.get<CommandConnectionEntry::byIP>().end())
    {
        pendingCommandConnections.get<CommandConnectionEntry::byIP>().erase(resultIterator);
        return true;
    }
    else
    {
        return false;
    }
}

bool NetworkManagement_Handlers::ConnectionDataStore::discardDataConnectionData
(const DeviceID deviceID, const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    auto resultIterator = pendingDataConnections.get<DataConnectionEntry::byID>().find(boost::make_tuple(deviceID, transientID));
    if(resultIterator != pendingDataConnections.get<DataConnectionEntry::byID>().end())
    {
        pendingDataConnections.get<DataConnectionEntry::byID>().erase(resultIterator);
        return true;
    }
    else
    {
        return false;
    }
}

void NetworkManagement_Handlers::ConnectionDataStore::discardDataConnectionData
(const IPAddress address, const IPPort port)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    auto iterators = pendingDataConnections.get<DataConnectionEntry::byIP>().equal_range(boost::make_tuple(address, port));
    pendingDataConnections.get<DataConnectionEntry::byIP>().erase(iterators.first, iterators.second);
}

bool NetworkManagement_Handlers::ConnectionDataStore::discardInitConnectionData
(const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    auto resultIterator = pendingInitConnections.get<InitConnectionEntry::byID>().find(transientID);
    if(resultIterator != pendingInitConnections.get<InitConnectionEntry::byID>().end())
    {
        pendingInitConnections.get<InitConnectionEntry::byID>().erase(resultIterator);
        return true;
    }
    else
    {
        return false;
    }
}

bool NetworkManagement_Handlers::ConnectionDataStore::discardInitConnectionData
(const IPAddress address, const IPPort port)
{
    if(address == INVALID_IP_ADDRESS || port == INVALID_IP_PORT)
    {
        throw std::logic_error("ConnectionDataStore::discardInitConnectionData(IPAddress, IPPort) >"
                " Cannot discard data with invalid address/port .");
    }

    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    auto resultIterator = pendingInitConnections.get<InitConnectionEntry::byIP>().find(boost::make_tuple(address, port));
    if(resultIterator != pendingInitConnections.get<InitConnectionEntry::byIP>().end())
    {
        pendingInitConnections.get<InitConnectionEntry::byIP>().erase(resultIterator);
        return true;
    }
    else
    {
        return false;
    }
}

bool NetworkManagement_Handlers::ConnectionDataStore::isCommandConnectionDataAvailable
(const DeviceID deviceID)
{
    boost::lock_guard<boost::mutex> pendingCommandConnectionsLock(pendingCommandConnectionsMutex);
    auto resultIterator = pendingCommandConnections.get<CommandConnectionEntry::byID>().find(deviceID);
    return resultIterator != pendingCommandConnections.get<CommandConnectionEntry::byID>().end();
}

bool NetworkManagement_Handlers::ConnectionDataStore::isDataConnectionDataAvailable
(const DeviceID deviceID, const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingDataConnectionsLock(pendingDataConnectionsMutex);
    auto resultIterator = pendingDataConnections.get<DataConnectionEntry::byID>().find(boost::make_tuple(deviceID, transientID));
    return resultIterator != pendingDataConnections.get<DataConnectionEntry::byID>().end();
}

bool NetworkManagement_Handlers::ConnectionDataStore::isInitConnectionDataAvailable
(const TransientConnectionID transientID)
{
    boost::lock_guard<boost::mutex> pendingInitConnectionsLock(pendingInitConnectionsMutex);
    auto resultIterator = pendingInitConnections.get<InitConnectionEntry::byID>().find(transientID);
    return resultIterator != pendingInitConnections.get<InitConnectionEntry::byID>().end();
}