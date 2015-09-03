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

#ifndef CRYPTO_CONTAINERS_H
#define	CRYPTO_CONTAINERS_H

#include <boost/thread/future.hpp>
#include <boost/shared_ptr.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/rsa.h>
#include <cryptopp/authenc.h>
#include <cryptopp/osrng.h>
#include "../Types/Types.h"

using SecurityManagement_Types::CryptoPPByte;
using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::IVData;
using SecurityManagement_Types::KeyData;

using CryptoPP::AuthenticatedSymmetricCipherBase;
using CryptoPP::SecByteBlock;

using SecurityManagement_Types::RSAPrivateKey;
using SecurityManagement_Types::RSAPublicKey;
using SecurityManagement_Types::ECPrivateKey;
using SecurityManagement_Types::ECPublicKey;
using SecurityManagement_Types::ECDHPrivateKey;
using SecurityManagement_Types::ECDHPublicKey;

namespace SecurityManagement_Crypto
{
    /**
     * Container class for symmetric crypto data.
     */
    class SymmetricCryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.
             * 
             * Note: Once the encryptor & decryptor are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param ivData initialization vector
             * @param saltData salt (if any)
             * @param keyData symmetric key
             * @param decryptor symmetric decryptor
             * @param encryptor symmetric encryptor
             */
            SymmetricCryptoDataContainer(const IVData ivData, const SaltData saltData, KeyData keyData,
                                         AuthenticatedSymmetricCipherBase * decryptor,
                                         AuthenticatedSymmetricCipherBase * encryptor)
            : iv(ivData), salt(saltData), key(keyData), decryptor(decryptor), encryptor(encryptor)
            {}
            
            /**
             * Deletes the encryptor and the decryptor.
             */
            ~SymmetricCryptoDataContainer()
            {
                delete decryptor;
                delete encryptor;
            }
            
            SymmetricCryptoDataContainer() = delete;
            SymmetricCryptoDataContainer(const SymmetricCryptoDataContainer&) = delete;
            SymmetricCryptoDataContainer& operator=(const SymmetricCryptoDataContainer&) = delete;
            
            /**
             * Updates the salt associated with the container.
             * 
             * Note: Can only be done if there is no salt set already (size==0).
             * 
             * @param newSalt the salt to be added to the container
             */
            void updateSalt(const SaltData newSalt) { if(salt.size() == 0) salt = newSalt; }
            
            /**
             * Updates the IV associated with the container.
             * 
             * Notes:
             * - The IV is updated in the encryptor and decryptor as well.
             * - The IV must be the same size or larger than the current one.
             * - The new IV is truncated, if it is larger than the current one.
             * 
             * @param newIV the new IV
             * @throws invalid_argument if the new IV is smaller than the existing one
             */
            void updateIV(const IVData newIV)
            {
                if(newIV.size() >= iv.size())
                {
                    iv.Assign(newIV.BytePtr(), newIV.size());
                }
                else
                {
                    throw std::invalid_argument("SymmetricCryptoDataContainer::updateIV() > Insufficiently large IV specified.");
                }
                
                encryptor->SetKeyWithIV(key, key.size(), iv, iv.size());
                decryptor->SetKeyWithIV(key, key.size(), iv, iv.size());
            }
            
            /** Retrieves the stored initialization vector.\n\n@return the IV */
            const IVData getIV() const { return iv; }
            /** Retrieves the size of stored initialization vector.\n\n@return the size of the IV */
            const unsigned int getIVSize() const { return iv.size(); }
            /** Retrieves the stored salt.\n\n@return the salt (if any) */
            const SaltData getSalt() const { return salt; }
            /** Retrieves the stored key.\n\n@return the key */
            const KeyData getKey() const { return key; }
            
            /**
             * Retrieves a reference to the stored decryptor.
             * 
             * @return the decryptor
             * 
             * @throw runtime_error, if the decryptor is not set
             */
            AuthenticatedSymmetricCipherBase & getDecryptor()
            {
                if(decryptor == nullptr)
                {
                    throw std::runtime_error("SymmetricCryptoDataContainer::getDecryptor() > "
                                             "The decryptor is not set.");
                }
                else
                    return *decryptor;
            }
            
            /**
             * Retrieves a reference to the stored encryptor.
             * 
             * @return the encryptor
             * 
             * @throw runtime_error, if the encryptor is not set
             */
            AuthenticatedSymmetricCipherBase & getEncryptor()
            {
                if(encryptor == nullptr)
                {
                    throw std::runtime_error("SymmetricCryptoDataContainer::getDecryptor() > "
                                             "The encryptor is not set.");
                }
                else
                    return *encryptor;
            }
            
