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
#include "../../Common/Types.h"
#include "../../SecurityManagement/Types/Types.h"

using DatabaseManagement_Types::DataTransferType;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::IPAddress;
using Common_Types::IPPort;
using Common_Types::Timestamp;
using Common_Types::INVALID_DATE_TIME;
using Common_Types::INVALID_IP_ADDRESS;
using Common_Types::INVALID_IP_PORT;
using SecurityManagement_Types::SaltSize;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::PasswordData;

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
             * @param pass device password
             * @param owner device owner
             * @param transferType default transfer type
             */
            DeviceDataContainer(std::string name, PasswordData pass, UserID owner, DataTransferType transferType) 
                : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::DEVICE),
                  deviceOwner(owner), ipAddress(INVALID_IP_ADDRESS), ipPort(INVALID_IP_PORT),
                  xferType(transferType), deviceProvidedID("UNDEFINED"), deviceName(name),
                  password(pass), deviceInfo("UNDEFINED"), isLocked(false),
                  timestampLastSuccessfulAuthentication(INVALID_DATE_TIME),
                  timestampLastFailedAuthentication(INVALID_DATE_TIME), failedAuthenticationAttempts(0)
            {}
            
            /**
             * Creates a new device data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id device ID
             * @param providedID ID provided by the device itself
             * @param name device name
             * @param pass device password
             * @param owner device owner
             * @param address device IP address
             * @param port device IP port
             * @param transferType default transfer type
             * @param info device info
             * @param locked denotes whether the device is locked or not
             * @param lastSuccessfulAuthTime 
             * @param lastFailedAuthTime 
             * @param failedAuthAttempts 
             */
            DeviceDataContainer(DeviceID id, std::string providedID, std::string name, PasswordData pass, UserID owner,
                                IPAddress address, IPPort port, DataTransferType transferType,
                                std::string info, bool locked, Timestamp lastSuccessfulAuthTime,
                                Timestamp lastFailedAuthTime, unsigned int failedAuthAttempts)
                : DataContainer(id, DatabaseObjectType::DEVICE), deviceOwner(owner), ipAddress(address),
                  ipPort(port), xferType(transferType), deviceProvidedID(providedID), deviceName(name),
                  password(pass), deviceInfo(info), isLocked(locked), timestampLastSuccessfulAuthentication(lastSuccessfulAuthTime),
                  timestampLastFailedAuthentication(lastFailedAuthTime), failedAuthenticationAttempts(failedAuthAttempts)
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
            bool isDeviceLocked()              const { return isLocked; }
            Timestamp getLastSuccessfulAuthenticationTimestamp()    const { return timestampLastSuccessfulAuthentication; }
            Timestamp getLastFailedAuthenticationTimestamp()        const { return timestampLastFailedAuthentication; }
            unsigned int getFailedAuthenticationAttempts()          const { return failedAuthenticationAttempts; }
            bool passwordsMatch(const PasswordData & otherPassword) const{ return (password == otherPassword); }
            const PasswordData & getPasswordData()                  const { return password; }
            
            const SaltData getPasswordSalt(SaltSize size) const
            {
                return ((size >= password.size() || size == 0) ? SaltData() : SaltData(password.data(), size));
            }
            
            void resetPassword(const PasswordData & newPassword)    { if(newPassword.size() > 0) { password = newPassword; modified = true; } }
            void setDeviceAddress(const IPAddress newAddress)       { ipAddress = newAddress; modified = true; }
            void setDevicePort(const IPPort newPort)                { ipPort = newPort; modified = true; }
            void setTransferType(const DataTransferType newType)    { xferType = newType; modified = true; }
            void setDeviceProvidedID(const std::string newID)       { if(!newID.empty()) { deviceProvidedID = newID; modified = true; } }
            void setDeviceName(const std::string newName)           { if(!newName.empty()) { deviceName = newName; modified = true; } }
            void setDeviceInfo(const std::string newInfo)           { if(!newInfo.empty()) { deviceInfo = newInfo; modified = true; } }
            void setLockedState(const bool locked)                  { isLocked = locked; modified = true; }
            
            void setLastSuccessfulAuthenticationTimestamp()
            {
                timestampLastSuccessfulAuthentication = boost::posix_time::second_clock::universal_time();
                timestampLastFailedAuthentication = INVALID_DATE_TIME;
                failedAuthenticationAttempts = 0;
                modified = true;
            }
            
            void setLastFailedAuthenticationTimestamp()
            {
                timestampLastFailedAuthentication = boost::posix_time::second_clock::universal_time();
                ++failedAuthenticationAttempts;
                modified = true;
            }
            
        private:
            UserID deviceOwner;
            IPAddress ipAddress;
            IPPort ipPort;
            DataTransferType xferType;
            std::string deviceProvidedID;
            std::string deviceName;
            PasswordData password;
            std::string deviceInfo;
            bool isLocked;
            Timestamp timestampLastSuccessfulAuthentication;
            Timestamp timestampLastFailedAuthentication;
            unsigned int failedAuthenticationAttempts;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::DeviceDataContainer> DeviceDataContainerPtr;
}

#endif	/* DEVICEDATACONTAINER_H */

