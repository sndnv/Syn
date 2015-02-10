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

#include "SessionManager.h"

namespace Instructions = InstructionManagement_Sets::SessionManagerInstructions;
namespace InstructionResults = InstructionManagement_Sets::SessionManagerInstructions::Results;

SyncServer_Core::SessionManager::SessionManager
(const SessionManagerParameters & params, Utilities::FileLogger * debugLogger)
: threadPool(params.threadPoolSize, debugLogger), debugLogger(debugLogger),
  databaseManager(params.databaseManager), securityManager(params.securityManager),
  maxSessionsPerUser(params.maxSessionsPerUser), maxSessionsPerDevice(params.maxSessionsPerDevice),
  dataCommit(params.dataCommit), inactiveSessionExpirationTime(params.inactiveSessionExpirationTime),
  unauthenticatedSessionExpirationTime(params.unauthenticatedSessionExpirationTime),
  lastSessionID(INVALID_INTERNAL_SESSION_ID), nextExpirationHandlerInvocation(INVALID_DATE_TIME),
  currentScheduledExpirationHandlers(0), totalExpirationHandlerInvocations(0)
{
    if(dataCommit == SessionDataCommitType::INVALID)
    {
        throw std::invalid_argument("SessionManager::() > Invalid session data commit type encountered.");
    }
}

SyncServer_Core::SessionManager::~SessionManager()
{
    boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);
    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    authorizationTokens.clear();

    if(activeSessions.size() > 0)
    {
        logDebugMessage("(~) > [" + Convert::toString(activeSessions.size()) + "] active sessions found.");

        userSessions.clear();
        deviceSessions.clear();

        for(auto currentSessionData : activeSessions)
        {
            {
                boost::lock_guard<boost::mutex> sessionDataLock(*currentSessionData.second->dataMutex);
                
                securityManager.removeAuthenticationToken(currentSessionData.second->token->getID(),
                                                          currentSessionData.second->data->getUser());
                
                currentSessionData.second->data->closeSession();

                if(dataCommit == SessionDataCommitType::ON_CLOSE
                   || dataCommit == SessionDataCommitType::ON_REAUTH
                   || dataCommit == SessionDataCommitType::ON_UPDATE)
                {//commits the session container to the database
                    if(currentSessionData.second->addedToDatabase)
                        databaseManager.Sessions().updateSession(currentSessionData.second->data);
                    else
                        databaseManager.Sessions().addSession(currentSessionData.second->data);
                }
            }

            delete currentSessionData.second->dataMutex;
            currentSessionData.second->dataMutex = nullptr;
        }

        activeSessions.clear();
    }

    onSessionExpired.disconnect_all_slots();
    onReauthenticationRequired.disconnect_all_slots();

    //forces the thread pool to stop
    if(nextExpirationHandlerInvocation != INVALID_DATE_TIME)
        threadPool.stopThreadPool();

    debugLogger = nullptr;
}

void SyncServer_Core::SessionManager::postAuthorizationToken(const AuthorizationTokenPtr token)
{
    if(getType() != token->getAuthorizedSet())
    {
        throw std::logic_error("SessionManager::postAuthorizationToken() > The token with ID ["
                + Convert::toString(token->getID()) + "] is not for the expected instruction set.");
    }
    
    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    if(authorizationTokens.find(token->getID()) == authorizationTokens.end())
    {
        authorizationTokens.insert({token->getID(), token});
    }
    else
    {
        throw std::logic_error("SessionManager::postAuthorizationToken() > A token with ID ["
                + Convert::toString(token->getID()) + "] is already present.");
    }
}

