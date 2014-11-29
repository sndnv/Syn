/**
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

#ifndef DEVICEDATACONTAINER_H
#define	DEVICEDATACONTAINER_H

#include <string>
#include <typeinfo>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"

using DatabaseManagement_Types::DataTransferType;
using DatabaseManagement_Types::UserID;
using DatabaseManagement_Types::DeviceID;
using Common_Types::IPAddress;
using Common_Types::IPPort;
using Common_Types::INVALID_IP_ADDRESS;
using Common_Types::INVALID_IP_PORT;

namespace DatabaseManagement_Containers
{
    class DeviceDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            /**
             * Creates a new device data container.
             * 
             * Note: Used when supplying data to the database.
             * 
             * @param name device name
             * @param owner device owner
             * @param transferType default transfer type
             */
            DeviceDataContainer(std::string name, UserID owner, DataTransferType transferType) 
                : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::DEVICE), deviceOwner(owner), ipAddress(INVALID_IP_ADDRESS), ipPort(INVALID_IP_PORT),
                  xferType(transferType), deviceProvidedID("UNDEFINED"), deviceName(name),   deviceInfo("UNDEFINED")
            {}
            
            /**
             * Creates a new device data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id device ID
             * @param providedID ID provided by the device itself
             * @param name device name
             * @param owner device owner
             * @param address device IP address
             * @param port device IP port
             * @param transferType default transfer type
             * @param info device info
             */
            DeviceDataContainer(DeviceID id, std::string providedID, std::string name, UserID owner, IPAddress address, IPPort port, DataTransferType transferType, std::string info)
                : DataContainer(id, DatabaseObjectType::DEVICE), deviceOwner(owner), ipAddress(address), ipPort(port), xferType(transferType), deviceProvidedID(providedID), deviceName(name), deviceInfo(info)
            {}
            
            DeviceDataContainer() = delete;                                         //No default constructor
            DeviceDataContainer(const DeviceDataContainer&) = default;              //Default copy constructor
            ~DeviceDataContainer() = default;                                       //Default destructor
            DeviceDataContainer& operator=(const DeviceDataContainer&) = default;   //Default assignment operator
            
            DeviceID getDeviceID()             const { return containerID; }
            UserID getDeviceOwner()            const { return deviceOwner; }
            IPAddress getDeviceAddress()       const { return ipAddress; }
            IPPort getDevicePort()             const { return ipPort; }
            DataTransferType getTransferType() const { return xferType; }
            std::string getDeviceProvidedID()  const { return deviceProvidedID; }
            std::string getDeviceName()        const { return deviceName; }
            std::string getDeviceInfo()        const { return deviceInfo; }
            
            void setDeviceAddress(const IPAddress newAddress)       { ipAddress = newAddress; modified = true; }
            void setDevicePort(const IPPort newPort)                { ipPort = newPort; modified = true; }
            void setTransferType(const DataTransferType newType)    { xferType = newType; modified = true; }
            void setDeviceProvidedID(const std::string newID)       { if(!newID.empty()) { deviceProvidedID = newID; modified = true; } }
            void setDeviceName(const std::string newName)           { if(!newName.empty()) { deviceName = newName; modified = true; } }
            void setDeviceInfo(const std::string newInfo)           { if(!newInfo.empty()) { deviceInfo = newInfo; modified = true; } }
            
        private:
            UserID deviceOwner;
            IPAddress ipAddress;
            IPPort ipPort;
            DataTransferType xferType;
            std::string deviceProvidedID;
            std::string deviceName;
            std::string deviceInfo;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::DeviceDataContainer> DeviceDataContainerPtr;
}

#endif	/* DEVICEDATACONTAINER_H */

