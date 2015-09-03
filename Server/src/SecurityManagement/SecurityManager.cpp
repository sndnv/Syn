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

#include "SecurityManager.h"

SyncServer_Core::SecurityManager::PasswordHashingParameters::PasswordHashingParameters
(SaltSize userPassSalt, SaltSize devicePassSalt, HashAlgorithmType userPassHashAlgo, HashAlgorithmType devicePassHashAlgo)
: userPasswordSaltSize(userPassSalt), devicePasswordSaltSize(devicePassSalt),
  userPasswordHashAlgorithm(userPassHashAlgo), devicePasswordHashAlgorithm(devicePassHashAlgo)
{
    isEmpty = false;
}

SyncServer_Core::SecurityManager::PasswordHashingParameters::PasswordHashingParameters()
: PasswordHashingParameters(0, 0, HashAlgorithmType::INVALID, HashAlgorithmType::INVALID)
{
    isEmpty = true;
}

SyncServer_Core::SecurityManager::SecurityManager
(const SecurityManagerParameters & params, Utilities::FileLogger * debugLogger)
: threadPool(params.threadPoolSize, debugLogger), debugLogger(debugLogger),
  databaseManager(params.databaseManager), instructionDispatcher(params.instructionDispatcher),
  maxUserDataEntries(params.maxUserDataEntries),
  maxDeviceDataEntries(params.maxDeviceDataEntries), userEviction(params.userEviction),
  deviceEviction(params.deviceEviction), userCacheAge(0), deviceCacheAge(0),
  lastAuthorizationTokenID(INVALID_TOKEN_ID), lastAuthenticationTokenID(INVALID_TOKEN_ID),
  authorizationTokenSignatureSize(params.authorizationTokenSignatureSize),
  authenticationTokenSignatureSize(params.authenticationTokenSignatureSize),
  authenticationTokenValidityDuration(params.authenticationTokenValidityDuration),
  lastNameRuleID(0), lastPasswordRuleID(0),
  currentHashingConfig(params.currentPasswordHashingConfiguration),
  previousHashingConfig(params.previousPasswordHashingConfiguration),
  keyGenerator(params.keyGeneratorConfig.derivedKeyParams,
               params.keyGeneratorConfig.symKeyParams,
               params.keyGeneratorConfig.asymKeyParams,
               debugLogger),
  userDelayConfig(params.userDelayConfig), deviceDelayConfig(params.deviceDelayConfig),
  totalRequestsNumber(0), successfulRequestsNumber(0)
{
    if(userEviction != CacheEvictionType::LRU && userEviction != CacheEvictionType::MRU)
        throw std::invalid_argument("SecurityManager::() > Invalid user cache eviction type encountered.");

    if(deviceEviction != CacheEvictionType::LRU && deviceEviction != CacheEvictionType::MRU)
        throw std::invalid_argument("SecurityManager::() > Invalid device cache eviction type encountered.");

    if(authorizationTokenSignatureSize <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The authorization token signature size is set to [" 
                + Convert::toString(authorizationTokenSignatureSize) + "].");
    }

    if(authenticationTokenSignatureSize <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The authentication token signature size is set to [" 
                + Convert::toString(authenticationTokenSignatureSize) + "].");
    }

    if(authenticationTokenValidityDuration <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The authentication token validity duration is set to [" 
                + Convert::toString(authenticationTokenValidityDuration) + "].");
    }

    for(const NameRule * currentRule : params.userNameRules)
    {
        if(currentRule != nullptr)
            userNameRules.insert({++lastNameRuleID, currentRule});
        else
            throw std::invalid_argument("SecurityManager::() > Invalid user name rule encountered.");
    }

    for(const NameRule * currentRule : params.deviceNameRules)
    {
        if(currentRule != nullptr)
            deviceNameRules.insert({++lastNameRuleID, currentRule});
        else
            throw std::invalid_argument("SecurityManager::() > Invalid device name rule encountered.");
    }

    for(const PasswordRule * currentRule : params.userPasswordRules)
    {
        if(currentRule != nullptr)
            userPasswordRules.insert({++lastPasswordRuleID, currentRule});
        else
            throw std::invalid_argument("SecurityManager::() > Invalid user password rule encountered.");
    }

    for(const PasswordRule * currentRule : params.devicePasswordRules)
    {
        if(currentRule != nullptr)
            devicePasswordRules.insert({++lastPasswordRuleID, currentRule});
        else
            throw std::invalid_argument("SecurityManager::() > Invalid device password rule encountered.");
    }

    if(currentHashingConfig.userPasswordSaltSize <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The user password salt size is set to [" 
                + Convert::toString(currentHashingConfig.userPasswordSaltSize)
                + "] (current configuration).");
    }

    if(currentHashingConfig.devicePasswordSaltSize <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The device password salt size is set to [" 
                + Convert::toString(currentHashingConfig.devicePasswordSaltSize)
                + "] (current configuration).");
    }

    if(currentHashingConfig.userPasswordHashAlgorithm == HashAlgorithmType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::() > Invalid user password hashing algorithm "
                "type encountered (current configuration).");
    }

    if(currentHashingConfig.devicePasswordHashAlgorithm == HashAlgorithmType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::() > Invalid device password hashing algorithm "
                "type encountered (current configuration).");
    }
    
    if(!previousHashingConfig.isEmpty)
    {
        if(previousHashingConfig.userPasswordSaltSize <= 0)
        {
            logMessage(LogSeverity::Warning, "() > The user password salt size is set to [" 
                    + Convert::toString(previousHashingConfig.userPasswordSaltSize)
                    + "] (previous configuration).");
        }

        if(previousHashingConfig.devicePasswordSaltSize <= 0)
        {
            logMessage(LogSeverity::Warning, "() > The device password salt size is set to [" 
                    + Convert::toString(previousHashingConfig.devicePasswordSaltSize)
                    + "] (previous configuration).");
        }

        if(previousHashingConfig.userPasswordHashAlgorithm == HashAlgorithmType::INVALID)
        {
            throw std::invalid_argument("SecurityManager::() > Invalid user password hashing algorithm "
                    "type encountered (previous configuration).");
        }

        if(previousHashingConfig.devicePasswordHashAlgorithm == HashAlgorithmType::INVALID)
        {
            throw std::invalid_argument("SecurityManager::() > Invalid device password hashing algorithm "
                    "type encountered (previous configuration).");
        }
    }
    
    if(userDelayConfig.delayBase <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The user authentication delay base is set to [" 
                    + Convert::toString(userDelayConfig.delayBase) + "] seconds.");
    }
    
    if(userDelayConfig.escalationType == DelayEscalationType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::() > Invalid user authentication delay type encountered.");
    }
    
    if(deviceDelayConfig.delayBase <= 0)
    {
        logMessage(LogSeverity::Warning, "() > The device authentication delay base is set to [" 
                    + Convert::toString(userDelayConfig.delayBase) + "] seconds.");
    }
    
    if(deviceDelayConfig.escalationType == DelayEscalationType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::() > Invalid device authentication delay type encountered.");
    }
}

