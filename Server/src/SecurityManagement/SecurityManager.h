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

#ifndef SECURITYMANAGER_H
#define	SECURITYMANAGER_H

#include <atomic>
#include <string>
#include <deque>
#include <vector>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "../Common/Types.h"
#include "../Utilities/Strings/Security.h"
#include "../Utilities/Strings/Common.h"
#include "../Utilities/FileLogger.h"
#include "../Utilities/ThreadPool.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../DatabaseManagement/Containers/UserDataContainer.h"
#include "../DatabaseManagement/Containers/DeviceDataContainer.h"
#include "../InstructionManagement/InstructionDispatcher.h"

#include "Types/Types.h"
#include "Types/Exceptions.h"
#include "Types/SecurityTokens.h"
#include "Types/SecurityRequests.h"
#include "Interfaces/Securable.h"
#include "Crypto/Containers.h"
#include "Crypto/KeyGenerator.h"
#include "Crypto/SaltGenerator.h"
#include "Crypto/HashGenerator.h"
#include "Rules/AuthorizationRules.h"
#include "Rules/AuthenticationRules.h"

//Common
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::Seconds;

//Database and Instruction Management
using SyncServer_Core::DatabaseManager;
using SyncServer_Core::InstructionDispatcher;
using InstructionManagement_Types::InstructionSetType;
using DatabaseManagement_Containers::UserDataContainerPtr;
using DatabaseManagement_Containers::DeviceDataContainerPtr;

//Rules
using SecurityManagement_Rules::NameRule;
using SecurityManagement_Rules::PasswordRule;
using SecurityManagement_Rules::UserAuthorizationRule;

//Tokens
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::AuthorizationToken;
using SecurityManagement_Types::AuthenticationToken;
using SecurityManagement_Types::AuthorizationTokenPtr;
using SecurityManagement_Types::AuthenticationTokenPtr;
using SecurityManagement_Types::AuthorizationTokenPromise;
using SecurityManagement_Types::AuthorizationTokenPromisePtr;
using SecurityManagement_Types::AuthenticationTokenPromise;
using SecurityManagement_Types::AuthenticationTokenPromisePtr;

//Requests
using SecurityManagement_Types::AuthorizationRequest;
using SecurityManagement_Types::UserAuthenticationRequest;
using SecurityManagement_Types::DeviceAuthenticationRequest;
using SecurityManagement_Types::DerivedCryptoDataGenerationRequest;
using SecurityManagement_Types::SymmetricCryptoDataGenerationRequest;


//Exceptions
using SecurityManagement_Types::UserLockedException;
using SecurityManagement_Types::DeviceLockedException;
using SecurityManagement_Types::UserNotFoundException;
using SecurityManagement_Types::DeviceNotFoundException;
using SecurityManagement_Types::InvalidPassswordException;
using SecurityManagement_Types::UnexpectedDeviceException;
using SecurityManagement_Types::InstructionNotAllowedException;
using SecurityManagement_Types::InsufficientUserAccessException;
using SecurityManagement_Types::UserNotAuthenticatedException;

//Crypto
using SecurityManagement_Crypto::HashGenerator;
using SecurityManagement_Crypto::SaltGenerator;
using SecurityManagement_Crypto::KeyGenerator;
using SecurityManagement_Crypto::SymmetricCryptoDataContainer;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPromise;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPromisePtr;

//Misc
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::SaltSize;
using SecurityManagement_Types::RandomData;
using SecurityManagement_Types::RandomDataSize;
using SecurityManagement_Types::PasswordData;
using SecurityManagement_Types::CacheSize;
using SecurityManagement_Types::CacheHits;
using SecurityManagement_Types::CacheEvictionType;
using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::DelayEscalationType;
using SecurityManagement_Types::SecurableComponentType;
using SecurityManagement_Types::MAX_CACHE_HITS;
using SecurityManagement_Types::INVALID_TOKEN_ID;
using SecurityManagement_Types::INVALID_RANDOM_DATA_SIZE;

using SecurityManagement_Interfaces::Securable;

namespace Convert = Utilities::Strings;

