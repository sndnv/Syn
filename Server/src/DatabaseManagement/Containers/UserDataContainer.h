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

#ifndef USERDATACONTAINER_H
#define	USERDATACONTAINER_H

#include <deque>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"
#include "../../SecurityManagement/Types/Types.h"
#include "../../SecurityManagement/Rules/AuthorizationRules.h"

using Common_Types::INVALID_DATE_TIME;
using Common_Types::Timestamp;
using Common_Types::UserAccessLevel;
using Common_Types::UserID;
using SecurityManagement_Types::SaltSize;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::PasswordData;
using SecurityManagement_Rules::UserAuthorizationRule;

namespace DatabaseManagement_Containers
{
    class UserDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            /**
             * Creates a new user data container.
             * 
             * Note: Used when supplying data to the database.
             * 
             * @param user
             * @param pass
             * @param access
             * @param forcePassReset
             */
            UserDataContainer(std::string user, PasswordData pass, UserAccessLevel access, bool forcePassReset)
            : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::USER),
              username(user), password(pass), accessLevel(access), forcePasswordReset(forcePassReset), isLocked(false),
              timestampCreation(INVALID_DATE_TIME), timestampLastSuccessfulAuthentication(INVALID_DATE_TIME),
              timestampLastFailedAuthentication(INVALID_DATE_TIME), failedAuthenticationAttempts(0)
            {}
            
            /**
             * Creates a new user data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id
             * @param user
             * @param pass
             * @param access
             * @param forcePassReset
             * @param locked
             * @param createTime
             * @param lastSuccessfulAuthTime
             * @param lastFailedAuthTime
             * @param failedAuthAttempts
             * @param accessRules
             */
            UserDataContainer(UserID id, std::string user, PasswordData pass, UserAccessLevel access,
                              bool forcePassReset, bool locked, Timestamp createTime, Timestamp lastSuccessfulAuthTime,
                              Timestamp lastFailedAuthTime, unsigned int failedAuthAttempts,
                              std::deque<UserAuthorizationRule> accessRules)
            : DataContainer(id, DatabaseObjectType::USER), username(user), password(pass), accessLevel(access),
              forcePasswordReset(forcePassReset), isLocked(locked), timestampCreation(createTime),
              timestampLastSuccessfulAuthentication(lastSuccessfulAuthTime),
              timestampLastFailedAuthentication(lastFailedAuthTime), failedAuthenticationAttempts(failedAuthAttempts),
              rules(accessRules)
            {}
            
            UserDataContainer() = delete;
            UserDataContainer(const UserDataContainer&) = default;
            ~UserDataContainer() = default;
            UserDataContainer& operator=(const UserDataContainer&) = default;
            
            UserID getUserID()                                  const { return containerID; }
            std::string getUsername()                           const { return username; }
            UserAccessLevel getUserAccessLevel()                const { return accessLevel; }
            bool getForcePasswordReset()                        const { return forcePasswordReset; }
            bool isUserLocked()                                 const { return isLocked; }
            Timestamp getCreationTimestamp()                    const { return timestampCreation; }
            Timestamp getLastSuccessfulAuthenticationTimestamp()const { return timestampLastSuccessfulAuthentication; }
            Timestamp getLastFailedAuthenticationTimestamp()    const { return timestampLastFailedAuthentication; }
            unsigned int getFailedAuthenticationAttempts()      const { return failedAuthenticationAttempts; }
            const std::deque<UserAuthorizationRule> & getAccessRules() const { return rules; }
            bool passwordsMatch(const PasswordData & otherPassword)    const { return (password == otherPassword); }
            const PasswordData & getPasswordData()              const { return password; }
            
            const SaltData getPasswordSalt(SaltSize size) const
            {
                return ((size >= password.size() || size == 0) ? SaltData() : SaltData(password.data(), size));
            }
            
            void resetPassword(const PasswordData & newPassword)    { if(newPassword.size() > 0) { password = newPassword; modified = true; } }
            void forceUserPasswordReset()                           { forcePasswordReset = true; modified = true; }
            void setUserAccessLevel(UserAccessLevel newLevel)       { accessLevel = newLevel; modified = true; }
            void setLockedState(bool locked)                        { isLocked = locked; modified = true; }
            void addAccessRule(UserAuthorizationRule rule)          { rules.push_back(rule); modified = true; }
            void removeAccessRule(UserAuthorizationRule rule)       { rules.erase(std::remove(rules.begin(), rules.end(), rule));  modified = true; }
            void clearAccessRules()                                 { rules.clear(); modified = true; }
            
            void resetFailedAuthenticationAttempts()
            {
                failedAuthenticationAttempts = 0;
                modified = true;
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

        private:
            std::string username;
            PasswordData password;
            UserAccessLevel accessLevel;
            bool forcePasswordReset;
            bool isLocked;
            Timestamp timestampCreation;
            Timestamp timestampLastSuccessfulAuthentication;
            Timestamp timestampLastFailedAuthentication;
            unsigned int failedAuthenticationAttempts;
            std::deque<UserAuthorizationRule> rules;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::UserDataContainer> UserDataContainerPtr;
}

#endif	/* USERDATACONTAINER_H */