SyncServer_Core::SecurityManager::~SecurityManager()
{
    boost::lock_guard<boost::mutex> authDataLock(authDataMutex);
    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);
    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);
    
    userDataCache.clear();
    deviceDataCache.clear();
    
    if(authenticationTokens.size() > 0)
    {
        logMessage(LogSeverity::Debug, "(~) > Token(s) found for ["
                + Convert::toString(authenticationTokens.size()) + "] users.");

        for(auto currentUserTokens : authenticationTokens)
        {
            logMessage(LogSeverity::Debug, "(~) > [" + Convert::toString(authenticationTokens.size())
                    + "] token(s) found for user [" + Convert::toString(currentUserTokens.first) + "].");
            currentUserTokens.second.clear();
        }
        
        authenticationTokens.clear();
    }
    
    for(auto currentRule : userNameRules)
        delete currentRule.second;
    
    userNameRules.clear();
    
    for(auto currentRule : deviceNameRules)
        delete currentRule.second;
    
    deviceNameRules.clear();
    
    for(auto currentRule : userPasswordRules)
        delete currentRule.second;
    
    userPasswordRules.clear();
    
    for(auto currentRule : devicePasswordRules)
        delete currentRule.second;
    
    devicePasswordRules.clear();
    
    debugLogger = nullptr;
}

void SyncServer_Core::SecurityManager::registerSecurableComponent(Securable & component)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);

    auto existingComponent = components.find(component.getComponentType());
    if(existingComponent == components.end())
    {
        components.insert({component.getComponentType(), &component});
    }
    else
    {
        throw std::invalid_argument("SecurityManager::registerSecurableComponent() > A component of type ["
                + Convert::toString(component.getComponentType()) + "] is already present.");
    }
}

void SyncServer_Core::SecurityManager::deregisterSecurableComponent(SecurableComponentType type)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);

    auto existingComponent = components.find(type);
    if(existingComponent != components.end())
    {
        components.erase(existingComponent);
    }
    else
    {
        throw std::invalid_argument("SecurityManager::registerSecurableComponent() > A component of type ["
                + Convert::toString(type) + "] is not present.");
    }
}

unsigned int SyncServer_Core::SecurityManager::addUserNameRule(const NameRule * rule)
{
    if(rule == nullptr)
        throw std::invalid_argument("SecurityManager::addUserNameRule() > Invalid name rule supplied.");

    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    unsigned int newRuleID = ++lastNameRuleID;
    userNameRules.insert({newRuleID, rule});

    return newRuleID;
}

void SyncServer_Core::SecurityManager::removeUserNameRule(unsigned int ruleID)
{
    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    auto nameRule = userNameRules.find(ruleID);
    if(nameRule != userNameRules.end())
    {
        delete nameRule->second;
        userNameRules.erase(nameRule);
    }
    else
    {
        throw std::runtime_error("SecurityManager::removeUserNameRule() > The specified rule ["
                + Convert::toString(ruleID) + "] was not found.");
    }
}

unsigned int SyncServer_Core::SecurityManager::addDeviceNameRule(const NameRule * rule)
{
    if(rule == nullptr)
        throw std::invalid_argument("SecurityManager::addDeviceNameRule() > Invalid name rule supplied.");

    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    unsigned int newRuleID = ++lastNameRuleID;
    deviceNameRules.insert({newRuleID, rule});

    return newRuleID;
}

void SyncServer_Core::SecurityManager::removeDeviceNameRule(unsigned int ruleID)
{
    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    auto nameRule = deviceNameRules.find(ruleID);
    if(nameRule != deviceNameRules.end())
    {
        delete nameRule->second;
        deviceNameRules.erase(nameRule);
    }
    else
    {
        throw std::runtime_error("SecurityManager::removeDeviceNameRule() > The specified rule ["
                + Convert::toString(ruleID) + "] was not found.");
    }
}

unsigned int SyncServer_Core::SecurityManager::addUserPasswordRule(const PasswordRule * rule)
{
    if(rule == nullptr)
        throw std::invalid_argument("SecurityManager::addUserPasswordRule() > Invalid password rule supplied.");

    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    unsigned int newRuleID = ++lastPasswordRuleID;
    userPasswordRules.insert({newRuleID, rule});

    return newRuleID;
}

void SyncServer_Core::SecurityManager::removeUserPasswordRule(unsigned int ruleID)
{
    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    auto passwordRule = userPasswordRules.find(ruleID);
    if(passwordRule != userPasswordRules.end())
    {
        delete passwordRule->second;
        userPasswordRules.erase(passwordRule);
    }
    else
    {
        throw std::runtime_error("SecurityManager::removeUserPasswordRule() > The specified rule ["
                + Convert::toString(ruleID) + "] was not found.");
    }
}

unsigned int SyncServer_Core::SecurityManager::addDevicePasswordRule(const PasswordRule * rule)
{
    if(rule == nullptr)
        throw std::invalid_argument("SecurityManager::addDevicePasswordRule() > Invalid password rule supplied.");

    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    unsigned int newRuleID = ++lastPasswordRuleID;
    devicePasswordRules.insert({newRuleID, rule});

    return newRuleID;
}

void SyncServer_Core::SecurityManager::removeDevicePasswordRule(unsigned int ruleID)
{
    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    auto passwordRule = devicePasswordRules.find(ruleID);
    if(passwordRule != devicePasswordRules.end())
    {
        delete passwordRule->second;
        devicePasswordRules.erase(passwordRule);
    }
    else
    {
        throw std::runtime_error("SecurityManager::removeDevicePasswordRule() > The specified rule ["
                + Convert::toString(ruleID) + "] was not found.");
    }
}

bool SyncServer_Core::SecurityManager::isUserNameValid
(const std::string & name, std::string & failureMessage) const
{
    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    if(userNameRules.size() <= 0)
    {
        failureMessage = "User name validation failed; no rules were found.";
        throw std::runtime_error("SecurityManager::isUserNameValid() > User name validation failed; "
                "no rules were found.");
    }

    for(auto currentRule : userNameRules)
    {
        if(!currentRule.second->isNameValid(name))
        {
            failureMessage = currentRule.second->getErrorMessage();
            return false;
        }
    }

    return true;
}

bool SyncServer_Core::SecurityManager::isDeviceNameValid
(const std::string & name, std::string & failureMessage) const
{
    boost::lock_guard<boost::mutex> nameDataLock(nameDataMutex);

    if(deviceNameRules.size() <= 0)
    {
        failureMessage = "Device name validation failed; no rules were found.";
        throw std::runtime_error("SecurityManager::isDeviceNameValid() > Device name validation failed; "
                "no rules were found.");
    }

    for(auto currentRule : deviceNameRules)
    {
        if(!currentRule.second->isNameValid(name))
        {
            failureMessage = currentRule.second->getErrorMessage();
            return false;
        }
    }

    return true;
}