namespace SyncServer_Core
{
    /**
     * Class for managing security-related activities.
     * 
     * Note: Requires making <code>CryptoPP::SecBlock</code>'s append operator const:
     * <code>(SecBlock operator+(const SecBlock &t))</code> in <code>secblock.h</code>, 
     * line 342 (Crypto++ version 5.6.2).
     * 
     */
    class SecurityManager
    {
        public:
            /** Parameters structure for holding user and device password hashing configuration. */
            struct PasswordHashingParameters
            {
                /** Constructs a non-empty parameters container. */
                PasswordHashingParameters(SaltSize userPassSalt, SaltSize devicePassSalt,
                HashAlgorithmType userPassHashAlgo, HashAlgorithmType devicePassHashAlgo);
                
                /** Constructs an empty parameters container. */
                PasswordHashingParameters();
                
                /** Salt size for user password hash generation */
                SaltSize userPasswordSaltSize;
                /** Salt size for device password hash generation */
                SaltSize devicePasswordSaltSize;
                /** User password hashing algorithm */
                HashAlgorithmType userPasswordHashAlgorithm;
                /** Device password hashing algorithm */
                HashAlgorithmType devicePasswordHashAlgorithm;
                /** Denotes whether the parameters have been set */
                bool isEmpty;
            };
            
            /** Parameters structure for holding <code>KeyGenerator</code> configuration. */
            struct KeyGeneratorParameters
            {
                /** Parameters for derived key generation */
                KeyGenerator::DerivedKeysParameters derivedKeyParams;
                /** Parameters for symmetric key generation */
                KeyGenerator::SymmetricKeysParameters symKeyParams;
                /** Parameters for asymmetric key generation */
                KeyGenerator::AsymmetricKeysParameters asymKeyParams;
            };
            
            /** Parameters structure for holding failed authentication delay handling configuration. */
            struct FailedAuthenticationDelayParameters
            {
                /**
                 * The base delay time (in seconds).
                 * 
                 * The actual delay time also depends on the escalation type.
                 */
                Seconds delayBase;
                
                /**
                 * Delay escalation type.
                 * 
                 * The actual delay time is calculated the following way:
                 * 
                 * "delay time" = X^Y
                 * 
                 * ... where Y is based on the escalation type:<br>
                 * CONSTANT  -> Y = 1<br>
                 * LINEAR    -> Y = N<br>
                 * QUADRATIC -> Y = N^2<br>
                 * 
                 * ... and 'X' is the delay base
                 * 
                 * ... and 'N' is the number of failed authentication attempts.
                 * 
                 * Examples:<br>
                 *  Base = 2 seconds<br>
                 *  Escalation type = LINEAR<br>
                 *  2^1 = 2 seconds of delay after 1st failed attempt<br>
                 *  2^2 = 4 seconds of delay after 2nd failed attempt<br>
                 *  2^3 = 8 seconds of delay after 5th failed attempt<br>
                 */
                DelayEscalationType escalationType;
                
                /**
                 * The maximum number of failed attempts, before permanently locking
                 * the user/device.
                 * 
                 * 0 == Unlimited attempts
                 */
                unsigned int maxAttempts;
                
                /**
                 * The number of failed attempts to ignore, before enforcing a delay.
                 */
                unsigned int ignoredAttempts;
            };
            
            /** Parameters structure holding <code>SecurityManager</code> configuration. */
            struct SecurityManagerParameters
            {
                /** Number of threads to create in the internal thread pool */
                unsigned long threadPoolSize;
                /** Reference to a valid database manager instance */
                DatabaseManager & databaseManager;
                /** Reference to a valid instruction dispatcher instance */
                InstructionDispatcher & instructionDispatcher;
                /** Maximum allowed user data entries in the cache */
                CacheSize maxUserDataEntries;
                /** Maximum allowed device data entries in the cache */
                CacheSize maxDeviceDataEntries;
                /** Cache eviction type for user data entries */
                CacheEvictionType userEviction;
                /** Cache eviction type for device data entries */
                CacheEvictionType deviceEviction;
                /** Signature size for authorization tokens */
                RandomDataSize authorizationTokenSignatureSize;
                /** Signature size for authentication tokens */
                RandomDataSize authenticationTokenSignatureSize;
                /** Authentication token validity duration (in seconds) */
                Seconds authenticationTokenValidityDuration;
                /** List of user name validity rules */
                std::vector<const NameRule *> userNameRules;
                /** List of device name validity rules */
                std::vector<const NameRule *> deviceNameRules;
                /** List of user password validity rules */
                std::vector<const PasswordRule *> userPasswordRules;
                /** List of device password validity rules */
                std::vector<const PasswordRule *> devicePasswordRules;
                /** Current user and device password hashing parameters */
                PasswordHashingParameters currentPasswordHashingConfiguration;
                /** Previous user and device password hashing parameters (if any) */
                PasswordHashingParameters previousPasswordHashingConfiguration;
                /** Key generator parameters */
                KeyGeneratorParameters keyGeneratorConfig;
                /** Failed authentication delay parameters for users */
                FailedAuthenticationDelayParameters userDelayConfig;
                /** Failed authentication delay parameters for devices */
                FailedAuthenticationDelayParameters deviceDelayConfig;
            };
            
