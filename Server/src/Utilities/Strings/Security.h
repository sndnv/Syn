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

#ifndef UTILITIES_STRINGS_SECURITY_H
#define	UTILITIES_STRINGS_SECURITY_H

#include <string>
#include <boost/unordered_map.hpp>
#include "../../SecurityManagement/Types/Types.h"

using SecurityManagement_Types::SecurableComponentType;
using SecurityManagement_Types::CacheEvictionType;
using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::AsymmetricCipherType;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::KeyExchangeType;

namespace Utilities
{
    namespace Strings
    {
        struct SecurityMaps
        {
            static const boost::unordered_map<SecurableComponentType, std::string> securableComponentTypeToString;
            static const boost::unordered_map<std::string, SecurableComponentType> stringToSecurableComponentType;
            static const boost::unordered_map<CacheEvictionType, std::string> cacheEvictionTypeToString;
            static const boost::unordered_map<std::string, CacheEvictionType> stringToCacheEvictionType;
            static const boost::unordered_map<HashAlgorithmType, std::string> hashAlgorithmTypeToString;
            static const boost::unordered_map<std::string, HashAlgorithmType> stringToHashAlgorithmType;
            static const boost::unordered_map<SymmetricCipherType, std::string> symmetricCipherTypeToString;
            static const boost::unordered_map<std::string, SymmetricCipherType> stringToSymmetricCipherType;
            static const boost::unordered_map<AuthenticatedSymmetricCipherModeType, std::string> authenticatedSymmetricCipherModeTypeToString;
            static const boost::unordered_map<std::string, AuthenticatedSymmetricCipherModeType> stringToAuthenticatedSymmetricCipherModeType;
            static const boost::unordered_map<AsymmetricCipherType, std::string> asymmetricCipherTypeToString;
            static const boost::unordered_map<std::string, AsymmetricCipherType> stringToAsymmetricCipherType;
            static const boost::unordered_map<KeyExchangeType, std::string> keyExchangeTypeToString;
            static const boost::unordered_map<std::string, KeyExchangeType> stringToKeyExchangeType;
        };
        
        std::string toString(SecurableComponentType var);
        std::string toString(CacheEvictionType var);
        std::string toString(HashAlgorithmType var);
        std::string toString(SymmetricCipherType var);
        std::string toString(AsymmetricCipherType var);
        std::string toString(AuthenticatedSymmetricCipherModeType var);
        std::string toString(KeyExchangeType var);
        SecurableComponentType toSecurableComponentType(std::string var);
        CacheEvictionType toCacheEvictionType(std::string var);
        HashAlgorithmType toHashAlgorithmType(std::string var);
        AsymmetricCipherType toAsymmetricCipherType(std::string var);
        SymmetricCipherType toSymmetricCipherType(std::string var);
        AuthenticatedSymmetricCipherModeType toAuthenticatedSymmetricCipherModeType(std::string var);
        KeyExchangeType toKeyExchangeType(std::string var);
    }
}

#endif	/* UTILITIES_STRINGS_SECURITY_H */