const PasswordData SyncServer_Core::SecurityManager::hashUserPassword
(const std::string & rawPassword) const
{
    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    if(userPasswordRules.size() <= 0)
    {
        throw std::runtime_error("SecurityManager::hashUserPassword() > User password hashing failed; "
                "no password rules were found.");
    }

    for(auto currentRule : userPasswordRules)
    {
        if(!currentRule.second->isPasswordValid(rawPassword))
            throw InvalidPassswordException(currentRule.second->getErrorMessage());
    }

    const SaltData & newSalt =
            SaltGenerator::getRandomSalt(currentHashingConfig.userPasswordSaltSize);
    
    const HashData & newHash =
            HashGenerator::getHash(currentHashingConfig.userPasswordHashAlgorithm, newSalt, rawPassword);

    return (newSalt + newHash);
}

const PasswordData SyncServer_Core::SecurityManager::hashDevicePassword
(const std::string & rawPassword) const
{
    boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);

    if(devicePasswordRules.size() <= 0)
    {
        throw std::runtime_error("SecurityManager::hashDevicePassword() > User password hashing failed; "
                "no password rules were found.");
    }

    for(auto currentRule : devicePasswordRules)
    {
        if(!currentRule.second->isPasswordValid(rawPassword))
            throw InvalidPassswordException(currentRule.second->getErrorMessage());
    }

    const SaltData & newSalt =
            SaltGenerator::getRandomSalt(currentHashingConfig.devicePasswordSaltSize);
    const HashData & newHash =
            HashGenerator::getHash(currentHashingConfig.devicePasswordHashAlgorithm, newSalt, rawPassword);

    return (newSalt + newHash);
}

void SyncServer_Core::SecurityManager::updatePasswordHashingParameters
(const PasswordHashingParameters & newConfiguration)
{
    if(newConfiguration.isEmpty)
    {
       throw std::invalid_argument("SecurityManager::updatePasswordHashingParameters() > "
               "An empty parameters container was provided.");
    }

    if(newConfiguration.userPasswordSaltSize <= 0)
    {
        logMessage(LogSeverity::Warning, "(updatePasswordHashingParameters) > The user password salt size will be set to [" 
                + Convert::toString(newConfiguration.userPasswordSaltSize) + "].");
    }

    if(newConfiguration.devicePasswordSaltSize <= 0)
    {
        logMessage(LogSeverity::Warning, "(updatePasswordHashingParameters) > The device password salt size will be set to [" 
                + Convert::toString(newConfiguration.devicePasswordSaltSize) + "].");
    }

    if(newConfiguration.userPasswordHashAlgorithm == HashAlgorithmType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::(updatePasswordHashingParameters) > "
                "Invalid user password hashing algorithm type encountered.");
    }
    
    if(newConfiguration.devicePasswordHashAlgorithm == HashAlgorithmType::INVALID)
    {
        throw std::invalid_argument("SecurityManager::(updatePasswordHashingParameters) > "
                "Invalid device password hashing algorithm type encountered.");
    }
    
    if(!previousHashingConfig.isEmpty)
    {
        throw std::runtime_error("SecurityManager::updatePasswordHashingParameters() > "
                "Previous password hashing configuration is present.");
    }
    
    previousHashingConfig = currentHashingConfig;
    currentHashingConfig = newConfiguration;
}

void SyncServer_Core::SecurityManager::discardPreviousPasswordHashingParameters()
{
    previousHashingConfig = PasswordHashingParameters();
}

void SyncServer_Core::SecurityManager::removeAuthenticationToken(TokenID tokenID, UserID userID)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);

    auto userTokens = authenticationTokens.find(userID);
    if(userTokens != authenticationTokens.end())
    {
        auto currentToken = userTokens->second.begin();
        while(currentToken != userTokens->second.end() && (*currentToken)->getID() != tokenID)
        {
            ++currentToken;
        }

        if(currentToken != userTokens->second.end())
        {
            userTokens->second.erase(currentToken);
        }
        else
        {
            throw std::invalid_argument("SecurityManager::removeAuthenticationToken() > "
                    "Token [" + Utilities::Strings::toString(tokenID) + "] was not found for user ["
                    + Utilities::Strings::toString(userID) + "].");
        }

        if(userTokens->second.size() == 0)
            authenticationTokens.erase(userTokens);
    }
    else
    {
        throw std::invalid_argument("SecurityManager::removeAuthenticationToken() > "
                "No tokens were found for user [" + Utilities::Strings::toString(userID) + "].");
    }
}