bool SyncServer_Core::SessionManager::registerInstructionSet
(InstructionManagement_Sets::InstructionSetPtr<SessionManagerInstructionType> set) const
{
    if(set)
    {
        set->setMinimumAccessLevel(UserAccessLevel::ADMIN);

        try
        {
            set->bindInstructionHandler(SessionManagerInstructionType::GET_SESSION,
                                        getSessionHandlerBind);
            
            set->bindInstructionHandler(SessionManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT,
                                        getSessionsByConstraintHandlerBind);
            
            set->bindInstructionHandler(SessionManagerInstructionType::FORCE_SESSION_EXPIRATION,
                                        forceSessionExpirationHandlerBind);
            
            set->bindInstructionHandler(SessionManagerInstructionType::FORCE_SESSION_REAUTHENTICATION,
                                        forceSessionReauthenticationHandlerBind);
            
            set->bindInstructionHandler(SessionManagerInstructionType::FORCE_EXPIRATION_PROCESS,
                                        forceExpirationProcessHandlerBind);
            
            set->bindInstructionHandler(SessionManagerInstructionType::DEBUG_GET_STATE,
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

InternalSessionID SyncServer_Core::SessionManager::openSession
(const std::string & username, const std::string & rawPassword, SessionType type, bool isPersistent)
{
    if(type != SessionType::COMMAND && type != SessionType::DATA)
        throw std::invalid_argument("SessionManager::openSession() > Invalid session type supplied.");

    //posts the user authentication request
    UserAuthenticationRequest authenticationRequest(username, rawPassword, *this);
    AuthenticationTokenPromisePtr promise = securityManager.postRequest(authenticationRequest);

    try
    {
        AuthenticationTokenPtr sessionToken = promise->get_future().get();

        boost::lock_guard<boost::mutex> globalSessionsDataLock(globalSessionDataMutex);

        auto userSessionIDs = userSessions.find(sessionToken->getUserID());

        if(maxSessionsPerUser > 0)
        {
            if(userSessionIDs != userSessions.end()
               && userSessionIDs->second.size() >= maxSessionsPerUser)
            {
                securityManager.removeAuthenticationToken(sessionToken->getID(),
                                                          sessionToken->getUserID());
                
                throw TooManyUserSessionsException("(openSession) > The user [" + username
                        + "] has reached the maximum allowed concurrent sessions <"
                        + Convert::toString(maxSessionsPerUser) + ">.");
            }
        }

        //builds the new session data entries
        SessionDataContainerPtr newSessionContainer
        (
            new SessionDataContainer(type, INVALID_DEVICE_ID, sessionToken->getUserID(), isPersistent)
        );
        
        InternalSessionID newSessionID = ++lastSessionID;
        bool newSessionAddedToDB;
        boost::mutex * newSessionDataMutex = new boost::mutex();

        if(dataCommit == SessionDataCommitType::ON_CLOSE
           || dataCommit == SessionDataCommitType::ON_REAUTH
           || dataCommit == SessionDataCommitType::ON_UPDATE)
        {//commits the session container to the database
            databaseManager.Sessions().addSession(newSessionContainer);
            newSessionAddedToDB = true;
        }
        else
            newSessionAddedToDB = false;

        if(userSessionIDs != userSessions.end())
        {
            userSessionIDs->second.push_back(newSessionID);
        }
        else
        {
            userSessions.insert({sessionToken->getUserID(),
                                 std::deque<InternalSessionID>{newSessionID}});
        }

        boost::shared_ptr<SessionData> internalDataContainer
        (
            new SessionData
            {
                newSessionContainer,
                sessionToken,
                sessionToken->getExpirationTime(),
                newSessionAddedToDB,
                false,
                false,
                newSessionDataMutex
            }
        );
        
        activeSessions.insert({newSessionID, internalDataContainer});

        //schedules the expiration handler, if necessary
        Timestamp nextHandlerInvocation;

        if(inactiveSessionExpirationTime > 0)
        {
            nextHandlerInvocation = bptime::second_clock::universal_time()
                                    + bptime::seconds(inactiveSessionExpirationTime);
            
            if(nextHandlerInvocation > activeSessions[newSessionID]->tokenExpirationTime)
                nextHandlerInvocation = activeSessions[newSessionID]->tokenExpirationTime;
        }
        else
            nextHandlerInvocation = activeSessions[newSessionID]->tokenExpirationTime;

        if(nextExpirationHandlerInvocation == INVALID_DATE_TIME
           || nextExpirationHandlerInvocation > nextHandlerInvocation)
        {
            ++currentScheduledExpirationHandlers;
            nextExpirationHandlerInvocation = nextHandlerInvocation;
            threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                       nextExpirationHandlerInvocation);
        }
        
        return newSessionID;
    }
    catch(const std::exception & ex)
    {
        logDebugMessage("(openSession) > An exception was encountered"
                        " while opening a new user session: [" + std::string(ex.what()) + "].");
        throw;
    }
}

InternalSessionID SyncServer_Core::SessionManager::openSession
(const DeviceID & deviceID, const std::string & rawPassword, SessionType type, bool isPersistent)
{
    if(type != SessionType::COMMAND && type != SessionType::DATA)
        throw std::invalid_argument("SessionManager::openSession() > Invalid session type supplied.");

    //posts the device authentication request
    DeviceAuthenticationRequest authenticationRequest(deviceID, rawPassword, *this);
    AuthenticationTokenPromisePtr promise = securityManager.postRequest(authenticationRequest);

    try
    {
        AuthenticationTokenPtr sessionToken = promise->get_future().get();

        boost::lock_guard<boost::mutex> globalSessionsDataLock(globalSessionDataMutex);

        auto deviceSessionIDs = deviceSessions.find(deviceID);

        if(maxSessionsPerDevice > 0)
        {
            if(deviceSessionIDs != deviceSessions.end()
               && deviceSessionIDs->second.size() >= maxSessionsPerDevice)
            {
                securityManager.removeAuthenticationToken(sessionToken->getID(),
                                                          sessionToken->getUserID());
                
                throw TooManyDeviceSessionsException("(openSession) > The device ["
                        + Convert::toString(deviceID)
                        + "] has reached the maximum allowed concurrent sessions <"
                        + Convert::toString(maxSessionsPerDevice) + ">.");
            }
        }

        //builds the new session data entries
        SessionDataContainerPtr newSessionContainer
        (
            new SessionDataContainer(type, deviceID, sessionToken->getUserID(), isPersistent)
        );
        
        InternalSessionID newSessionID = ++lastSessionID;
        bool newSessionAddedToDB;
        boost::mutex * newSessionDataMutex = new boost::mutex();

        if(dataCommit == SessionDataCommitType::ON_CLOSE
           || dataCommit == SessionDataCommitType::ON_REAUTH
           || dataCommit == SessionDataCommitType::ON_UPDATE)
        {//commits the session container to the database
            databaseManager.Sessions().addSession(newSessionContainer);
            newSessionAddedToDB = true;
        }
        else
            newSessionAddedToDB = false;

        if(deviceSessionIDs != deviceSessions.end())
        {
            deviceSessionIDs->second.push_back(newSessionID);
        }
        else
        {
            deviceSessions.insert({sessionToken->getDeviceID(), std::deque<InternalSessionID>{newSessionID}});
        }

        boost::shared_ptr<SessionData> internalDataContainer
        (
            new SessionData
            {
                newSessionContainer,
                sessionToken,
                sessionToken->getExpirationTime(),
                newSessionAddedToDB,
                false,
                false,
                newSessionDataMutex
            }
        );
        
        activeSessions.insert({newSessionID, internalDataContainer});

        //schedules the expiration handler, if necessary
        Timestamp nextHandlerInvocation;

        if(inactiveSessionExpirationTime > 0)
        {
            nextHandlerInvocation = bptime::second_clock::universal_time()
                                    + bptime::seconds(inactiveSessionExpirationTime);
            
            if(nextHandlerInvocation > activeSessions[newSessionID]->tokenExpirationTime)
                nextHandlerInvocation = activeSessions[newSessionID]->tokenExpirationTime;
        }
        else
            nextHandlerInvocation = activeSessions[newSessionID]->tokenExpirationTime;

        if(nextExpirationHandlerInvocation == INVALID_DATE_TIME
           || nextExpirationHandlerInvocation > nextHandlerInvocation)
        {
            ++currentScheduledExpirationHandlers;
            nextExpirationHandlerInvocation = nextHandlerInvocation;
            threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                       nextExpirationHandlerInvocation);
        }

        return newSessionID;
    }
    catch(const std::exception & ex)
    {
        logDebugMessage("(openSession) > An exception was encountered"
                        " while opening a new device session: [" + std::string(ex.what()) + "].");
        throw;
    }
}

void SyncServer_Core::SessionManager::reauthenticateSession
(InternalSessionID session, const std::string & username, const std::string & rawPassword)
{
    boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

    auto sessionData = activeSessions.find(session);
    if(sessionData == activeSessions.end())
    {
        throw std::invalid_argument("SessionManager::reauthenticateSession(user) >"
                                    " Unable to find session with id ["
                                    + Convert::toString(session) + "].");
    }

    if(!sessionData->second->waitingForReauthentication)
    {
        throw std::logic_error("SessionManager::reauthenticateSession(user) > Session ["
                               + Convert::toString(session)
                               + "] is not eligible for re-authentication.");
    }

    //discards the expired token
    securityManager.removeAuthenticationToken(sessionData->second->token->getID(),
                                              sessionData->second->token->getUserID());

    //posts a request for a new token
    UserAuthenticationRequest authenticationRequest(username, rawPassword, *this);
    AuthenticationTokenPromisePtr promise = securityManager.postRequest(authenticationRequest);

    try
    {
        AuthenticationTokenPtr newSessionToken = promise->get_future().get();

        //replaces the token associated with the session
        sessionData->second->token = newSessionToken;
        sessionData->second->tokenExpirationTime = newSessionToken->getExpirationTime();
        sessionData->second->waitingForReauthentication = false;
    }
    catch(const std::exception & ex)
    {
        logDebugMessage("(reauthenticateSession) > An exception was encountered"
                        " while attempting to authenticate user session ["
                        + Convert::toString(session) + "]: [" + std::string(ex.what()) + "].");
        throw;
    }

    if(dataCommit == SessionDataCommitType::ON_REAUTH)
    {//commits the session container to the database
        databaseManager.Sessions().updateSession(sessionData->second->data);
    }

    if(nextExpirationHandlerInvocation > sessionData->second->tokenExpirationTime)
    {//schedules an expiration handler invocation
        ++currentScheduledExpirationHandlers;
        nextExpirationHandlerInvocation = sessionData->second->tokenExpirationTime;
        threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                   nextExpirationHandlerInvocation);
    }
}

void SyncServer_Core::SessionManager::reauthenticateSession
(InternalSessionID session, const DeviceID & deviceID, const std::string & rawPassword)
{
    boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

    auto sessionData = activeSessions.find(session);
    if(sessionData == activeSessions.end())
    {
        throw std::invalid_argument("SessionManager::reauthenticateSession(device) >"
                                    " Unable to find session with id ["
                                    + Convert::toString(session) + "].");
    }

    if(!sessionData->second->waitingForReauthentication)
    {
        throw std::logic_error("SessionManager::reauthenticateSession(user) > Session ["
                               + Convert::toString(session)
                               + "] is not eligible for re-authentication.");
    }

    //discards the expired token
    securityManager.removeAuthenticationToken(sessionData->second->token->getID(),
                                              sessionData->second->token->getUserID());

    //posts a request for a new token
    DeviceAuthenticationRequest authenticationRequest(deviceID, rawPassword, *this);
    AuthenticationTokenPromisePtr promise = securityManager.postRequest(authenticationRequest);

    try
    {
        AuthenticationTokenPtr newSessionToken = promise->get_future().get();

        //replaces the token associated with the session
        sessionData->second->token = newSessionToken;
        sessionData->second->tokenExpirationTime = newSessionToken->getExpirationTime();
        sessionData->second->waitingForReauthentication = false;
    }
    catch(const std::exception & ex)
    {
        logDebugMessage("(reauthenticateSession) > An exception was encountered"
                        " while attempting to authenticate device session ["
                        + Convert::toString(session) + "]: [" + std::string(ex.what()) + "].");
        throw;
    }

    if(dataCommit == SessionDataCommitType::ON_REAUTH)
    {//commits the session container to the database
        databaseManager.Sessions().updateSession(sessionData->second->data);
    }

    if(nextExpirationHandlerInvocation > sessionData->second->tokenExpirationTime)
    {//schedules an expiration handler invocation
        ++currentScheduledExpirationHandlers;
        nextExpirationHandlerInvocation = sessionData->second->tokenExpirationTime;
        threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                   nextExpirationHandlerInvocation);
    }
}

