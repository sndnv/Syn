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

#ifndef KEYGENERATOR_H
#define	KEYGENERATOR_H

#include <string>

#include <cryptopp/aes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/rsa.h>
#include <cryptopp/serpent.h>
#include <cryptopp/twofish.h>
#include <cryptopp/gcm.h>
#include <cryptopp/ccm.h>
#include <cryptopp/eax.h>
#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>

#include "../Types/Types.h"
#include "Containers.h"
#include "SaltGenerator.h"

#include "../../Utilities/FileLogger.h"

using SecurityManagement_Types::KeySize;
using SecurityManagement_Types::IVSize;

using SecurityManagement_Types::CryptoPPByte;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::IVData;
using SecurityManagement_Types::KeyData;

using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::UnauthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::PasswordDerivationFunction;
using SecurityManagement_Types::EllipticCurveType;
using SecurityManagement_Types::AsymmetricKeyValidationLevel;

using SecurityManagement_Crypto::SymmetricCryptoDataContainer;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;

using SecurityManagement_Types::ECDecryptor;
using SecurityManagement_Types::ECEncryptor;
using SecurityManagement_Types::RSADecryptor;
using SecurityManagement_Types::RSAEncryptor;

using SecurityManagement_Crypto::RSACryptoDataContainer;
using SecurityManagement_Crypto::ECCryptoDataContainer;
using SecurityManagement_Crypto::RSACryptoDataContainerPtr;
using SecurityManagement_Crypto::ECCryptoDataContainerPtr;

namespace SecurityManagement_Crypto
{
    /**
     * Class for generating cryptographic keys.
     */
    class KeyGenerator
    {
        public:
            /** Minimum IV size for CCM mode (7 bytes). */
            const unsigned int CCM_MIN_IV_SIZE = 7;
            /** Maximum IV size for CCM mode (13 bytes). */
            const unsigned int CCM_MAX_IV_SIZE = 13;
            /** Minimum IV size for GCM mode (1 byte). */
            const unsigned int GCM_MIN_IV_SIZE = 1;
            
            /** Parameters structure for holding <code>KeyGenerator</code> configuration data for derived keys. */
            struct DerivedKeysParameters
            {
                /** Password-Based Key Derivation Function */
                PasswordDerivationFunction derivedKeyFunction;
                /** Number of iterations for the key */
                unsigned int derivedKeyIterations;
                /** Derived key size (in bytes) */
                KeySize derivedKeySize;
                /** Minimum derived key salt size (in bytes) */
                SaltSize derivedKeyMinSaltSize;
                /** Default derived key salt size (in bytes) */
                SaltSize derivedKeyDefaultSaltSize;
            };
            
            /** Parameters structure for holding <code>KeyGenerator</code> configuration data for symmetric keys. */
            struct SymmetricKeysParameters
            {
                /** Default symmetric cipher */
                SymmetricCipherType defaultSymmetricCipher;
                /** Default symmetric cipher mode */
                AuthenticatedSymmetricCipherModeType defaultSymmetricCipherMode;
                /** Default symmetric key IV size (in bytes) */
                IVSize defaultIVSize;
                /** Minimum symmetric key size (in bytes) */
                KeySize minSymmetricKeySize;
                /** Default symmetric key size (in bytes) */
                KeySize defaultSymmetricKeySize;
            };
            
            /** Parameters structure for holding <code>KeyGenerator</code> configuration data for asymmetric keys. */
            struct AsymmetricKeysParameters
            {
                /** Minimum RSA key size (in bytes) */
                KeySize minRSAKeySize;
                /** Default RSA key size (in bytes) */
                KeySize defaultRSAKeySize;
                /** Default elliptic curve */
                EllipticCurveType defaultEllipticCurve;
                /** Asymmetric key validation level */
                AsymmetricKeyValidationLevel keyValidationLevel;
            };
            
            /**
             * Creates a new key generator with the specified parameters.
             * 
             * @param dkParams derived key generation parameters
             * @param skParams symmetric key generation parameters
             * @param akParams asymmetric key generation parameters
             * @param logger logger for debugging, if any
             */
            KeyGenerator(DerivedKeysParameters dkParams, SymmetricKeysParameters skParams, 
                         AsymmetricKeysParameters akParams, Utilities::FileLogger * logger = nullptr)
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
            
