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

#ifndef SALTGENERATOR_H
#define	SALTGENERATOR_H

#include <cryptopp/osrng.h>

#include "../Types/Types.h"

using SecurityManagement_Types::CryptoPPByte;
using SecurityManagement_Types::SaltSize;
using SecurityManagement_Types::SaltData;

namespace SecurityManagement_Crypto
{
    /**
     * Class for generating cryptographic salts.
     */
    class SaltGenerator
    {
        public:
            /**
             * Generates a new random salt with the specified size.
             * 
             * @param size the size of the generated salt
             * @return the requested salt
             */
            static const SaltData getRandomSalt(SaltSize size)
            {
                CryptoPP::AutoSeededRandomPool rng;
                SaltData salt(size);
                rng.GenerateBlock(salt, size);
                
                return salt;
            }
            
        private:
            SaltGenerator() {}
    };
}

#endif	/* SALTGENERATOR_H */

