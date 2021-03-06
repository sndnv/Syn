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

#ifndef SESSIONMANAGER_H
#define	SESSIONMANAGER_H

#include <string>
#include <deque>

#include <boost/thread/mutex.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/unordered_map.hpp>

#include "../Common/Types.h"
#include "../Utilities/FileLogger.h"
#include "../Utilities/ThreadPool.h"
#include "../SecurityManagement/SecurityManager.h"
#include "../SecurityManagement/Types/SecurityTokens.h"
#include "../SecurityManagement/Types/SecurityRequests.h"
#include "../SecurityManagement/Types/Exceptions.h"
#include "../SecurityManagement/Interfaces/Securable.h"
#include "../DatabaseManagement/DatabaseManager.h"
#include "../DatabaseManagement/Containers/SessionDataContainer.h"
#include "../InstructionManagement/Sets/SessionManagerInstructionSet.h"

#include "Types/Types.h"
#include "Types/Exceptions.h"
#include "../EntityManagement/Interfaces/DatabaseLoggingSource.h"

//Common
using Common_Types::LogSeverity;
using Common_Types::UserID;
using Common_Types::DeviceID;
using Common_Types::SessionID;
using Common_Types::Seconds;
using Common_Types::Timestamp;
using Common_Types::TransferredDataAmount;
using Common_Types::SessionType;
using Common_Types::UserAccessLevel;
using Common_Types::INVALID_DATE_TIME;
using Common_Types::INVALID_TRANSFERRED_DATA_AMOUNT;

//Database and Security Management
using SyncServer_Core::DatabaseManager;
using SyncServer_Core::SecurityManager;

//Exceptions
using SessionManagement_Types::TooManyUserSessionsException;
using SessionManagement_Types::TooManyDeviceSessionsException;

//Containers
using DatabaseManagement_Containers::SessionDataContainer;
using DatabaseManagement_Containers::SessionDataContainerPtr;

//Security
using SecurityManagement_Types::TokenID;
using SecurityManagement_Types::AuthorizationTokenPtr;
using SecurityManagement_Types::AuthenticationTokenPtr;
using SecurityManagement_Types::AuthenticationTokenPromisePtr;
using SecurityManagement_Types::UserAuthenticationRequest;
using SecurityManagement_Types::DeviceAuthenticationRequest;
using SecurityManagement_Types::InvalidAuthorizationTokenException;

//Misc
using SessionManagement_Types::InternalSessionID;
using SessionManagement_Types::SessionDataCommitType;
using SessionManagement_Types::GetSessionsConstraintType;
using SessionManagement_Types::INVALID_INTERNAL_SESSION_ID;
using InstructionManagement_Types::SessionManagerInstructionType;

namespace SyncServer_Core
{
    /**
     * Class for managing session-related activities.
     */
    class SessionManager final
    : public SecurityManagement_Interfaces::Securable,
      public InstructionManagement_Interfaces::InstructionTarget<SessionManagerInstructionType>,
      public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            /** Parameters structure holding <code>SessionManager</code> configuration. */
            struct SessionManagerParameters
            {
                /** Number of threads to create in the internal thread pool */
                unsigned long threadPoolSize;
                /** Reference to a valid database manager instance */
                DatabaseManager & databaseManager;
                /** Reference to a valid security manager instance */
                SecurityManager & securityManager;
                /** Maximum number of sessions per user (0 == unlimited) */
                unsigned int maxSessionsPerUser;
                /** Maximum number of sessions per device (0 == unlimited) */
                unsigned int maxSessionsPerDevice;
                /** Type of session data to database commit */
                SessionDataCommitType dataCommit;
                /** Session expiration time, after last activity (in seconds; 0 == unlimited) */
                Seconds inactiveSessionExpirationTime;
                /** Session expiration time, after its associated token has expired (in seconds; 0 == expire immediately) */
                Seconds unauthenticatedSessionExpirationTime;
            };
            
            /**
             * Constructs a new session manager object with the specified configuration.
             * 
             * @param params the manager configuration
             * @param debugLogger pointer to an initialised <code>FileLogger</code> (if any)
             * 
             * @throw invalid_argument if the specified session data commit type is not valid
             */
            SessionManager(const SessionManagerParameters & params, Utilities::FileLoggerPtr debugLogger = Utilities::FileLoggerPtr());
            
            /**
             * Clears all data structures, frees all memory associated with the
             * sessions and disconnects all events.
             */
            ~SessionManager();
            
            SessionManager() = delete;
            SessionManager(const SessionManager&) = delete;
            SessionManager& operator=(const SessionManager&) = delete;
            
            void postAuthorizationToken(const AuthorizationTokenPtr token) override;
            
            SecurityManagement_Types::SecurableComponentType getComponentType() const override
            {
                return SecurityManagement_Types::SecurableComponentType::SESSION_MANAGER;
            }
            
            bool registerInstructionSet(InstructionManagement_Sets::InstructionSetPtr<SessionManagerInstructionType> set) const override;
            
