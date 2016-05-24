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

#include <vector>
#include "../../BasicSpec.h"
#include "../../../main/Utilities/Strings/Common.h"
#include "../../../main/SecurityManagement/Types/Types.h"
#include "../../../main/SecurityManagement/Types/Exceptions.h"
#include "../../../main/SecurityManagement/Crypto/PasswordGenerator.h"

using SecurityManagement_Crypto::PasswordGenerator;

SCENARIO("A password generator creates random passwords",
         "[PasswordGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("A PasswordGenerator")
    {
        unsigned int minPasswordSize = 1;
        unsigned int maxPasswordSize = 100;
        
        THEN("random passwords are generated")
        {
            std::vector<std::string> passwords;
            
            for(unsigned int i = minPasswordSize; i < maxPasswordSize; i++)
            {
                INFO("Checking random passwords with length [" + Utilities::Strings::toString(i) + "] ...");
                
                std::string newPassword = PasswordGenerator::getRandomASCIIPassword(i);
                CHECK(newPassword.size() == i);
                
                CHECK(std::find(passwords.begin(), passwords.end(), newPassword) == passwords.end());
                passwords.push_back(newPassword);
            }
            
            CHECK(passwords.size() == (maxPasswordSize - minPasswordSize));
        }
    }
}

SCENARIO("A password generator creates random validated passwords",
         "[PasswordGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("A PasswordGenerator and a validation function")
    {
        unsigned int minPassowrdSize = 10;
        unsigned int maxPasswordSize = 100;
        
        auto validationFn = [&](const std::string & password) -> SecurityManagement_Types::PasswordData
        {
            if(password.size() < minPassowrdSize)
                throw SecurityManagement_Types::InvalidPassswordException("Password too short");
            
            return SecurityManagement_Types::PasswordData();
        };
        
        THEN("random passwords are generated")
        {
            std::vector<std::string> passwords;
            
            for(unsigned int i = minPassowrdSize; i < maxPasswordSize; i++)
            {
                INFO("Checking validated random passwords with length [" + Utilities::Strings::toString(i) + "] ...");
                
                std::string newPassword = PasswordGenerator::getValidRandomASCIIPassword(i, validationFn, 3);
                CHECK(newPassword.size() == i);
                
                CHECK(std::find(passwords.begin(), passwords.end(), newPassword) == passwords.end());
                passwords.push_back(newPassword);
            }
            
            CHECK(passwords.size() == (maxPasswordSize - minPassowrdSize));
        }
    }
}

SCENARIO("A password generator fails to create passwords if validation fails",
         "[PasswordGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("A PasswordGenerator and a failing validation function")
    {
        unsigned int minPasswordSize = 1;
        unsigned int maxPasswordSize = 100;
        
        auto validationFn = [&](const std::string & password) -> SecurityManagement_Types::PasswordData
        {
            throw SecurityManagement_Types::InvalidPassswordException("test message");
        };
        
        THEN("password generation fails")
        {
            for(unsigned int i = minPasswordSize; i < maxPasswordSize; i++)
            {
                INFO("Checking failing passwords with length [" + Utilities::Strings::toString(i) + "] ...");
                
                CHECK_THROWS_AS(
                        PasswordGenerator::getValidRandomASCIIPassword(i, validationFn, 3),
                        std::runtime_error);
            }
        }
    }
}

SCENARIO("A password generator fails to create passwords when invalid arguments are supplied",
         "[PasswordGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("A PasswordGenerator and a password length of 0")
    {
        auto validationFn = [&](const std::string & password) -> SecurityManagement_Types::PasswordData
        {
            return SecurityManagement_Types::PasswordData();
        };
        
        THEN("password generation fails")
        {
            CHECK_THROWS_AS(
                    PasswordGenerator::getRandomASCIIPassword(0),
                    std::invalid_argument);
            
            CHECK_THROWS_AS(
                    PasswordGenerator::getValidRandomASCIIPassword(0, validationFn, 3),
                    std::invalid_argument);
        }
    }
}