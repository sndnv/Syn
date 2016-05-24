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

#ifndef LOCALAUTHENTICATIONDATASTORE_H
#define	LOCALAUTHENTICATIONDATASTORE_H

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/unordered_map.hpp>

#include "../../Common/Types.h"
#include "../Types/Types.h"
#include "../../Utilities/Strings/Common.h"

using SecurityManagement_Types::LocalPeerAuthenticationEntry;

namespace SecurityManagement_Crypto
{
    /**
     * Data store for local authentication data.
     */
    class LocalAuthenticationDataStore
    {
        public:
            /**
             * Creates a new empty data store.
             */
            LocalAuthenticationDataStore()
            {}
            
            /**
             * Creates a new data store with the supplied data.
             * 
             * @param data the data to be used for the initialization
             */
            explicit LocalAuthenticationDataStore(boost::unordered_map<DeviceID, LocalPeerAuthenticationEntry> data)
            : authenticationData(data)
            {}
            
            /**
             * Clears the data store.
             */
            ~LocalAuthenticationDataStore()
            {
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                authenticationData.clear();
            }
            
            LocalAuthenticationDataStore(const LocalAuthenticationDataStore&) = delete;
            LocalAuthenticationDataStore& operator=(const LocalAuthenticationDataStore&) = delete;
            
            /**
             * Retrieves authentication data for the specified device.
             * 
             * @param deviceID the ID of the device
             * @throw runtime_error if no data can be found for the specified device
             * @return the requested data
             */
            const LocalPeerAuthenticationEntry & getData(const Common_Types::DeviceID & deviceID)
            {
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                auto deviceData = authenticationData.find(deviceID);
                if(deviceData != authenticationData.end())
                {
                    return deviceData->second;
                }
                else
                {
                    throw std::runtime_error("LocalAuthenticationDataStore::getData() > "
                            "Failed to retrieve authentication data for device ["
                            + Utilities::Strings::toString(deviceID) + "].");
                }
            }
            
            /**
             * Updates authentication data for the specified device.
             * 
             * @param deviceID the ID of the device
             * @param entry the data to be updated
             * @throw runtime_error if no data can be found for the specified device
             */
            void updateData(const Common_Types::DeviceID & deviceID, const LocalPeerAuthenticationEntry & entry)
            {
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                auto deviceData = authenticationData.find(deviceID);
                if(deviceData != authenticationData.end())
                {
                    deviceData->second = entry;
                }
                else
                {
                    throw std::runtime_error("LocalAuthenticationDataStore::updateData() > "
                            "Failed to retrieve authentication data for device ["
                            + Utilities::Strings::toString(deviceID) + "].");
                }
            }
            
            /**
             * Adds authentication data for the specified device.
             * 
             * @param deviceID the ID of the device
             * @param entry the data to be added
             * @throw logic_error if an entry already exists for the specified device
             */
            void addData(const Common_Types::DeviceID & deviceID, const LocalPeerAuthenticationEntry & entry)
            {
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                auto deviceData = authenticationData.find(deviceID);
                if(deviceData == authenticationData.end())
                {
                    authenticationData.insert(std::pair<DeviceID, LocalPeerAuthenticationEntry>(deviceID, entry));
                }
                else
                {
                    throw std::logic_error("LocalAuthenticationDataStore::addData() > "
                            "Authentication data found for device ["
                            + Utilities::Strings::toString(deviceID) + "].");
                }
            }
            
            /**
             * Retrieves all data from the store.
             * 
             * Note: Should be used for persistent storage only.
             * 
             * @return the requested data
             */
            const boost::unordered_map<DeviceID, LocalPeerAuthenticationEntry> getAllDataForStorage()
            {
                boost::lock_guard<boost::mutex> dataLock(dataMutex);
                return authenticationData;
            }
            
        private:
            boost::mutex dataMutex;
            boost::unordered_map<DeviceID, LocalPeerAuthenticationEntry> authenticationData;
    };
}

#endif	/* LOCALAUTHENTICATIONDATASTORE_H */

