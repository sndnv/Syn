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

#include "Security.h"

using Maps = Utilities::Strings::SecurityMaps;

const boost::unordered_map<SecurableComponentType, std::string> Maps::securableComponentTypeToString
{
    {SecurableComponentType::DATABASE_MANAGER,  "DATABASE_MANAGER"},
    {SecurableComponentType::NETWORK_MANAGER,   "NETWORK_MANAGER"},
    {SecurableComponentType::SECURITY_MANAGER,  "SECURITY_MANAGER"},
    {SecurableComponentType::STORAGE_MANAGER,   "STORAGE_MANAGER"},
    {SecurableComponentType::SESSION_MANAGER,   "SESSION_MANAGER"},
    {SecurableComponentType::INVALID,           "INVALID"},
};

const boost::unordered_map<std::string, SecurableComponentType> Maps::stringToSecurableComponentType
{
    {"DATABASE_MANAGER",    SecurableComponentType::DATABASE_MANAGER},
    {"NETWORK_MANAGER",     SecurableComponentType::NETWORK_MANAGER},
    {"SECURITY_MANAGER",    SecurableComponentType::SECURITY_MANAGER},
    {"STORAGE_MANAGER",     SecurableComponentType::STORAGE_MANAGER},
    {"SESSION_MANAGER",     SecurableComponentType::SESSION_MANAGER},
    {"INVALID",             SecurableComponentType::INVALID},
};

const boost::unordered_map<CacheEvictionType, std::string> Maps::cacheEvictionTypeToString
{
    {CacheEvictionType::LRU,     "LRU"},
    {CacheEvictionType::MRU,     "MRU"},
    {CacheEvictionType::INVALID, "INVALID"},
};

const boost::unordered_map<std::string, CacheEvictionType> Maps::stringToCacheEvictionType
{
    {"LRU",     CacheEvictionType::LRU},
    {"MRU",     CacheEvictionType::MRU},
    {"INVALID", CacheEvictionType::INVALID},
};

const boost::unordered_map<HashAlgorithmType, std::string> Maps::hashAlgorithmTypeToString
{
    {HashAlgorithmType::RIPEMD_160, "RIPEMD_160"},
    {HashAlgorithmType::RIPEMD_256, "RIPEMD_256"},
    {HashAlgorithmType::RIPEMD_320, "RIPEMD_320"},
    {HashAlgorithmType::SHA3_224,   "SHA3_224"},
    {HashAlgorithmType::SHA3_256,   "SHA3_256"},
    {HashAlgorithmType::SHA3_384,   "SHA3_384"},
    {HashAlgorithmType::SHA3_512,   "SHA3_512"},
    {HashAlgorithmType::SHA_224,    "SHA_224"},
    {HashAlgorithmType::SHA_256,    "SHA_256"},
    {HashAlgorithmType::SHA_384,    "SHA_384"},
    {HashAlgorithmType::SHA_512,    "SHA_512"},
    {HashAlgorithmType::WHIRLPOOL,  "WHIRLPOOL"},
    {HashAlgorithmType::INVALID,    "INVALID"}  
};

const boost::unordered_map<std::string, HashAlgorithmType> Maps::stringToHashAlgorithmType
{
    {"RIPEMD_160",  HashAlgorithmType::RIPEMD_160},
    {"RIPEMD_256",  HashAlgorithmType::RIPEMD_256},
    {"RIPEMD_320",  HashAlgorithmType::RIPEMD_320},
    {"SHA3_224",    HashAlgorithmType::SHA3_224},
    {"SHA3_256",    HashAlgorithmType::SHA3_256},
    {"SHA3_384",    HashAlgorithmType::SHA3_384},
    {"SHA3_512",    HashAlgorithmType::SHA3_512},
    {"SHA_224",     HashAlgorithmType::SHA_224},
    {"SHA_256",     HashAlgorithmType::SHA_256},
    {"SHA_384",     HashAlgorithmType::SHA_384},
    {"SHA_512",     HashAlgorithmType::SHA_512},
    {"WHIRLPOOL",   HashAlgorithmType::WHIRLPOOL},
    {"INVALID",     HashAlgorithmType::INVALID}   
};

const boost::unordered_map<AsymmetricCipherType, std::string> Maps::asymmetricCipherTypeToString
{
    {AsymmetricCipherType::RSA,     "RSA"},
    {AsymmetricCipherType::INVALID, "INVALID"},
};

const boost::unordered_map<std::string, AsymmetricCipherType> Maps::stringToAsymmetricCipherType
{
    {"RSA",     AsymmetricCipherType::RSA},
    {"INVALID", AsymmetricCipherType::INVALID}
};

const boost::unordered_map<SymmetricCipherType, std::string> Maps::symmetricCipherTypeToString
{
    {SymmetricCipherType::AES,      "AES"},
    {SymmetricCipherType::TWOFISH,  "TWOFISH"},
    {SymmetricCipherType::SERPENT,  "SERPENT"},
    {SymmetricCipherType::INVALID,  "INVALID"},
};

const boost::unordered_map<std::string, SymmetricCipherType> Maps::stringToSymmetricCipherType
{
    {"AES",     SymmetricCipherType::AES},
    {"TWOFISH", SymmetricCipherType::TWOFISH},
    {"SERPENT", SymmetricCipherType::SERPENT},
    {"INVALID", SymmetricCipherType::INVALID}
};

const boost::unordered_map<AuthenticatedSymmetricCipherModeType, std::string> Maps::authenticatedSymmetricCipherModeTypeToString
{
    {AuthenticatedSymmetricCipherModeType::GCM,     "GCM"},
    {AuthenticatedSymmetricCipherModeType::CCM,     "CCM"},
    {AuthenticatedSymmetricCipherModeType::EAX,     "EAX"},
    {AuthenticatedSymmetricCipherModeType::INVALID, "INVALID"},
};