            /** 
             * Destroys the key generator.\n
             * 
             * Note: Requires a virtual destructor to be added to 
             * <code>PasswordBasedKeyDerivationFunction</code> in CryptoPP's <code>'pwdbased.h'</code>
             */
            ~KeyGenerator()
            {
                delete derivedKeyGenerator;
                debugLogger = nullptr;
            }
            
            KeyGenerator() = delete;
            KeyGenerator(const KeyGenerator& orig) = delete;
            KeyGenerator& operator=(const KeyGenerator& orig) = delete;
            
            //<editor-fold defaultstate="collapsed" desc="Symmetric Crypto">
            /**
             * Generates new symmetric crypto data using the default cipher and mode.
             * 
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData() const
            {
                return getSymmetricCryptoData(defaultSymmetricCipher, defaultSymmetricCipherMode);
            }
            
            /**
             * Generates symmetric crypto data using the default cipher and mode, with the specified key and IV.
             * 
             * @param key an existing symmetric key
             * @param iv the IV associated with the key
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData
            (const KeyData & key, const IVData & iv) const
            {
                return getSymmetricCryptoData(defaultSymmetricCipher, defaultSymmetricCipherMode, key, iv);
            }
            
            /**
             * Generates new symmetric crypto data using the specified cipher and mode.
             * 
             * @param cipher the cipher to be used
             * @param mode the cipher mode to be used
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData
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
            
            /**
             * Generates symmetric crypto data using the specified parameters.
             * 
             * @param cipher the cipher to be used
             * @param mode the cipher mode to be used
             * @param key an existing symmetric key
             * @param iv the IV associated with the key
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData
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
            
            /**
             * Generates symmetric crypto data using the specified parameters.
             * 
             * @param (template) TCipher template parameter denoting the cipher to be used
             * @param mode the cipher mode to be used
             * @param key an existing symmetric key
             * @param iv the IV associated with the key
             * @return the generated data
             */
            template <typename TCipher>
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForCipher
            (AuthenticatedSymmetricCipherModeType mode, const KeyData & key, const IVData & iv) const
            {
                if(key.size() < minSymmetricKeySize)
                    throw std::invalid_argument("KeyGenerator::getSymmetricCryptoDataForCipher() > Insufficiently large key was supplied.");
                else if (key.size() < defaultSymmetricKeySize)
                    logDebugMessage("(getSymmetricCryptoDataForCipher) > The supplied symmetric key is smaller than the default key size.");
                
                SymmetricCryptoDataContainerPtr result;
                
                switch(mode)
                {
                    case AuthenticatedSymmetricCipherModeType::GCM:
                    {
                        if(iv.size() < GCM_MIN_IV_SIZE)
                        {
                            throw std::invalid_argument("KeyGenerator::getSymmetricCryptoDataForCipher() > "
                                                        "Insufficiently large IV was supplied.");
                        }
                        
                        //ownership of the en/decryptors is given to the crypto container
                        typename CryptoPP::GCM<TCipher>::Encryption * encr = new typename CryptoPP::GCM<TCipher>::Encryption();
                        typename CryptoPP::GCM<TCipher>::Decryption * decr = new typename CryptoPP::GCM<TCipher>::Decryption();
                        
                        encr->SetKeyWithIV(key, key.size(), iv, iv.size());
                        decr->SetKeyWithIV(key, key.size(), iv, iv.size());
                        result = SymmetricCryptoDataContainerPtr(new SymmetricCryptoDataContainer(iv, SaltData(), key, encr, decr));
                    } break;
                    
                    case AuthenticatedSymmetricCipherModeType::CCM:
                    {
                        bool truncateIV = false;
                        if(iv.size() < CCM_MIN_IV_SIZE)
                        {
                            throw std::invalid_argument("KeyGenerator::getSymmetricCryptoDataForCipher() > "
                                                        "Insufficiently large IV was supplied.");
                        }
                        else if(iv.size() > CCM_MAX_IV_SIZE)
                        {
                            truncateIV = true;
                            logDebugMessage("(getSymmetricCryptoDataForCipher) > The supplied IV is too large for CCM mode;"
                                            " the IV will be truncated to the maximum IV size for CCM.");
                        }
                        
                        //ownership of the en/decryptors is given to the crypto container
                        typename CryptoPP::CCM<TCipher>::Encryption * encr = new typename CryptoPP::CCM<TCipher>::Encryption();
                        typename CryptoPP::CCM<TCipher>::Decryption * decr = new typename CryptoPP::CCM<TCipher>::Decryption();
                        
                        encr->SetKeyWithIV(key, key.size(), iv, truncateIV ? CCM_MAX_IV_SIZE : iv.size());
                        decr->SetKeyWithIV(key, key.size(), iv, truncateIV ? CCM_MAX_IV_SIZE : iv.size());
                        result = SymmetricCryptoDataContainerPtr(new SymmetricCryptoDataContainer(iv, SaltData(), key, encr, decr));
                    } break;
                    
                    case AuthenticatedSymmetricCipherModeType::EAX:
                    {
                        //ownership of the en/decryptors is given to the crypto container
                        typename CryptoPP::EAX<TCipher>::Encryption * encr = new typename CryptoPP::EAX<TCipher>::Encryption();
                        typename CryptoPP::EAX<TCipher>::Decryption * decr = new typename CryptoPP::EAX<TCipher>::Decryption();
                        
                        encr->SetKeyWithIV(key, key.size(), iv, iv.size());
                        decr->SetKeyWithIV(key, key.size(), iv, iv.size());
                        result = SymmetricCryptoDataContainerPtr(new SymmetricCryptoDataContainer(iv, SaltData(), key, encr, decr));
                    } break;
                    
                    default:
                    {
                        throw std::invalid_argument("KeyGenerator::getSymmetricCryptoDataForCipher() > "
                                                    "Unexpected cipher mode encountered.");
                    };
                }
                
                return result;
            }
            
