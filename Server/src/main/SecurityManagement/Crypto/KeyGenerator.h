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

#include <cryptopp/dh.h>
#include <cryptopp/ecp.h>
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
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::PasswordDerivationFunction;
using SecurityManagement_Types::EllipticCurveType;
using SecurityManagement_Types::AsymmetricKeyValidationLevel;

using SecurityManagement_Crypto::SymmetricCryptoDataContainer;
using SecurityManagement_Crypto::SymmetricCryptoDataContainerPtr;

using SecurityManagement_Types::ECDecryptor;
using SecurityManagement_Types::ECEncryptor;
using SecurityManagement_Types::RSAPrivateKey;
using SecurityManagement_Types::RSAPublicKey;
using SecurityManagement_Types::ECDHPrivateKey;
using SecurityManagement_Types::ECDHPublicKey;
using SecurityManagement_Types::ECDH;

using SecurityManagement_Crypto::RSACryptoDataContainer;
using SecurityManagement_Crypto::ECCryptoDataContainer;
using SecurityManagement_Crypto::RSACryptoDataContainerPtr;
using SecurityManagement_Crypto::ECCryptoDataContainerPtr;
using SecurityManagement_Crypto::ECDHCryptoDataContainerPtr;


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
             const std::string & passphrase, unsigned int iterations,
             const SaltData & salt, const IVData & iv) const
            {
                KeyData key = getDerivedSymmetricKey(passphrase, iterations, salt);
                SymmetricCryptoDataContainerPtr result = getSymmetricCryptoData(cipher, mode, key, iv);
                result->updateSalt(salt);
                return result;
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default elliptic curve, symmetric cipher and mode.
             * 
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
            {
                return getSymmetricCryptoDataForDHExchange(defaultEllipticCurve, localPrivateKey, remotePublicKey, defaultSymmetricCipher, defaultSymmetricCipherMode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default symmetric cipher and mode.
             * 
             * @param curve the curve to be used for the key agreement
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
            {
                return getSymmetricCryptoDataForDHExchange(curve, localPrivateKey, remotePublicKey, defaultSymmetricCipher, defaultSymmetricCipherMode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm.
             * 
             * @param curve the curve to be used for the key agreement
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @param cipher the cipher for the symmetric crypto data
             * @param mode the mode for the symmetric cipher
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, 
             SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
            {
                IVData iv = getIV();
                return getSymmetricCryptoDataForDHExchange(curve, localPrivateKey, remotePublicKey, iv, cipher, mode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default elliptic curve, symmetric cipher and mode.
             * 
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @param iv the IV for the symmetric crypto data
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, const IVData & iv)
            {
                return getSymmetricCryptoDataForDHExchange(defaultEllipticCurve, localPrivateKey, remotePublicKey, iv, defaultSymmetricCipher, defaultSymmetricCipherMode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default symmetric cipher and mode.
             * 
             * @param curve the curve to be used for the key agreement
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @param iv the IV for the symmetric crypto data
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, const IVData & iv)
            {
                return getSymmetricCryptoDataForDHExchange(curve, localPrivateKey, remotePublicKey, iv, defaultSymmetricCipher, defaultSymmetricCipherMode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default elliptic curve.
             * 
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @param iv the IV for the symmetric crypto data
             * @param cipher the cipher for the symmetric crypto data
             * @param mode the mode for the symmetric cipher
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, 
             const IVData & iv, SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
            {
                return getSymmetricCryptoDataForDHExchange(defaultEllipticCurve, localPrivateKey, remotePublicKey, iv, cipher, mode);
            }
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm.
             * 
             * @param curve the curve to be used for the key agreement
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @param iv the IV for the symmetric crypto data
             * @param cipher the cipher for the symmetric crypto data
             * @param mode the mode for the symmetric cipher
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, 
             const IVData & iv, SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode)
            {
                KeyData key = getDiffieHellmanKeyEncryptionKey(curve, localPrivateKey, remotePublicKey);
                return getSymmetricCryptoData(cipher, mode, key, iv);
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
            
            /**
             * Generates new elliptic curve crypto data with the default primer curve.
             * 
             * @return the generated data
             */
            ECDHCryptoDataContainerPtr getECDHCryptoData() const
            {
                return getECDHCryptoData(defaultEllipticCurve);
            }
            
            /**
             * Generates new elliptic curve crypto data with the specified prime curve.
             * 
             * @param curveType the curve to be used
             * @return the generated data
             */
            ECDHCryptoDataContainerPtr getECDHCryptoData(EllipticCurveType curve) const
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
                    throw std::invalid_argument("KeyGenerator::getDerivedSymmetricKey() > Insufficiently large salt was supplied.");
                
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
             * Generates a symmetric key using the specified passphrase, iterations and salt.
             * 
             * @param passphrase the passphrase to be used
             * @param iterations the number of iterations to be used when generating the key
             * @param salt the salt associated with the passphrase
             * @return the generated key
             */
            KeyData getDerivedSymmetricKey(const std::string & passphrase, unsigned int iterations, const SaltData & salt) const
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
                                                (iterations != 0) ? iterations : derivedKeyIterations
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
                
                CryptoPP::AutoSeededRandomPool rng;
                KeyData key(keySize);
                rng.GenerateBlock(key, key.size());
                
                return key;
            }
            
            /**
             * Generates a new symmetric key encryption key, based on the Diffie-Hellman
             * key agreement algorithm, using the default elliptic curve.
             * 
             * Note: The size of the resulting key is set to the default symmetric
             * key size of the generator,
             * 
             * @param localPrivateKey the local peer's private key
             * @param remotePublicKey the remote peer's public key
             * @return the generated data
             * 
             * @throws runtime_error if a key could not be generated
             */
            KeyData getDiffieHellmanKeyEncryptionKey
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey)
            {
                return getDiffieHellmanKeyEncryptionKey(defaultEllipticCurve, localPrivateKey, remotePublicKey);
            }
            
            /**
             * Generates a new symmetric key encryption key, based on the Diffie-Hellman
             * key agreement algorithm.
             * 
             * Note: The size of the resulting key is set to the default symmetric
             * key size of the generator,
             * 
             * @param localPrivateKey the local peer's private key
             * @param remotePublicKey the remote peer's public key
             * @return the generated data
             * 
             * @throws runtime_error if a key could not be generated
             */
            KeyData getDiffieHellmanKeyEncryptionKey
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
            
            /**
             * Generates a new initialization vector.
             * 
             * @param size the size of the IV (in bytes); 0 == default size
             * @return the generated IV
             */
            IVData getIV(IVSize size = 0) const
            {
                CryptoPP::AutoSeededRandomPool rng;
                return KeyGenerator::getIV((size == 0) ? defaultIVSize : size, rng);
            }
            
            /**
             * Generates a new initialization vector.
             * 
             * @param size the size of the IV (in bytes)
             * @param rng reference to a random number generator
             * @return the generated IV
             */
            static const IVData getIV(IVSize size, CryptoPP::AutoSeededRandomPool & rng)
            {
                IVData iv(size);
                rng.GenerateBlock(iv, iv.size());
                return iv;
            }
            //</editor-fold>
            
            /**
             * Retrieves the default symmetric cipher used by the key generator.
             * 
             * @return the default symmetric cipher
             */
            SymmetricCipherType getDefaultSymmetricCipher() const
            {
                return defaultSymmetricCipher;
            }
            
            /**
             * Retrieves the default symmetric cipher mode used by the key generator.
             * 
             * @return the default symmetric cipher mode
             */
            AuthenticatedSymmetricCipherModeType getDefaultSymmetricCipherMode() const
            {
                return defaultSymmetricCipherMode;
            }
            
            /**
             * Retrieves the default asymmetric key validation level.
             * 
             * Note:
             * 
             * //Taken from CryptoPP's documentation:
             * //0 - using this object won't cause a crash or exception (rng is ignored) 
             * //1 - this object will probably function (encrypt, sign, etc.) correctly (but may not check for weak keys and such)
             * //2 - make sure this object will function correctly, and do reasonable security checks
             8 //3 - do checks that may take a long time
             * 
             * @return the default asymmetric key validation level
             */
            unsigned int getDefaultKeyValidationLevel() const
            {
                return keyValidationLevel;
            }
            
            /**
             * Retrieves the default elliptic curve used by the generator.
             * 
             * @return the default elliptic curve
             */
            EllipticCurveType getDefaultEllipticCurve() const
            {
                return defaultEllipticCurve;
            }
            
            /**
             * Retrieves the default IV size used by the generator.
             * 
             * @return the default IV size
             */
            IVSize getDefaultIVSize() const
            {
                return defaultIVSize;
            }
            
            /**
             * Retrieves the minimum symmetric key size allowed by the generator.
             * 
             * @return the min symmetric key size
             */
            KeySize getMinSymmetricKeySize() const
            {
                return minSymmetricKeySize;
            }
            
            /**
             * Retrieves the default symmetric key size used by the generator.
             * 
             * @return the default symmetric key size
             */
            KeySize getDefaultSymmetricKeySize() const
            {
                return defaultSymmetricKeySize;
            }
            
            /**
             * Retrieves the minimum RSA key size allowed by the generator.
             * 
             * @return the min RSA key size
             */
            KeySize getMinRSAKeySize() const
            {
                return minRSAKeySize;
            }
            
            /**
             * Retrieves the default RSA key size used by the generator.
             * 
             * @return the default RSA key size
             */
            KeySize getDefaultRSAKeySize() const
            {
                return defaultRSAKeySize;
            }
            
            /**
             * Retrieves the derived symmetric key size used by the generator.
             * 
             * @return the derived key size
             */
            KeySize getDerivedKeySize() const
            {
                return derivedKeySize;
            }
            
            /**
             * Retrieves the minimum allowed salt size for derived keys.
             * 
             * @return the min salt size for derived keys
             */
            SaltSize getDerivedKeyMinSaltSize() const
            {
                return derivedKeyMinSaltSize;
            }
            
            /**
             * Retrieves the default salt size for derived keys.
             * 
             * @return the default salt size
             */
            SaltSize getDerivedKeyDefaultSaltSize() const
            {
                return derivedKeyDefaultSaltSize;
            }
            
            /**
             * Retrieves the default iterations count for derived keys.
             * 
             * @return the default iterations count
             */
            unsigned int getDerivedKeyDefaultIterationsCount() const
            {
                return derivedKeyIterations;
            }
            
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
             * Creates a new ECDH domain object using the specified elliptic curve.
             * 
             * Note: The caller assumes responsibility for deleting the object.
             * 
             * @param curve the curve to be used
             * @return a pointer to the new object
             * 
             * @throws invalid_argument if the specified curve is not available
             * @throws runtime_error if any of the validations fail
             */
            ECDH * createECDHObject(EllipticCurveType curve) const
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