void SyncServer_Core::SessionManager::closeSession(InternalSessionID session)
{
    boost::lock_guard<boost::mutex> globalSessionsDataLock(globalSessionDataMutex);

    auto sessionData = activeSessions.find(session);
    if(sessionData != activeSessions.end())
    {//the session was found
        boost::mutex * sessionDataMutex = sessionData->second->dataMutex;

        {
            boost::lock_guard<boost::mutex> sessionDataLock(*sessionDataMutex);

            //invalidates the authentication token and closes the session
            securityManager.removeAuthenticationToken(sessionData->second->token->getID(),
                                                      sessionData->second->data->getUser());
            
            sessionData->second->data->closeSession();

            if(dataCommit == SessionDataCommitType::ON_CLOSE
               || dataCommit == SessionDataCommitType::ON_REAUTH
               || dataCommit == SessionDataCommitType::ON_UPDATE)
            {//commits the session container to the database
                if(sessionData->second->addedToDatabase)
                    databaseManager.Sessions().updateSession(sessionData->second->data);
                else
                    databaseManager.Sessions().addSession(sessionData->second->data);
            }

            if(sessionData->second->data->getDevice() == INVALID_DEVICE_ID)
            {//it is a user session
                auto userSessionIDs = userSessions.find(sessionData->second->data->getUser());

                if(userSessionIDs->second.size() == 1)
                    userSessions.erase(userSessionIDs);
                else
                    userSessionIDs->second.erase(std::remove(userSessionIDs->second.begin(),
                                                             userSessionIDs->second.end(),
                                                             session));
            }
            else
            {//it is a device session
                auto deviceSessionIDs = deviceSessions.find(sessionData->second->data->getDevice());

                if(deviceSessionIDs->second.size() == 1)
                    deviceSessions.erase(deviceSessionIDs);
                else
                    deviceSessionIDs->second.erase(std::remove(deviceSessionIDs->second.begin(),
                                                               deviceSessionIDs->second.end(),
                                                               session));
            }
        }

        sessionData->second->dataMutex = nullptr;
        activeSessions.erase(sessionData);
        delete sessionDataMutex;
    }
    else
    {
        throw std::invalid_argument("SessionManager::closeSession() > Unable to find session with id ["
                                    + Convert::toString(session) + "].");
    }
}