            /**
             * Generates new symmetric crypto data using the specified passphrase,
             * with the default symmetric key generation configuration.
             * 
             * @param passphrase the passphrase to be used
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataFromPassphrase
            (const std::string & passphrase) const
            {
                SaltData salt = SaltGenerator::getRandomSalt(derivedKeyDefaultSaltSize);
                KeyData key = getDerivedSymmetricKey(passphrase, salt);
                IVData iv = getIV();
                
                SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(key, iv);
                result->updateSalt(salt);
                return result;
            }
            
            /**
             * Generates symmetric crypto data using the specified parameters,
             * with the default symmetric key generation configuration.
             * 
             * @param passphrase the passphrase to be used
             * @param salt the salt associated with the passphrase
             * @param iv the IV associated with the passphrase
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataFromPassphrase
            (const std::string & passphrase, const SaltData & salt, const IVData & iv) const
            {
                KeyData key = getDerivedSymmetricKey(passphrase, salt);
                SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(key, iv);
                result->updateSalt(salt);
                return result;
            }
            
            /**
             * Generates new symmetric crypto data using the specified parameters.
             * 
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param passphrase the passphrase to be used
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataFromPassphrase
            (SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, const std::string & passphrase) const
            {
                SaltData salt = SaltGenerator::getRandomSalt(derivedKeyDefaultSaltSize);
                KeyData key = getDerivedSymmetricKey(passphrase, salt);
                IVData iv = getIV();
                SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
                result->updateSalt(salt);
                return result;
            }
            
            /**
             * Generates symmetric crypto data using the specified parameters.
             * 
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param passphrase the passphrase to be used
             * @param salt the salt associated with the passphrase
             * @param iv the IV associated with the passphrase
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataFromPassphrase
            (SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, 
             const std::string & passphrase, const SaltData & salt, const IVData & iv) const
            {
                KeyData key = getDerivedSymmetricKey(passphrase, salt);
                SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
                result->updateSalt(salt);
                return result;
            }
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Asymmetric Crypto">
            /**
             * Generates RSA crypto data.
             * 
             * @param keySize the size of the RSA key (in bytes); 0 == default size
             * @return the generated data
             */
            RSACryptoDataContainerPtr getRSACryptoData(KeySize keySize = 0) const
            {
                if(keySize == 0)
                    keySize = defaultRSAKeySize;
                
                if(keySize < minRSAKeySize)
                    throw std::invalid_argument("KeyGenerator::getRSACryptoData() > Insufficiently large key was supplied.");
                
                CryptoPP::AutoSeededRandomPool rnd;
                
                RSADecryptor * decr = new RSADecryptor();
                decr->AccessKey().GenerateRandomWithKeySize(rnd, keySize);
                RSAEncryptor * encr = new RSAEncryptor(*decr);
                
                if(!decr->AccessKey().Validate(rnd, keyValidationLevel))
                    throw std::runtime_error("KeyGenerator::getRSACryptoData() > New private key failed validation.");
                
                return RSACryptoDataContainerPtr(new RSACryptoDataContainer(decr, encr));
            }
            
