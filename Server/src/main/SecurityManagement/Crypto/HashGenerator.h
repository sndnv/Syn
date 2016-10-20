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

#ifndef HASHGENERATOR_H
#define	HASHGENERATOR_H

#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>
#include <cryptopp/whrlpool.h>
#include <cryptopp/ripemd.h>

#include "../Types/Types.h"

using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::HashData;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::CryptoPPByte;

namespace SecurityManagement_Crypto
{
    /**
     * Class for generating cryptographic hashes.
     */
    class HashGenerator
    {
        public:
            /**
             * Generates a hash with the specified parameters but without a salt.
             * 
             * @param algorithm the hashing algorithm to be used
             * @param message the message to be hashed
             * @return the hash generated for the specified message
             */
            static const HashData getHash(HashAlgorithmType algorithm, const std::string & message)
            {
                return getHash(algorithm, SaltData(), message);
            }
            
            /**
             * Generates a hash with the specified parameters.
             * 
             * @param algorithm the hashing algorithm to be used
             * @param salt the salt to be used for hashing
             * @param message the message to be hashed
             * @return the hash generated for the specified message
             */
            static const HashData getHash(HashAlgorithmType algorithm, const SaltData & salt, const std::string & message)
            {
                switch(algorithm)
                {
                    case HashAlgorithmType::SHA_224: return getHashSHA_224(message, salt);
                    case HashAlgorithmType::SHA_256: return getHashSHA_256(message, salt);
                    case HashAlgorithmType::SHA_384: return getHashSHA_384(message, salt);
                    case HashAlgorithmType::SHA_512: return getHashSHA_512(message, salt);
                    
                    case HashAlgorithmType::SHA3_224: return getHashSHA3_224(message, salt);
                    case HashAlgorithmType::SHA3_256: return getHashSHA3_256(message, salt);
                    case HashAlgorithmType::SHA3_384: return getHashSHA3_384(message, salt);
                    case HashAlgorithmType::SHA3_512: return getHashSHA3_512(message, salt);
                    
                    case HashAlgorithmType::RIPEMD_160: return getHashRIPEMD_160(message, salt);
                    case HashAlgorithmType::RIPEMD_256: return getHashRIPEMD_256(message, salt);
                    case HashAlgorithmType::RIPEMD_320: return getHashRIPEMD_320(message, salt);
                    
                    case HashAlgorithmType::WHIRLPOOL: return getHashWHIRLPOOL(message, salt);
                    
                    default: throw std::invalid_argument("HashGenerator::getHash() > Unexpected hash algorithm encountered.");
                }
            }
            
            /**
             * Generates a hash with the specified parameters.
             * 
             * @param message the message to be hashed
             * @param salt the salt to be used for hashing
             * @param (template) THashAlgorithm template parameter denoting the hashing algorithm to be used
             * @return the hash generated for the specified message
             */
            template <typename THashAlgorithm>
            static const HashData getHash(const std::string & message, const SaltData & salt)
            {
                HashData digest(THashAlgorithm::DIGESTSIZE);
                
                THashAlgorithm hash;
                hash.Update(salt, salt.size());
                hash.Update(reinterpret_cast<const CryptoPPByte *>(message.c_str()), message.length());
                hash.Final(digest);
                
                return digest;
            }
            
            /** Generates a hash for the specified message and salt using the SHA224 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA_224(const std::string & message, const SaltData & salt)    { return getHash<CryptoPP::SHA224>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA256 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA_256(const std::string & message, const SaltData & salt)    { return getHash<CryptoPP::SHA256>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA384 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA_384(const std::string & message, const SaltData & salt)    { return getHash<CryptoPP::SHA384>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA512 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA_512(const std::string & message, const SaltData & salt)    { return getHash<CryptoPP::SHA512>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA3_224 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA3_224(const std::string & message, const SaltData & salt)   { return getHash<CryptoPP::SHA3_224>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA3_256 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA3_256(const std::string & message, const SaltData & salt)   { return getHash<CryptoPP::SHA3_256>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA3_384 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA3_384(const std::string & message, const SaltData & salt)   { return getHash<CryptoPP::SHA3_384>(message, salt); }
            /** Generates a hash for the specified message and salt using the SHA3_512 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashSHA3_512(const std::string & message, const SaltData & salt)   { return getHash<CryptoPP::SHA3_512>(message, salt); }
            /** Generates a hash for the specified message and salt using the Whirlpool algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashWHIRLPOOL(const std::string & message, const SaltData & salt)  { return getHash<CryptoPP::Whirlpool>(message, salt); }
            /** Generates a hash for the specified message and salt using the RIPEMD160 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashRIPEMD_160(const std::string & message, const SaltData & salt) { return getHash<CryptoPP::RIPEMD160>(message, salt); }
            /** Generates a hash for the specified message and salt using the RIPEMD256 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashRIPEMD_256(const std::string & message, const SaltData & salt) { return getHash<CryptoPP::RIPEMD256>(message, salt); }
            /** Generates a hash for the specified message and salt using the RIPEMD320 algorithm.\n\n@return the hash generated for the specified message*/
            static const HashData getHashRIPEMD_320(const std::string & message, const SaltData & salt) { return getHash<CryptoPP::RIPEMD320>(message, salt); }
            
        private:
            HashGenerator() {}
    };
}
#endif	/* HASHGENERATOR_H */