        private:
            IVData iv;
            SaltData salt;
            KeyData key;
            AuthenticatedSymmetricCipherBase * decryptor;
            AuthenticatedSymmetricCipherBase * encryptor;
    };
    
    typedef boost::shared_ptr<SymmetricCryptoDataContainer> SymmetricCryptoDataContainerPtr;
    typedef boost::promise<SymmetricCryptoDataContainerPtr> SymmetricCryptoDataContainerPromise;
    typedef boost::shared_ptr<SymmetricCryptoDataContainerPromise> SymmetricCryptoDataContainerPromisePtr;
    
    /**
     * Container class for RSA crypto data.
     */
    class RSACryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.
             * 
             * Note: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey RSA private key
             * @param pbKey RSA public key
             */
            RSACryptoDataContainer(RSAPrivateKey * pvKey, RSAPublicKey * pbKey)
            : privateKey(pvKey), publicKey(pbKey)
            {}
            
            /**
             * Deletes the keys.
             */
            ~RSACryptoDataContainer()
            {
                delete privateKey;
                delete publicKey;
            }
            
            RSACryptoDataContainer(const RSACryptoDataContainer&) = delete;
            RSACryptoDataContainer& operator=(const RSACryptoDataContainer&) = delete;
            
            /**
             * Creates a new container pointer from the supplied private key
             * and creates a public key based on the private one.
             * 
             * Note 1: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>RSACryptoDataContainerPtr</code> object.
             * 
             * Note 2: The key validation level can be retrieved from a
             * <code>KeyGenerator</code>.
             * 
             * Note 3: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey the private key to be used for creating the container
             * @param keyValidationLevel the level to be used for key validation
             * 
             * @return a pointer to the newly created container.
             * 
             * @throw invalid_argument if the key fails the validation
             */
            static RSACryptoDataContainer * getContainerPtrFromPrivateKey(const std::string & pvKey, unsigned int keyValidationLevel)
            {
                RSACryptoDataContainer * container = new RSACryptoDataContainer();
                
                try
                {
                    CryptoPP::StringSource source(pvKey, true);
                    CryptoPP::ByteQueue queue;
                    source.TransferTo(queue);
                    queue.MessageEnd();
                    
                    container->privateKey = new RSAPrivateKey();
                    container->privateKey->BERDecodePrivateKey(queue, false, queue.MaxRetrievable());

                    CryptoPP::AutoSeededRandomPool rnd;
                    if(!container->privateKey->Validate(rnd, keyValidationLevel))
                        throw std::invalid_argument("RSACryptoDataContainer::getContainerFromPrivateKey() > Private key failed validation.");

                    container->publicKey = new RSAPublicKey(*container->privateKey);
                }
                catch(...)
                {
                    delete container;
                    throw;
                }
                
                return container;
            }
            
            /**
             * Creates a new container pointer from the supplied public key.
             * 
             * Note 1: Only the public key will be available through the container.
             * 
             * Note 2: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>RSACryptoDataContainerPtr</code> object.
             * 
             * Note 3: The key validation level can be retrieved from a
             * <code>KeyGenerator</code>.
             * 
             * Note 4: Once the public key is assigned to a container,
             * the container becomes responsible for managing its life-cycle
             * and the key will be deleted when the container is destroyed.
             * 
             * @param pbKey the public key to be used for creating the container
             * @param keyValidationLevel the level to be used for key validation
             * 
             * @return a pointer to the newly created container.
             * 
             * @throw invalid_argument if the key fails the validation
             */
            static RSACryptoDataContainer * getContainerPtrFromPublicKey(const std::string & pbKey, unsigned int keyValidationLevel)
            {
                RSACryptoDataContainer * container = new RSACryptoDataContainer();
                
                try
                {
                    CryptoPP::StringSource source(pbKey, true);
                    CryptoPP::ByteQueue queue;
                    source.TransferTo(queue);
                    queue.MessageEnd();

                    container->publicKey = new RSAPublicKey();
                    container->publicKey->Load(queue);

                    CryptoPP::AutoSeededRandomPool rnd;
                    if(!container->publicKey->Validate(rnd, keyValidationLevel))
                        throw std::invalid_argument("RSACryptoDataContainer::getContainerPtrFromPublicKey() > Public key failed validation.");
                }
                catch(...)
                {
                    delete container;
                    throw;
                }
                
                return container;
            }
            
            /**
             * Retrieves a reference to the stored private key.
             * 
             * @return the private key
             * 
             * @throw runtime_error, if the key is not set
             */
            const RSAPrivateKey & getPrivateKey() const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("RSACryptoDataContainer::getPrivateKey() > "
                                             "The key is not set.");
                }
                else
                    return *privateKey;
            }
            
            /**
             * Retrieves a reference to the stored public key.
             * 
             * @return the public key
             * 
             * @throw runtime_error, if the key is not set
             */
            const RSAPublicKey & getPublicKey() const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("RSACryptoDataContainer::getPublicKey() > "
                                             "The key is not set.");
                }
                else
                    return *publicKey;
            }
            
            /**
             * Checks if the private key is set in the container.
             * 
             * @return <code>true</code>, if the private key is set
             */
            bool isPrivateKeySet() const
            {
                return (privateKey != nullptr);
            }
            
            /**
             * Checks if the public key is set in the container.
             * 
             * @return <code>true</code>, if the public key is set
             */
            bool isPublicKeySet() const
            {
                return (publicKey != nullptr);
            }
            
            /**
             * Retrieves the private key, for persistent storage purposes.
             * 
             * @param output the string to be used for storing the private key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPrivateKeyForStorage(std::string & output) const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("RSACryptoDataContainer::getPrivateKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::ByteQueue queue;
                    privateKey->DEREncodePrivateKey(queue);
                    CryptoPP::StringSink outputSink(output);
                    queue.TransferTo(outputSink);
                    outputSink.MessageEnd();
                }
            }
            
            /**
             * Retrieves the public key, for persistent storage purposes.
             * 
             * @param output the string to be used for storing the public key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPublicKeyForStorage(std::string & output) const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("RSACryptoDataContainer::getPublicKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::ByteQueue queue;
                    publicKey->Save(queue);
                    CryptoPP::StringSink outputSink(output);
                    queue.TransferTo(outputSink);
                    outputSink.MessageEnd();
                }
            }
            
        private:
            RSACryptoDataContainer()
            : privateKey(nullptr), publicKey(nullptr)
            {}
            
            RSAPrivateKey * privateKey;
            RSAPublicKey * publicKey;
    };
    
    /**
     * Container class for elliptic curve crypto data.
     */
    class ECCryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.
             * 
             * Note: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey EC private key
             * @param pbKey EC public key
             */
            ECCryptoDataContainer(ECPrivateKey * pvKey, ECPublicKey * pbKey)
            : privateKey(pvKey), publicKey(pbKey)
            {}
            
            /**
             * Deletes the keys.
             */
            ~ECCryptoDataContainer()
            {
                delete privateKey;
                delete publicKey;
            }
            
            ECCryptoDataContainer(const ECCryptoDataContainer&) = delete;
            ECCryptoDataContainer& operator=(const ECCryptoDataContainer&) = delete;
            
            /**
             * Creates a new container pointer from the supplied private key
             * and creates a public key based on the private one.
             * 
             * Note 1: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>ECCryptoDataContainerPtr</code> object.
             * 
             * Note 2: The key validation level can be retrieved from a
             * <code>KeyGenerator</code>.
             * 
             * Note 3: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey the private key to be used for creating the container
             * @param keyValidationLevel the level to be used for key validation
             * 
             * @return a pointer to the newly created container.
             * 
             * @throw invalid_argument if the key fails the validation
             */
            static ECCryptoDataContainer * getContainerPtrFromPrivateKey(const std::string & pvKey, unsigned int keyValidationLevel)
            {
                ECCryptoDataContainer * container = new ECCryptoDataContainer();
                
                try
                {
                    CryptoPP::StringSource source(pvKey, true);
                    CryptoPP::ByteQueue queue;
                    source.TransferTo(queue);
                    queue.MessageEnd();

                    container->privateKey = new ECPrivateKey();
                    container->privateKey->Load(queue);

                    CryptoPP::AutoSeededRandomPool rnd;
                    if(!container->privateKey->Validate(rnd, keyValidationLevel))
                        throw std::invalid_argument("ECCryptoDataContainer::getContainerFromPrivateKey() > Private key failed validation.");

                    container->publicKey = new ECPublicKey();
                    container->privateKey->MakePublicKey(*container->publicKey);
                }
                catch(...)
                {
                    delete container;
                    throw;
                }
                
                return container;
            }
            
            /**
             * Creates a new container pointer from the supplied public key.
             * 
             * Note 1: Only the public key will be available through the container.
             * 
             * Note 2: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>ECCryptoDataContainerPtr</code> object.
             * 
             * Note 3: The key validation level can be retrieved from a
             * <code>KeyGenerator</code>.
             * 
             * Note 4: Once the public key is assigned to a container,
             * the container becomes responsible for managing its life-cycle
             * and the key will be deleted when the container is destroyed.
             * 
             * @param pbKey the public key to be used for creating the container
             * @param keyValidationLevel the level to be used for key validation
             * 
             * @return a pointer to the newly created container.
             * 
             * @throw invalid_argument if the key fails the validation
             */
            static ECCryptoDataContainer * getContainerPtrFromPublicKey(const std::string & pbKey, unsigned int keyValidationLevel)
            {
                ECCryptoDataContainer * container = new ECCryptoDataContainer();
                
                try
                {
                    CryptoPP::StringSource source(pbKey, true);
                    CryptoPP::ByteQueue queue;
                    source.TransferTo(queue);
                    queue.MessageEnd();

                    container->publicKey = new ECPublicKey();
                    container->publicKey->Load(queue);

                    CryptoPP::AutoSeededRandomPool rnd;
                    if(!container->publicKey->Validate(rnd, keyValidationLevel))
                        throw std::invalid_argument("ECCryptoDataContainer::getContainerPtrFromPublicKey() > Public key failed validation.");
                }
                catch(...)
                {
                    delete container;
                    throw;
                }
                
                return container;
            }
            
            /**
             * Retrieves a reference to the stored private key.
             * 
             * @return the private key
             * 
             * @throw runtime_error, if the key is not set
             */
            const ECPrivateKey & getPrivateKey() const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("ECCryptoDataContainer::getPrivateKey() > "
                                             "The key is not set.");
                }
                else
                    return *privateKey;
            }
            
            /**
             * Retrieves a reference to the stored public key.
             * 
             * @return the public key
             * 
             * @throw runtime_error, if the key is not set
             */
            const ECPublicKey & getPublicKey() const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("ECCryptoDataContainer::getPublicKey() > "
                                             "The key is not set.");
                }
                else
                    return *publicKey;
            }
            
            /**
             * Checks if the private key is set in the container.
             * 
             * @return <code>true</code>, if the private key is set
             */
            bool isPrivateKeySet() const
            {
                return (privateKey != nullptr);
            }
            
            /**
             * Checks if the public key is set in the container.
             * 
             * @return <code>true</code>, if the public key is set
             */
            bool isPublicKeySet() const
            {
                return (publicKey != nullptr);
            }
            
            /**
             * Retrieves a DER encoded version of the private key, for persistent
             * storage purposes.
             * 
             * @param output the string to be used for storing the DER encoded private key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPrivateKeyForStorage(std::string & output) const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("ECCryptoDataContainer::getPrivateKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::ByteQueue queue;
                    privateKey->Save(queue);
                    CryptoPP::StringSink outputSink(output);
                    queue.TransferTo(outputSink);
                    outputSink.MessageEnd();
                }
            }
            
            /**
             * Retrieves a DER encoded version of the public key, for persistent
             * storage purposes.
             * 
             * @param output the string to be used for storing the DER encoded public key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPublicKeyForStorage(std::string & output) const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("ECCryptoDataContainer::getPublicKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::ByteQueue queue;
                    publicKey->Save(queue);
                    CryptoPP::StringSink outputSink(output);
                    queue.TransferTo(outputSink);
                    outputSink.MessageEnd();
                }
            }
            
        private:
            ECCryptoDataContainer()
            : privateKey(nullptr), publicKey(nullptr)
            {}
            
            ECPrivateKey * privateKey;
            ECPublicKey * publicKey;
    };
    
    /**
     * Container class for elliptic curve Diffie-Hellman crypto data.
     */
    class ECDHCryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.
             * 
             * Note: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey EC private key
             * @param pbKey EC public key
             */
            ECDHCryptoDataContainer(ECDHPrivateKey * pvKey, ECDHPublicKey * pbKey)
            : privateKey(pvKey), publicKey(pbKey)
            {}
            
            /**
             * Deletes the keys.
             */
            ~ECDHCryptoDataContainer()
            {
                delete privateKey;
                delete publicKey;
            }
            
            ECDHCryptoDataContainer(const ECDHCryptoDataContainer&) = delete;
            ECDHCryptoDataContainer& operator=(const ECDHCryptoDataContainer&) = delete;
            
            /**
             * Creates a new container pointer from the supplied private and public keys.
             * 
             * Note 1: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>ECDHCryptoDataContainerPtr</code> object.
             * 
             * Note 2: Once the keys are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param pvKey the private key to be used for creating the container
             * @param pbKey the public key to be used for creating the container
             * 
             * @return a pointer to the newly created container.
             */
            static ECDHCryptoDataContainer * getContainerFromKeyPair(const std::string & pvKey, const std::string & pbKey)
            {
                if(pvKey.size() == 0 || pbKey.size() == 0)
                {
                    throw std::invalid_argument("ECDHCryptoDataContainer::getContainerFromKeyPair() > "
                            "One or both of the supplied keys are empty.");
                }
                
                ECDHCryptoDataContainer * container = new ECDHCryptoDataContainer();
                container->privateKey = new ECDHPrivateKey(reinterpret_cast<const unsigned char*>(pvKey.data()), pvKey.size());
                container->publicKey = new ECDHPublicKey(reinterpret_cast<const unsigned char*>(pbKey.data()), pbKey.size());
                
                return container;
            }
            
            /**
             * Creates a new container pointer from the supplied  public key.
             * 
             * Note 1: Only the public key will be available through the container.
             * 
             * Note 2: The caller assumes responsibility for deleting the data and
             * it should be used for creating a <code>ECDHCryptoDataContainerPtr</code> object.
             * 
             * Note 3: Once the public key is assigned to a container,
             * the container becomes responsible for managing its life-cycle
             * and the key will be deleted when the container is destroyed.
             * 
             * @param pbKey the public key to be used for creating the container
             * 
             * @return a pointer to the newly created container.
             */
            static ECDHCryptoDataContainer * getContainerFromPublicKey(const std::string & pbKey)
            {
                if(pbKey.size() == 0)
                {
                    throw std::invalid_argument("ECDHCryptoDataContainer::getContainerFromPublicKey() > "
                            "The supplied public key is empty.");
                }
                
                ECDHCryptoDataContainer * container = new ECDHCryptoDataContainer();
                container->publicKey = new ECDHPublicKey(reinterpret_cast<const unsigned char*>(pbKey.data()), pbKey.size());
                
                return container;
            }
            
            /**
             * Retrieves a reference to the stored private key.
             * 
             * @return the private key
             * 
             * @throw runtime_error, if the key is not set
             */
            const ECDHPrivateKey & getPrivateKey() const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("ECDHCryptoDataContainer::getPrivateKey() > "
                                             "The key is not set.");
                }
                else
                    return *privateKey;
            }
            
            /**
             * Retrieves a reference to the stored public key.
             * 
             * @return the public key
             * 
             * @throw runtime_error, if the key is not set
             */
            const ECDHPublicKey & getPublicKey() const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("ECDHCryptoDataContainer::getPublicKey() > "
                                             "The key is not set.");
                }
                else
                    return *publicKey;
            }
            
            /**
             * Checks if the private key is set in the container.
             * 
             * @return <code>true</code>, if the private key is set
             */
            bool isPrivateKeySet() const
            {
                return (privateKey != nullptr);
            }
            
            /**
             * Checks if the public key is set in the container.
             * 
             * @return <code>true</code>, if the public key is set
             */
            bool isPublicKeySet() const
            {
                return (publicKey != nullptr);
            }
            
            /**
             * Retrieves the private key, for persistent storage purposes.
             * 
             * @param output the string to be used for storing the private key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPrivateKeyForStorage(std::string & output) const
            {
                if(privateKey == nullptr)
                {
                    throw std::runtime_error("ECDHCryptoDataContainer::getPrivateKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::StringSource source(privateKey->BytePtr(), privateKey->SizeInBytes(), true, new CryptoPP::StringSink(output));
                }
            }
            
            /**
             * Retrieves the public key, for persistent storage purposes.
             * 
             * @param output the string to be used for storing the public key
             * 
             * @throw runtime_error, if the key is not set
             */
            void getPublicKeyForStorage(std::string & output) const
            {
                if(publicKey == nullptr)
                {
                    throw std::runtime_error("ECDHCryptoDataContainer::getPublicKeyForStorage() > "
                                             "The key is not set.");
                }
                else
                {
                    CryptoPP::StringSource source(publicKey->BytePtr(), publicKey->SizeInBytes(), true, new CryptoPP::StringSink(output));
                }
            }
            
        private:
            ECDHCryptoDataContainer()
            : privateKey(nullptr), publicKey(nullptr)
            {}
            
            ECDHPrivateKey * privateKey;
            ECDHPublicKey * publicKey;
    };
    
    typedef boost::shared_ptr<RSACryptoDataContainer> RSACryptoDataContainerPtr;
    typedef boost::shared_ptr<ECCryptoDataContainer> ECCryptoDataContainerPtr;
    typedef boost::shared_ptr<ECDHCryptoDataContainer> ECDHCryptoDataContainerPtr;
}

#endif	/* CRYPTO_CONTAINERS_H */