            /**
             * Constructs a new security manager object with the specified configuration.
             * 
             * Note 1: Ownership of all rule objects is transferred from the caller to the
             * <code>SecurityManager</code> and it becomes responsible for their life-cycle.
             * The rules will be deleted (and memory freed) either when the manager
             * is destroyed or when a rule is explicitly removed with a call
             * to the appropriate method.
             * 
             * Note 2: The above is only true if the constructor completes successfully.
             * Should an exception be thrown, the caller retains ownership of the rules.
             * 
             * @param params the manager configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             * 
             * @throw invalid_argument if the specified cache eviction types are not valid,
             * or if any of the supplied rules are not valid, or if the specified hashing 
             * algorithms are not valid, or if the specified authentication delay parameters
             * are not valid
             */
            SecurityManager(const SecurityManagerParameters & params, Utilities::FileLogger * debugLogger = nullptr);
            
            /**
             * Clears all data structures and frees all memory associated with the rules.
             */
            ~SecurityManager();
            
            SecurityManager() = delete;                                   //No default constructor
            SecurityManager(const SecurityManager&) = delete;             //Copying not allowed (pass/access only by reference/pointer)
            SecurityManager& operator=(const SecurityManager&) = delete;  //Copying not allowed (pass/access only by reference/pointer)
            
            /**
             * Posts the supplied authorization request for asynchronous processing.
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @return a smart pointer to the promise that will have the result of the operation
             */
            AuthorizationTokenPromisePtr postRequest(const AuthorizationRequest & request)
            {
                AuthorizationTokenPromisePtr newPromise(new AuthorizationTokenPromise());
                threadPool.assignTask(boost::bind(&SyncServer_Core::SecurityManager::processAuthorizationRequest,
                                                  this, boost::ref(request), newPromise));
                
                return newPromise;
            }
            
            /**
             * Posts the supplied user authentication request for asynchronous processing.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>UserNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * Note 1: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * Note 2: All tokens must be removed with a call to <code>removeAuthenticationToken()</code>
             * after they become of no further use.
             * 
             * @param request reference to the request to be processed
             * @return a smart pointer to the promise that will have the result of the operation
             */
            AuthenticationTokenPromisePtr postRequest(const UserAuthenticationRequest & request)
            {
                AuthenticationTokenPromisePtr newPromise(new AuthenticationTokenPromise());
                threadPool.assignTask(boost::bind(&SyncServer_Core::SecurityManager::processUserAuthenticationRequest,
                                                  this, boost::ref(request), newPromise));
                
                return newPromise;
            }
            
            /**
             * Posts the supplied device authentication request for asynchronous processing.
             * 
             * Note1 : The caller must ensure the request is not destroyed until processing is finished.
             * 
             * Note 2: All tokens must be removed with a call to <code>removeAuthenticationToken()</code>
             * after they become of no further use.
             * 
             * @param request reference to the request to be processed
             * @return a smart pointer to the promise that will have the result of the operation
             */
            AuthenticationTokenPromisePtr postRequest(const DeviceAuthenticationRequest & request)
            {
                AuthenticationTokenPromisePtr newPromise(new AuthenticationTokenPromise());
                threadPool.assignTask(boost::bind(&SyncServer_Core::SecurityManager::processDeviceAuthenticationRequest,
                                                  this, boost::ref(request), newPromise));
                
                return newPromise;
            }
            
            /**
             * Posts the supplied derived crypto data generation request for asynchronous processing.
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @return a smart pointer to the promise that will have the result of the operation
             */
            SymmetricCryptoDataContainerPromisePtr postRequest(const DerivedCryptoDataGenerationRequest & request)
            {
                SymmetricCryptoDataContainerPromisePtr newPromise(new SymmetricCryptoDataContainerPromise());
                threadPool.assignTask(boost::bind(&SyncServer_Core::SecurityManager::processDerivedCryptoDataGenerationRequest,
                                                  this, boost::ref(request), newPromise));
                
                return newPromise;
            }
            