void SyncServer_Core::SessionManager::addDataSent
(InternalSessionID session, TransferredDataAmount amount)
{
    boost::shared_ptr<SessionData> sessionData;

    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        auto sessionDataIterator = activeSessions.find(session);
        if(sessionDataIterator != activeSessions.end())
            sessionData = sessionDataIterator->second;
        else
        {
            throw std::invalid_argument("SessionManager::addDataSent() > Unable to find session with id ["
                                        + Convert::toString(session) + "].");
        }
    }

    boost::lock_guard<boost::mutex> sessionDataLock(*sessionData->dataMutex);

    if(sessionData->data->getSessionType() != SessionType::DATA)
    {
        throw std::logic_error("SessionManager::addDataSent() > Cannot add transferred"
                               " data amount to session of type ["
                               + Convert::toString(sessionData->data->getSessionType())
                               + "]; [" + Convert::toString(session) + "].");
    }

    if(sessionData->data->isSessionActive())
    {
        sessionData->data->addDataSent(amount);

        if(dataCommit == SessionDataCommitType::ON_UPDATE)
        {
            databaseManager.Sessions().updateSession(sessionData->data);
        }
    }
    else
    {
        throw std::runtime_error("SessionManager::addDataSent() > Session ["
                                 + Convert::toString(session) + "] is not in an active state.");
    }
}