void SyncServer_Core::SecurityManager::processAuthorizationRequest
(const AuthorizationRequest & request, AuthorizationTokenPromisePtr promise)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);
    ++totalRequestsNumber;

    auto sourceComponent = components.find(request.getSource());
    auto targetComponent = components.find(request.getTarget());
    if(sourceComponent == components.end() || targetComponent == components.end())
    {
        try
        {
            boost::throw_exception(std::logic_error("SecurityManager::processAuthorizationRequest() > "
                "Source [" + Convert::toString(request.getSource()) + "] or target ["
                + Convert::toString(request.getTarget()) + "] component not found."));
        }
        catch(const std::logic_error &)
        {
            promise->set_exception(boost::current_exception());
            throw;
        }
    }

    const UserData * userData = getUserData(request.getUser());

    if(userData == nullptr)
    {//authorization is not given if no user data is found
        logMessage(LogSeverity::Error, "(processAuthorizationRequest) > No data was found for user <"
                + Convert::toString(request.getUser()) + ">.");

        try
        {
            boost::throw_exception
            (
                UserNotFoundException("(processAuthorizationRequest) > No data was found for user <"
                    + Convert::toString(request.getUser()) + ">.")
            );
        }
        catch(const UserNotFoundException &) { promise->set_exception(boost::current_exception()); }
        return;
    }
    
    auto userTokens = authenticationTokens.find(request.getUser());
    if(userTokens == authenticationTokens.end())
    {//authorization is not given if the user is not authenticated
        logMessage(LogSeverity::Error, "(processAuthorizationRequest) > No authentication tokens were found for user <"
                + Convert::toString(request.getUser()) + ">.");

        try
        {
            boost::throw_exception
            (
                UserNotAuthenticatedException("(processAuthorizationRequest) > "
                    "No authentication tokens were found for user <"
                    + Convert::toString(request.getUser()) + ">.")
            );
        }
        catch(const UserNotAuthenticatedException &) { promise->set_exception(boost::current_exception()); }
        return;
    }
    else
    {
        bool validTokenFound = false;
        for(AuthenticationTokenPtr currentToken : userTokens->second)
        {
            if(!currentToken->isExpired()
               && currentToken->getUserID() == request.getUser()
               && currentToken->getDeviceID() == request.getDevice())
            {
                validTokenFound = true;
                break;
            }
        }
        
        if(!validTokenFound)
        {//authorization is not given if no valid token is found
            logMessage(LogSeverity::Error, "(processAuthorizationRequest) > No valid authentication token was found for user <"
                    + Convert::toString(request.getUser()) + ">.");

            try
            {
                boost::throw_exception
                (
                    UserNotAuthenticatedException("(processAuthorizationRequest) > "
                        "No valid authentication token was found for user <"
                        + Convert::toString(request.getUser()) + ">.")
                );
            }
            catch(const UserNotAuthenticatedException &) { promise->set_exception(boost::current_exception()); }
            return;
        }
    }

    UserAccessLevel minAccessLevel =
            instructionDispatcher.getMinimumAccessLevelForSet(request.getInstruction()->getParentSet());
    
    if(minAccessLevel == UserAccessLevel::INVALID)
    {//authorization is not given if there are issues with the access level for the set
        try
        {
            boost::throw_exception
            (
                std::logic_error("SecurityManager::processAuthorizationRequest() > Set ["
                    + Convert::toString(request.getInstruction()->getParentSet())
                    + "] not found or its minimum access level is not defined.")
            );
        }
        catch(const std::logic_error &)
        {
            promise->set_exception(boost::current_exception());
            throw;
        }
    }
    else if(userData->data->getUserAccessLevel() < minAccessLevel)
    {//authorization is not given if the user does not have the minimum required access level
        logMessage(LogSeverity::Error, "(processAuthorizationRequest) > Insufficient access level for user <"
                        + Convert::toString(request.getUser()) + ">.");

        try
        {
            boost::throw_exception
            (
                InsufficientUserAccessException("(processAuthorizationRequest) > "
                    "Insufficient access level for user <"
                    + Convert::toString(request.getUser()) + ">.")
            );
        }
        catch(const InsufficientUserAccessException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }

    bool userActionAllowed = false;
    for(const UserAuthorizationRule & currentRule : userData->rules)
    {
        if(!currentRule.isValid())
        {
            try
            {
                boost::throw_exception
                (
                    std::logic_error("SecurityManager::processAuthorizationRequest() > "
                        "An invalid user access rule was found for user ["
                        + Convert::toString(request.getUser()) + "] and set ["
                        + Convert::toString(currentRule.getSetType()) + "].")
                );
            }
            catch(const std::logic_error &)
            {
                promise->set_exception(boost::current_exception());
                throw;
            }
        }

        if(currentRule.getSetType() == request.getInstruction()->getParentSet())
        {
            userActionAllowed = true;
        }
    }

    if(!userActionAllowed)
    {//authorization is not given if the user is not explicitly allowed to use the instruction set
        logMessage(LogSeverity::Error, "(processAuthorizationRequest) > Instruction not allowed for user <"
                        + Convert::toString(request.getUser()) + ">.");

        try
        {
            boost::throw_exception
            (
                InstructionNotAllowedException("(processAuthorizationRequest) > "
                    "Instruction not allowed for user <"
                    + Convert::toString(request.getUser()) + ">.")
            );
        }
        catch(const InstructionNotAllowedException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }

    if(request.getDevice() != INVALID_DEVICE_ID)
    {//the authorization is required for a device
        const DeviceData * deviceData = getDeviceData(request.getDevice());

        if(deviceData == nullptr)
        {//authorization is not given if no data is found for the device
            logMessage(LogSeverity::Error, "(processAuthorizationRequest) > No data was found for device ["
                    + Convert::toString(request.getDevice()) + "] belonging to user ["
                    + Convert::toString(request.getUser()) + "].");

            try
            {
                boost::throw_exception
                (
                    DeviceNotFoundException("(processAuthorizationRequest) > "
                        "No data was found for device [" + Convert::toString(request.getDevice()) 
                        + "] belonging to user [" + Convert::toString(request.getUser()) + "].")
                );
            }
            catch(const DeviceNotFoundException &)
            {
                promise->set_exception(boost::current_exception());
            }
            
            return;
        }

        if(deviceData->data->getDeviceOwner() != request.getUser())
        {//authorization is not given if the device does not belong to the user
            logMessage(LogSeverity::Error, "(processAuthorizationRequest) > Device ["
                    + Convert::toString(request.getDevice()) + "] does not belong to user [" 
                    + Convert::toString(request.getUser()) + "], as expected.");
            try
            {
                boost::throw_exception
                (
                    UnexpectedDeviceException("(processAuthorizationRequest) > Device ["
                        + Convert::toString(request.getDevice()) + "] does not belong to user [" 
                        + Convert::toString(request.getUser()) + "], as expected.")
                );
            }
            catch(const UnexpectedDeviceException &)
            {
                promise->set_exception(boost::current_exception());
            }
            
            return;
        }
        
        if(deviceData->data->isDeviceLocked())
        {//authorization is not given if the device is locked
            logMessage(LogSeverity::Error, "(processAuthorizationRequest) > Device <"
                            + Convert::toString(request.getDevice()) + "> is locked.");

            try
            {
                boost::throw_exception
                (
                    DeviceLockedException("(processAuthorizationRequest) > Device <"
                        + Convert::toString(request.getDevice()) + "> is locked.")
                );
            }
            catch(const DeviceLockedException &)
            {
                promise->set_exception(boost::current_exception());
            }
            
            return;
        }
    }

    //no issues were encountered; authorization is given & the token is generated
    AuthorizationToken * newToken = 
            new AuthorizationToken
            (
                ++lastAuthorizationTokenID,
                SecurityManagement_Crypto::SaltGenerator::getRandomSalt(authorizationTokenSignatureSize),
                request.getInstruction()->getParentSet(),
                request.getUser(),
                request.getDevice()
            );

    AuthorizationTokenPtr newTokenPtr(newToken);

    ++successfulRequestsNumber;

    //the token is first sent to the target component for later verification
    targetComponent->second->postAuthorizationToken(newTokenPtr);
    promise->set_value(newTokenPtr);
    return;
}

void SyncServer_Core::SecurityManager::processUserAuthenticationRequest
(const UserAuthenticationRequest & request, AuthenticationTokenPromisePtr promise)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);
    ++totalRequestsNumber;

    auto sourceComponent = components.find(request.getSource());
    if(sourceComponent == components.end())
    {
        try
        {
            boost::throw_exception
            (
                std::logic_error("SecurityManager::processUserAuthenticationRequest() > Source ["
                    + Convert::toString(request.getSource()) + "] component not found.")
            );
        }
        catch(const std::logic_error &)
        {
            promise->set_exception(boost::current_exception());
            throw;
        }
    }

    const UserData * userData = getUserData(request.getUsername());
    
    if(userData == nullptr)
    {//access is not allowed if no user data is found
        logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > No data was found for user <"
                        + request.getUsername() + ">.");

        try
        {
            boost::throw_exception
            (
                UserNotFoundException("(processUserAuthenticationRequest) > "
                    "No data was found for user <" + request.getUsername() + ">.")
            );
        }
        catch(const UserNotFoundException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }
    
    if(userData->data->isUserLocked())
    {//access is not allowed if the user is locked
        logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > User <"
                        + request.getUsername() + "> is locked.");

        try
        {
            boost::throw_exception
            (
                UserLockedException("(processUserAuthenticationRequest) > User <"
                    + request.getUsername() + "> is locked.")
            );
        }
        catch(const UserLockedException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }
    
    //checks if the authentication is allowed, based on the previous failed attempts
    unsigned int failedAuthentications = userData->data->getFailedAuthenticationAttempts();
    if(failedAuthentications > 0 && failedAuthentications >= userDelayConfig.ignoredAttempts)
    {
        Seconds delayTime = calculateAuthenticationDelay(userDelayConfig, failedAuthentications);
        
        if(delayTime > 0)
        {
            Timestamp lastAttempt = userData->data->getLastFailedAuthenticationTimestamp();
            
            if((lastAttempt + boost::posix_time::seconds(delayTime)) 
                    > boost::posix_time::second_clock::universal_time())
            {//access is not allowed if the required time between attempts has not passed
                logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > User <"
                        + request.getUsername() + "> is locked for ["
                        + Convert::toString(delayTimeRemaining(lastAttempt, delayTime))
                        + "] more seconds because of [" + Convert::toString(failedAuthentications)
                        + "] failed authentication attempts.");

                try
                {
                    boost::throw_exception
                    (
                        UserLockedException("(processUserAuthenticationRequest) > User <"
                        + request.getUsername() + "> is locked for ["
                        + Convert::toString(delayTimeRemaining(lastAttempt, delayTime))
                        + "] more seconds because of ["
                        + Convert::toString(failedAuthentications)
                        + "] failed authentication attempts.")
                    );
                }
                catch(const UserLockedException &)
                {
                    promise->set_exception(boost::current_exception());
                }
                
                return;
            }
        }
    }
    
    {//verifies the supplied password
        boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);
        
        bool passwordVerificationFailed = true;
        
        const SaltData & passwordSalt =
                userData->data->getPasswordSalt(currentHashingConfig.userPasswordSaltSize);
        
        const HashData & hashedPassword =
                HashGenerator::getHash(currentHashingConfig.userPasswordHashAlgorithm,
                                       passwordSalt,
                                       request.getRawPassword());

        if(!userData->data->passwordsMatch(passwordSalt + hashedPassword))
        {
            if(!previousHashingConfig.isEmpty)
            {
                const SaltData & previousConfigPasswordSalt =
                        userData->data->getPasswordSalt(previousHashingConfig.userPasswordSaltSize);
                
                const HashData & previousConfigHashedPassword =
                        HashGenerator::getHash(previousHashingConfig.userPasswordHashAlgorithm,
                                               previousConfigPasswordSalt,
                                               request.getRawPassword());
                
                if(userData->data->passwordsMatch(previousConfigPasswordSalt + previousConfigHashedPassword))
                {
                    logMessage(LogSeverity::Warning, "(processUserAuthenticationRequest) > User password for <"
                                    + request.getUsername() + "> authenticated with previous configuration.");
                    passwordVerificationFailed = false;
                }
            }
        }
        else
            passwordVerificationFailed = false;
        
        if(passwordVerificationFailed)
        {//access is not allowed if the passwords do not match
            userData->data->setLastFailedAuthenticationTimestamp();
            if(userData->data->getFailedAuthenticationAttempts() >= userDelayConfig.maxAttempts)
            {
                logMessage(LogSeverity::Info, "(processUserAuthenticationRequest) > User <" + request.getUsername()
                                + "> locked because of too many failed authentication attempts.");
                userData->data->setLockedState(true);
            }
            
            databaseManager.Users().updateUser(userData->data);
            
            logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > Invalid password supplied for user <"
                            + request.getUsername() + ">.");
            
            try
            {
                boost::throw_exception
                (
                    InvalidPassswordException("(processUserAuthenticationRequest) > "
                        "Invalid password supplied for user <" + request.getUsername() + ">.")
                );
            }
            catch(const InvalidPassswordException &)
            {
                promise->set_exception(boost::current_exception());
            }
            
            return;
        }
    }

    if(userData->data->getUserAccessLevel() != UserAccessLevel::USER
       && userData->data->getUserAccessLevel() != UserAccessLevel::ADMIN)
    {//access is not allowed if the user a valid & useful access level
        logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > User <" + request.getUsername()
                        + "> does not have the required access level.");

        try
        {
            boost::throw_exception
            (
                InsufficientUserAccessException("(processUserAuthenticationRequest) > User <"
                    + request.getUsername() + "> does not have the required access level.")
            );
        }
        catch(const InsufficientUserAccessException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }
    else if(userData->data->getAccessRules().size() == 0)
    {//access is not allowed if the user does not have any access rules
        logMessage(LogSeverity::Error, "(processUserAuthenticationRequest) > User <" + request.getUsername()
                        + "> does not have any access permissions.");

        try
        {
            boost::throw_exception
            (
                InsufficientUserAccessException("(processUserAuthenticationRequest) > User <"
                    + request.getUsername() + "> does not have any access permissions.")
            );
        }
        catch(const InsufficientUserAccessException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }

    //updates the user authentication data
    userData->data->setLastSuccessfulAuthenticationTimestamp();
    databaseManager.Users().updateUser(userData->data);
    
    //no issues were encountered; access is allowed & the token is generated
    AuthenticationToken * newToken =
            new AuthenticationToken
            (
                ++lastAuthenticationTokenID,
                SecurityManagement_Crypto::SaltGenerator::getRandomSalt(authenticationTokenSignatureSize),
                (boost::posix_time::second_clock::universal_time()
                    + boost::posix_time::seconds(authenticationTokenValidityDuration)),
                userData->data->getUserID()
            );

    AuthenticationTokenPtr newTokenPtr(newToken);
    auto userTokens = authenticationTokens.find(userData->data->getUserID());
    if(userTokens != authenticationTokens.end())
    {
        userTokens->second.push_back(newTokenPtr);
    }
    else
    {
        authenticationTokens.insert({userData->data->getUserID(), std::deque<AuthenticationTokenPtr>{newTokenPtr}});
    }

    ++successfulRequestsNumber;

    promise->set_value(newTokenPtr);
    return;
}

