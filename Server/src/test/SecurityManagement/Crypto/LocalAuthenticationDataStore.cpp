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

#include <boost/unordered_map.hpp>
#include <boost/uuid/random_generator.hpp>
#include "../../BasicSpec.h"
#include "../../../main/SecurityManagement/Types/Types.h"
#include "../../../main/SecurityManagement/Crypto/LocalAuthenticationDataStore.h"

using SecurityManagement_Crypto::LocalAuthenticationDataStore;
using SecurityManagement_Types::LocalPeerAuthenticationEntry;

SCENARIO("A data store can store, update and retrieve authentication data",
         "[LocalAuthenticationDataStore][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("An empty LocalAuthenticationDataStore and a set of authentication data")
    {
        LocalAuthenticationDataStore testStore;
        
        auto localDeviceID_1 = boost::uuids::random_generator()();
        auto localDeviceID_2 = boost::uuids::random_generator()();
        auto localDeviceID_3 = boost::uuids::random_generator()();
        
        //remote id -> password
        LocalPeerAuthenticationEntry data_1{boost::uuids::random_generator()(), "password_1"};
        LocalPeerAuthenticationEntry data_2{boost::uuids::random_generator()(), "password_2"};
        LocalPeerAuthenticationEntry data_3{boost::uuids::random_generator()(), "password_3"};
        
        WHEN("data is stored")
        {
            testStore.addData(localDeviceID_1, data_1);
            testStore.addData(localDeviceID_2, data_2);
            testStore.addData(localDeviceID_3, data_3);
            
            CHECK(testStore.getAllDataForStorage().size() == 3);
            
            THEN("it can be successfully updated and retrieved")
            {
                LocalPeerAuthenticationEntry updatedData{boost::uuids::random_generator()(), "new_password_1"};
                
                testStore.updateData(localDeviceID_1, updatedData);
                
                auto retrievedData_1 = testStore.getData(localDeviceID_1);
                auto retrievedData_2 = testStore.getData(localDeviceID_2);
                auto retrievedData_3 = testStore.getData(localDeviceID_3);
                
                CHECK(retrievedData_1.id == updatedData.id);
                CHECK(retrievedData_1.plaintextPassword == updatedData.plaintextPassword);
                CHECK(retrievedData_2.id == data_2.id);
                CHECK(retrievedData_2.plaintextPassword == data_2.plaintextPassword);
                CHECK(retrievedData_3.id == data_3.id);
                CHECK(retrievedData_3.plaintextPassword == data_3.plaintextPassword);
            }
        }
    }
    
    GIVEN("A LocalAuthenticationDataStore with data in it and a set of additional authentication data")
    {
        auto localDeviceID_1 = boost::uuids::random_generator()();
        auto localDeviceID_2 = boost::uuids::random_generator()();
        auto localDeviceID_3 = boost::uuids::random_generator()();
        auto localDeviceID_4 = boost::uuids::random_generator()();
        auto localDeviceID_5 = boost::uuids::random_generator()();
        
        //remote id -> password
        LocalPeerAuthenticationEntry data_1{boost::uuids::random_generator()(), "password_1"};
        LocalPeerAuthenticationEntry data_2{boost::uuids::random_generator()(), "password_2"};
        LocalPeerAuthenticationEntry data_3{boost::uuids::random_generator()(), "password_3"};
        LocalPeerAuthenticationEntry data_4{boost::uuids::random_generator()(), "password_4"};
        LocalPeerAuthenticationEntry data_5{boost::uuids::random_generator()(), "password_5"};
        
        boost::unordered_map<DeviceID, LocalPeerAuthenticationEntry> initData;
        initData.insert(std::pair<DeviceID, LocalPeerAuthenticationEntry>(localDeviceID_4, data_4));
        initData.insert(std::pair<DeviceID, LocalPeerAuthenticationEntry>(localDeviceID_5, data_5));
        
        LocalAuthenticationDataStore testStore(initData);
        CHECK(testStore.getAllDataForStorage().size() == 2);
        
        WHEN("data is stored")
        {
            testStore.addData(localDeviceID_1, data_1);
            testStore.addData(localDeviceID_2, data_2);
            testStore.addData(localDeviceID_3, data_3);
            
            CHECK(testStore.getAllDataForStorage().size() == 5);
            
            THEN("it can be successfully updated and retrieved")
            {
                LocalPeerAuthenticationEntry updatedData{boost::uuids::random_generator()(), "new_password_1"};
                
                testStore.updateData(localDeviceID_1, updatedData);
                auto retrievedData_1 = testStore.getData(localDeviceID_1);
                auto retrievedData_2 = testStore.getData(localDeviceID_2);
                auto retrievedData_3 = testStore.getData(localDeviceID_3);
                auto retrievedData_4 = testStore.getData(localDeviceID_4);
                auto retrievedData_5 = testStore.getData(localDeviceID_5);
                
                CHECK(retrievedData_1.id == updatedData.id);
                CHECK(retrievedData_1.plaintextPassword == updatedData.plaintextPassword);
                CHECK(retrievedData_2.id == data_2.id);
                CHECK(retrievedData_2.plaintextPassword == data_2.plaintextPassword);
                CHECK(retrievedData_3.id == data_3.id);
                CHECK(retrievedData_3.plaintextPassword == data_3.plaintextPassword);
                CHECK(retrievedData_4.id == data_4.id);
                CHECK(retrievedData_4.plaintextPassword == data_4.plaintextPassword);
                CHECK(retrievedData_5.id == data_5.id);
                CHECK(retrievedData_5.plaintextPassword == data_5.plaintextPassword);
            }
        }
    }
}