const boost::unordered_map<std::string, AuthenticatedSymmetricCipherModeType> Maps::stringToAuthenticatedSymmetricCipherModeType
{
    {"GCM",     AuthenticatedSymmetricCipherModeType::GCM},
    {"CCM",     AuthenticatedSymmetricCipherModeType::CCM},
    {"EAX",     AuthenticatedSymmetricCipherModeType::EAX},
    {"INVALID", AuthenticatedSymmetricCipherModeType::INVALID}
};

const boost::unordered_map<KeyExchangeType, std::string> Maps::keyExchangeTypeToString
{
    {KeyExchangeType::RSA,      "RSA"},
    {KeyExchangeType::EC_DH,    "EC_DH"},
    {KeyExchangeType::INVALID,  "INVALID"},
};

const boost::unordered_map<std::string, KeyExchangeType> Maps::stringToKeyExchangeType
{
    {"RSA",     KeyExchangeType::RSA},
    {"EC_DH",   KeyExchangeType::EC_DH},
    {"INVALID", KeyExchangeType::INVALID}
};

std::string Utilities::Strings::toString(SecurableComponentType var)
{
    if(Maps::securableComponentTypeToString.find(var) != Maps::securableComponentTypeToString.end())
        return Maps::securableComponentTypeToString.at(var);
    else
        return "INVALID";
}

SecurableComponentType Utilities::Strings::toSecurableComponentType(std::string var)
{
    if(Maps::stringToSecurableComponentType.find(var) != Maps::stringToSecurableComponentType.end())
        return Maps::stringToSecurableComponentType.at(var);
    else
        return SecurableComponentType::INVALID;
}

std::string Utilities::Strings::toString(CacheEvictionType var)
{
    if(Maps::cacheEvictionTypeToString.find(var) != Maps::cacheEvictionTypeToString.end())
        return Maps::cacheEvictionTypeToString.at(var);
    else
        return "INVALID";
}

CacheEvictionType Utilities::Strings::toCacheEvictionType(std::string var)
{
    if(Maps::stringToCacheEvictionType.find(var) != Maps::stringToCacheEvictionType.end())
        return Maps::stringToCacheEvictionType.at(var);
    else
        return CacheEvictionType::INVALID;
}

std::string Utilities::Strings::toString(HashAlgorithmType var)
{
    if(Maps::hashAlgorithmTypeToString.find(var) != Maps::hashAlgorithmTypeToString.end())
        return Maps::hashAlgorithmTypeToString.at(var);
    else
        return "INVALID";
}

HashAlgorithmType Utilities::Strings::toHashAlgorithmType(std::string var)
{
    if(Maps::stringToHashAlgorithmType.find(var) != Maps::stringToHashAlgorithmType.end())
        return Maps::stringToHashAlgorithmType.at(var);
    else
        return HashAlgorithmType::INVALID;
}

std::string Utilities::Strings::toString(AsymmetricCipherType var)
{
    if(Maps::asymmetricCipherTypeToString.find(var) != Maps::asymmetricCipherTypeToString.end())
        return Maps::asymmetricCipherTypeToString.at(var);
    else
        return "INVALID";
}

AsymmetricCipherType Utilities::Strings::toAsymmetricCipherType(std::string var)
{
    if(Maps::stringToAsymmetricCipherType.find(var) != Maps::stringToAsymmetricCipherType.end())
        return Maps::stringToAsymmetricCipherType.at(var);
    else
        return AsymmetricCipherType::INVALID;
}

std::string Utilities::Strings::toString(SymmetricCipherType var)
{
    if(Maps::symmetricCipherTypeToString.find(var) != Maps::symmetricCipherTypeToString.end())
        return Maps::symmetricCipherTypeToString.at(var);
    else
        return "INVALID";
}

SymmetricCipherType Utilities::Strings::toSymmetricCipherType(std::string var)
{
    if(Maps::stringToSymmetricCipherType.find(var) != Maps::stringToSymmetricCipherType.end())
        return Maps::stringToSymmetricCipherType.at(var);
    else
        return SymmetricCipherType::INVALID;
}

std::string Utilities::Strings::toString(AuthenticatedSymmetricCipherModeType var)
{
    if(Maps::authenticatedSymmetricCipherModeTypeToString.find(var) != Maps::authenticatedSymmetricCipherModeTypeToString.end())
        return Maps::authenticatedSymmetricCipherModeTypeToString.at(var);
    else
        return "INVALID";
}

AuthenticatedSymmetricCipherModeType Utilities::Strings::toAuthenticatedSymmetricCipherModeType(std::string var)
{
    if(Maps::stringToAuthenticatedSymmetricCipherModeType.find(var) != Maps::stringToAuthenticatedSymmetricCipherModeType.end())
        return Maps::stringToAuthenticatedSymmetricCipherModeType.at(var);
    else
        return AuthenticatedSymmetricCipherModeType::INVALID;
}

std::string Utilities::Strings::toString(KeyExchangeType var)
{
    if(Maps::keyExchangeTypeToString.find(var) != Maps::keyExchangeTypeToString.end())
        return Maps::keyExchangeTypeToString.at(var);
    else
        return "INVALID";
}

KeyExchangeType Utilities::Strings::toKeyExchangeType(std::string var)
{
    if(Maps::stringToKeyExchangeType.find(var) != Maps::stringToKeyExchangeType.end())
        return Maps::stringToKeyExchangeType.at(var);
    else
        return KeyExchangeType::INVALID;
}