void SyncServer_Core::SecurityManager::processDeviceAuthenticationRequest
(const DeviceAuthenticationRequest & request, AuthenticationTokenPromisePtr promise)
{
    boost::lock_guard<boost::mutex> dataLock(authDataMutex);
    ++totalRequestsNumber;

    auto sourceComponent = components.find(request.getSource());
    if(sourceComponent == components.end())
    {
        try
        {
            boost::throw_exception
            (
                std::logic_error("SecurityManager::processDeviceAuthenticationRequest() > Source ["
                    + Convert::toString(request.getSource()) + "] component not found.")
            );
        }
        catch(const std::logic_error &)
        {
            promise->set_exception(boost::current_exception());
            throw;
        }
    }

    const DeviceData * deviceData = getDeviceData(request.getDevice());

    if(deviceData == nullptr)
    {//access is not allowed if no device data is found
        logMessage(LogSeverity::Error, "(processDeviceAuthenticationRequest) > No data was found for device ["
                        + Convert::toString(request.getDevice()) + "].");

        try
        {
            boost::throw_exception
            (
                DeviceNotFoundException("(processDeviceAuthenticationRequest) > "
                    "No data was found for device ["
                    + Convert::toString(request.getDevice()) + "].")
            );
        }
        catch(const DeviceNotFoundException &)
        {
            promise->set_exception(boost::current_exception());
        }
        
        return;
    }

    if(deviceData->data->isDeviceLocked())
    {//access is not allowed if the device is locked
        logMessage(LogSeverity::Error, "(processDeviceAuthenticationRequest) > Device <"
                        + Convert::toString(request.getDevice()) + "> is locked.");

        try
        {
            boost::throw_exception
            (
                DeviceLockedException("(processDeviceAuthenticationRequest) > Device <"
                    + Convert::toString(request.getDevice()) + "> is locked.")
            );
        }
        catch(const DeviceLockedException &)
        {
            promise->set_exception(boost::current_exception());
        }
        return;
    }

    //checks if the authentication is allowed, based on the previous failed attempts
    unsigned int failedAuthentications = deviceData->data->getFailedAuthenticationAttempts();
    if(failedAuthentications > 0 && failedAuthentications >= deviceDelayConfig.ignoredAttempts)
    {
        Seconds delayTime = calculateAuthenticationDelay(deviceDelayConfig, failedAuthentications);
        
        if(delayTime > 0)
        {
            Timestamp lastAttempt = deviceData->data->getLastFailedAuthenticationTimestamp();
            
            if((lastAttempt + boost::posix_time::seconds(delayTime))
                    > boost::posix_time::second_clock::universal_time())
            {//access is not allowed if the required time between attempts has not passed
                logMessage(LogSeverity::Error, "(processDeviceAuthenticationRequest) > Device <"
                        + Convert::toString(request.getDevice()) + "> is locked for ["
                        + Convert::toString(delayTimeRemaining(lastAttempt, delayTime))
                        + "] more seconds because of ["
                        + Convert::toString(failedAuthentications)
                        + "] failed authentication attempts.");

                try
                {
                    boost::throw_exception
                    (
                        DeviceLockedException("(processDeviceAuthenticationRequest) Device <"
                            + Convert::toString(request.getDevice()) + "> is locked for ["
                            + Convert::toString(delayTimeRemaining(lastAttempt, delayTime))
                            + "] more seconds because of ["
                            + Convert::toString(failedAuthentications)
                            + "] failed authentication attempts.")
                    );
                }
                catch(const DeviceLockedException &)
                {
                    promise->set_exception(boost::current_exception());
                }
                
                return;
            }
        }
    }
    
    {//verifies the supplied password
        boost::lock_guard<boost::mutex> passwordDataLock(passwordDataMutex);
        bool passwordVerificationFailed = true;
        
        const SaltData & passwordSalt =
                deviceData->data->getPasswordSalt(currentHashingConfig.devicePasswordSaltSize);
        
        const HashData & hashedPassword =
                HashGenerator::getHash(currentHashingConfig.devicePasswordHashAlgorithm,
                                       passwordSalt,
                                       request.getRawPassword());

        if(!deviceData->data->passwordsMatch(passwordSalt + hashedPassword))
        {//the password verification failed with the current parameters
            if(!previousHashingConfig.isEmpty)
            {//attempts to verify the password with the previous parameters
                const SaltData & previousConfigPasswordSalt =
                        deviceData->data->getPasswordSalt(previousHashingConfig.devicePasswordSaltSize);
                
                const HashData & previousConfigHashedPassword =
                        HashGenerator::getHash(previousHashingConfig.devicePasswordHashAlgorithm,
                                               previousConfigPasswordSalt,
                                               request.getRawPassword());
                
                if(deviceData->data->passwordsMatch(previousConfigPasswordSalt + previousConfigHashedPassword))
                {
                    logMessage(LogSeverity::Warning, "(processDeviceAuthenticationRequest) > Device password for <"
                                    + Convert::toString(request.getDevice())
                                    + "> authenticated with previous configuration.");
                    
                    passwordVerificationFailed = false;
                }
            }
        }
        else
            passwordVerificationFailed = false;
        
        if(passwordVerificationFailed)
        {//access is not allowed if the passwords do not match
            deviceData->data->setLastFailedAuthenticationTimestamp();
            if(deviceData->data->getFailedAuthenticationAttempts() >= deviceDelayConfig.maxAttempts)
            {
                logMessage(LogSeverity::Info, "(processDeviceAuthenticationRequest) > Device <"
                                + Convert::toString(request.getDevice())
                                + "> locked because of too many failed authentication attempts.");
                
                deviceData->data->setLockedState(true);
            }
            
            databaseManager.Devices().updateDevice(deviceData->data);
            
            logMessage(LogSeverity::Error, "(processDeviceAuthenticationRequest) > Invalid password supplied for device <"
                            + Convert::toString(request.getDevice()) + ">.");

            try
            {
                boost::throw_exception
                (
                    InvalidPassswordException("(processDeviceAuthenticationRequest) > "
                        "Invalid password supplied for device <"
                        + Convert::toString(request.getDevice()) + ">.")
                );
            }
            catch(const InvalidPassswordException &)
            {
                promise->set_exception(boost::current_exception());
            }
            
            return;
        }
    }

    //updates the device authentication data
    deviceData->data->setLastSuccessfulAuthenticationTimestamp();
    databaseManager.Devices().updateDevice(deviceData->data);
    
    //no issues were encountered; access is allowed & the token is generated
    AuthenticationToken * newToken =
            new AuthenticationToken
            (
                ++lastAuthenticationTokenID,
                SecurityManagement_Crypto::SaltGenerator::getRandomSalt(authenticationTokenSignatureSize),
                (boost::posix_time::second_clock::universal_time()
                    + boost::posix_time::seconds(authenticationTokenValidityDuration)),
                deviceData->data->getDeviceOwner(),
                deviceData->data->getDeviceID()
            );

    AuthenticationTokenPtr newTokenPtr(newToken);
    auto userTokens = authenticationTokens.find(deviceData->data->getDeviceOwner());
    if(userTokens != authenticationTokens.end())
    {
        userTokens->second.push_back(newTokenPtr);
    }
    else
    {
        authenticationTokens.insert({deviceData->data->getDeviceOwner(), std::deque<AuthenticationTokenPtr>{newTokenPtr}});
    }
    
    ++successfulRequestsNumber;

    promise->set_value(newTokenPtr);
    return;
}