            /**
             * Posts the supplied symmetric crypto data generation request for asynchronous processing.
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @return a smart pointer to the promise that will have the result of the operation
             */
            SymmetricCryptoDataContainerPromisePtr postRequest(const SymmetricCryptoDataGenerationRequest & request)
            {
                SymmetricCryptoDataContainerPromisePtr newPromise(new SymmetricCryptoDataContainerPromise());
                threadPool.assignTask(boost::bind(&SyncServer_Core::SecurityManager::processSymmetricCryptoDataGenerationRequest,
                                                  this, boost::ref(request), newPromise));
                
                return newPromise;
            }
            
            /**
             * Registers the supplied <code>Securable</code> component with the manager.
             * 
             * @param component the component to be registered
             * 
             * @throw invalid_argument if the supplied component is of a type that
             * is already registered
             */
            void registerSecurableComponent(Securable & component);
            
            /**
             * Deregisters the component with the specified type from the manager.
             * 
             * @param type the type of the component to be deregistered
             * 
             * @throw invalid_argument if a component of the specified type is not present
             */
            void deregisterSecurableComponent(SecurableComponentType type);
            
            /**
             * Adds the supplied rule to the set that governs user names.
             * 
             * Note: Ownership of the object is transferred from the caller to the
             * <code>SecurityManager</code> and it becomes responsible for its life-cycle.
             * The rule will be deleted (and memory freed) either when the manager
             * is destroyed or when the rule is explicitly removed with a call
             * to the appropriate method.
             * 
             * @param rule pointer the rule to be added (must be allocated with <code>new</code>)
             * @return the ID associated with the rule
             * 
             * @throw invalid_argument if an invalid rule is supplied (nullptr)
             */
            unsigned int addUserNameRule(const NameRule * rule);
            
            /**
             * Removes the user name rule associated with the specified ID.
             * 
             * Note: The memory associated with the rule is freed.
             * 
             * @param ruleID the ID of the rule to be removed
             * 
             * @throw runtime_error if the requested rule is not found
             */
            void removeUserNameRule(unsigned int ruleID);
            
            /**
             * Adds the supplied rule to the set that governs device names.
             * 
             * Note: Ownership of the object is transferred from the caller to the
             * <code>SecurityManager</code> and it becomes responsible for its life-cycle.
             * The rule will be deleted (and memory freed) either when the manager
             * is destroyed or when the rule is explicitly removed with a call
             * to the appropriate method.
             * 
             * @param rule pointer to the rule to be added (must be allocated with <code>new</code>)
             * @return the ID associated with the rule
             * 
             * @throw invalid_argument if an invalid rule is supplied (nullptr)
             */
            unsigned int addDeviceNameRule(const NameRule * rule);
            
            /**
             * Removes the device name rule associated with the specified ID.
             * 
             * Note: The memory associated with the rule is freed.
             * 
             * @param ruleID the ID of the rule to be removed
             * 
             * @throw runtime_error if the requested rule is not found
             */
            void removeDeviceNameRule(unsigned int ruleID);
            
            /**
             * Adds the supplied rule to the set that governs user passwords.
             * 
             * Note: Ownership of the object is transferred from the caller to the
             * <code>SecurityManager</code> and it becomes responsible for its life-cycle.
             * The rule will be deleted (and memory freed) either when the manager
             * is destroyed or when the rule is explicitly removed with a call
             * to the appropriate method.
             * 
             * @param rule pointer to the rule to be added (must be allocated with <code>new</code>)
             * @return the ID associated with the rule
             * 
             * @throw invalid_argument if an invalid rule is supplied (nullptr)
             */
            unsigned int addUserPasswordRule(const PasswordRule * rule);
            
            /**
             * Removes the user password rule associated with the specified ID.
             * 
             * Note: The memory associated with the rule is freed.
             * 
             * @param ruleID the ID of the rule to be removed
             * 
             * @throw runtime_error if the requested rule is not found
             */
            void removeUserPasswordRule(unsigned int ruleID);
            