            InstructionManagement_Types::InstructionSetType getType() const override
            { return InstructionManagement_Types::InstructionSetType::SESSION_MANAGER; }
            
            std::string getSourceName() const override
            {
                return "SessionManager";
            }
            
            bool registerLoggingHandler(const std::function<void(LogSeverity, const std::string &)> handler) override
            {
                if(!dbLogHandler)
                {
                    dbLogHandler = handler;
                    return true;
                }
                else
                {
                    logMessage(LogSeverity::Error, "(registerLoggingHandler) >"
                            " The database logging handler is already set.");
                    
                    return false;
                }
            }
            
            /**
             * Attempts to open a new user session.
             * 
             * Exceptions thrown by the authentication process:
             * - <code>UserNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * @param username the name of the user
             * @param rawPassword the raw user password
             * @param type the type of session to be opened
             * @param isPersistent denotes whether the session is to be persistent or not
             * @return the internal ID of the new session 
             * 
             * @throw invalid argument if an invalid session type is specified
             * @throw TooManyUserSessionsException if the user has exceeded the 
             * maximum number of concurrent sessions
             */
            InternalSessionID openSession(const std::string & username, const std::string & rawPassword,
                                          SessionType type, bool isPersistent = false);
            
            /**
             * Attempts to open a new device session.
             * 
             * Exceptions thrown by the authentication process:
             * - <code>DeviceNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * @param deviceID the ID of the device
             * @param rawPassword the raw device password
             * @param type the type of session to be opened
             * @param isPersistent denotes whether the session is to be persistent or not
             * @return the internal ID of the new session
             * 
             * @throw invalid argument if an invalid session type is specified
             * @throw TooManyDeviceSessionsException if the device has exceeded the 
             * maximum number of concurrent sessions
             */
            InternalSessionID openSession(const DeviceID & deviceID, const std::string & rawPassword,
                                          SessionType type, bool isPersistent = false);
            
            /**
             * Attempts to re-authenticate a user session for which the authentication
             * token has expired.
             * 
             * Exceptions thrown by the authentication process:
             * - <code>UserNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * @param session the internal ID of the session to re-authenticate
             * @param username the name of the user associated with the session
             * @param rawPassword the raw password associated with the user
             * 
             * @throw invalid_argument if the session cannot be found
             */
            void reauthenticateSession(InternalSessionID session, const std::string & username,
                                       const std::string & rawPassword);
            
            /**
             * Attempts to re-authenticate a user session for which the authentication
             * token has expired.
             * 
             * Exceptions thrown by the authentication process:
             * - <code>DeviceNotFoundException</code><br>
             * - <code>InvalidPassswordException</code><br>
             * - <code>UserLockedException</code><br>
             * - <code>InsufficientUserAccessException</code><br>
             * 
             * @param session the internal ID of the session to re-authenticate
             * @param deviceID the ID of the device associated with the session
             * @param rawPassword the raw password associated with the device
             * 
             * @throw invalid_argument if the session cannot be found
             */
            void reauthenticateSession(InternalSessionID session, const DeviceID & deviceID, 
                                       const std::string & rawPassword);
            
            /**
             * Attempts to close the specified session.
             * 
             * @param session the internal ID of the session to be closed
             * 
             * @throw invalid_argument if the session could not be found
             */
            void closeSession(InternalSessionID session);
            
            /**
             * Adds the specified amount of data sent to the specified session.
             * 
             * @param session the affected session
             * @param amount the amount of data to be added
             * 
             * @throw invalid_argument if the session ID is not valid
             * @throw logic_error if the specified session is not a DATA session
             * @throw runtime_error if the specified session is not in an active state
             */
            void addDataSent(InternalSessionID session, TransferredDataAmount amount);
            
            /**
             * Adds the specified amount of data received to the specified session.
             * 
             * @param session the affected session
             * @param amount the amount of data to be added
             * 
             * @throw invalid_argument if the session ID is not valid
             * @throw logic_error if the specified session is not a DATA session
             * @throw runtime_error if the specified session is not in an active state
             */
            void addDataReceived(InternalSessionID session, TransferredDataAmount amount);
            
            /**
             * Adds the specified amount of commands sent to the specified session.
             * 
             * @param session the affected session
             * @param amount the number of commands to be added
             * 
             * @throw invalid_argument if the session ID is not valid
             * @throw logic_error if the specified session is not a COMMAND session
             * @throw runtime_error if the specified session is not in an active state
             */
            void addCommandsSent(InternalSessionID session, unsigned long amount);
            
            /**
             * Adds the specified amount of commands received to the specified session.
             * 
             * @param session the affected session
             * @param amount the number of commands to be added
             * 
             * @throw invalid_argument if the session ID is not valid
             * @throw logic_error if the specified session is not a COMMAND session
             * @throw runtime_error if the specified session is not in an active state
             */
            void addCommandsReceived(InternalSessionID session, unsigned long amount);
            
