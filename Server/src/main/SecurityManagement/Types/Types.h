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

#ifndef SECURITY_MANAGEMENT_TYPES_H
#define	SECURITY_MANAGEMENT_TYPES_H

#include <string>
#include <climits>
#include <cryptopp/config.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include <cryptopp/dh.h>

#include "../../Common/Types.h"

namespace SecurityManagement_Types
{
    typedef byte CryptoPPByte;
    
    typedef unsigned int KeySize;
    const KeySize INVALID_KEY_SIZE = 0;
    
    typedef unsigned int IVSize;
    const IVSize INVALID_IV_SIZE = 0;
    
    typedef unsigned int RandomDataSize;
    const RandomDataSize INVALID_RANDOM_DATA_SIZE = 0;
    
    typedef CryptoPP::SecByteBlock KeyData;
    typedef CryptoPP::SecByteBlock IVData;
    typedef CryptoPP::SecByteBlock SaltData;
    typedef CryptoPP::SecByteBlock HashData;
    typedef CryptoPP::SecByteBlock RandomData;
    typedef CryptoPP::SecByteBlock PasswordData; // == SaltData+HashData
    typedef Common_Types::ByteData PlaintextData;
    typedef Common_Types::ByteData CiphertextData;
    typedef Common_Types::ByteData SignedData;
    typedef Common_Types::ByteData MixedData; //for storing cipher- and plaintext data
    
    const PlaintextData EMPTY_PLAINTEXT_DATA = "";
    const CiphertextData EMPTY_CIPHERTEXT_DATA = "";
    const SignedData EMPTY_SIGNED_DATA = "";
    
    enum class KeyExchangeType { INVALID, RSA, EC_DH };
    enum class AsymmetricCipherType { INVALID, RSA };
    enum class SymmetricCipherType { INVALID, AES, TWOFISH, SERPENT };
    enum class AuthenticatedSymmetricCipherModeType { INVALID, GCM, CCM, EAX };
    enum class PasswordDerivationFunction { INVALID, PBKDF2_SHA256, PBKDF2_SHA512, PBKDF2_SHA3_256, PBKDF2_SHA3_512 };
    enum class EllipticCurveType { INVALID, P192R1, P224R1, P256R1, P384R1, P521R1, BP_P160R1, BP_P192R1, BP_P224R1, BP_P256R1, BP_P320R1, BP_P384R1, BP_P512R1 };
    
    //Taken from CryptoPP's documentation:
    //0 - using this object won't cause a crash or exception (rng is ignored) 
    //1 - this object will probably function (encrypt, sign, etc.) correctly (but may not check for weak keys and such)
    //2 - make sure this object will function correctly, and do reasonable security checks
    //3 - do checks that may take a long time
    enum class AsymmetricKeyValidationLevel { INVALID, BASIC_0, LOW_1, HIGH_2, FULL_3 };
    
    typedef unsigned int SaltSize;
    const SaltSize INVALID_SALT_SIZE = 0;
    
    enum class HashAlgorithmType
    {
        INVALID,
        SHA_224, SHA_256, SHA_384, SHA_512,     //SHA2
        SHA3_224, SHA3_256, SHA3_384, SHA3_512, //SHA3
        WHIRLPOOL,                              //Whirlpool
        RIPEMD_160, RIPEMD_256, RIPEMD_320      //RIPEMD
    };
    
    typedef CryptoPP::RSA::PrivateKey RSAPrivateKey;
    typedef CryptoPP::RSA::PublicKey RSAPublicKey;
    typedef CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> ECPrivateKey;
    typedef CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> ECPublicKey;
    
    //ECDH
    typedef CryptoPP::ECDH<CryptoPP::ECP>::Domain ECDH;
    typedef CryptoPP::SecByteBlock ECDHPrivateKey;
    typedef CryptoPP::SecByteBlock ECDHPublicKey;
    
    //                      prime curves , COFACTOR_OPTION                             , DHAES_MODE
    typedef CryptoPP::ECIES<CryptoPP::ECP, CryptoPP::IncompatibleCofactorMultiplication, true>::Decryptor ECDecryptor;
    typedef CryptoPP::ECIES<CryptoPP::ECP, CryptoPP::IncompatibleCofactorMultiplication, true>::Encryptor ECEncryptor;
    
    typedef CryptoPP::RSAES_OAEP_SHA_Decryptor RSADecryptor;
    typedef CryptoPP::RSAES_OAEP_SHA_Encryptor RSAEncryptor;
    typedef CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Signer RSASigner;
    typedef CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA256>::Verifier RSAVerifier;
    
    enum class SecurableComponentType
    {
        INVALID,
        DATABASE_MANAGER,   //
        SECURITY_MANAGER,   //
        NETWORK_MANAGER,    //
        STORAGE_MANAGER,    //
        SESSION_MANAGER,    //
        USER_MANAGER,       //
        DEVICE_MANAGER,     //
        DATABASE_LOGGER
    };
    
    //TODO - make common & update DALCache
    typedef unsigned long CacheSize;
    typedef unsigned long CacheHits;
    const CacheHits MAX_CACHE_HITS = ULONG_MAX;
    
    enum class CacheEvictionType
    {
        INVALID,
        LRU,    //least recently used
        MRU     //most recently used
    };
    
    enum class DelayEscalationType
    {
        INVALID,
        CONSTANT, //1
        LINEAR,   //N
        QUADRATIC //N^2
    };
    
    /** Structure for holding local peer authentication data. */
    struct LocalPeerAuthenticationEntry
    {
        /** Device ID as register on remote server. */
        Common_Types::DeviceID id;
        /** Plaintext password, as expected on remote server. */
        std::string plaintextPassword;
    };
}

#endif	/* SECURITY_MANAGEMENT_TYPES_H */