            /**
             * Adds the supplied rule to the set that governs device passwords.
             * 
             * Note: Ownership of the object is transferred from the caller to the
             * <code>SecurityManager</code> and it becomes responsible for its life-cycle.
             * The rule will be deleted (and memory freed) either when the manager
             * is destroyed or when the rule is explicitly removed with a call
             * to the appropriate method.
             * 
             * @param rule pointer to the rule to be added (must be allocated with <code>new</code>)
             * @return the ID associated with the rule
             * 
             * @throw invalid_argument if an invalid rule is supplied (nullptr)
             */
            unsigned int addDevicePasswordRule(const PasswordRule * rule);
            
            /**
             * Removes the device password rule associated with the specified ID.
             * 
             * Note: The memory associated with the rule is freed.
             * 
             * @param ruleID the ID of the rule to be removed
             * 
             * @throw runtime_error if the requested rule is not found
             */
            void removeDevicePasswordRule(unsigned int ruleID);
            
            /**
             * Checks the validity of the supplied user name against the current rules.
             * 
             * @param name the name to be checked
             * @param failureMessage reference to a string for storing an error message (if the name is not valid)
             * @return <code>true</code>, if the user name is valid
             * 
             * @throw runtime_error if there are no available rules for user names
             */
            bool isUserNameValid(const std::string & name, std::string & failureMessage) const;
            
            /**
             * Checks the validity of the supplied device name against the current rules.
             * 
             * @param name the name to be checked
             * @param failureMessage reference to a string for storing an error message (if the name is not valid)
             * @return <code>true</code>, if the device name is valid
             * 
             * @throw runtime_error if there are no available rules for device names
             */
            bool isDeviceNameValid(const std::string & name, std::string & failureMessage) const;
            
            /**
             * Checks the validity of the supplied user password against the current
             * rules and creates a secure hash for it.
             * 
             * Note: A new random salt is used for every call of this method.
             * 
             * @param rawPassword the raw password to be validated and hashed
             * @return the salt used for the hashing process and actual password hash
             * 
             * @throw runtime_error if there are no available rules for user passwords
             * @throw InvalidPassswordException if the password fails the validation
             */
            const PasswordData hashUserPassword(const std::string & rawPassword) const;
            
            /**
             * Checks the validity of the supplied device password against the current
             * rules and creates a secure hash for it.
             * 
             * Note: A new random salt is used for every call of this method.
             * 
             * @param rawPassword the raw password to be validated and hashed
             * @return the salt used for the hashing process and actual password hash
             * 
             * @throw runtime_error if there are no available rules for device passwords
             * @throw InvalidPassswordException if the password fails the validation
             */
            const PasswordData hashDevicePassword(const std::string & rawPassword) const;
            
            /**
             * Sets a new password hashing configuration, while retaining the current one
             * for compatibility.
             * 
             * Note 1: New configuration can be set if only the current one is present
             * and there is no previous configuration. 
             * 
             * Note 2: Configuration types:<br>
             * - <code>New</code> -> newly supplied parameters<br>
             * - <code>Current</code> -> parameters used for authentication and generating new passwords<br>
             * - <code>Previous</code> -> parameters used for authentication of old passwords<br>
             * 
             * @param newConfiguration the new password hashing parameters
             * 
             * @throw invalid_argument if any of the supplied parameters are not valid
             * @throw runtime_error if a previous configuration is present
             */
            void updatePasswordHashingParameters(const PasswordHashingParameters & newConfiguration);
            
            /**
             * Discards the previous password hashing configuration parameters.
             */
            void discardPreviousPasswordHashingParameters();
            
            /**
             * Removes the specified token from the manager.
             * 
             * @param tokenID the ID of the token to be removed
             * @param userID the ID of the user associated with the token
             * 
             * @throw invalid_argument if the specified token for the specified user
             * cannot be found
             */
            void removeAuthenticationToken(TokenID tokenID, UserID userID);
            
        private:
            /** Data structure holding user cache data. */
            struct UserData
            {
                /** User data. */
                UserDataContainerPtr data;
                /** The number of cache hits done during the life-time of the cache entry. */
                CacheHits entryHits;
                /** Reference to the authorization rules associated with the user. */
                const std::deque<UserAuthorizationRule> & rules;
            };
            
            /** Data structure holding device cache data. */
            struct DeviceData
            {
                /** Device data */
                DeviceDataContainerPtr data;
                /** The number of cache hits done during the life-time of the cache entry. */
                CacheHits entryHits;
            };
            