            /**
             * Attaches the supplied handler to the <code>onSessionExpired</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onSessionExpiredEventAttach
            (std::function<void(InternalSessionID)> function)
            {
                return onSessionExpired.connect(function);
            }
            
            /**
             * Attaches the supplied handler to the <code>onReauthenticationRequired</code> event.
             * 
             * @param function the event handler to be attached
             * @return the resulting signal connection object
             */
            boost::signals2::connection onReauthenticationRequiredEventAttach
            (std::function<void(InternalSessionID)> function)
            {
                return onReauthenticationRequired.connect(function);
            }
            
        private:
            /** Data structure holding session data. */
            struct SessionData
            {
                /** Session data container */
                SessionDataContainerPtr data;
                /** Authentication token */
                AuthenticationTokenPtr token;
                /** Authentication token expiration time */
                Timestamp tokenExpirationTime;
                /** Denotes whether the container has been added to the database */
                bool addedToDatabase;
                /** Denotes whether re-authentication is pending for the session */
                bool waitingForReauthentication;
                /** Denotes whether termination is pending for the session */
                bool waitingForTermination;
                /** Local session data mutex; protects access to the session container's command & data fields */
                boost::mutex * dataMutex;
            };
            
            Utilities::ThreadPool threadPool;           //threads for handling request processing
            mutable Utilities::FileLoggerPtr debugLogger; //logger for debugging
            std::function<void(LogSeverity, const std::string &)> dbLogHandler;//database log handler
            
            //Required Managers
            DatabaseManager & databaseManager;
            SecurityManager & securityManager;
            
            //Configuration
            unsigned int maxSessionsPerUser;                //max concurrent sessions per user (0 == unlimited)
            unsigned int maxSessionsPerDevice;              //max concurrent sessions per device (0 == unlimited)
            SessionDataCommitType dataCommit;               //specifies when (if) session data is to be pushed to DB
            Seconds inactiveSessionExpirationTime;          //inactive session expiration time; in seconds (0 == unlimited)
            Seconds unauthenticatedSessionExpirationTime;   //expiration time for sessions waiting for re-auth; in seconds (0 == drop immediately)
            
            //Session Data
            mutable boost::mutex globalSessionDataMutex;    //mutex for all session data
            InternalSessionID lastSessionID;                //the last internal session ID that was assigned
            boost::unordered_map<UserID, std::deque<InternalSessionID>> userSessions;       //user name -> list of active sessions
            boost::unordered_map<DeviceID, std::deque<InternalSessionID>> deviceSessions;   //device id -> list of active sessions
            boost::unordered_map<InternalSessionID, boost::shared_ptr<SessionData>> activeSessions; //active session data
            
            //Events
            boost::signals2::signal<void (InternalSessionID)> onSessionExpired;
            boost::signals2::signal<void (InternalSessionID)> onReauthenticationRequired;
            
            //Expiration
            Timestamp nextExpirationHandlerInvocation;          //next scheduled expiration handler run
            unsigned long currentScheduledExpirationHandlers;   //number of currently scheduled handlers
            unsigned long totalExpirationHandlerInvocations;    //total number of schedule handler runs
            
            //Instruction Management
            boost::mutex instructionDataMutex;  //instruction data mutex
            boost::unordered_map<TokenID, AuthorizationTokenPtr> authorizationTokens; //expected authorization tokens
            
            /**
             * Session and authentication token expiration handler.
             */
            void expirationHandler();
            
            //Instruction Handlers
            void getSessionHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            void getSessionsByConstraintHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            void forceSessionExpirationHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            void forceSessionReauthenticationHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            void forceExpirationProcessHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            void debugGetStateHandler(InstructionPtr<SessionManagerInstructionType> instruction);
            
            //Instruction Handlers Function Binds
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> getSessionHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::getSessionHandler, this, _1);
            
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> getSessionsByConstraintHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::getSessionsByConstraintHandler, this, _1);
            
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> forceSessionExpirationHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::forceSessionExpirationHandler, this, _1);
            
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> forceSessionReauthenticationHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::forceSessionReauthenticationHandler, this, _1);
            
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> forceExpirationProcessHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::forceExpirationProcessHandler, this, _1);
            
            std::function<void(InstructionPtr<SessionManagerInstructionType>)> debugGetStateHandlerBind =
                boost::bind(&SyncServer_Core::SessionManager::debugGetStateHandler, this, _1);
            
            /**
             * Verifies the supplied authentication token.
             * 
             * Note: The token is removed the the list of expected authorization tokens
             * 
             * @param token the token to be verified
             * 
             * @throw InvalidAuthorizationTokenException if an invalid token is encountered
             */
            void verifyAuthorizationToken(AuthorizationTokenPtr token);
            
            /**
             * Logs the specified message, if the database log handler is set.
             * 
             * Note: If a debugging file logger is assigned, the message is sent to it.
             * 
             * @param severity the severity associated with the message/event
             * @param message the message to be logged
             */
            void logMessage(LogSeverity severity, const std::string & message) const
            {
                if(dbLogHandler)
                    dbLogHandler(severity, message);
                
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "SessionManager " + message);
            }
    };
}

#endif	/* SESSIONMANAGER_H */