void SyncServer_Core::SecurityManager::processDerivedCryptoDataGenerationRequest
(const DerivedCryptoDataGenerationRequest & request, SymmetricCryptoDataContainerPromisePtr promise)
{
    {
        boost::lock_guard<boost::mutex> dataLock(authDataMutex);
        ++totalRequestsNumber;

        auto sourceComponent = components.find(request.getSource());
        if(sourceComponent == components.end())
        {
            try
            {
                boost::throw_exception
                (
                    std::logic_error("SecurityManager::processDerivedCryptoDataGenerationRequest() > Source ["
                        + Convert::toString(request.getSource()) + "] component not found.")
                );
            }
            catch(const std::logic_error &)
            {
                promise->set_exception(boost::current_exception());
                throw;
            }
        }
    }

    try
    {
        SymmetricCryptoDataContainerPtr data;

        if(request.deriveFromExistingData())
        {
            data = keyGenerator.getSymmetricCryptoDataFromPassphrase
                   (
                       request.getCipher(),
                       request.getMode(),
                       request.getRawPassword(),
                       request.getIterationsCount(),
                       request.getSaltData(),
                       request.getIVData()
                   );
        }
        else
        {
            data = keyGenerator.getSymmetricCryptoDataFromPassphrase(request.getRawPassword());
        }

        promise->set_value(data);
        ++successfulRequestsNumber;
    }
    catch(const std::invalid_argument & ex)
    {
        logMessage(LogSeverity::Error, "(processDerivedCryptoDataGenerationRequest) > "
                        "Exception encountered during derived key generation: ["
                        + std::string(ex.what()) + "].");
        
        promise->set_exception(boost::current_exception());
    }
}

