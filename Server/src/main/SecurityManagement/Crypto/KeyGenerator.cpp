/**
 * Copyright (C) 2016 https://github.com/sndnv
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

#include "KeyGenerator.h"
#include <cryptopp/serpent.h>
#include <cryptopp/twofish.h>
#include <cryptopp/oids.h>

SecurityManagement_Crypto::KeyGenerator::KeyGenerator
(DerivedKeysParameters dkParams, SymmetricKeysParameters skParams,
 AsymmetricKeysParameters akParams, Utilities::FileLoggerPtr logger)
: debugLogger(logger), defaultSymmetricCipher(skParams.defaultSymmetricCipher),
  defaultSymmetricCipherMode(skParams.defaultSymmetricCipherMode),
  defaultIVSize(skParams.defaultIVSize), minSymmetricKeySize(skParams.minSymmetricKeySize),
  defaultSymmetricKeySize(skParams.defaultSymmetricKeySize),
  minRSAKeySize(akParams.minRSAKeySize), defaultRSAKeySize(akParams.defaultRSAKeySize),
  defaultEllipticCurve(akParams.defaultEllipticCurve),
  derivedKeyIterations(dkParams.derivedKeyIterations), derivedKeySize(dkParams.derivedKeySize),
  derivedKeyMinSaltSize(dkParams.derivedKeyMinSaltSize),
  derivedKeyDefaultSaltSize(dkParams.derivedKeyDefaultSaltSize)
{
    if(defaultSymmetricCipher != SymmetricCipherType::AES
        && defaultSymmetricCipher != SymmetricCipherType::SERPENT
        && defaultSymmetricCipher != SymmetricCipherType::TWOFISH)
    {
        throw std::invalid_argument("KeyGenerator::() > Invalid symmetric cipher supplied.");
    }

    if(defaultSymmetricCipherMode != AuthenticatedSymmetricCipherModeType::CCM
        && defaultSymmetricCipherMode != AuthenticatedSymmetricCipherModeType::EAX
        && defaultSymmetricCipherMode != AuthenticatedSymmetricCipherModeType::GCM)
    {
        throw std::invalid_argument("KeyGenerator::() > Invalid symmetric cipher mode supplied.");
    }

    if(defaultEllipticCurve != EllipticCurveType::BP_P160R1
        && defaultEllipticCurve != EllipticCurveType::BP_P192R1
        && defaultEllipticCurve != EllipticCurveType::BP_P224R1
        && defaultEllipticCurve != EllipticCurveType::BP_P256R1
        && defaultEllipticCurve != EllipticCurveType::BP_P320R1
        && defaultEllipticCurve != EllipticCurveType::BP_P384R1
        && defaultEllipticCurve != EllipticCurveType::BP_P512R1
        && defaultEllipticCurve != EllipticCurveType::P192R1
        && defaultEllipticCurve != EllipticCurveType::P224R1
        && defaultEllipticCurve != EllipticCurveType::P256R1
        && defaultEllipticCurve != EllipticCurveType::P384R1
        && defaultEllipticCurve != EllipticCurveType::P521R1)
    {
        throw std::invalid_argument("KeyGenerator::() > Invalid elliptic curve supplied.");
    }

    if(derivedKeyMinSaltSize > derivedKeyDefaultSaltSize)
    {
        throw std::invalid_argument("KeyGenerator::() > The default derived key salt size must be "
                                    "larger than or equal to the minimum derived key salt size.");
    }

    if(minSymmetricKeySize > defaultSymmetricKeySize)
    {
        throw std::invalid_argument("KeyGenerator::() > The default symmetric key size must be "
                                    "larger than or equal to the minimum symmetric key size.");
    }

    if(minRSAKeySize > defaultRSAKeySize)
    {
        throw std::invalid_argument("KeyGenerator::() > The default RSA key size must be "
                                    "larger than or equal to the minimum RSA key size.");
    }

    switch(dkParams.derivedKeyFunction)
    {
        case PasswordDerivationFunction::PBKDF2_SHA256:
        {
            derivedKeyGenerator = new CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256>();
        } break;

        case PasswordDerivationFunction::PBKDF2_SHA512:
        {
            derivedKeyGenerator = new CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA512>();
        } break;

        //TODO - fix issues with HMAC & SHA3
        //case PasswordDerivationFunction::PBKDF2_SHA3_256:
        //{
        //    derivedKeyGenerator = new CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA3_256>();
        //} break;
        //case PasswordDerivationFunction::PBKDF2_SHA3_512:
        //{
        //    derivedKeyGenerator = new CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA3_512>();
        //} break;

        default:
        {
            throw std::invalid_argument("KeyGenerator::() > Unexpected password derivation "
                                        "function encountered.");
        };
    }

    switch(akParams.keyValidationLevel)
    {
        case AsymmetricKeyValidationLevel::BASIC_0: keyValidationLevel = 0; break;
        case AsymmetricKeyValidationLevel::LOW_1: keyValidationLevel = 1; break;
        case AsymmetricKeyValidationLevel::HIGH_2: keyValidationLevel = 2; break;
        case AsymmetricKeyValidationLevel::FULL_3: keyValidationLevel = 3; break;

        default:
        {
            throw std::invalid_argument("KeyGenerator::() > Unexpected key validation "
                                        "level encountered.");
        };
    }
}

SecurityManagement_Crypto::KeyGenerator::~KeyGenerator()
{
    delete derivedKeyGenerator;
}

//<editor-fold defaultstate="collapsed" desc="Symmetric Crypto">
SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoData() const
{
    return getSymmetricCryptoData(
            defaultSymmetricCipher,
            defaultSymmetricCipherMode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoData
(const KeyData & key, const IVData & iv) const
{
    return getSymmetricCryptoData(
            defaultSymmetricCipher,
            defaultSymmetricCipherMode,
            key,
            iv);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoData
(SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode) const
{
    switch(cipher)
    {
        case SymmetricCipherType::AES:     return getSymmetricCryptoDataForCipher<CryptoPP::AES>(mode, getSymmetricKey<CryptoPP::AES>(), getIV());
        case SymmetricCipherType::SERPENT: return getSymmetricCryptoDataForCipher<CryptoPP::Serpent>(mode, getSymmetricKey<CryptoPP::Serpent>(), getIV());
        case SymmetricCipherType::TWOFISH: return getSymmetricCryptoDataForCipher<CryptoPP::Twofish>(mode, getSymmetricKey<CryptoPP::Twofish>(), getIV());

        default: throw std::invalid_argument("KeyGenerator::getSymmetricCryptoData() > Unexpected cipher type encountered.");
    }
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoData
(SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, const KeyData & key, const IVData & iv) const
{
    switch(cipher)
    {
        case SymmetricCipherType::AES:     return getSymmetricCryptoDataForCipher<CryptoPP::AES>(mode, key, iv);
        case SymmetricCipherType::SERPENT: return getSymmetricCryptoDataForCipher<CryptoPP::Serpent>(mode, key, iv);
        case SymmetricCipherType::TWOFISH: return getSymmetricCryptoDataForCipher<CryptoPP::Twofish>(mode, key, iv);

        default: throw std::invalid_argument("KeyGenerator::getSymmetricCryptoData() > Unexpected cipher type encountered.");
    }
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataFromPassphrase
(const std::string & passphrase) const
{
    SaltData salt = SaltGenerator::getRandomSalt(derivedKeyDefaultSaltSize);
    KeyData key = getDerivedSymmetricKey(passphrase, salt);
    IVData iv = getIV();

    SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(key, iv);
    result->updateSalt(salt);
    return result;
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataFromPassphrase
(const std::string & passphrase, const SaltData & salt, const IVData & iv) const
{
    KeyData key = getDerivedSymmetricKey(passphrase, salt);
    SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(key, iv);
    result->updateSalt(salt);
    return result;
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataFromPassphrase
(SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, const std::string & passphrase) const
{
    SaltData salt = SaltGenerator::getRandomSalt(derivedKeyDefaultSaltSize);
    KeyData key = getDerivedSymmetricKey(passphrase, salt);
    IVData iv = getIV();
    SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
    result->updateSalt(salt);
    return result;
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataFromPassphrase
(SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, 
 const std::string & passphrase, const SaltData & salt, const IVData & iv) const
{
    KeyData key = getDerivedSymmetricKey(passphrase, salt);
    SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
    result->updateSalt(salt);
    return result;
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataFromPassphrase
(SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, 
 const std::string & passphrase, unsigned int iterations,
 const SaltData & salt, const IVData & iv) const
{
    KeyData key = getDerivedSymmetricKey(passphrase, iterations, salt);
    SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
    result->updateSalt(salt);
    return result;
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
{
    return getSymmetricCryptoDataForDHExchange(
            defaultEllipticCurve,
            localPrivateKey,
            remotePublicKey,
            defaultSymmetricCipher,
            defaultSymmetricCipherMode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
 const ECDHPublicKey & remotePublicKey)
{
    return getSymmetricCryptoDataForDHExchange(
            curve,
            localPrivateKey,
            remotePublicKey,
            defaultSymmetricCipher,
            defaultSymmetricCipherMode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
 const ECDHPublicKey & remotePublicKey, SymmetricCipherType cipher,
 AuthenticatedSymmetricCipherModeType mode)
{
    IVData iv = getIV();
    return getSymmetricCryptoDataForDHExchange(
            curve,
            localPrivateKey,
            remotePublicKey,
            iv,
            cipher,
            mode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, const IVData & iv)
{
    return getSymmetricCryptoDataForDHExchange(
            defaultEllipticCurve,
            localPrivateKey,
            remotePublicKey,
            iv,
            defaultSymmetricCipher,
            defaultSymmetricCipherMode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
 const ECDHPublicKey & remotePublicKey, const IVData & iv)
{
    return getSymmetricCryptoDataForDHExchange(
            curve,
            localPrivateKey,
            remotePublicKey,
            iv,
            defaultSymmetricCipher,
            defaultSymmetricCipherMode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, 
 const IVData & iv, SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
{
    return getSymmetricCryptoDataForDHExchange(
            defaultEllipticCurve,
            localPrivateKey,
            remotePublicKey,
            iv,
            cipher,
            mode);
}

SymmetricCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getSymmetricCryptoDataForDHExchange
(EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
 const ECDHPublicKey & remotePublicKey, const IVData & iv,
 SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
{
    KeyData key = getDiffieHellmanKeyEncryptionKey(curve, localPrivateKey, remotePublicKey);
    return getSymmetricCryptoData(cipher, mode, key, iv);
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Asymmetric Crypto">
RSACryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getRSACryptoData(KeySize keySize) const
{
    if(keySize == 0)
        keySize = defaultRSAKeySize;

    if(keySize < minRSAKeySize)
        throw std::invalid_argument("KeyGenerator::getRSACryptoData() > Insufficiently large key was supplied.");

    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::InvertibleRSAFunction params;
    params.GenerateRandomWithKeySize(rng, keySize);
    RSAPrivateKey * privateKey = new RSAPrivateKey(params);
    RSAPublicKey * publicKey = new RSAPublicKey(params);

    if(!privateKey->Validate(rng, keyValidationLevel))
    {
        delete privateKey;
        delete publicKey;
        throw std::runtime_error("KeyGenerator::getRSACryptoData() > New private key failed validation.");
    }

    return RSACryptoDataContainerPtr(new RSACryptoDataContainer(privateKey, publicKey));
}
ECDHCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getECDHCryptoData() const
{
    return getECDHCryptoData(defaultEllipticCurve);
}

ECDHCryptoDataContainerPtr SecurityManagement_Crypto::KeyGenerator::getECDHCryptoData
(EllipticCurveType curve) const
{
    CryptoPP::AutoSeededRandomPool rng;

    ECDH * ecdh = createECDHObject(curve);
    ECDHPrivateKey * privateKey = new ECDHPrivateKey(ecdh->PrivateKeyLength());
    ECDHPublicKey * publicKey = new ECDHPublicKey(ecdh->PublicKeyLength());

    try
    {
        ecdh->GenerateKeyPair(rng, *privateKey, *publicKey);
    }
    catch(...)
    {
        delete ecdh;
        delete privateKey;
        delete publicKey;
        throw;
    }

    delete ecdh;
    return ECDHCryptoDataContainerPtr(new ECDHCryptoDataContainer(privateKey, publicKey));
}
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="Key & IV Generation">
KeyData SecurityManagement_Crypto::KeyGenerator::getDerivedSymmetricKey
(const std::string & passphrase, const SaltData & salt) const
{
    if(derivedKeyMinSaltSize > salt.size())
        throw std::invalid_argument("KeyGenerator::getDerivedSymmetricKey() > Insufficiently large salt was supplied.");

    KeyData key(derivedKeySize);
    derivedKeyGenerator->DeriveKey(
        key, key.size(), 
        0 /* purpose byte; unused */,
        reinterpret_cast<const CryptoPPByte *>(passphrase.c_str()), passphrase.length(),
        salt, salt.size(),
        derivedKeyIterations);

    return key;
}