SCENARIO("A data store fails to store, update or retrieve authentication data when invalid parameters are supplied",
         "[LocalAuthenticationDataStore][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("A LocalAuthenticationDataStore with data in it and a set of additional authentication data")
    {
        auto localDeviceID_4 = boost::uuids::random_generator()();
        auto localDeviceID_5 = boost::uuids::random_generator()();
        
        //remote id -> password
        LocalPeerAuthenticationEntry data_1{boost::uuids::random_generator()(), "password_1"};
        LocalPeerAuthenticationEntry data_2{boost::uuids::random_generator()(), "password_2"};
        LocalPeerAuthenticationEntry data_3{boost::uuids::random_generator()(), "password_3"};
        LocalPeerAuthenticationEntry data_4{boost::uuids::random_generator()(), "password_4"};
        LocalPeerAuthenticationEntry data_5{boost::uuids::random_generator()(), "password_5"};
        
        boost::unordered_map<DeviceID, LocalPeerAuthenticationEntry> initData;
        initData.insert(std::pair<DeviceID, LocalPeerAuthenticationEntry>(localDeviceID_4, data_4));
        initData.insert(std::pair<DeviceID, LocalPeerAuthenticationEntry>(localDeviceID_5, data_5));
        
        LocalAuthenticationDataStore testStore(initData);
        CHECK(testStore.getAllDataForStorage().size() == 2);
        
        WHEN("it is attempted to add data for the same device")
        {
            CHECK_THROWS_AS(testStore.addData(localDeviceID_4, data_1), std::logic_error);
            CHECK_THROWS_AS(testStore.addData(localDeviceID_5, data_2), std::logic_error);
            
            CHECK(testStore.getAllDataForStorage().size() == 2);
            
            THEN("no modification to the existing data is made")
            {
                auto retrievedData_4 = testStore.getData(localDeviceID_4);
                auto retrievedData_5 = testStore.getData(localDeviceID_5);
                
                CHECK(retrievedData_4.id == data_4.id);
                CHECK(retrievedData_4.plaintextPassword == data_4.plaintextPassword);
                CHECK(retrievedData_5.id == data_5.id);
                CHECK(retrievedData_5.plaintextPassword == data_5.plaintextPassword);
            }
        }
        
        WHEN("it is attempted to update data for a device that does not exist")
        {
            CHECK_THROWS_AS(testStore.updateData(boost::uuids::random_generator()(), data_3), std::runtime_error);
            CHECK_THROWS_AS(testStore.updateData(boost::uuids::random_generator()(), data_4), std::runtime_error);
            
            CHECK(testStore.getAllDataForStorage().size() == 2);
            
            THEN("no modification to the existing data is made")
            {
                auto retrievedData_4 = testStore.getData(localDeviceID_4);
                auto retrievedData_5 = testStore.getData(localDeviceID_5);
                
                CHECK(retrievedData_4.id == data_4.id);
                CHECK(retrievedData_4.plaintextPassword == data_4.plaintextPassword);
                CHECK(retrievedData_5.id == data_5.id);
                CHECK(retrievedData_5.plaintextPassword == data_5.plaintextPassword);
            }
        }
        
        WHEN("it is attempted to retrieve data for a device that does not exist")
        {
            THEN("an exception is thrown")
            {
                CHECK_THROWS_AS(testStore.getData(boost::uuids::random_generator()()), std::runtime_error);
                CHECK_THROWS_AS(testStore.getData(boost::uuids::random_generator()()), std::runtime_error);
            }
        }
    }
}