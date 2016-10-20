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
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"
#include "../../SecurityManagement/Types/Types.h"
#include "../../NetworkManagement/Types/Types.h"

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
using SecurityManagement_Types::KeyExchangeType;
using NetworkManagement_Types::PeerType;

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
             * @param type device type
             */
            DeviceDataContainer(std::string name, PasswordData pass, UserID owner,
                                DataTransferType transferType, PeerType type) 
            : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::DEVICE),
              deviceOwner(owner), commandIPAddress(INVALID_IP_ADDRESS),
              commandIPPort(INVALID_IP_PORT), dataIPAddress(INVALID_IP_ADDRESS),
              dataIPPort(INVALID_IP_PORT), initIPAddress(INVALID_IP_ADDRESS),
              initIPPort(INVALID_IP_PORT), xferType(transferType),
              deviceProvidedID("UNDEFINED"), deviceName(name),
              password(pass), deviceInfo("UNDEFINED"), isLocked(false),
              timestampLastSuccessfulAuthentication(INVALID_DATE_TIME),
              timestampLastFailedAuthentication(INVALID_DATE_TIME),
              failedAuthenticationAttempts(0), rawPublicKey("UNDEFINED"),
              expectedKeyExchange(KeyExchangeType::INVALID), deviceType(type)
            {}
            
            /**
             * Creates a new device data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id
             * @param providedID
             * @param name
             * @param pass
             * @param owner
             * @param commandAddress
             * @param commandPort
             * @param dataAddress
             * @param dataPort
             * @param transferType
             * @param info
             * @param locked
             * @param lastSuccessfulAuthTime
             * @param lastFailedAuthTime
             * @param failedAuthAttempts
             * @param publicKey
             * @param keyExchangeType
             * @param type
             */
            DeviceDataContainer(DeviceID id, std::string providedID, std::string name,
                                PasswordData pass, UserID owner, IPAddress commandAddress,
                                IPPort commandPort, IPAddress dataAddress, IPPort dataPort,
                                IPAddress initAddress, IPPort initPort,
                                DataTransferType transferType, std::string info, bool locked,
                                Timestamp lastSuccessfulAuthTime, Timestamp lastFailedAuthTime,
                                unsigned int failedAuthAttempts, std::string publicKey,
                                KeyExchangeType keyExchangeType, PeerType type)
            : DataContainer(id, DatabaseObjectType::DEVICE), deviceOwner(owner),
              commandIPAddress(commandAddress), commandIPPort(commandPort),
              dataIPAddress(dataAddress), dataIPPort(dataPort), initIPAddress(initAddress),
              initIPPort(initPort), xferType(transferType),
              deviceProvidedID(providedID), deviceName(name),
              password(pass), deviceInfo(info), isLocked(locked),
              timestampLastSuccessfulAuthentication(lastSuccessfulAuthTime),
              timestampLastFailedAuthentication(lastFailedAuthTime),
              failedAuthenticationAttempts(failedAuthAttempts),
              rawPublicKey(publicKey), expectedKeyExchange(keyExchangeType),
              deviceType(type)
            {}
            
            DeviceDataContainer() = delete;
            DeviceDataContainer(const DeviceDataContainer&) = default;
            ~DeviceDataContainer() = default;
            DeviceDataContainer& operator=(const DeviceDataContainer&) = default;
            
            DeviceID getDeviceID()              const { return containerID; }
            UserID getDeviceOwner()             const { return deviceOwner; }
            IPAddress getDeviceCommandAddress() const { return commandIPAddress; }
            IPPort getDeviceCommandPort()       const { return commandIPPort; }
            IPAddress getDeviceDataAddress()    const { return dataIPAddress; }
            IPPort getDeviceDataPort()          const { return dataIPPort; }
            IPAddress getDeviceInitAddress()    const { return initIPAddress; }
            IPPort getDeviceInitPort()          const { return initIPPort; }
            DataTransferType getTransferType()  const { return xferType; }
            std::string getDeviceProvidedID()   const { return deviceProvidedID; }
            std::string getDeviceName()         const { return deviceName; }
            std::string getDeviceInfo()         const { return deviceInfo; }
            bool isDeviceLocked()               const { return isLocked; }
            std::string getRawPublicKey()       const { return rawPublicKey; }
            KeyExchangeType getExpectedKeyExhange()                 const { return expectedKeyExchange; }
            PeerType getDeviceType()                                const { return deviceType; }
            Timestamp getLastSuccessfulAuthenticationTimestamp()    const { return timestampLastSuccessfulAuthentication; }
            Timestamp getLastFailedAuthenticationTimestamp()        const { return timestampLastFailedAuthentication; }
            unsigned int getFailedAuthenticationAttempts()          const { return failedAuthenticationAttempts; }
            bool passwordsMatch(const PasswordData & otherPassword) const { return (password == otherPassword); }
            const PasswordData & getPasswordData()                  const { return password; }
            
            const SaltData getPasswordSalt(SaltSize size) const
            {
                return ((size >= password.size() || size == 0) ? SaltData() : SaltData(password.data(), size));
            }
            
            void resetPassword(const PasswordData & newPassword)    { if(newPassword.size() > 0) { password = newPassword; modified = true; } }
            void setDeviceCommandAddress(const IPAddress newAddress){ commandIPAddress = newAddress; modified = true; }
            void setDeviceCommandPort(const IPPort newPort)         { commandIPPort = newPort; modified = true; }
            void setDeviceDataAddress(const IPAddress newAddress)   { dataIPAddress = newAddress; modified = true; }
            void setDeviceDataPort(const IPPort newPort)            { dataIPPort = newPort; modified = true; }
            void setDeviceInitAddress(const IPAddress newAddress)   { initIPAddress = newAddress; modified = true; }
            void setDeviceInitPort(const IPPort newPort)            { initIPPort = newPort; modified = true; }
            void setTransferType(const DataTransferType newType)    { xferType = newType; modified = true; }
            void setDeviceProvidedID(const std::string newID)       { if(!newID.empty()) { deviceProvidedID = newID; modified = true; } }
            void setDeviceName(const std::string newName)           { if(!newName.empty()) { deviceName = newName; modified = true; } }
            void setDeviceInfo(const std::string newInfo)           { if(!newInfo.empty()) { deviceInfo = newInfo; modified = true; } }
            void setLockedState(const bool locked)                  { isLocked = locked; modified = true; }
            
            void resetRawPublicKey(const std::string & publicKey, KeyExchangeType exchangeType)
            {
                rawPublicKey = publicKey;
                expectedKeyExchange = exchangeType;
            }
            
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
            
            void resetFailedAuthenticationAttempts()
            {
                failedAuthenticationAttempts = 0;
                modified = true;
            }
            
        private:
            UserID deviceOwner;
            IPAddress commandIPAddress;
            IPPort commandIPPort;
            IPAddress dataIPAddress;
            IPPort dataIPPort;
            IPAddress initIPAddress;
            IPPort initIPPort;
            DataTransferType xferType;
            std::string deviceProvidedID;
            std::string deviceName;
            PasswordData password;
            std::string deviceInfo;
            bool isLocked;
            Timestamp timestampLastSuccessfulAuthentication;
            Timestamp timestampLastFailedAuthentication;
            unsigned int failedAuthenticationAttempts;
            std::string rawPublicKey;
            KeyExchangeType expectedKeyExchange;
            PeerType deviceType;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::DeviceDataContainer> DeviceDataContainerPtr;
}

#endif	/* DEVICEDATACONTAINER_H */