KeyData SecurityManagement_Crypto::KeyGenerator::getDerivedSymmetricKey
(const std::string & passphrase, unsigned int iterations, const SaltData & salt) const
{
    if(derivedKeyMinSaltSize > salt.size())
        throw std::invalid_argument("KeyGenerator::getDerivedSymmetricKey() > Insufficiently large salt was supplied.");

    if(iterations < derivedKeyIterations && iterations != 0)
        throw std::invalid_argument("KeyGenerator::getDerivedSymmetricKey() > Insufficient number of iterations supplied.");

    KeyData key(derivedKeySize);
    derivedKeyGenerator->DeriveKey(
        key, key.size(), 
        0 /* purpose byte; unused */,
        reinterpret_cast<const CryptoPPByte *>(passphrase.c_str()), passphrase.length(),
        salt, salt.size(),
        (iterations != 0) ? iterations : derivedKeyIterations);

    return key;
}

KeyData SecurityManagement_Crypto::KeyGenerator::getSymmetricKey
(SymmetricCipherType keyType, KeySize keySize) const
{
    if(keySize == 0)
        keySize = defaultSymmetricKeySize;
    else if(keySize < minSymmetricKeySize)
        throw std::invalid_argument("KeyGenerator::getSymmetricKey() > Insufficiently large key size was specified.");
    else if(keySize < defaultSymmetricKeySize)
        logDebugMessage("(getSymmetricKey) > The supplied symmetric key size is smaller than the default size.");

    CryptoPP::AutoSeededRandomPool rng;

    KeySize maxKeySize;
    std::string algorithmName;

    switch(keyType)
    {
        case SymmetricCipherType::AES:      { maxKeySize = CryptoPP::AES::MAX_KEYLENGTH; algorithmName = "AES"; } break;
        case SymmetricCipherType::SERPENT:  { maxKeySize = CryptoPP::Serpent::MAX_KEYLENGTH; algorithmName = "Serpent"; } break;
        case SymmetricCipherType::TWOFISH:  { maxKeySize = CryptoPP::Twofish::MAX_KEYLENGTH; algorithmName = "Twofish"; } break;
        default: throw std::invalid_argument("KeyGenerator::getSymmetricKey() > Unexpected key type encountered.");
    }

    if(keySize > maxKeySize)
    {
        throw std::invalid_argument("KeyGenerator::getSymmetricKey() > The requested key size is"
                                    " too large for the specified cipher <" + algorithmName + ">.");
    }

    KeyData key(keySize);
    rng.GenerateBlock(key, key.size());

    return key;
}

