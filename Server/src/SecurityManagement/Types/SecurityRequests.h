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

#ifndef SECURITYREQUESTS_H
#define	SECURITYREQUESTS_H

#include <string>
#include "../../Common/Types.h"
#include "Types.h"
#include "../Interfaces/Securable.h"
#include "../../InstructionManagement/Types/Types.h"
#include "../../InstructionManagement/Sets/InstructionSet.h"
#include "../Types/Types.h"

using Common_Types::UserID;
using Common_Types::INVALID_USER_ID;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;
using InstructionManagement_Sets::InstructionBasePtr;
using SecurityManagement_Interfaces::Securable;
using SecurityManagement_Types::SecurableComponentType;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::IVData;

namespace SecurityManagement_Types
{
    /**
     * Class representing authorization requests for the <code>SecurityManager</code>.
     */
    class AuthorizationRequest
    {
        public:
            /**
             * Constructs s new user authorization request with the supplied parameters.
             * 
             * @param userID the ID of the user requesting to perform the instruction
             * @param sourceComponent reference to the component making the request (source)
             * @param targetComponent the type of the component that will perform the requested instruction (target)
             * @param instructionPtr the instruction to be authorized
             * 
             * @throw invalid_argument if any of the parameters is not valid
             */
            AuthorizationRequest(UserID userID, const Securable & sourceComponent,
                                 SecurableComponentType targetComponent, const InstructionBasePtr instructionPtr)
            : user(userID), device(INVALID_DEVICE_ID), source(sourceComponent.getComponentType()),
              target(targetComponent), instruction(instructionPtr)
            {
                if(user == INVALID_USER_ID)
                    throw std::invalid_argument("AuthorizationRequest::() > Invalid user ID supplied.");
                
                if(source == SecurableComponentType::INVALID)
                    throw std::invalid_argument("AuthorizationRequest::() > Invalid source component supplied.");
                
                if(target == SecurableComponentType::INVALID)
                    throw std::invalid_argument("AuthorizationRequest::() > Invalid target component supplied.");
                
                if(!instruction)
                    throw std::invalid_argument("AuthorizationRequest::() > Invalid instruction supplied.");
            }
            
            /**
             * Constructs a new device authorization request with the supplied parameters.
             * 
             * @param userID the ID of the user requesting to perform the instruction
             * @param deviceID the ID of the device requesting to perform the instruction
             * @param sourceComponent the component making the request (source)
             * @param targetComponent the type of the component that will perform the requested instruction (target)
             * @param instructionPtr the instruction to be authorized
             * 
             * @throw invalid_argument if any of the parameters is not valid
             */
            AuthorizationRequest(UserID userID, DeviceID deviceID, const Securable & sourceComponent,
                                 SecurableComponentType targetComponent, const InstructionBasePtr instructionPtr)
            : AuthorizationRequest(userID, sourceComponent, targetComponent, instructionPtr)
            {
                if(deviceID == INVALID_DEVICE_ID)
                    throw std::invalid_argument("AuthorizationRequest::() > Invalid device ID supplied.");
                
                device = deviceID;
            }
            
            ~AuthorizationRequest() {}
            
            AuthorizationRequest() = delete;
            AuthorizationRequest(const AuthorizationRequest&) = delete;
            AuthorizationRequest& operator=(const AuthorizationRequest&) = delete;
            
            /** Retrieves the ID of the user associated with the request.\n\n@return the associated user ID */
            UserID getUser() const { return user; }
            /** Retrieves the ID of the device associated with the request (if any).\n\n@return the associated device ID */
            DeviceID getDevice() const { return device; }
            /** Retrieves the type of the source component.\n\n@return the source component type */
            SecurableComponentType getSource() const { return source; }
            /** Retrieves the type of the target component.\n\n@return the target component type */
            SecurableComponentType getTarget() const { return target; }
            /** Retrieves the instruction that needs to be authorized.\n\n@return the instruction to be authorized */
            InstructionBasePtr getInstruction() const { return instruction; }
            
        private:
            UserID user;
            DeviceID device;
            SecurableComponentType source;
            SecurableComponentType target;
            InstructionBasePtr instruction;
    };
    
    /**
     * Class representing user authentication requests for the <code>SecurityManager</code>.
     */
    class UserAuthenticationRequest
    {
        public:
            /**
             * Constructs a new user authentication request with the supplied parameters.
             * 
             * Note: Authentication requests must always come from a session manager.
             * 
             * @param user the name of the user to be authenticated
             * @param password the raw password supplied by the user
             * @param sourceComponent reference to the component making the request (source)
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            UserAuthenticationRequest(const std::string user, const std::string password, const Securable & sourceComponent)
            : username(user), rawPassword(password), source(sourceComponent.getComponentType())
            {
                if(username.empty())
                    throw std::invalid_argument("UserAuthenticationRequest::() > Empty user name supplied.");
                
                if(rawPassword.empty())
                    throw std::invalid_argument("UserAuthenticationRequest::() > Empty password supplied.");
                
                if(source != SecurableComponentType::SESSION_MANAGER)
                    throw std::invalid_argument("UserAuthenticationRequest::() > Invalid source component supplied.");
            }
            
            ~UserAuthenticationRequest() {}
            
            UserAuthenticationRequest() = delete;
            UserAuthenticationRequest(const UserAuthenticationRequest&) = delete;
            UserAuthenticationRequest& operator=(const UserAuthenticationRequest&) = delete;
            
            /** Retrieves the name of the user associated with the request.\n\n@return the associated username */
            std::string getUsername() const { return username; }
            /** Retrieves the raw password associated with the user.\n\n@return the raw password */
            const std::string & getRawPassword() const { return rawPassword; }
            /** Retrieves the type of the source component.\n\n@return the source component type */
            SecurableComponentType getSource() const { return source; }
            