void SyncServer_Core::SessionManager::addDataReceived
(InternalSessionID session, TransferredDataAmount amount)
{
    boost::shared_ptr<SessionData> sessionData;

    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        auto sessionDataIterator = activeSessions.find(session);
        if(sessionDataIterator != activeSessions.end())
            sessionData = sessionDataIterator->second;
        else
        {
            throw std::invalid_argument("SessionManager::addDataReceived() >"
                                        " Unable to find session with id ["
                                        + Convert::toString(session) + "].");
        }
    }

    boost::lock_guard<boost::mutex> sessionDataLock(*sessionData->dataMutex);

    if(sessionData->data->getSessionType() != SessionType::DATA)
    {
        throw std::logic_error("SessionManager::addDataReceived() > Cannot add transferred"
                               " data amount to session of type ["
                               + Convert::toString(sessionData->data->getSessionType())
                               + "]; [" + Convert::toString(session) + "].");
    }

    if(sessionData->data->isSessionActive())
    {
        sessionData->data->addDataReceived(amount);

        if(dataCommit == SessionDataCommitType::ON_UPDATE)
        {
            databaseManager.Sessions().updateSession(sessionData->data);
        }
    }
    else
    {
        throw std::runtime_error("SessionManager::addDataReceived() > Session ["
                                 + Convert::toString(session)
                                 + "] is not in an active state.");
    }
}

void SyncServer_Core::SessionManager::addCommandsSent
(InternalSessionID session, unsigned long amount)
{
    boost::shared_ptr<SessionData> sessionData;

    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        auto sessionDataIterator = activeSessions.find(session);
        if(sessionDataIterator != activeSessions.end())
            sessionData = sessionDataIterator->second;
        else
        {
            throw std::invalid_argument("SessionManager::addCommandsSent() > "
                                        "Unable to find session with id ["
                                        + Convert::toString(session) + "].");
        }
    }

    boost::lock_guard<boost::mutex> sessionDataLock(*sessionData->dataMutex);

    if(sessionData->data->getSessionType() != SessionType::COMMAND)
    {
        throw std::logic_error("SessionManager::addCommandsSent() > Cannot add exchanged"
                               " commands amount to session of type ["
                               + Convert::toString(sessionData->data->getSessionType())
                               + "]; [" + Convert::toString(session) + "].");
    }

    if(sessionData->data->isSessionActive())
    {
        sessionData->data->addCommandsSent(amount);

        if(dataCommit == SessionDataCommitType::ON_UPDATE)
        {
            databaseManager.Sessions().updateSession(sessionData->data);
        }
    }
    else
    {
        throw std::runtime_error("SessionManager::addCommandsSent() > Session ["
                                 + Convert::toString(session)
                                 + "] is not in an active state.");
    }
}

void SyncServer_Core::SessionManager::addCommandsReceived
(InternalSessionID session, unsigned long amount)
{
     boost::shared_ptr<SessionData> sessionData;

    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        auto sessionDataIterator = activeSessions.find(session);
        if(sessionDataIterator != activeSessions.end())
            sessionData = sessionDataIterator->second;
        else
        {
            throw std::invalid_argument("SessionManager::addCommandsReceived() > "
                                        "Unable to find session with id ["
                                        + Convert::toString(session) + "].");
        }
    }

    boost::lock_guard<boost::mutex> sessionDataLock(*sessionData->dataMutex);

    if(sessionData->data->getSessionType() != SessionType::COMMAND)
    {
        throw std::logic_error("SessionManager::addCommandsReceived() > Cannot add exchanged"
                               " commands amount to session of type ["
                               + Convert::toString(sessionData->data->getSessionType())
                               + "]; [" + Convert::toString(session) + "].");
    }

    if(sessionData->data->isSessionActive())
    {
        sessionData->data->addCommandsReceived(amount);

        if(dataCommit == SessionDataCommitType::ON_UPDATE)
        {
            databaseManager.Sessions().updateSession(sessionData->data);
        }
    }
    else
    {
        throw std::runtime_error("SessionManager::addCommandsReceived() > Session ["
                                 + Convert::toString(session)
                                 + "] is not in an active state.");
    }
}

