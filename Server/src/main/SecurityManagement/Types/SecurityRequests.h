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

using Common_Types::UserID;
using Common_Types::INVALID_USER_ID;
using Common_Types::DeviceID;
using Common_Types::INVALID_DEVICE_ID;
using InstructionManagement_Sets::InstructionBasePtr;
using SecurityManagement_Interfaces::Securable;
using SecurityManagement_Types::SecurableComponentType;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::IVData;
using SecurityManagement_Types::ECDHPrivateKey;
using SecurityManagement_Types::ECDHPublicKey;
using SecurityManagement_Types::EllipticCurveType;

namespace SecurityManagement_Types
{
    /**
     * Class representing authorization requests for the <code>SecurityManager</code>.
     */
    class AuthorizationRequest
    {
        public:
            /**
             * Constructs a new user authorization request with the supplied parameters.
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
             * @param sourceComponent reference to the component making the request (source)
             * @param password the password from which the data is to be derived
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DerivedCryptoDataGenerationRequest(const Securable & sourceComponent, const std::string & password)
            : rawPassword(password), source(sourceComponent.getComponentType()), fromExistingData(false),
              iterationsCount(0), cipher(SymmetricCipherType::INVALID),
              mode(AuthenticatedSymmetricCipherModeType::INVALID)
            {
                if(rawPassword.empty())
                    throw std::invalid_argument("DeviceDerivedCryptoDataGenerationRequest::() > Empty password supplied.");
            }
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The crypto data is derived from the supplied password, IV and salt.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param password the password from which the data is to be derived
             * @param ivData IV from which to derive the crypto data
             * @param saltData salt from which to derive the key
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DerivedCryptoDataGenerationRequest(const Securable & sourceComponent, const std::string password,
                                               const IVData ivData, const SaltData saltData)
            : rawPassword(password), iv(ivData), salt(saltData), source(sourceComponent.getComponentType()),
              fromExistingData(true), iterationsCount(0), cipher(SymmetricCipherType::INVALID),
              mode(AuthenticatedSymmetricCipherModeType::INVALID)
            {
                if(rawPassword.empty())
                    throw std::invalid_argument("DeviceDerivedCryptoDataGenerationRequest::() > Empty password supplied.");
            }
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The crypto key is derived from the supplied password, IV and salt.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param password the password from which the data is to be derived
             * @param ivData IV from which to derive the crypto data
             * @param saltData salt from which to derive the key
             * @param iterations number of iterations to be used by the PBKD function
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * 
             * @throw invalid_argument if any of the supplied arguments are invalid
             */
            DerivedCryptoDataGenerationRequest(const Securable & sourceComponent, const std::string password,
                                               const IVData ivData, const SaltData saltData, unsigned int iterations,
                                               SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
            : rawPassword(password), iv(ivData), salt(saltData), source(sourceComponent.getComponentType()),
              fromExistingData(true), iterationsCount(iterations), cipher(cipher), mode(mode)
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
            /** Retrieves the number of iterations for the PBKD function.\n\n@return the iterations count*/
            unsigned int getIterationsCount() const { return iterationsCount; }
            
            /**
             * Retrieves the supplied cipher, if any
             * 
             * @return the cipher or <code>INVALID</code>, if it is not available
             */
            SymmetricCipherType getCipher() const { return cipher; }
            
            /**
             * Retrieves the supplied symmetric cipher mode, if any.
             * 
             * @return the cipher mode or <code>INVALID</code>, if it is not available
             */
            AuthenticatedSymmetricCipherModeType getMode() const { return mode; }
            
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
            unsigned int iterationsCount;
            SymmetricCipherType cipher;
            AuthenticatedSymmetricCipherModeType mode;
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
             * Note: The request is for generating new crypto data with the default
             * cipher/mode configuration.
             * 
             * @param sourceComponent reference to the component making the request (source)
             */
            explicit SymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent)
            : SymmetricCryptoDataGenerationRequest
              (sourceComponent, true, true, SymmetricCipherType::INVALID,
               AuthenticatedSymmetricCipherModeType::INVALID, nullptr, nullptr)
            {}
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating crypto data for the supplied key/IV
             * pair with the default cipher/mode configuration.
             * 
             * @param sourceComponent reference to the component making the request (source) 
             * @param key the key to be used
             * @param iv the IV to be used
             */
            SymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, const KeyData & key, const IVData & iv)
            : SymmetricCryptoDataGenerationRequest
              (sourceComponent, true, false, SymmetricCipherType::INVALID,
               AuthenticatedSymmetricCipherModeType::INVALID, &key, &iv)
            {}
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating new crypto data with the specified
             * cipher/mode configuration.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             */
            SymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, SymmetricCipherType cipher,
             AuthenticatedSymmetricCipherModeType mode)
            : SymmetricCryptoDataGenerationRequest
              (sourceComponent, false, true, cipher, mode, nullptr, nullptr)
            {}
            
            /**
             * Constructs a new crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating crypto data for the supplied key/IV
             * pair with the specified cipher/mode configuration.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param key the key to be used
             * @param iv the IV to be used
             */
            SymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, SymmetricCipherType cipher,
             AuthenticatedSymmetricCipherModeType mode, const KeyData & key,
             const IVData & iv)
            : SymmetricCryptoDataGenerationRequest
              (sourceComponent, false, false, cipher, mode, &key, &iv)
            {}
            
            ~SymmetricCryptoDataGenerationRequest() {}
            
            SymmetricCryptoDataGenerationRequest() = delete;
            SymmetricCryptoDataGenerationRequest(const SymmetricCryptoDataGenerationRequest&) = delete;
            SymmetricCryptoDataGenerationRequest& operator=(const SymmetricCryptoDataGenerationRequest&) = delete;
            
            /**
             * Retrieves the type of the source component.
             * 
             * @return the source component type
             */
            SecurableComponentType getSource() const { return source; }
            
            /**
             * Denotes whether the default crypto generation parameters should be used.
             * 
             * @return <code>true</code>, if the default crypto generation params are to be used
             */
            bool useDefaultParameters() const { return shouldUseDefaultParameters; }
            
            /**
             * Denotes whether the new crypto data is to be generated or if an
             * existing key/IV pair is to be used.
             * 
             * @return <code>true</code>, if new crypto data is to be generated
             */
            bool createNewData() const { return shouldCreateNewData; }
            
            /**
             * Retrieves the supplied cipher, if any
             * 
             * @return the cipher or <code>INVALID</code>, if it is not available
             */
            SymmetricCipherType getCipher() const { return cipher; }
            
            /**
             * Retrieves the supplied symmetric cipher mode, if any.
             * 
             * @return the cipher mode or <code>INVALID</code>, if it is not available
             */
            AuthenticatedSymmetricCipherModeType getMode() const { return mode; }
            
            /**
             * Retrieves a pointer to the supplied key, if any.
             * 
             * @return the key pointer or <code>nullptr</code>, if it is not available
             */
            const KeyData * getKey() const { return key; }
            
            /**
             * Retrieves a pointer to the supplied IV, if any.
             * 
             * @return the IV pointer or <code>nullptr</code>, if it is not available
             */
            const IVData * getIV() const { return iv; }
            
        private:
            SymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, bool useDefault, bool createNew,
             SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode,
             const KeyData * key, const IVData * iv)
            : source(sourceComponent.getComponentType()), shouldUseDefaultParameters(useDefault),
              shouldCreateNewData(createNew), cipher(cipher), mode(mode), key(key), iv(iv)
            {}
            
            SecurableComponentType source;
            bool shouldUseDefaultParameters;
            bool shouldCreateNewData;
            SymmetricCipherType cipher;
            AuthenticatedSymmetricCipherModeType mode;
            const KeyData * key;
            const IVData * iv;
    };
    
    /**
     * Class representing Elliptic Curve Diffie-Hellman-based symmetric crypto data
     * generation requests for the <code>SecurityManager</code>.
     */
    class ECDHSymmetricCryptoDataGenerationRequest
    {
        public:
            /**
             * Constructs a new EC DH crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating new crypto data with the default
             * cipher/mode/curve configuration.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param pvKey the local private key to be used
             * @param pbKey the remote public key to be used
             */
            ECDHSymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, const ECDHPrivateKey & pvKey,
             const ECDHPublicKey & pbKey)
            : ECDHSymmetricCryptoDataGenerationRequest
              (sourceComponent, true, true, SymmetricCipherType::INVALID,
               AuthenticatedSymmetricCipherModeType::INVALID, &pvKey, &pbKey, nullptr)
            {}
            
            /**
             * Constructs a new EC DH crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating crypto data with the supplied IV
             * and the default cipher/mode/curve configuration.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param pvKey the local private key to be used
             * @param pbKey the remote public key to be used
             * @param iv the IV to be used
             */
            ECDHSymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, const ECDHPrivateKey & pvKey,
             const ECDHPublicKey & pbKey, const IVData & iv)
            : ECDHSymmetricCryptoDataGenerationRequest
              (sourceComponent, true, false, SymmetricCipherType::INVALID,
               AuthenticatedSymmetricCipherModeType::INVALID, &pvKey, &pbKey, &iv)
            {}
            
            /**
             * Constructs a new EC DH crypto data request with the supplied parameters.
             * 
             * Note: The request is for generating new crypto data with the
             * specified cipher/mode configuration and the default elliptic curve.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param pvKey the local private key to be used
             * @param pbKey the remote public key to be used
             */
            ECDHSymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, SymmetricCipherType cipher,
             AuthenticatedSymmetricCipherModeType mode, const ECDHPrivateKey & pvKey,
             const ECDHPublicKey & pbKey)
            : ECDHSymmetricCryptoDataGenerationRequest
              (sourceComponent, false, true, cipher, mode, &pvKey, &pbKey, nullptr)
            {}
            
            /**
             * Constructs a new EC DH crypto data request with the supplied parameters.
             * 
             * Note 1: The request is for generating crypto data with supplied IV
             * and the specified cipher/mode configuration.
             * 
             * Note 2: The default elliptic curve will be used.
             * 
             * @param sourceComponent reference to the component making the request (source)
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param pvKey the local private key to be used
             * @param pbKey the remote public key to be used
             * @param iv the IV to be used
             */
            ECDHSymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, SymmetricCipherType cipher,
             AuthenticatedSymmetricCipherModeType mode, const ECDHPrivateKey & pvKey,
             const ECDHPublicKey & pbKey, const IVData & iv)
            : ECDHSymmetricCryptoDataGenerationRequest
              (sourceComponent, false, false, cipher, mode, &pvKey, &pbKey, &iv)
            {}
            
            ~ECDHSymmetricCryptoDataGenerationRequest() {}
            
            ECDHSymmetricCryptoDataGenerationRequest() = delete;
            ECDHSymmetricCryptoDataGenerationRequest(const ECDHSymmetricCryptoDataGenerationRequest&) = delete;
            ECDHSymmetricCryptoDataGenerationRequest& operator=(const ECDHSymmetricCryptoDataGenerationRequest&) = delete;
            
            /**
             * Retrieves the type of the source component.
             * 
             * @return the source component type
             */
            SecurableComponentType getSource() const { return source; }
            
            /**
             * Denotes whether the default crypto generation parameters should be used.
             * 
             * @return <code>true</code>, if the default crypto generation params are to be used
             */
            bool useDefaultParameters() const { return shouldUseDefaultParameters; }
            
            /**
             * Denotes whether the new crypto data is to be generated or if an
             * existing key/IV pair is to be used.
             * 
             * @return <code>true</code>, if new crypto data is to be generated
             */
            bool createNewData() const { return shouldCreateNewData; }
            
            /**
             * Retrieves the supplied cipher, if any
             * 
             * @return the cipher or <code>INVALID</code>, if it is not available
             */
            SymmetricCipherType getCipher() const { return cipher; }
            
            /**
             * Retrieves the supplied symmetric cipher mode, if any.
             * 
             * @return the cipher mode or <code>INVALID</code>, if it is not available
             */
            AuthenticatedSymmetricCipherModeType getMode() const { return mode; }
            
            /**
             * Retrieves a pointer to the supplied IV, if any.
             * 
             * @return the IV pointer or <code>nullptr</code>, if it is not available
             */
            const IVData * getIV() const { return iv; }
            
            /**
             * Retrieves a pointer to the supplied PRIVATE key, if any.
             * 
             * @return the key pointer or <code>nullptr</code>, if it is not available
             */
            const ECDHPrivateKey * getPrivateKey() const { return privateKey; }
            
            /**
             * Retrieves a pointer to the supplied PUBLIC key, if any.
             * 
             * @return the key pointer or <code>nullptr</code>, if it is not available
             */
            const ECDHPublicKey * getPublicKey() const { return publicKey; }
            
        private:
            ECDHSymmetricCryptoDataGenerationRequest
            (const Securable & sourceComponent, bool useDefault, bool createNew,
             SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode,
             const ECDHPrivateKey * pvKey, const ECDHPublicKey * pbKey, const IVData * iv)
            : source(sourceComponent.getComponentType()), shouldUseDefaultParameters(useDefault),
              shouldCreateNewData(createNew), cipher(cipher), mode(mode), iv(iv),
              privateKey(pvKey), publicKey(pbKey)
            {}
            
            SecurableComponentType source;
            bool shouldUseDefaultParameters;
            bool shouldCreateNewData;
            SymmetricCipherType cipher;
            AuthenticatedSymmetricCipherModeType mode;
            const IVData * iv;
            const ECDHPrivateKey * privateKey;
            const ECDHPublicKey * publicKey;
    };
}

#endif	/* SECURITYREQUESTS_H */