void SyncServer_Core::SecurityManager::processSymmetricCryptoDataGenerationRequest
(const SymmetricCryptoDataGenerationRequest & request, SymmetricCryptoDataContainerPromisePtr promise)
{
    {
        boost::lock_guard<boost::mutex> dataLock(authDataMutex);
        ++totalRequestsNumber;

        auto sourceComponent = components.find(request.getSource());
        if(sourceComponent == components.end())
        {
            try
            {
                boost::throw_exception
                (
                    std::logic_error("SecurityManager::processSymmetricCryptoDataGenerationRequest() > "
                        "Source [" + Convert::toString(request.getSource())
                        + "] component not found.")
                );
            }
            catch(const std::logic_error &)
            {
                promise->set_exception(boost::current_exception());
                throw;
            }
        }
    }

    try
    {
        SymmetricCryptoDataContainerPtr data;
        
        if(request.createNewData())
        {
            if(request.useDefaultParameters())
            {
                data = keyGenerator.getSymmetricCryptoData();
            }
            else
            {
                data = keyGenerator.getSymmetricCryptoData
                        (request.getCipher(), request.getMode());
            }
        }
        else
        {
            if(request.useDefaultParameters())
            {
                data = keyGenerator.getSymmetricCryptoData
                        (*request.getKey(), *request.getIV());
            }
            else
            {
                data = keyGenerator.getSymmetricCryptoData
                        (request.getCipher(), request.getMode(),
                         *request.getKey(), *request.getIV());
            }
        }
        
        promise->set_value(data);
        ++successfulRequestsNumber;
    }
    catch(const std::invalid_argument & ex)
    {
        logMessage(LogSeverity::Error, "(processSymmetricCryptoDataGenerationRequest) > "
                        "Exception encountered during symmetric key generation: ["
                        + std::string(ex.what()) + "].");
        
        promise->set_exception(boost::current_exception());
    }
}

void SyncServer_Core::SecurityManager::processECDHSymmetricCryptoDataGenerationRequest
(const ECDHSymmetricCryptoDataGenerationRequest & request, SymmetricCryptoDataContainerPromisePtr promise)
{
    {
        boost::lock_guard<boost::mutex> dataLock(authDataMutex);
        ++totalRequestsNumber;

        auto sourceComponent = components.find(request.getSource());
        if(sourceComponent == components.end())
        {
            try
            {
                boost::throw_exception
                (
                    std::logic_error("SecurityManager::processECDHSymmetricCryptoDataGenerationRequest() > "
                        "Source [" + Convert::toString(request.getSource())
                        + "] component not found.")
                );
            }
            catch(const std::logic_error &)
            {
                promise->set_exception(boost::current_exception());
                throw;
            }
        }
    }

    try
    {
        SymmetricCryptoDataContainerPtr data;
        
        if(request.createNewData())
        {
            if(request.useDefaultParameters())
            {
                data = keyGenerator.getSymmetricCryptoDataForDHExchange
                        (*request.getPrivateKey(), *request.getPublicKey());
            }
            else
            {
                data = keyGenerator.getSymmetricCryptoDataForDHExchange
                        (keyGenerator.getDefaultEllipticCurve(),
                         *request.getPrivateKey(), *request.getPublicKey(),
                         request.getCipher(), request.getMode());
            }
        }
        else
        {
            if(request.useDefaultParameters())
            {
                data = keyGenerator.getSymmetricCryptoDataForDHExchange
                        (*request.getPrivateKey(), *request.getPublicKey(),
                        *request.getIV());
            }
            else
            {
                data = keyGenerator.getSymmetricCryptoDataForDHExchange
                        (*request.getPrivateKey(), *request.getPublicKey(),
                         *request.getIV(), request.getCipher(), request.getMode());
            }
        }
        
        promise->set_value(data);
        ++successfulRequestsNumber;
    }
    catch(const std::invalid_argument & ex)
    {
        logMessage(LogSeverity::Error, "(processECDHSymmetricCryptoDataGenerationRequest) > "
                        "Exception encountered during symmetric key generation: ["
                        + std::string(ex.what()) + "].");
        
        promise->set_exception(boost::current_exception());
    }
}

const SyncServer_Core::SecurityManager::DeviceData *
SyncServer_Core::SecurityManager::getDeviceData(DeviceID device)
{
    auto deviceData = deviceDataCache.find(device);
    if(deviceData == deviceDataCache.end())
    {//device not found in cache
        //checks if the cache is full
        if(maxDeviceDataEntries > 0 && deviceDataCache.size() >= maxDeviceDataEntries)
            evictDevice();

        DeviceDataContainerPtr currentDeviceContainer = 
                databaseManager.Devices().getDevice(device);

        if(!currentDeviceContainer)
            return nullptr;

        deviceData = deviceDataCache.insert
                     (
                        deviceDataCache.begin(), //hint (ignored by unordered_map)
                        {//container pair
                            currentDeviceContainer->getDeviceID(),  //device ID
                            {//DeviceData
                                currentDeviceContainer, //device container
                                deviceCacheAge //cache hits/age
                            }
                        }
                     );
    }
    else
    {//device found in cache
        ++deviceData->second.entryHits;
        ++deviceCacheAge;
    }

    return &deviceData->second;
}

const SyncServer_Core::SecurityManager::UserData *
SyncServer_Core::SecurityManager::getUserData(std::string username)
{
    auto userData = userDataCache.end();

    auto userID = userNameMap.find(username);
    if(userID == userNameMap.end())
    {//user not found in cache
        //checks if the cache is full
        if(maxUserDataEntries > 0 && userDataCache.size() >= maxUserDataEntries)
            evictUser();

        UserDataContainerPtr currentUserContainer = databaseManager.Users().getUser(username);

        if(!currentUserContainer)
            return nullptr;

        userData = userDataCache.insert
                   (
                        userDataCache.begin(), //hint (ignored by unordered_map)
                        {//container pair
                            currentUserContainer->getUserID(),  //user ID
                            {//UserData
                                currentUserContainer, //user container
                                userCacheAge, //cache hits/age
                                currentUserContainer->getAccessRules() //ref to user rules
                            }
                        }
                   );
        
        userNameMap.insert({currentUserContainer->getUsername(), currentUserContainer->getUserID()});
    }
    else
    {//user found in cache
        userData = userDataCache.find(userID->second);
        ++userData->second.entryHits;
        ++userCacheAge;
    }

    return &userData->second;
}