KeyData SecurityManagement_Crypto::KeyGenerator::getDiffieHellmanKeyEncryptionKey
(const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
{
    return getDiffieHellmanKeyEncryptionKey(defaultEllipticCurve, localPrivateKey, remotePublicKey);
}

KeyData SecurityManagement_Crypto::KeyGenerator::getDiffieHellmanKeyEncryptionKey
(EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
{
    ECDH * ecdh = createECDHObject(curve);

    if(ecdh->PrivateKeyLength() != localPrivateKey.SizeInBytes()
            || ecdh->PublicKeyLength() != remotePublicKey.SizeInBytes())
    {
        delete ecdh;
        throw std::runtime_error("KeyGenerator::getDiffieHellmanKeyEncryptionKey() > Unexpected key sizes encountered.");
    }

    KeyData sharedSecret(ecdh->AgreedValueLength());

    if(ecdh->Agree(sharedSecret, localPrivateKey, remotePublicKey, true))
    {
        if(sharedSecret.SizeInBytes() >= defaultSymmetricKeySize)
        {
            KeyData keyEncryptionKey(sharedSecret.BytePtr(), defaultSymmetricKeySize);
            delete ecdh;
            return keyEncryptionKey;
        }
        else
        {
            delete ecdh;
            throw std::runtime_error("KeyGenerator::getDiffieHellmanKeyEncryptionKey() > Failed to create shared secret of sufficient size.");
        }
    }
    else
    {
        delete ecdh;
        throw std::runtime_error("KeyGenerator::getDiffieHellmanKeyEncryptionKey() > Failed to reach shared secret.");
    }
}

IVData SecurityManagement_Crypto::KeyGenerator::getIV(IVSize size) const
{
    CryptoPP::AutoSeededRandomPool rng;
    return KeyGenerator::getIV((size == 0) ? defaultIVSize : size, rng);
}
//</editor-fold>

ECDH * SecurityManagement_Crypto::KeyGenerator::createECDHObject(EllipticCurveType curve) const
{
    ECDH * ecdh;

    switch(curve)
    {
        case EllipticCurveType::P192R1: ecdh = new ECDH(CryptoPP::ASN1::secp192r1()); break;
        case EllipticCurveType::P224R1: ecdh = new ECDH(CryptoPP::ASN1::secp224r1()); break;
        case EllipticCurveType::P256R1: ecdh = new ECDH(CryptoPP::ASN1::secp256r1()); break;
        case EllipticCurveType::P384R1: ecdh = new ECDH(CryptoPP::ASN1::secp384r1()); break;
        case EllipticCurveType::P521R1: ecdh = new ECDH(CryptoPP::ASN1::secp521r1()); break;
        case EllipticCurveType::BP_P160R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP160r1()); break;
        case EllipticCurveType::BP_P192R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP192r1()); break;
        case EllipticCurveType::BP_P224R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP224r1()); break;
        case EllipticCurveType::BP_P256R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP256r1()); break;
        case EllipticCurveType::BP_P320R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP320r1()); break;
        case EllipticCurveType::BP_P384R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP384r1()); break;
        case EllipticCurveType::BP_P512R1: ecdh = new ECDH(CryptoPP::ASN1::brainpoolP512r1()); break;
        default: 
        {
            throw std::invalid_argument("KeyGenerator::createECDHObject() > Unexpected elliptic curve type encountered.");
        }
    }

    CryptoPP::AutoSeededRandomPool rng;
    if(!ecdh->AccessMaterial().Validate(rng, keyValidationLevel))
    {
        delete ecdh;
        throw std::runtime_error("KeyGenerator::createECDHObject() > ECDH material validation failed.");
    }

    if(!ecdh->AccessGroupParameters().Validate(rng, keyValidationLevel))
    {
        delete ecdh;
        throw std::runtime_error("KeyGenerator::createECDHObject() > ECDH group parameters validation failed.");
    }

    return ecdh;
}
