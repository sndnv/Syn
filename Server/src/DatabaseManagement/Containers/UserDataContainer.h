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

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"

using Common_Types::INVALID_DATE_TIME;
using Common_Types::Timestamp;
using DatabaseManagement_Types::UserAccessLevel;
using DatabaseManagement_Types::UserID;

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
             * @param maximumFileSize
             * @param maximumNumberOfFiles
             * @param access
             * @param forcePassReset
             */
            UserDataContainer(std::string user, std::string pass, unsigned long maximumFileSize, unsigned long maximumNumberOfFiles, UserAccessLevel access, bool forcePassReset)
                    : DataContainer(boost::uuids::random_generator()(), DatabaseObjectType::USER), username(user), password(pass), maxFileSize(maximumFileSize), maxNumberOfFiles(maximumNumberOfFiles),
                      accessLevel(access), forcePasswordReset(forcePassReset), isLocked(false), timestampCreation(INVALID_DATE_TIME), timestampLastLogin(INVALID_DATE_TIME)
            {}
            
            /**
             * Creates a new user data container.
             * 
             * Note: Used when supplying data from the database.
             * 
             * @param id
             * @param user
             * @param pass
             * @param maximumFileSize
             * @param maximumNumberOfFiles
             * @param access
             * @param forcePassReset
             * @param locked
             * @param createTime
             * @param lastLoginTime
             */
            UserDataContainer(UserID id, std::string user, std::string pass, unsigned long maximumFileSize, unsigned long maximumNumberOfFiles, UserAccessLevel access,
                              bool forcePassReset, bool locked, Timestamp createTime, Timestamp lastLoginTime)
                    : DataContainer(id, DatabaseObjectType::USER), username(user), password(pass), maxFileSize(maximumFileSize), maxNumberOfFiles(maximumNumberOfFiles), 
                      accessLevel(access), forcePasswordReset(forcePassReset), isLocked(locked), timestampCreation(createTime), timestampLastLogin(lastLoginTime)
            {}
            
            UserDataContainer() = delete;                                       //No default constructor
            UserDataContainer(const UserDataContainer&) = default;              //Default copy constructor
            ~UserDataContainer() = default;                                     //Default destructor
            UserDataContainer& operator=(const UserDataContainer&) = default;   //Default assignment operator
            
            UserID getUserID()                                  const { return containerID; }
            std::string getUsername()                           const { return username; }
            unsigned long getMaxFileSize()                      const { return maxFileSize; }
            unsigned long getMaxNumberOfFiles()                 const { return maxNumberOfFiles; }
            UserAccessLevel getUserAccessLevel()                const { return accessLevel; }
            bool getForcePasswordReset()                        const { return forcePasswordReset; }
            bool getLockedState()                               const { return isLocked; }
            Timestamp getCreationTimestamp()                    const { return timestampCreation; }
            Timestamp getLastLoginTimestamp()                   const { return timestampLastLogin; }
            
            void resetPassword(std::string newPassword)             { password = newPassword; modified = true; }
            void forceUserPasswordReset()                           { forcePasswordReset = true; modified = true; }
            void setUserAccessLevel(UserAccessLevel newLevel)       { accessLevel = newLevel; modified = true; }
            void setMaxFileSize(unsigned long newSize)              { maxFileSize = newSize; modified = true; }
            void setMaxNumberOfFiles(unsigned long numberOfFiles)   { maxNumberOfFiles = numberOfFiles; modified = true; }
            void lockUser()                                         { isLocked = true; modified = true; }
            void unlockUser()                                       { isLocked = false; modified = true; }
            void setLastLoginTimestamp()                            { timestampLastLogin = boost::posix_time::second_clock::local_time(); modified = true; }

        private:
            std::string getPassword() const { return password; }
            
            std::string username;
            std::string password;
            unsigned long maxFileSize;
            unsigned long maxNumberOfFiles;
            UserAccessLevel accessLevel;
            bool forcePasswordReset;
            bool isLocked;
            Timestamp timestampCreation;
            Timestamp timestampLastLogin;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::UserDataContainer> UserDataContainerPtr;
}

#endif	/* USERDATACONTAINER_H */

