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

#ifndef PASSWORDGENERATOR_H
#define	PASSWORDGENERATOR_H

#include <string>
#include <cryptopp/osrng.h>
#include "../Types/Types.h"
#include "../Types/Exceptions.h"

using SecurityManagement_Types::PasswordData;
using SecurityManagement_Types::InvalidPassswordException;

namespace SecurityManagement_Crypto
{
    /**
     * Class for generating random passwords.
     */
    class PasswordGenerator
    {
        public:
            /**
             * Generates a new random password with the ASCII character set.
             * 
             * @param length the length of the generated password
             * @throw runtime_error if a password could not be generated
             * @throw invalid_argument if a password length of 0 is supplied
             * @return the generated password
             */
            static const std::string getRandomASCIIPassword(unsigned int length)
            {
                if(length == 0)
                {
                    throw std::invalid_argument("PasswordGenerator::getRandomASCIIPassword >"
                            " Password length of 0 is not valid.");
                }
                
                static const std::string charsetASCII = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,./;'[]<>?:{}|\\\"`~!@#$%^&*()_+";
                CryptoPP::AutoSeededRandomPool rng;
                
                std::string password;
                password.reserve(length);
                
                for(unsigned int i = 0; i < length; i++)
                {
                    password.push_back(charsetASCII[rng.GenerateWord32(0, charsetASCII.size())]);
                }
                
                if(password.size() != length)
                {
                    throw std::runtime_error("PasswordGenerator::getRandomASCIIPassword >"
                            " Failed to generate a password.");
                }
                
                return password;
            }
            
            /**
             * Generates a new valid random password with the ASCII character set.
             * 
             * @param length the length of the generated password
             * @param validationFunction password validation function
             * @param maxAttempts the maximum number of attempts to be made to generate a password
             * @throw runtime_error if a valid password could not be generated
             * @throw invalid_argument if a password length of 0 is supplied
             * @return the generated password
             */
            static const std::string getValidRandomASCIIPassword
            (unsigned int length,
             std::function<PasswordData (const std::string & rawPassword)> validationFunction,
             unsigned int maxAttempts = 10)
            {
                if(length == 0)
                {
                    throw std::invalid_argument("PasswordGenerator::getValidRandomASCIIPassword >"
                            " Password length of 0 is not valid.");
                }
                
                static const std::string charsetASCII = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,./;'[]<>?:{}|\\\"`~!@#$%^&*()_+";
                CryptoPP::AutoSeededRandomPool rng;
                
                std::string password;
                password.reserve(length);
                
                unsigned int attempts = 0;
                bool done = false;
                while(!done)
                {
                    try
                    {
                        for(unsigned int i = 0; i < length; i++)
                        {
                            password.push_back(charsetASCII[rng.GenerateWord32(0, charsetASCII.size())]);
                        }
                        
                        validationFunction(password);
                        done = true;
                    }
                    catch(InvalidPassswordException & e)
                    {
                        password.clear();
                        ++attempts;
                        if(attempts >= maxAttempts)
                            done = true;
                    }
                }
                
                if(password.size() != length)
                {
                    throw std::runtime_error("PasswordGenerator::getValidRandomASCIIPassword >"
                            " Failed to generate a valid password.");
                }
                
                return password;
            }
            
        private:
            PasswordGenerator() {}
    };
}

#endif	/* PASSWORDGENERATOR_H */