void SyncServer_Core::SessionManager::expirationHandler()
{
    std::deque<InternalSessionID> expiredSessionsForReauthentication;
    std::deque<InternalSessionID> expiredSessionsForTermination;

    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        ++totalExpirationHandlerInvocations;
        Timestamp nextHandlerInvocation = INVALID_DATE_TIME;

        for(auto currentSessionData : activeSessions)
        {
            if(currentSessionData.second->waitingForTermination)
            {//session is waiting for termination
                continue;
            }

            if(inactiveSessionExpirationTime > 0
               && !currentSessionData.second->data->isSessionPersistent()
               && (currentSessionData.second->data->getLastActivityTimestamp()
                   + bptime::seconds(inactiveSessionExpirationTime)) <= bptime::second_clock::universal_time())
            {//session has expired
                expiredSessionsForTermination.push_back(currentSessionData.first);
                currentSessionData.second->waitingForTermination = true;

                if(currentSessionData.second->waitingForReauthentication)
                {
                    logDebugMessage("(expirationHandler) > Session ["
                                    + Convert::toString(currentSessionData.first)
                                    + "] waiting for re-authentication has expired.");
                }

                continue;
            }

            if(currentSessionData.second->tokenExpirationTime <= bptime::second_clock::universal_time())
            {//token has expired
                if(!currentSessionData.second->waitingForReauthentication
                   && unauthenticatedSessionExpirationTime > 0)
                {//marks the session for re-authentication
                    expiredSessionsForReauthentication.push_back(currentSessionData.first);
                    currentSessionData.second->waitingForReauthentication = true;
                    currentSessionData.second->tokenExpirationTime +=
                            bptime::seconds(unauthenticatedSessionExpirationTime);
                }
                else
                {//marks the session for termination
                    logDebugMessage("(expirationHandler) > Session ["
                                    + Convert::toString(currentSessionData.first)
                                    + "] with expired token ["
                                    + Convert::toString(currentSessionData.second->token->getID())
                                    + "] scheduled for termination.");

                    expiredSessionsForTermination.push_back(currentSessionData.first);
                    currentSessionData.second->waitingForTermination = true;
                    continue;
                }
            }

            //re-calculates the next handler invocation time
            if(inactiveSessionExpirationTime == 0)
            {
                if(nextHandlerInvocation == INVALID_DATE_TIME
                   || nextHandlerInvocation > currentSessionData.second->tokenExpirationTime)
                {
                    nextHandlerInvocation = currentSessionData.second->tokenExpirationTime;
                }
            }
            else
            {
                if(nextHandlerInvocation == INVALID_DATE_TIME
                   || nextHandlerInvocation > currentSessionData.second->tokenExpirationTime
                   || nextHandlerInvocation > currentSessionData.second->data->getLastActivityTimestamp())
                {
                    Timestamp sessionExpirationTime =
                            currentSessionData.second->data->getLastActivityTimestamp()
                            + bptime::seconds(inactiveSessionExpirationTime);

                    nextHandlerInvocation =
                            (currentSessionData.second->tokenExpirationTime < sessionExpirationTime)
                            ? currentSessionData.second->tokenExpirationTime
                            : sessionExpirationTime;
                }
            }
        }

        //schedules the next handler invocation, if needed
        if(nextHandlerInvocation != INVALID_DATE_TIME
           && (nextExpirationHandlerInvocation <= bptime::second_clock::universal_time()
               || nextExpirationHandlerInvocation > nextHandlerInvocation))
        {
            logDebugMessage("(expirationHandler) > Scheduled next handler invocation for ["
                            + Convert::toString(nextHandlerInvocation) + "].");
            
            nextExpirationHandlerInvocation = nextHandlerInvocation;
            threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                       nextHandlerInvocation);
        }
        else if(currentScheduledExpirationHandlers == 1)
        {
            currentScheduledExpirationHandlers = 0;
            nextExpirationHandlerInvocation = INVALID_DATE_TIME;
        }
        else
            --currentScheduledExpirationHandlers;
    }

    for(InternalSessionID currentSessionID : expiredSessionsForReauthentication)
        onReauthenticationRequired(currentSessionID);

    for(InternalSessionID currentSessionID : expiredSessionsForTermination)
        onSessionExpired(currentSessionID);
}

