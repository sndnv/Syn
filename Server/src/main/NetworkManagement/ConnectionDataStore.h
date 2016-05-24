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

#ifndef CONNECTIONDATASTORE_H
#define	CONNECTIONDATASTORE_H

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/FileLogger.h"

#include "../DatabaseManagement/Types/Types.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"

#include "../SecurityManagement/Crypto/Containers.h"
#include "Types/Containers.h"

using boost::multi_index_container;
using boost::multi_index::indexed_by;
using boost::multi_index::hashed_unique;
using boost::multi_index::hashed_non_unique;
using boost::multi_index::member;
using boost::multi_index::composite_key;
using boost::multi_index::tag;

//Common
using Common_Types::LogSeverity;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;

//Database
using DatabaseManagement_Containers::DeviceDataContainerPtr;

//Networking
using NetworkManagement_Types::ConnectionID;
using NetworkManagement_Types::TransientConnectionID;
using NetworkManagement_Types::INVALID_TRANSIENT_CONNECTION_ID;
using NetworkManagement_Types::PendingInitConnectionConfigPtr;
using NetworkManagement_Types::PendingDataConnectionConfigPtr;
using NetworkManagement_Types::ActiveConnectionData;
using NetworkManagement_Types::ActiveConnectionDataPtr;

namespace NetworkManagement_Handlers
{
    /**
     * Data store for connections data.
     */
    class ConnectionDataStore
    {
        public:
            /**
             * Creates a new empty data store.
             */
            ConnectionDataStore()
            {}
            
            /**
             * Clears the data store.
             */
            ~ConnectionDataStore();
            
            /**
             * Adds the supplied command connection container to the store.
             * 
             * @param data the data to be added
             */
            void addCommandConnectionData(DeviceDataContainerPtr data);
            
            /**
             * Adds the supplied data connection container to the store.
             * 
             * @param data the data to be added
             */
            void addDataConnectionData(PendingDataConnectionConfigPtr data);
            
            /**
             * Adds the supplied init connection container to the store.
             * 
             * @param data the data to be added
             */
            void addInitConnectionData(
                const IPAddress initAddress, const IPPort initPort,
                PendingInitConnectionConfigPtr data);
            
            /**
             * Retrieves command connection data for the specified device.
             * 
             * @param deviceID the ID of the device
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            DeviceDataContainerPtr getCommandConnectionData(const DeviceID deviceID);
            
            /**
             * Retrieves command connection data for the specified address/port.
             * 
             * @param address the command address of the device
             * @param port the command port of the device
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            DeviceDataContainerPtr getCommandConnectionData(
                const IPAddress address, const IPPort port);
            
            /**
             * Retrieves data connection data for the specified device and transient connection.
             * 
             * @param deviceID the ID of the device
             * @param transientID the ID of the transient connection
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            PendingDataConnectionConfigPtr getDataConnectionData(
                const DeviceID deviceID, const TransientConnectionID transientID);
            
            /**
             * Retrieves data connection data for the specified address/port.
             * 
             * @param address the data address of the device
             * @param port the data port of the device
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            PendingDataConnectionConfigPtr getDataConnectionData(
                const IPAddress address, const IPPort port);
            
            /**
             * Retrieves init connection data for the specified transient connection.
             * 
             * @param transientID the ID of the transient connection
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            PendingInitConnectionConfigPtr getInitConnectionData(
                const TransientConnectionID transientID);
            
            /**
             * Retrieves init connection data for the specified address/port.
             * 
             * @param address the init address of the device
             * @param port the init port of the device
             * @throw runtime_error if no data could be found
             * @return the requested data
             */
            PendingInitConnectionConfigPtr getInitConnectionData(
                const IPAddress address, const IPPort port);
            
            /**
             * Discards command connection data for the specified device.
             * 
             * @param deviceID the ID of the device
             * @return <code>true</code> if the discard operation was successful
             */
            bool discardCommandConnectionData(const DeviceID deviceID);
            
            /**
             * Discards command connection data for the specified address/port.
             * 
             * @param address the command address of the device
             * @param port the command port of the device
             * @return <code>true</code> if the discard operation was successful
             */
            bool discardCommandConnectionData(
                const IPAddress address, const IPPort port);
            
            /**
             * Discards data connection data for the specified device and transient connection.
             * 
             * @param deviceID the ID of the device
             * @param transientID the ID of the transient connection
             * @return <code>true</code> if the discard operation was successful
             */
            bool discardDataConnectionData(
                const DeviceID deviceID, const TransientConnectionID transientID);
            
            /**
             * Discards data connection data for the specified address/port.
             * 
             * @param address the data address of the device
             * @param port the data port of the device
             */
            void discardDataConnectionData(const IPAddress address, const IPPort port);
            
            /**
             * Discards init connection data for the specified transient connection.
             * 
             * @param transientID the ID of the transient connection
             * @return <code>true</code> if the discard operation was successful
             */
            bool discardInitConnectionData(const TransientConnectionID transientID);
            
