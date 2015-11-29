/* 
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

#ifndef AUTHENTICATIONRULES_H
#define	AUTHENTICATIONRULES_H

#include <string>
#include <boost/regex.h>
#include "../Types/Types.h"
#include "../../Utilities/Strings/Common.h"

namespace Convert = Utilities::Strings;

namespace SecurityManagement_Rules
{
    /**
     * Base class for password rules.
     */
    class PasswordRule
    {
        public:
            virtual ~PasswordRule() {}
            
            /**
             * Checks if the specified password is valid against the parameters set in the rule.
             * 
             * @param rawPassword the password to be checked
             * @return <code>true</code>, if the password is valid
             */
            virtual bool isPasswordValid(const std::string & rawPassword) const = 0;
            
            /**
             * Retrieves the error message associated with the rule.
             * 
             * @return the error message
             */
            virtual std::string getErrorMessage() const = 0;
    };
    
    /**
     * Base class for name rules.
     */
    class NameRule
    {
        public:
            virtual ~NameRule() {}
            
            /**
             * Checks if the specified name is valid against the parameters set in the rule.
             * 
             * @param name the name to be checked
             * @return <code>true</code>, if the name is valid
             */
            virtual bool isNameValid(const std::string & name) const = 0;
            
            /**
             * Retrieves the error message associated with the rule.
             * 
             * @return the error message
             */
            virtual std::string getErrorMessage() const = 0;
    };
    
    /**
     * Minimum password length rule.
     */
    class MinPasswordLength : public PasswordRule
    {
        public:
            /**
             * Constructs the rule with the specified minimum password length.
             * 
             * @param minimumLength the minimum allowed password length
             */
            MinPasswordLength(unsigned int minimumLength)
            : minLength(minimumLength)
            {}
            
            bool isPasswordValid(const std::string & rawPassword) const
            {
                return (rawPassword.size() >= minLength);
            }
            
            std::string getErrorMessage() const
            {
                return "The password is below the minimum required length of ["
                        + Convert::toString(minLength) + "].";
            }
            
        private:
            unsigned int minLength;
    };
    
    /**
     * Allowed password structure rule (based on regular expressions).
     */
    class AllowedPasswordStructure : public PasswordRule
    {
        public:
            /**
             * Constructs the rule with the specified regular expression for the
             * required password structure.
             * 
             * @param regularExpression the regular expression to be used
             */
            AllowedPasswordStructure(std::string regularExpression)
            : allowedStruct(regularExpression)
            {}
            
            bool isPasswordValid(const std::string & rawPassword) const
            {
                return boost::regex_match(rawPassword, boost::regex(allowedStruct));
            }
            
            std::string getErrorMessage() const
            {
                return "The password does not match the allowed structure ["
                        + allowedStruct + "].";
            }
            
        private:
            std::string allowedStruct;
    };
    
    /**
     * Minimum name length rule.
     */
    class MinNameLength : public NameRule
    {
        public:
            /**
             * Constructs the rule with the specified minimum name length.
             * 
             * @param minimumLength the minimum allowed name length
             */
            MinNameLength(unsigned int minimumLength)
            : minLength(minimumLength)
            {}
            
            bool isNameValid(const std::string & name) const
            {
                return (name.size() >= minLength);
            }
            
            std::string getErrorMessage() const
            {
                return "The name is below the minimum required length of ["
                        + Convert::toString(minLength) + "].";
            }
            
        private:
            unsigned int minLength;
    };
    
    /**
     * Maximum name length rule.
     * 
     * Warning: The maximum length set here must not exceed the number of
     * characters the database is able to store.
     */
    class MaxNameLength : public NameRule
    {
        public:
            /**
             * Constructs the rule with the specified maximum name length.
             * 
             * @param maximumLength the maximum allowed name length
             */
            MaxNameLength(unsigned int maximumLength)
            : maxLength(maximumLength)
            {}
            
            bool isNameValid(const std::string & name) const
            {
                return (name.size() <= maxLength);
            }
            
            std::string getErrorMessage() const
            {
                return "The name is above the maximum allowed length of ["
                        + Convert::toString(maxLength) + "].";
            }
            
        private:
            unsigned int maxLength;
    };
    
    /**
     * Allowed name characters rule.
     */
    class AllowedNameCharacters : public NameRule
    {
        public:
            /**
             * Constructs the rule with the specified list of allowed characters.
             * 
             * @param allowedCharactersList the list of allowed characters
             */
            AllowedNameCharacters(std::string allowedCharactersList)
            : allowedChars(allowedCharactersList)
            {}
            
            bool isNameValid(const std::string & name) const
            {
                return (name.find_first_not_of(allowedChars) == std::string::npos);
            }
            
            std::string getErrorMessage() const
            {
                return "The name contains one or more characters not in the allowed set ["
                        + allowedChars + "].";
            }
            
        private:
            std::string allowedChars;
    };
    
    /**
     * Allowed name structure rule (based on regular expressions).
     */
    class AllowedNameStructure : public NameRule
    {
        public:
            /**
             * Constructs the rule with the specified regular expression for the
             * required name structure.
             * 
             * @param regularExpression the regular expression to be used
             */
            AllowedNameStructure(std::string regularExpression)
            : allowedStruct(regularExpression)
            {}
            
            bool isNameValid(const std::string & name) const
            {
                return boost::regex_match(name, boost::regex(allowedStruct));
            }
            
            std::string getErrorMessage() const
            {
                return "The name does not match the allowed structure ["
                        + allowedStruct + "].";
            }
            
        private:
            std::string allowedStruct;
    };
    
    /**
     * Required name characters rule.
     */
    class RequiredNameCharacters : public NameRule
    {
        public:
            /**
             * Constructs the rule with the specified list of required characters.
             * 
             * @param requiredCharactersList the list of required characters
             */
            RequiredNameCharacters(std::string requiredCharactersList)
            : requiredChars(requiredCharactersList)
            {}
            
            bool isNameValid(const std::string & name) const
            {
                for(char currentChar : requiredChars)
                {
                    if(name.find_first_of(currentChar) == std::string::npos)
                        return false;
                }
                
                return true;
            }
            
            std::string getErrorMessage() const
            {
                return "The name does not have one or more of the required characters ["
                        + requiredChars + "].";
            }
            
        private:
            std::string requiredChars;
    };
}

#endif	/* AUTHENTICATIONRULES_H */

