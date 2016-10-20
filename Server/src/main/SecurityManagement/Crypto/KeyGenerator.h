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

#include <cryptopp/osrng.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/gcm.h>
#include <cryptopp/ccm.h>
#include <cryptopp/eax.h>

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
                /** Default symmetric key size (in bytes).
                 *  Note: Sizes beyond 20 bytes can cause failures during ECDH
                 *  key generation (depending on selected curve).
                 *  For example, if BP_P384R1 is used, the default symmetric
                 *  key size can be at most 48 bytes. */
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
                         AsymmetricKeysParameters akParams, Utilities::FileLoggerPtr logger = Utilities::FileLoggerPtr());
            
            /** 
             * Destroys the key generator.\n
             * 
             * Note: Requires a virtual destructor to be added to 
             * <code>PasswordBasedKeyDerivationFunction</code> in CryptoPP's <code>'pwdbased.h'</code>
             */
            ~KeyGenerator();
            
            KeyGenerator() = delete;
            KeyGenerator(const KeyGenerator& orig) = delete;
            KeyGenerator& operator=(const KeyGenerator& orig) = delete;
            
            //<editor-fold defaultstate="collapsed" desc="Symmetric Crypto">
            /**
             * Generates new symmetric crypto data using the default cipher and mode.
             * 
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData() const;
            
            /**
             * Generates symmetric crypto data using the default cipher and mode, with the specified key and IV.
             * 
             * @param key an existing symmetric key
             * @param iv the IV associated with the key
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData
            (const KeyData & key, const IVData & iv) const;
            
            /**
             * Generates new symmetric crypto data using the specified cipher and mode.
             * 
             * @param cipher the cipher to be used
             * @param mode the cipher mode to be used
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoData
            (SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode) const;
            
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
            (SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode, const KeyData & key, const IVData & iv) const;
            
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
            (const std::string & passphrase) const;
            
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
            (const std::string & passphrase, const SaltData & salt, const IVData & iv) const;
            
            /**
             * Generates new symmetric crypto data using the specified parameters.
             * 
             * @param cipher the cipher to be used
             * @param mode the mode to be used
             * @param passphrase the passphrase to be used
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataFromPassphrase
            (SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode,
             const std::string & passphrase) const;
            
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
             const std::string & passphrase, const SaltData & salt, const IVData & iv) const;
            
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
             const SaltData & salt, const IVData & iv) const;
            
            /**
             * Generates new symmetric crypto data, based on the Diffie-Hellman
             * key agreement algorithm, with the default elliptic curve, symmetric cipher and mode.
             * 
             * @param localPrivateKey the local peer's private key to be used for the agreement
             * @param remotePublicKey the remote peer's public key to be used for the agreement
             * @return the generated data
             */
            SymmetricCryptoDataContainerPtr getSymmetricCryptoDataForDHExchange
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey);
            
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
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
             const ECDHPublicKey & remotePublicKey);
            
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
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
             const ECDHPublicKey & remotePublicKey, SymmetricCipherType cipher,
             AuthenticatedSymmetricCipherModeType mode);
            
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
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey, const IVData & iv);
            
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
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
             const ECDHPublicKey & remotePublicKey, const IVData & iv);
            
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
             const IVData & iv, SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode);
            
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
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey,
             const ECDHPublicKey & remotePublicKey, const IVData & iv,
             SymmetricCipherType cipher, AuthenticatedSymmetricCipherModeType mode);
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Asymmetric Crypto">
            /**
             * Generates RSA crypto data.
             * 
             * @param keySize the size of the RSA key (in bytes); 0 == default size
             * @return the generated data
             */
            RSACryptoDataContainerPtr getRSACryptoData(KeySize keySize = 0) const;
            
            /**
             * Generates new elliptic curve crypto data with the default primer curve.
             * 
             * @return the generated data
             */
            ECDHCryptoDataContainerPtr getECDHCryptoData() const;
            
            /**
             * Generates new elliptic curve crypto data with the specified prime curve.
             * 
             * @param curveType the curve to be used
             * @return the generated data
             */
            ECDHCryptoDataContainerPtr getECDHCryptoData(EllipticCurveType curve) const;
            //</editor-fold>
            
            //<editor-fold defaultstate="collapsed" desc="Key & IV Generation">
            /**
             * Generates a symmetric key using the specified passphrase and salt.
             * 
             * @param passphrase the passphrase to be used
             * @param salt the salt associated with the passphrase
             * @return the generated key
             */
            KeyData getDerivedSymmetricKey(const std::string & passphrase, const SaltData & salt) const;
            
            /**
             * Generates a symmetric key using the specified passphrase, iterations and salt.
             * 
             * @param passphrase the passphrase to be used
             * @param iterations the number of iterations to be used when generating the key
             * @param salt the salt associated with the passphrase
             * @return the generated key
             */
            KeyData getDerivedSymmetricKey(const std::string & passphrase, unsigned int iterations, const SaltData & salt) const;
            
            /**
             * Generates a new symmetric key with the specified parameters.
             * 
             * @param keyType the type of key to generate
             * @param keySize the size of the key (in bytes); 0 == default size
             * @return the generated key
             */
            KeyData getSymmetricKey(SymmetricCipherType keyType, KeySize keySize = 0) const;
            
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
            (const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey);
            
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
            (EllipticCurveType curve, const ECDHPrivateKey & localPrivateKey, const ECDHPublicKey & remotePublicKey);
            
            /**
             * Generates a new initialization vector.
             * 
             * @param size the size of the IV (in bytes); 0 == default size
             * @return the generated IV
             */
            IVData getIV(IVSize size = 0) const;
            
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
            mutable Utilities::FileLoggerPtr debugLogger;//debugging logger
            
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
            ECDH * createECDHObject(EllipticCurveType curve) const;
            
            /**
             * Logs the specified message, if a debugging file logger is assigned to the generator.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logDebugMessage(const std::string & message) const
            {
                if(debugLogger)
                    debugLogger->logMessage(Utilities::FileLogSeverity::Debug, "KeyGenerator / " + message);
            }
    };
}
#endif	/* KEYGENERATOR_H */