const SyncServer_Core::SecurityManager::UserData *
SyncServer_Core::SecurityManager::getUserData(UserID user)
{
    auto userData = userDataCache.find(user);
    if(userData == userDataCache.end())
    {//user not found in cache
        //checks if the cache is full
        if(maxUserDataEntries > 0 && userDataCache.size() >= maxUserDataEntries)
            evictUser();

        UserDataContainerPtr currentUserContainer = databaseManager.Users().getUser(user);

        if(!currentUserContainer)
            return nullptr;

        userData = userDataCache.insert
                   (
                        userDataCache.begin(), //hint (ignored by unordered_map)
                        {//container pair
                            currentUserContainer->getUserID(),  //user ID
                            {//UserData
                                currentUserContainer, //user container
                                userCacheAge, //cache hits/age
                                currentUserContainer->getAccessRules() //ref to user rules
                            }
                        }
                   );
                   
        userNameMap.insert({currentUserContainer->getUsername(), user});
    }
    else
    {//user found in cache
        ++userData->second.entryHits;
        ++userCacheAge;
    }

    return &userData->second;
}

void SyncServer_Core::SecurityManager::evictDevice()
{
    DeviceID evictionTarget = INVALID_DEVICE_ID;
    CacheHits targetCacheHits = 0;
    bool isTargetSet = false;
    for(auto currentDeviceData : deviceDataCache)
    {
        switch(deviceEviction)
        {
            case CacheEvictionType::LRU:
            {
                if(currentDeviceData.second.entryHits < targetCacheHits || !isTargetSet)
                {
                    evictionTarget = currentDeviceData.first;
                    targetCacheHits = currentDeviceData.second.entryHits;
                    isTargetSet = true;
                }
            }; break;

            case CacheEvictionType::MRU:
            {
                if(currentDeviceData.second.entryHits > targetCacheHits || !isTargetSet)
                {
                    evictionTarget = currentDeviceData.first;
                    targetCacheHits = currentDeviceData.second.entryHits;
                    isTargetSet = true;
                }
            }; break;

            default:
            {
                logMessage(LogSeverity::Error, "(evictDevice) > Device eviction failed; "
                                "unexpected eviction type encountered.");
            } break;
        }
    }

    if(evictionTarget != INVALID_DEVICE_ID)
    {
        deviceDataCache.erase(evictionTarget);
        logMessage(LogSeverity::Debug, "(evictDevice) > Device [" + Convert::toString(evictionTarget)
                        + "] evicted from cache.");
    }
    else
        logMessage(LogSeverity::Debug, "(evictDevice) > Device eviction failed; no device for eviction was found.");
}

void SyncServer_Core::SecurityManager::evictUser()
{
    UserID evictionTarget = INVALID_USER_ID;
    std::string targetName;
    CacheHits targetCacheHits = MAX_CACHE_HITS;
    bool isTargetSet = false;
    for(auto currentUserData : userDataCache)
    {
        switch(userEviction)
        {
            case CacheEvictionType::LRU:
            {
                if(currentUserData.second.entryHits < targetCacheHits || !isTargetSet)
                {
                    evictionTarget = currentUserData.first;
                    targetCacheHits = currentUserData.second.entryHits;
                    targetName = currentUserData.second.data->getUsername();
                    isTargetSet = true;
                }
            }; break;

            case CacheEvictionType::MRU:
            {
                if(currentUserData.second.entryHits > targetCacheHits || !isTargetSet)
                {
                    evictionTarget = currentUserData.first;
                    targetCacheHits = currentUserData.second.entryHits;
                    targetName = currentUserData.second.data->getUsername();
                    isTargetSet = true;
                }
            }; break;

            default:
            {
                logMessage(LogSeverity::Error, "(evictUser) > User eviction failed; "
                                "unexpected eviction type encountered.");
            }; break;
        }
    }

    if(evictionTarget != INVALID_DEVICE_ID)
    {
        userDataCache.erase(evictionTarget);
        userNameMap.erase(targetName);
        logMessage(LogSeverity::Debug, "(evictUser) > User [" + Convert::toString(evictionTarget)
                        + "] evicted from cache.");

        std::vector<DeviceID> userDevices;
        for(auto currentDeviceData : deviceDataCache)
        {
            if(currentDeviceData.second.data->getDeviceOwner() == evictionTarget)
                userDevices.push_back(currentDeviceData.first);
        }

        for(const DeviceID currentUserDevice : userDevices)
        {
            deviceDataCache.erase(currentUserDevice);
            logMessage(LogSeverity::Debug, "(evictUser) > Device [" + Convert::toString(currentUserDevice)
                    + "] for user [" + Convert::toString(evictionTarget) 
                    + "] evicted from cache.");
        }
    }
    else
        logMessage(LogSeverity::Debug, "(evictUser) > User eviction failed; no user for eviction was found.");
}

Seconds SyncServer_Core::SecurityManager::calculateAuthenticationDelay
(const FailedAuthenticationDelayParameters & params, unsigned int failedAttempts)
{
    Seconds result = 0;

    switch(params.escalationType)
    {
        case DelayEscalationType::CONSTANT:
        {
            result = params.delayBase;
        }; break;

        case DelayEscalationType::LINEAR:
        {
            try
            {
                result = Utilities::Tools::powerof(params.delayBase, failedAttempts);
            }
            catch(const std::overflow_error &)
            {
                logMessage(LogSeverity::Error, "(calculateAuthenticationDelay) > An overflow was encountered; "
                                "the delay was set to its maximum possible value.");
                result = Common_Types::MAX_SECONDS;
            }
        }; break;

        case DelayEscalationType::QUADRATIC:
        {
            try
            {
                result = Utilities::Tools::powerof(params.delayBase, 
                                                   Utilities::Tools::powerof(failedAttempts, 2));
            }
            catch(const std::overflow_error &)
            {
                logMessage(LogSeverity::Error, "(calculateAuthenticationDelay) > An overflow was encountered; "
                                "the delay was set to its maximum possible value.");
                result = Common_Types::MAX_SECONDS;
            }
        }; break;

        default:
        {
            throw std::logic_error("SecurityManager::calculateAuthenticationDelay() > "
                                   "Unexpected escalation type encountered.");
        };
    }

    return result;
}

Seconds SyncServer_Core::SecurityManager::delayTimeRemaining
(const Timestamp & lastFailedAuthenticationTimestamp, Seconds fullDelayTime)
{
    boost::posix_time::time_duration difference = 
            boost::posix_time::second_clock::universal_time() - lastFailedAuthenticationTimestamp;
    
    int result = difference.total_seconds();

    if(result >= 0)
    {
        return ((fullDelayTime > (Seconds)result) ? (fullDelayTime - result) : 0);
    }
    else
    {
        throw std::logic_error
              (
                   "SecurityManager::delayTimeRemaining() > "
                   "Unexpected timestamp difference encountered; current ["
                   + Convert::toString(boost::posix_time::second_clock::universal_time())
                   +"]; last [" + Convert::toString(lastFailedAuthenticationTimestamp) + "]."
              );
    }
}