            /**
             * Discards init connection data for the specified address/port.
             * 
             * @param address the init address of the device
             * @param port the init port of the device
             * @return <code>true</code> if the discard operation was successful
             */
            bool discardInitConnectionData(const IPAddress address, const IPPort port);
            
            /**
             * Checks if command connection data is stored for the specified device.
             * 
             * @param deviceID the ID of the device
             * @return <code>true</code> if command connection data is available
             */
            bool isCommandConnectionDataAvailable(const DeviceID deviceID);
            
            /**
             * Checks if data connection data is stored for the specified device and transient ID.
             * 
             * @param deviceID the ID of the device
             * @param transientID transient connection ID
             * @return <code>true</code> if data connection data is available
             */
            bool isDataConnectionDataAvailable(
                const DeviceID deviceID, const TransientConnectionID transientID);
            
            /**
             * Checks if init connection data is stored for the specified transient ID.
             * 
             * @param transientID transient connection ID
             * @return <code>true</code> if init connection data is available
             */
            bool isInitConnectionDataAvailable(const TransientConnectionID transientID);
            
            ConnectionDataStore(const ConnectionDataStore&) = delete;
            ConnectionDataStore& operator=(const ConnectionDataStore&) = delete;
            
        private:
            //<editor-fold defaultstate="collapsed" desc="Store Types">
            struct CommandConnectionEntry
            {
                DeviceID id;
                IPAddress address;
                IPPort port;
                DeviceDataContainerPtr data;
                
                struct byID{};  //tag for retrieval by device ID
                struct byIP{};  //tag for retrieval by IP address and port
            };
            
            struct DataConnectionEntry
            {
                DeviceID deviceID;
                TransientConnectionID transientID;
                IPAddress address;
                IPPort port;
                PendingDataConnectionConfigPtr data;
                
                struct byID{};  //tag for retrieval by device and transient IDs
                struct byIP{};  //tag for retrieval by IP address and port
            };
            
            struct InitConnectionEntry
            {
                TransientConnectionID transientID;
                IPAddress address;
                IPPort port;
                PendingInitConnectionConfigPtr data;
                
                struct byID{};  //tag for retrieval by  transient ID
                struct byIP{};  //tag for retrieval by IP address and port
            };
            
            typedef
            multi_index_container<
                CommandConnectionEntry,
                indexed_by<
                    hashed_unique<
                        tag<CommandConnectionEntry::byID>,
                        member<CommandConnectionEntry, DeviceID, &CommandConnectionEntry::id>
                    >,
                    hashed_unique<
                        tag<CommandConnectionEntry::byIP>,
                        composite_key<
                            CommandConnectionEntry,
                            member<CommandConnectionEntry, IPAddress, &CommandConnectionEntry::address>,
                            member<CommandConnectionEntry, IPPort, &CommandConnectionEntry::port>
                        >
                    >
                >
            >
            CommandConnectionStore;
            
            typedef
            multi_index_container<
                DataConnectionEntry,
                indexed_by<
                    hashed_unique<
                        tag<DataConnectionEntry::byID>,
                        composite_key<
                            DataConnectionEntry,
                            member<DataConnectionEntry, DeviceID, &DataConnectionEntry::deviceID>,
                            member<DataConnectionEntry, TransientConnectionID, &DataConnectionEntry::transientID>
                        >
                    >,
                    hashed_non_unique<
                        tag<DataConnectionEntry::byIP>,
                        composite_key<
                            DataConnectionEntry,
                            member<DataConnectionEntry, IPAddress, &DataConnectionEntry::address>,
                            member<DataConnectionEntry, IPPort, &DataConnectionEntry::port>
                        >
                    >
                >
            >
            DataConnectionStore;
            
            typedef
            multi_index_container<
                InitConnectionEntry,
                indexed_by<
                    hashed_non_unique<
                        tag<InitConnectionEntry::byIP>,
                        composite_key<
                            InitConnectionEntry,
                            member<InitConnectionEntry, IPAddress, &InitConnectionEntry::address>,
                            member<InitConnectionEntry, IPPort, &InitConnectionEntry::port>
                        >
                    >,
                    hashed_unique<
                        tag<InitConnectionEntry::byID>,
                        member<InitConnectionEntry, TransientConnectionID, &InitConnectionEntry::transientID>
                    >
                >
            >
            InitConnectionStore;
            //</editor-fold>
            
            boost::mutex pendingCommandConnectionsMutex;
            CommandConnectionStore pendingCommandConnections;
            
            boost::mutex pendingDataConnectionsMutex;
            DataConnectionStore pendingDataConnections;
            
            boost::mutex pendingInitConnectionsMutex;
            InitConnectionStore pendingInitConnections;
    };
}

#endif	/* CONNECTIONDATASTORE_H */
