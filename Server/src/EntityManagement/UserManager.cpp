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

#include "UserManager.h"

namespace Convert = Utilities::Strings;
namespace Instructions = InstructionManagement_Sets::UserManagerInstructions;
namespace InstructionResults = InstructionManagement_Sets::UserManagerInstructions::Results;

EntityManagement::UserManager::UserManager
(const UserManagerParameters & params,  Utilities::FileLogger * debugLogger)
: debugLogger(debugLogger), databaseManager(params.databaseManager),
  securityManager(params.securityManager), instructionsReceived(0), instructionsProcessed(0)
{}

EntityManagement::UserManager::~UserManager()
{
    logDebugMessage("(~) > Destruction initiated.");

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    authorizationTokens.clear();

    debugLogger = nullptr;
}

void EntityManagement::UserManager::postAuthorizationToken
(const SecurityManagement_Types::AuthorizationTokenPtr token)
{
    if(UserManagerAdminInstructionTarget::getType() != token->getAuthorizedSet()
       && UserManagerSelfInstructionTarget::getType() != token->getAuthorizedSet())
    {
        throw std::logic_error("UserManager::postAuthorizationToken() > The token with ID ["
                + Convert::toString(token->getID()) + "] is not for the expected instruction sets.");
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    if(authorizationTokens.find(token->getID()) == authorizationTokens.end())
    {
        authorizationTokens.insert({token->getID(), token});
    }
    else
    {
        throw std::logic_error("UserManager::postAuthorizationToken() > A token with ID ["
                + Convert::toString(token->getID()) + "] is already present.");
    }
}

bool EntityManagement::UserManager::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<UserManagerAdminInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::ADMIN);

        try
        {
            set->bindInstructionHandler(UserManagerAdminInstructionType::GET_USER,
                                        adminGetUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::GET_USERS_BY_CONSTRAINT,
                                        adminGetUsersHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::ADD_USER,
                                        adminAddUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::REMOVE_USER,
                                        adminRemoveUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::RESET_PASSWORD,
                                        adminResetPasswordHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::FORCE_PASSWORD_RESET,
                                        adminForcePasswordResetHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::LOCK_USER,
                                        adminLockUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::UNLOCK_USER,
                                        adminUnlockUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::UPDATE_ACCESS_LEVEL,
                                        adminUpdateAccessLevelBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::RESET_FAILED_AUTHENTICATION_ATTEMPTS,
                                        adminResetFailedAuthenticationAttemptsHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::ADD_AUTHORIZATION_RULE,
                                        adminAddAuthorizationRuleHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::REMOVE_AUTHORIZATION_RULE,
                                        adminRemoveAuthorizationRuleHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::CLEAR_AUTHORIZATION_RULES,
                                        adminClearAuthorizationRulesHandlerBind);
            
            set->bindInstructionHandler(UserManagerAdminInstructionType::DEBUG_GET_STATE,
                                        debugGetStateHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logDebugMessage("(registerInstructionSet) > Exception encountered: <"
                            + std::string(ex.what()) + ">");
            return false;
        }

        return true;
    }
    else
    {
        logDebugMessage("(registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}

bool EntityManagement::UserManager::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<UserManagerSelfInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::USER);

        try
        {
            set->bindInstructionHandler(UserManagerSelfInstructionType::GET_USER,
                                        selfGetUserHandlerBind);
            
            set->bindInstructionHandler(UserManagerSelfInstructionType::RESET_PASSWORD,
                                        selfResetPasswordHandlerBind);
        }
        catch(const std::invalid_argument & ex)
        {
            logDebugMessage("(registerInstructionSet) > Exception encountered: <"
                            + std::string(ex.what()) + ">");
            
            return false;
        }

        return true;
    }
    else
    {
        logDebugMessage("(registerInstructionSet) > The supplied set is not initialised.");
        return false;
    }
}