            /**
             * Generates RSA crypto data with the specified private key.
             * 
             * @param privateKey the private key to be used
             * @return the generated data
             */
            RSACryptoDataContainerPtr getRSACryptoData(const CryptoPP::RSA::PrivateKey & privateKey) const
            {
                CryptoPP::AutoSeededRandomPool rnd;
                if(!privateKey.Validate(rnd, keyValidationLevel))
                    throw std::invalid_argument("KeyGenerator::getRSACryptoData() > Existing private key failed validation.");
                
                RSADecryptor * decr = new RSADecryptor(privateKey);
                RSAEncryptor * encr = new RSAEncryptor(*decr);
                
                return RSACryptoDataContainerPtr(new RSACryptoDataContainer(decr, encr));
            }
            
            /**
             * Generates new elliptic curve crypto data with the default primer curve.
             * 
             * @return the generated data
             */
            ECCryptoDataContainerPtr getECCryptoData() const
            {
                return getECCryptoData(defaultEllipticCurve);
            }
            
            /**
             * Generates new elliptic curve crypto data with the specified prime curve.
             * 
             * @param curveType the curve to be used
             * @return the generated data
             */
            ECCryptoDataContainerPtr getECCryptoData(EllipticCurveType curve) const
            {
                CryptoPP::AutoSeededRandomPool rnd;
                
                ECDecryptor * decr;
                
                switch(curve)
                {
                    case EllipticCurveType::P192R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::secp192r1()); break;
                    case EllipticCurveType::P224R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::secp224r1()); break;
                    case EllipticCurveType::P256R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::secp256r1()); break;
                    case EllipticCurveType::P384R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::secp384r1()); break;
                    case EllipticCurveType::P521R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::secp521r1()); break;
                    case EllipticCurveType::BP_P160R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP160r1()); break;
                    case EllipticCurveType::BP_P192R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP192r1()); break;
                    case EllipticCurveType::BP_P224R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP224r1()); break;
                    case EllipticCurveType::BP_P256R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP256r1()); break;
                    case EllipticCurveType::BP_P320R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP320r1()); break;
                    case EllipticCurveType::BP_P384R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP384r1()); break;
                    case EllipticCurveType::BP_P512R1: decr = new ECDecryptor(rnd, CryptoPP::ASN1::brainpoolP512r1()); break;
                    default: throw std::runtime_error("KeyGenerator::getECCryptoData() > Unexpected elliptic curve type encountered.");
                }
                
                ECEncryptor * encr = new ECEncryptor(*decr);
                
                if(!decr->AccessKey().Validate(rnd, keyValidationLevel))
                    throw std::runtime_error("KeyGenerator::getECCryptoData() > New private key failed validation.");
                
                return ECCryptoDataContainerPtr(new ECCryptoDataContainer(decr, encr));
            }
            
            /**
             * Generates elliptic curve crypto data with the specified private key.
             * 
             * @param privateKey the private key to be used
             * @return the generated data
             */
            ECCryptoDataContainerPtr getECCryptoData(const CryptoPP::PrivateKey & privateKey) const
            {
                CryptoPP::AutoSeededRandomPool rnd;
                if(!privateKey.Validate(rnd, keyValidationLevel))
                    throw std::invalid_argument("KeyGenerator::getECCryptoData() > Existing private key failed validation.");
                
                ECDecryptor * decr = new ECDecryptor(privateKey);
                ECEncryptor * encr = new ECEncryptor(*decr);
                
                return ECCryptoDataContainerPtr(new ECCryptoDataContainer(decr, encr));
            }
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Key & IV Generation">
            /**
             * Generates a symmetric key using the specified passphrase and salt.
             * 
             * @param passphrase the passphrase to be used
             * @param salt the salt associated with the passphrase
             * @return the generated key
             */
            KeyData getDerivedSymmetricKey(const std::string & passphrase, const SaltData & salt) const
            {
                if(derivedKeyMinSaltSize > salt.size())
                    throw std::invalid_argument("KeyGenerator::deriveKey() > Insufficiently large salt was supplied.");
                
                KeyData key(derivedKeySize);
                derivedKeyGenerator->DeriveKey(
                                                key, key.size(), 
                                                0 /* purpose byte; unused */,
                                                reinterpret_cast<const CryptoPPByte *>(passphrase.c_str()), passphrase.length(),
                                                salt, salt.size(),
                                                derivedKeyIterations
                                              );
                
                return key;
            }
            
            /**
             * Generates a new symmetric key with the specified parameters.
             * 
             * @param keyType the type of key to generate
             * @param keySize the size of the key (in bytes); 0 == default size
             * @return the generated key
             */
            KeyData getSymmetricKey(SymmetricCipherType keyType, KeySize keySize = 0) const
            {
                if(keySize == 0)
                    keySize = defaultSymmetricKeySize;
                else if(keySize < minSymmetricKeySize)
                    throw std::invalid_argument("KeyGenerator::getSymmetricKey() > Insufficiently large key size was specified.");
                else if(keySize < defaultSymmetricKeySize)
                    logDebugMessage("(getSymmetricKey) > The supplied symmetric key size is smaller than the default size.");
                
                CryptoPP::AutoSeededRandomPool rnd;
                
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
                rnd.GenerateBlock(key, key.size());
                
                return key;
            }
            
            /**
             * Generates a new symmetric key with the specified parameters.
             * 
             * @param (template) TCipher template parameter denoting the cipher to be used
             * @param keySize the size of the key (in bytes); 0 == default size
             * @return the generated key
             */
            template <typename TCipher>
            KeyData getSymmetricKey(KeySize keySize = 0) const
            {
                if(keySize == 0)
                    keySize = defaultSymmetricKeySize;
                else if(keySize < minSymmetricKeySize)
                    throw std::invalid_argument("KeyGenerator::getSymmetricKey() > Insufficiently large key size was specified.");
                else if(keySize < defaultSymmetricKeySize)
                    logDebugMessage("(getSymmetricKey) > The supplied symmetric key size is smaller than the default size.");
                
                if(keySize > TCipher::MAX_KEYLENGTH)
                {
                    throw std::invalid_argument("KeyGenerator::getSymmetricKey() > The requested key size is too large"
                                                " for the specified cipher <" + std::string(TCipher::StaticAlgorithmName()) + ">.");
                }
                
                CryptoPP::AutoSeededRandomPool rnd;
                KeyData key(keySize);
                rnd.GenerateBlock(key, key.size());
                
                return key;
            }
            
            /**
             * Generates a new initialization vector.
             * 
             * @param size the size of the IV (in bytes); 0 == default size
             * @return the generated IV
             */
            IVData getIV(IVSize size = 0) const
            {
                CryptoPP::AutoSeededRandomPool rnd;
                IVData iv((size == 0) ? defaultIVSize : size);
                rnd.GenerateBlock(iv, iv.size());
                return iv;
            }
            //</editor-fold>
            
        private:
            //Debugging
            mutable Utilities::FileLogger * debugLogger;//debugging logger
            
            //Symmetric key configuration
            SymmetricCipherType defaultSymmetricCipher; //symmetric cipher (for crypto data & keys)
            AuthenticatedSymmetricCipherModeType defaultSymmetricCipherMode;
            IVSize defaultIVSize;                       //default initialization vector size (in bytes)
            KeySize minSymmetricKeySize;                //minimum symmetric key size (in bytes)
            KeySize defaultSymmetricKeySize;            //default symmetric key size (in bytes)
            
            //Asymmetric key configuration
            KeySize minRSAKeySize;                      //minimum RSA key size (in bytes)
            KeySize defaultRSAKeySize;                  //default RSA key size (in bytes)
            EllipticCurveType defaultEllipticCurve;     //default elliptic curve
            
            unsigned int keyValidationLevel;            //asymmetric key validation level
            
            //Derived key generation configuration
            CryptoPP::PasswordBasedKeyDerivationFunction * derivedKeyGenerator;
            unsigned int derivedKeyIterations;          //number of derived key iterations
            KeySize derivedKeySize;                     //derived key size (in bytes)
            SaltSize derivedKeyMinSaltSize;             //minimum salt size (in bytes) for the derived keys
            SaltSize derivedKeyDefaultSaltSize;         //default salt size (in bytes) for the derived keys
            
            /**
             * Logs the specified message, if a debugging file logger is assigned to the generator.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string message) const
            {
                if(debugLogger != nullptr)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "KeyGenerator / " + message);
            }
    };
}
#endif	/* KEYGENERATOR_H */