void SyncServer_Core::SessionManager::getSessionHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
    
    SessionDataContainerPtr resultData;
    boost::shared_ptr<Instructions::GetSession> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::GetSession>(instruction);

    if(actualInstruction)
    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

        auto sessionData = activeSessions.find(actualInstruction->sessionID);
        if(sessionData != activeSessions.end())
        {
            resultData = sessionData->second->data;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::GetSession>
                    (new InstructionResults::GetSession{resultData});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::getSessionsByConstraintHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
    
    std::vector<SessionDataContainerPtr> resultData;
    boost::shared_ptr<Instructions::GetSessionsByConstraint> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::GetSessionsByConstraint>(instruction);

    if(actualInstruction)
    {
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);
        
        switch(actualInstruction->constraintType)
        {
            case GetSessionsConstraintType::ALL:
            {
                for(auto currentSessionData : activeSessions)
                {
                    resultData.push_back(currentSessionData.second->data);
                }
            }; break;
            
            case GetSessionsConstraintType::ALL_DEVICE:
            {
                for(auto currentSessionIDs : deviceSessions)
                {
                    for(InternalSessionID currentSessionID : currentSessionIDs.second)
                    {
                        resultData.push_back(activeSessions.at(currentSessionID)->data);
                    }
                }
            }; break;
            
            case GetSessionsConstraintType::ALL_USER:
            {
                for(auto currentUserSessionIDs : userSessions)
                {
                    for(InternalSessionID currentSessionID : currentUserSessionIDs.second)
                    {
                        resultData.push_back(activeSessions.at(currentSessionID)->data);
                    }
                }
            }; break;
            
            case GetSessionsConstraintType::ALL_FOR_DEVICE:
            {
                auto deviceSessionData = deviceSessions.find(actualInstruction->constraintValue);
                if(deviceSessionData != deviceSessions.end())
                {
                    for(InternalSessionID currentSessionID : deviceSessionData->second)
                    {
                        resultData.push_back(activeSessions.at(currentSessionID)->data);
                    }
                }
            }; break;
            
            case GetSessionsConstraintType::ALL_FOR_USER:
            {
                auto userSessionData = userSessions.find(actualInstruction->constraintValue);
                if(userSessionData != userSessions.end())
                {
                    for(InternalSessionID currentSessionID : userSessionData->second)
                    {
                        resultData.push_back(activeSessions.at(currentSessionID)->data);
                    }
                }
            }; break;
            
            default: ; break;
        }
    }

    auto result = boost::shared_ptr<InstructionResults::GetSessionsByConstraint>
                    (new InstructionResults::GetSessionsByConstraint{resultData});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::forceSessionExpirationHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
    boost::shared_ptr<Instructions::ForceSessionExpiration> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::ForceSessionExpiration>(instruction);

    if(actualInstruction)
    {
        {
            boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

            auto sessionData = activeSessions.find(actualInstruction->sessionID);
            if(sessionData == activeSessions.end())
            {
                try
                {
                    boost::throw_exception
                    (
                        std::invalid_argument("SessionManager::forceSessionExpirationHandler() >"
                                " Unable to find session with id ["
                                + Convert::toString(actualInstruction->sessionID) + "].")
                    );
                }
                catch(const std::invalid_argument &)
                {
                    instruction->getPromise().set_exception(boost::current_exception());
                    return;
                }
            }

            if(!sessionData->second->waitingForTermination)
            {
                sessionData->second->waitingForTermination = true;

                if(sessionData->second->waitingForReauthentication)
                {
                    logDebugMessage("(forceSessionExpirationHandler) > Session ["
                            + Convert::toString(sessionData->first)
                            + "] waiting for re-authentication has expired.");
                }
            }
        }

        resultValue = true;
        onSessionExpired(actualInstruction->sessionID);
    }

    auto result = boost::shared_ptr<InstructionResults::ForceSessionExpiration>
                    (new InstructionResults::ForceSessionExpiration{resultValue});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::forceSessionReauthenticationHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
    boost::shared_ptr<Instructions::ForceSessionReauthentication> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::ForceSessionReauthentication>(instruction);

    if(actualInstruction)
    {
        {
            boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);

            auto sessionData = activeSessions.find(actualInstruction->sessionID);
            if(sessionData == activeSessions.end())
            {
                try
                {
                    boost::throw_exception
                    (
                        std::invalid_argument("SessionManager::forceSessionReauthenticationHandler() > "
                                              "Unable to find session with id ["
                                              + Convert::toString(actualInstruction->sessionID) + "].")
                    );
                }
                catch(const std::invalid_argument &)
                {
                    instruction->getPromise().set_exception(boost::current_exception());
                    return;
                }
            }

            if(!sessionData->second->waitingForReauthentication)
            {
                sessionData->second->waitingForReauthentication = true;
                sessionData->second->tokenExpirationTime = 
                        bptime::second_clock::universal_time()
                        + bptime::seconds(unauthenticatedSessionExpirationTime);
            }
        }

        resultValue = true;
        onReauthenticationRequired(actualInstruction->sessionID);
    }

    auto result = boost::shared_ptr<InstructionResults::ForceSessionReauthentication>
                    (new InstructionResults::ForceSessionReauthentication{resultValue});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::forceExpirationProcessHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
    boost::shared_ptr<Instructions::ForceExpirationProcess> actualInstruction =
            boost::dynamic_pointer_cast<Instructions::ForceExpirationProcess>(instruction);

    if(actualInstruction)
    {
        Timestamp nextHandlerInvocation = bptime::second_clock::universal_time()
                                          + bptime::seconds(actualInstruction->delayTime);
        
        ++currentScheduledExpirationHandlers;
        if(nextExpirationHandlerInvocation > nextHandlerInvocation)
            nextExpirationHandlerInvocation = nextHandlerInvocation;
        
        threadPool.assignTimedTask(boost::bind(&SyncServer_Core::SessionManager::expirationHandler, this),
                                   nextHandlerInvocation);
        resultValue = true;
    }

    auto result = boost::shared_ptr<InstructionResults::ForceExpirationProcess>
                    (new InstructionResults::ForceExpirationProcess{resultValue});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::debugGetStateHandler