            Utilities::ThreadPool threadPool;       //threads for handling request processing
            Utilities::FileLogger * debugLogger;    //logger for debugging
            boost::mutex authDataMutex;             //authentication/authorization configuration mutex
            
            DatabaseManager & databaseManager;
            InstructionDispatcher & instructionDispatcher;
            
            //Securable Components
            boost::unordered_map<SecurableComponentType, Securable *> components;
            
            //Cache Management
            CacheSize maxUserDataEntries;           //max size of user data cache (0 == unlimited)
            CacheSize maxDeviceDataEntries;         //max size of device data cache (0 == unlimited)
            CacheEvictionType userEviction;         //user data eviction type
            CacheEvictionType deviceEviction;       //device data eviction type
            CacheHits userCacheAge;                 //user data cache age
            CacheHits deviceCacheAge;               //device data cache age
            boost::unordered_map<UserID, UserData> userDataCache;       //cached user data
            boost::unordered_map<std::string, UserID> userNameMap;      //user name to ID map of cached data
            boost::unordered_map<DeviceID, DeviceData> deviceDataCache; //cached device data
            
            //Tokens
            boost::unordered_map<UserID, std::deque<AuthenticationTokenPtr>> authenticationTokens; //authentication tokens
            TokenID lastAuthorizationTokenID;                   //the ID of the last token to be generated
            TokenID lastAuthenticationTokenID;                  //the ID of the last token to be generated
            RandomDataSize authorizationTokenSignatureSize;     //signature size for authorization tokens
            RandomDataSize authenticationTokenSignatureSize;    //signature size for authentication tokens
            Seconds authenticationTokenValidityDuration;        //authentication token validity duration (0 != unlimited; in seconds)
            
            //User & Device Names
            mutable boost::mutex nameDataMutex; //name configuration mutex
            unsigned int lastNameRuleID;        //the last name rule ID that was assigned
            boost::unordered_map<unsigned int, const NameRule *> userNameRules;   //list of user name rules
            boost::unordered_map<unsigned int, const NameRule *> deviceNameRules; //list of device name rules
            
            //Passwords
            mutable boost::mutex passwordDataMutex;         //password configuration mutex
            unsigned int lastPasswordRuleID;                //the last password rule ID that was assigned
            boost::unordered_map<unsigned int, const PasswordRule *> userPasswordRules;   //list of user password rules
            boost::unordered_map<unsigned int, const PasswordRule *> devicePasswordRules; //list of device password rules
            PasswordHashingParameters currentHashingConfig; //current password hashing configuration
            PasswordHashingParameters previousHashingConfig;//previous password hashing configuration
            
            //Crypto
            KeyGenerator keyGenerator;
            
            //Failed Authentication Delay
            FailedAuthenticationDelayParameters userDelayConfig;    //user authentication delay parameters
            FailedAuthenticationDelayParameters deviceDelayConfig;  //device authentication delay parameters
            
            //Stats
            unsigned long long totalRequestsNumber;                      //total number of requests made
            std::atomic<unsigned long long> successfulRequestsNumber;    //total number of requests successfully completed
            
            /**
             * Authorization request handler.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>UserNotFoundException</code><br>
             * - <code>DeviceNotFoundException</code><br>
             * - <code>InstructionNotAllowedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * - <code>UnrecognisedDeviceException</code><br>
             * - <code>DeviceLockedException</code><br>
             * - <code>UserNotAuthenticatedException</code><br>
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @param promise a smart pointer to the promise that will have the result of the operation
             * 
             * @throw logic_error if the source or target components cannot be found, or if
             * the minimum access lever for the instruction set is not valid, or if invalid
             * access rule data is found
             */
            void processAuthorizationRequest
            (const AuthorizationRequest & request, AuthorizationTokenPromisePtr promise);
            
            /**
             * User authentication request handler.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>UserNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @param promise a smart pointer to the promise that will have the result of the operation
             * 
             * @throw logic_error if the source component cannot be found
             */
            void processUserAuthenticationRequest
            (const UserAuthenticationRequest & request, AuthenticationTokenPromisePtr promise);
            
            /**
             * Device authentication request handler.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>DeviceNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * Note: The caller must ensure the request is not destroyed until processing is finished.
             * 
             * @param request reference to the request to be processed
             * @param promise a smart pointer to the promise that will have the result of the operation
             * 
             * @throw logic_error if the source component cannot be found
             */
            void processDeviceAuthenticationRequest
            (const DeviceAuthenticationRequest & request, AuthenticationTokenPromisePtr promise);
            