void EntityManagement::UserManager::adminGetUserHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    UserDataContainerPtr resultData;
    boost::shared_ptr<Instructions::AdminGetUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminGetUser>(instruction);

    if(actualInstruction)
    {
        if(actualInstruction->userID != INVALID_USER_ID)
        {
            resultData = databaseManager.Users().getUser(actualInstruction->userID);
        }
        else
        {
            resultData = databaseManager.Users().getUser(actualInstruction->username);
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminGetUser>(
        new InstructionResults::AdminGetUser{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminGetUsersHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    std::vector<UserDataContainerPtr> resultData;
    boost::shared_ptr<Instructions::AdminGetUsersByConstraint> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminGetUsersByConstraint>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Users().getUsersByConstraint
                (
                    actualInstruction->constraintType,
                    actualInstruction->constraintValue
                );
    }

    auto result = boost::shared_ptr<InstructionResults::AdminGetUsersByConstraint>(
        new InstructionResults::AdminGetUsersByConstraint{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminAddUserHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminAddUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminAddUser>(instruction);

    if(actualInstruction)
    {
        try
        {
            std::string nameValidationErrorMessage;
            if(securityManager.isUserNameValid(actualInstruction->username, nameValidationErrorMessage))
            {
                PasswordData newUserPassword(securityManager.hashUserPassword(actualInstruction->rawPassword));
                
                UserDataContainerPtr newUserContainer(new UserDataContainer(actualInstruction->username,
                                                                            newUserPassword,
                                                                            actualInstruction->accessLevel,
                                                                            actualInstruction->forcePasswordReset));
                
                resultValue = databaseManager.Users().addUser(newUserContainer);
            }
            else
            {
                throwInstructionException("UserManager::adminAddUserHandler() > Invalid user name supplied: ["
                                          + nameValidationErrorMessage + "].", instruction);
                return;
            }
        }
        catch(const std::exception & ex)
        {
            logDebugMessage("(adminAddUserHandler) > Exception encountered: ["
                            + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminAddUser>(
        new InstructionResults::AdminAddUser{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminRemoveUserHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminRemoveUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminRemoveUser>(instruction);

    if(actualInstruction)
    {
        resultValue = databaseManager.Users().removeUser(actualInstruction->userID);
    }

    auto result = boost::shared_ptr<InstructionResults::AdminRemoveUser>(
        new InstructionResults::AdminRemoveUser{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminResetPasswordHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminResetPassword> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminResetPassword>(instruction);

    if(actualInstruction)
    {
        try
        {
            UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
            if(userData)
            {
                PasswordData newUserPassword(securityManager.hashUserPassword(actualInstruction->rawPassword));

                userData->resetPassword(newUserPassword);
                resultValue = databaseManager.Users().updateUser(userData);
            }
            else
            {
                logDebugMessage("(adminResetPasswordHandler) > User ["
                                + Convert::toString(actualInstruction->userID)
                                + "] not found.");
                
                throwInstructionException("UserManager::adminResetPasswordHandler() > User ["
                                          + Convert::toString(actualInstruction->userID)
                                          + "] not found.", instruction);
                return;
            }
        }
        catch(const std::exception & ex)
        {
            logDebugMessage("(adminResetPasswordHandler) > Exception encountered: ["
                            + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminResetPassword>(
        new InstructionResults::AdminResetPassword{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminForcePasswordResetHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminForcePasswordReset> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminForcePasswordReset>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        
        if(userData)
        {
            userData->forceUserPasswordReset();
            resultValue = databaseManager.Users().updateUser(userData);
        }
        else
        {
            logDebugMessage("(adminForcePasswordResetHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminResetPasswordHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminForcePasswordReset>(
        new InstructionResults::AdminForcePasswordReset{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminLockUserHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminLockUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminLockUser>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            if(!userData->isUserLocked())
            {
                userData->setLockedState(true);
                resultValue = databaseManager.Users().updateUser(userData);
            }
        }
        else
        {
            logDebugMessage("(adminLockUserHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminLockUserHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminLockUser>(
        new InstructionResults::AdminLockUser{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminUnlockUserHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminUnlockUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminUnlockUser>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            if(userData->isUserLocked())
            {
                userData->setLockedState(false);
                resultValue = databaseManager.Users().updateUser(userData);
            }
        }
        else
        {
            logDebugMessage("(adminUnlockUserHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminUnlockUserHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminUnlockUser>(
        new InstructionResults::AdminUnlockUser{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminUpdateAccessLevel
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminUpdateAccessLevel> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminUpdateAccessLevel>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            if(userData->getUserAccessLevel() != actualInstruction->level)
            {
                userData->setUserAccessLevel(actualInstruction->level);
                resultValue = databaseManager.Users().updateUser(userData);
            }
        }
        else
        {
            logDebugMessage("(adminUpdateAccessLevel) > User ["
                            + Convert::toString(actualInstruction->userID)
                            + "] not found.");
            
            throwInstructionException("UserManager::adminUpdateAccessLevel() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminUpdateAccessLevel>(
        new InstructionResults::AdminUpdateAccessLevel{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminResetFailedAuthenticationAttemptsHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminResetFailedAuthenticationAttempts> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminResetFailedAuthenticationAttempts>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            if(userData->getFailedAuthenticationAttempts() > 0)
            {
                userData->resetFailedAuthenticationAttempts();
                resultValue = databaseManager.Users().updateUser(userData);
            }
        }
        else
        {
            logDebugMessage("(adminResetFailedAuthenticationAttemptsHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminResetFailedAuthenticationAttemptsHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminResetFailedAuthenticationAttempts>(
        new InstructionResults::AdminResetFailedAuthenticationAttempts{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminAddAuthorizationRuleHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminAddAuthorizationRule> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminAddAuthorizationRule>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            userData->addAccessRule(actualInstruction->rule);
            resultValue = databaseManager.Users().updateUser(userData);
        }
        else
        {
            logDebugMessage("(adminAddAuthorizationRuleHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminAddAuthorizationRuleHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminAddAuthorizationRule>(
        new InstructionResults::AdminAddAuthorizationRule{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminRemoveAuthorizationRuleHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminRemoveAuthorizationRule> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminRemoveAuthorizationRule>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            userData->removeAccessRule(actualInstruction->rule);
            resultValue = databaseManager.Users().updateUser(userData);
        }
        else
        {
            logDebugMessage("(adminRemoveAuthorizationRuleHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminRemoveAuthorizationRuleHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminRemoveAuthorizationRule>(
        new InstructionResults::AdminRemoveAuthorizationRule{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::adminClearAuthorizationRulesHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::AdminClearAuthorizationRules> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::AdminClearAuthorizationRules>(instruction);

    if(actualInstruction)
    {
        UserDataContainerPtr userData = databaseManager.Users().getUser(actualInstruction->userID);
        if(userData)
        {
            if(userData->getAccessRules().size() > 0)
            {
                userData->clearAccessRules();
                resultValue = databaseManager.Users().updateUser(userData);
            }
        }
        else
        {
            logDebugMessage("(adminClearAuthorizationRulesHandler) > User ["
                            + Convert::toString(actualInstruction->userID) + "] not found.");
            
            throwInstructionException("UserManager::adminClearAuthorizationRulesHandler() > User ["
                                      + Convert::toString(actualInstruction->userID)
                                      + "] not found.", instruction);
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::AdminClearAuthorizationRules>(
        new InstructionResults::AdminClearAuthorizationRules{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::debugGetStateHandler
(InstructionPtr<UserManagerAdminInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    std::string resultData;
    boost::shared_ptr<Instructions::DebugGetState> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::DebugGetState>(instruction);

    if(actualInstruction)
    {
        resultData += "instructionsReceived;" + Convert::toString(instructionsReceived) + "\n";
        resultData += "instructionsProcessed;" + Convert::toString(instructionsProcessed) + "\n";
        resultData += "authorizationTokens size;" + Convert::toString(authorizationTokens.size()) + "\n";
    }

    auto result = boost::shared_ptr<InstructionResults::DebugGetState>(
        new InstructionResults::DebugGetState{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::selfGetUserHandler
(InstructionPtr<UserManagerSelfInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    UserDataContainerPtr resultData;
    boost::shared_ptr<Instructions::SelfGetUser> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::SelfGetUser>(instruction);

    if(actualInstruction)
    {
        resultData = databaseManager.Users().getUser(actualInstruction->getToken()->getUserID());
    }

    auto result = boost::shared_ptr<InstructionResults::SelfGetUser>(
        new InstructionResults::SelfGetUser{resultData});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::selfResetPasswordHandler
(InstructionPtr<UserManagerSelfInstructionType> instruction)
{
    //verifies that the instruction has a valid and expected authorization token
    try
    {
        verifyAuthorizationToken(instruction->getToken());
    }
    catch(const InvalidAuthorizationTokenException &)
    {
        instruction->getPromise().set_exception(boost::current_exception());
        return;
    }

    bool resultValue = false;
    boost::shared_ptr<Instructions::SelfResetPassword> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::SelfResetPassword>(instruction);

    if(actualInstruction)
    {
        try
        {
            UserDataContainerPtr userData =
                    databaseManager.Users().getUser(actualInstruction->getToken()->getUserID());
            
            if(userData)
            {
                PasswordData newUserPassword(securityManager.hashUserPassword(actualInstruction->rawPassword));

                userData->resetPassword(newUserPassword);
                resultValue = databaseManager.Users().updateUser(userData);
            }
            else
            {
                logDebugMessage("(selfResetPasswordHandler) > User ["
                                + Convert::toString(actualInstruction->getToken()->getUserID())
                                + "] not found.");
                
                throwInstructionException("UserManager::selfResetPasswordHandler() > User ["
                                          + Convert::toString(actualInstruction->getToken()->getUserID())
                                          + "] not found.", instruction);
                return;
            }
        }
        catch(const std::exception & ex)
        {
            logDebugMessage("(selfResetPasswordHandler) > Exception encountered: ["
                            + std::string(ex.what()) + "].");
            
            instruction->getPromise().set_exception(boost::current_exception());
            return;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::SelfResetPassword>(
        new InstructionResults::SelfResetPassword{resultValue});

    instruction->getPromise().set_value(result);
}

void EntityManagement::UserManager::verifyAuthorizationToken(AuthorizationTokenPtr token)
{
    ++instructionsReceived;
    if(!token)
    {
       throw InvalidAuthorizationTokenException("UserManager::verifyAuthorizationToken() > "
                "An empty token was supplied."); 
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    auto requestedToken = authorizationTokens.find(token->getID());
    if(requestedToken != authorizationTokens.end())
    {
        if(*(requestedToken->second) == *token
           && (token->getAuthorizedSet() == UserManagerAdminInstructionTarget::getType()
               || token->getAuthorizedSet() == UserManagerSelfInstructionTarget::getType()))
        {
            authorizationTokens.erase(requestedToken);
            ++instructionsProcessed;
        }
        else
        {
            throw InvalidAuthorizationTokenException("UserManager::verifyAuthorizationToken() > "
                    "The supplied token [" + Convert::toString(token->getID())
                    + "] does not match the one expected by the manager.");
        }
    }
    else
    {
        throw InvalidAuthorizationTokenException("UserManager::verifyAuthorizationToken() > "
                "The supplied token [" + Convert::toString(token->getID()) + "] was not found.");
    }
}