(InstructionPtr<SessionManagerInstructionType> instruction)
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
        boost::lock_guard<boost::mutex> globalSessionDataLock(globalSessionDataMutex);
        
        resultData += "threadPool size;" + Convert::toString(threadPool.getPoolSize()) + "\n";
        resultData += std::string("debugLogger;") + ((debugLogger != nullptr) ? "defined\n" : "not defined\n");
        resultData += "maxSessionsPerUser;" + Convert::toString(maxSessionsPerUser) + "\n";
        resultData += "maxSessionsPerDevice;" + Convert::toString(maxSessionsPerDevice) + "\n";
        resultData += "dataCommit;" + Convert::toString(dataCommit) + "\n";
        resultData += "inactiveSessionExpirationTime;" + Convert::toString(inactiveSessionExpirationTime) + "\n";
        resultData += "unauthenticatedSessionExpirationTime;" + Convert::toString(unauthenticatedSessionExpirationTime) + "\n";
        resultData += "lastSessionID;" + Convert::toString(lastSessionID) + "\n";
        resultData += "activeSessions size;" + Convert::toString(activeSessions.size()) + "\n";
        resultData += "onSessionExpired slots;" + Convert::toString(onSessionExpired.num_slots()) + "\n";
        resultData += "onReauthenticationRequired slots;" + Convert::toString(onReauthenticationRequired.num_slots()) + "\n";
        resultData += "nextExpirationHandlerInvocation;" + Convert::toString(nextExpirationHandlerInvocation) + "\n";
        resultData += "currentScheduledExpirationHandlers;" + Convert::toString(currentScheduledExpirationHandlers) + "\n";
        resultData += "totalExpirationHandlerInvocations;" + Convert::toString(totalExpirationHandlerInvocations) + "\n";
        resultData += "authorizationTokens size;" + Convert::toString(authorizationTokens.size()) + "\n";
        
        resultData += "userSessions size;" + Convert::toString(userSessions.size()) + "\n";
        for(auto userSessionsData : userSessions)
        {
            for(InternalSessionID currentID : userSessionsData.second)
            resultData += "US _ [" + Convert::toString(userSessionsData.first) + "];"
                        + Convert::toString(currentID) + ","
                        + Convert::toString(activeSessions.at(currentID)->token->getID()) + ","
                        + Convert::toString(activeSessions.at(currentID)->addedToDatabase) + ","
                        + Convert::toString(activeSessions.at(currentID)->waitingForReauthentication) + ","
                        + Convert::toString(activeSessions.at(currentID)->waitingForTermination)+ "\n";
        }
        
        resultData += "deviceSessions size;" + Convert::toString(userSessions.size()) + "\n";
        for(auto deviceSessionsData : deviceSessions)
        {
            for(InternalSessionID currentID : deviceSessionsData.second)
            resultData += "DS _ [" + Convert::toString(deviceSessionsData.first) + "];"
                        + Convert::toString(currentID) + ","
                        + Convert::toString(activeSessions.at(currentID)->token->getID()) + ","
                        + Convert::toString(activeSessions.at(currentID)->addedToDatabase) + ","
                        + Convert::toString(activeSessions.at(currentID)->waitingForReauthentication) + ","
                        + Convert::toString(activeSessions.at(currentID)->waitingForTermination)+ "\n";
        }
    }

    auto result = boost::shared_ptr<InstructionResults::DebugGetState>
                    (new InstructionResults::DebugGetState{resultData});
                    
    instruction->getPromise().set_value(result);
}

void SyncServer_Core::SessionManager::verifyAuthorizationToken(AuthorizationTokenPtr token)
{
    if(!token)
    {
       throw InvalidAuthorizationTokenException("SessionManager::verifyAuthorizationToken() > "
                "An empty token was supplied."); 
    }

    boost::lock_guard<boost::mutex> instructionDataLock(instructionDataMutex);

    auto requestedToken = authorizationTokens.find(token->getID());
    if(requestedToken != authorizationTokens.end())
    {
        if(*(requestedToken->second) == *token && token->getAuthorizedSet() == getType())
            authorizationTokens.erase(requestedToken);
        else
        {
            throw InvalidAuthorizationTokenException("SessionManager::verifyAuthorizationToken() > "
                    "The supplied token [" + Convert::toString(token->getID())
                    + "] does not match the one expected by the manager.");
        }
    }
    else
    {
        throw InvalidAuthorizationTokenException("SessionManager::verifyAuthorizationToken() > "
                "The supplied token [" + Convert::toString(token->getID()) + "] was not found.");
    }
}