            /**
             * Derived crypto data generation request handler.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>invalid_argument</code><br>
             * 
             * @param request reference to the request to be processed
             * @param promise a smart pointer to the promise that will have the result of the operation
             * 
             * @throw logic_error if the source component cannot be found
             */
            void processDerivedCryptoDataGenerationRequest
            (const DerivedCryptoDataGenerationRequest & request, SymmetricCryptoDataContainerPromisePtr promise);
            
            /**
             * Symmetric crypto data generation request handler.
             * 
             * Exceptions that can be thrown by the promise/future:<br>
             * - <code>invalid_argument</code><br>
             * 
             * @param request reference to the request to be processed
             * @param promise a smart pointer to the promise that will have the result of the operation
             * 
             * @throw logic_error if the source component cannot be found
             */
            void processSymmetricCryptoDataGenerationRequest
            (const SymmetricCryptoDataGenerationRequest & request, SymmetricCryptoDataContainerPromisePtr promise);
            
            /**
             * Attempts to retrieve the data for the specified device.
             * 
             * Note 1: NOT thread-safe.
             * 
             * Note 2: Retrieves the data from the cache, if it is available,
             * or retrieves it from the database and caches it (depending on configuration).
             * 
             * @param device the ID of the device for which to retrieve data
             * @return the requested device data or <code>nullptr</code> if it
             * could not be retrieved
             */
            const DeviceData * getDeviceData(DeviceID device);
            
            /**
             * Attempts to retrieve the data for the specified user.
             * 
             * Note 1: NOT thread-safe.
             * 
             * Note 2: Retrieves the data from the cache, if it is available,
             * or retrieves it from the database and caches it (depending on configuration).
             * 
             * @param username the name of the user for whom to retrieve data
             * @return the requested user data or <code>nullptr</code> if it
             * could not be retrieved
             */
            const UserData * getUserData(std::string username);
            
            /**
             * Attempts to retrieve the data for the specified user.
             * 
             * Note 1: NOT thread-safe.
             * 
             * Note 2: Retrieves the data from the cache, if it is available,
             * or retrieves it from the database and caches it (depending on configuration).
             * 
             * @param user the ID of the user for whom to retrieve data
             * @return the requested user data or <code>nullptr</code> if it
             * could not be retrieved
             */
            const UserData * getUserData(UserID user);
            
            /**
             * Attempts to evict a device from the device cache.
             * 
             * Note: NOT thread-safe.
             */
            void evictDevice();
            
            /**
             * Attempts to evict a user from the user cache.
             * 
             * Note: NOT thread-safe.
             */
            void evictUser();
            
            /**
             * Calculates the authentication delay time for the specified number
             * of failed attempts, based on the supplied parameters.
             * 
             * Note: If an overflow occurs during the calculation, the maximum
             * number of seconds that can fit into the data type is returned.
             * 
             * @param params the parameters to be used in the calculation
             * @param failedAttempts the number of failed attempts
             * @return the delay time (in seconds)
             * 
             * @throw logic_error if an unexpected escalation type is encountered
             */
            Seconds calculateAuthenticationDelay
            (const FailedAuthenticationDelayParameters & params, unsigned int failedAttempts);
            
            /**
             * Calculates the remaining delay time, until a new authentication
             * attempt will be allowed.
             * 
             * Warning: This method is to be used for informational purposes only
             * (as it is unclear if/how <code>time_duration::total_seconds()</code>
             * handles overflow). No security action/decision should depend on it.
             * 
             * @param lastFailedAuthenticationTimestamp timestamp of the last failed attempt
             * @param fullDelayTime the full delay time (in seconds)
             * @return the remaining delay time (in seconds)
             * 
             * @throw logic_error if the difference between now and the last failed
             * attempt timestamp results in a negative value (i.e. the failed attempt
             * is in the future).
             */
            Seconds delayTimeRemaining
            (const Timestamp & lastFailedAuthenticationTimestamp, Seconds fullDelayTime);
            
            /**
             * Logs the specified message, if a debugging file logger is assigned to the manager.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string message)
            {
                if(debugLogger != nullptr)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "SecurityManager " + message);
            }
    };
}

#endif	/* SECURITYMANAGER_H */