        private:
            std::string username;
            std::string rawPassword;
            SecurableComponentType source;
    };
    
    /**
     * Class representing device authentication requests for the <code>SecurityManager</code>.
     */
    class DeviceAuthenticationRequest
    {
        public:
            /**
             * Constructs a new device authentication request with the supplied parameters.
             * 
             * Note: Authentication requests must always come from a session manager.
             * 
             * @param deviceID the ID of the device to be authenticated
             * @param password the raw password supplied for the device
             * @param sourceComponent reference to the component making the request (source)
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DeviceAuthenticationRequest(DeviceID deviceID, const std::string password, const Securable & sourceComponent)
            : device(deviceID), rawPassword(password), source(sourceComponent.getComponentType())
            {
                if(device == INVALID_DEVICE_ID)
                    throw std::invalid_argument("DeviceAuthenticationRequest::() > Invalid device ID supplied.");
                
                if(rawPassword.empty())
                    throw std::invalid_argument("DeviceAuthenticationRequest::() > Empty password supplied.");
                
                if(source != SecurableComponentType::SESSION_MANAGER)
                    throw std::invalid_argument("DeviceAuthenticationRequest::() > Invalid source component supplied.");
            }
            
            ~DeviceAuthenticationRequest() {}
            
            DeviceAuthenticationRequest() = delete;
            DeviceAuthenticationRequest(const DeviceAuthenticationRequest&) = delete;
            DeviceAuthenticationRequest& operator=(const DeviceAuthenticationRequest&) = delete;
            
            /** Retrieves the ID of the device associated with the request.\n\n@return the associated device ID */
            DeviceID getDevice() const { return device; }
            /** Retrieves the raw password associated with the device.\n\n@return the raw password */
            const std::string & getRawPassword() const { return rawPassword; }
            /** Retrieves the type of the source component.\n\n@return the source component type */
            SecurableComponentType getSource() const { return source; }
            
        private:
            DeviceID device;
            std::string rawPassword;
            SecurableComponentType source;
    };
    
    /**
     * Class representing derived crypto data generation requests for the <code>SecurityManager</code>.
     */
    class DerivedCryptoDataGenerationRequest
    {
        public:
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The crypto data is derived from the supplied password.
             * 
             * @param password the password from which the data is to be derived
             * @param sourceComponent reference to the component making the request (source)
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DerivedCryptoDataGenerationRequest(const std::string & password, const Securable & sourceComponent)
            : rawPassword(password), source(sourceComponent.getComponentType()), fromExistingData(false)
            {
                if(rawPassword.empty())
                    throw std::invalid_argument("DeviceDerivedCryptoDataGenerationRequest::() > Empty password supplied.");
            }
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The crypto data is derived from the supplied password, IV and salt.
             * 
             * @param password the password from which the data is to be derived
             * @param ivData IV from which to derive the crypto data
             * @param saltData salt from which to derive the key
             * @param sourceComponent reference to the component making the request (source)
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DerivedCryptoDataGenerationRequest(const std::string password, const IVData ivData,
                                               const SaltData saltData, const Securable & sourceComponent)
            : rawPassword(password), iv(ivData), salt(saltData), source(sourceComponent.getComponentType()),
              fromExistingData(true)
            {
                if(rawPassword.empty())
                    throw std::invalid_argument("DeviceDerivedCryptoDataGenerationRequest::() > Empty password supplied.");
            }
            
            ~DerivedCryptoDataGenerationRequest() {}
            
            DerivedCryptoDataGenerationRequest() = delete;
            DerivedCryptoDataGenerationRequest(const DerivedCryptoDataGenerationRequest&) = delete;
            DerivedCryptoDataGenerationRequest& operator=(const DerivedCryptoDataGenerationRequest&) = delete;
            
            /** Retrieves the raw password associated with the request.\n\n@return the raw password */
            const std::string & getRawPassword() const { return rawPassword; }
            /** Retrieves the IV data associated with the request.\n\n@return the IV (if any) */
            const IVData & getIVData() const { return iv; }
            /** Retrieves the salt data associated with the request.\n\n@return the salt (if any) */
            const SaltData & getSaltData() const { return salt; }
            /** Retrieves the type of the source component.\n\n@return the source component type */
            SecurableComponentType getSource() const { return source; }
            
            /**
             * Denotes whether the request is for deriving crypto data from existing IV and salt.
             * 
             * @return <code>true</code>, if existing IV and salt are supplied
             */
            bool deriveFromExistingData() const { return fromExistingData; }
            
        private:
            std::string rawPassword;
            IVData iv;
            SaltData salt;
            SecurableComponentType source;
            bool fromExistingData;
    };
    
    /**
     * Class representing symmetric crypto data generation requests for the <code>SecurityManager</code>.
     */
    class SymmetricCryptoDataGenerationRequest
    {
        public:
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * @param sourceComponent reference to the component making the request (source)
             */
            SymmetricCryptoDataGenerationRequest(const Securable & sourceComponent)
            : source(sourceComponent.getComponentType())
            {}
            
            ~SymmetricCryptoDataGenerationRequest() {}
            
            SymmetricCryptoDataGenerationRequest() = delete;
            SymmetricCryptoDataGenerationRequest(const SymmetricCryptoDataGenerationRequest&) = delete;
            SymmetricCryptoDataGenerationRequest& operator=(const SymmetricCryptoDataGenerationRequest&) = delete;
            
            /** Retrieves the type of the source component.\n\n@return the source component type */
            SecurableComponentType getSource() const { return source; }
            
        private:
            SecurableComponentType source;
    };
}

#endif	/* SECURITYREQUESTS_H */

