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

#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/rsa.h>
#include <cryptopp/authenc.h>
#include "../Types/Types.h"

using SecurityManagement_Types::CryptoPPByte;
using SecurityManagement_Types::HashAlgorithmType;
using SecurityManagement_Types::SaltData;
using SecurityManagement_Types::IVData;
using SecurityManagement_Types::KeyData;

using CryptoPP::AuthenticatedSymmetricCipherBase;
using CryptoPP::SecByteBlock;

using SecurityManagement_Types::ECDecryptor;
using SecurityManagement_Types::ECEncryptor;
using SecurityManagement_Types::RSADecryptor;
using SecurityManagement_Types::RSAEncryptor;

namespace SecurityManagement_Crypto
{
    /**
     * Container class for symmetric crypto data.
     */
    class SymmetricCryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.\n
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
            
            //TODO default constructors, move assignments, etc ?
            
            /**
             * Updates the salt associated with the container.\n
             * 
             * Note: Can only be done if there is no salt set already (size==0).
             * 
             * @param newSalt the salt to be added to the container
             */
            void updateSalt(const SaltData newSalt) { if(salt.size() == 0) salt = newSalt; }
            
            /** Retrieves the stored initialization vector.\n\n@return the IV */
            const IVData getIV() const { return iv; }
            /** Retrieves the stored salt.\n\n@return the salt (if any) */
            const SaltData getSalt() const { return salt; }
            /** Retrieves the stored key.\n\n@return the key */
            const KeyData getKey() const { return key; }
            
            /**
             * Retrieves a pointer to the stored decryptor.
             * 
             * @return the decryptor
             * 
             * @throw runtime_error, if the decryptor is not set
             */
            AuthenticatedSymmetricCipherBase * getDecryptor()
            {
                if(decryptor == nullptr)
                    throw std::runtime_error("SymmetricCryptoDataContainer::getDecryptor() > The decryptor is not set.");
                else
                    return decryptor;
            }
            
            /**
             * Retrieves a pointer to the stored encryptor.
             * 
             * @return the encryptor
             * 
             * @throw runtime_error, if the encryptor is not set
             */
            AuthenticatedSymmetricCipherBase * getEncryptor()
            {
                if(encryptor == nullptr)
                    throw std::runtime_error("SymmetricCryptoDataContainer::getDecryptor() > The encryptor is not set.");
                else
                    return encryptor;
            }
            
        private:
            IVData iv;
            SaltData salt;
            KeyData key;
            AuthenticatedSymmetricCipherBase * decryptor;
            AuthenticatedSymmetricCipherBase * encryptor;
    };
    
    /** Boost unique_ptr deleter for symmetric crypto data containers. */
    struct SymmetricCryptoDataContainerDeleter { void operator()(SymmetricCryptoDataContainer * container) {  delete container; } };
    typedef boost::interprocess::unique_ptr<SymmetricCryptoDataContainer, SymmetricCryptoDataContainerDeleter> SymmetricCryptoDataContainerPtr;
    
    /**
     * Container class for RSA crypto data.
     */
    class RSACryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.\n
             * 
             * Note: Once the encryptor & decryptor are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param decryptor RSA decryptor
             * @param encryptor RSA encryptor
             */
            RSACryptoDataContainer(RSADecryptor * decryptor, RSAEncryptor * encryptor)
            : decryptor(decryptor), encryptor(encryptor)
            {}
            
            /**
             * Deletes the encryptor and the decryptor.
             */
            ~RSACryptoDataContainer()
            {
                delete decryptor;
                delete encryptor;
            }
            
            /**
             * Retrieves a pointer to the stored decryptor.
             * 
             * @return the decryptor
             * 
             * @throw runtime_error, if the decryptor is not set
             */
            RSADecryptor * getDecryptor()
            {
                if(decryptor == nullptr)
                    throw std::runtime_error("RSACryptoDataContainer::getDecryptor() > The decryptor is not set.");
                else
                    return decryptor;
            }
            
            /**
             * Retrieves a pointer to the stored encryptor.
             * 
             * @return the encryptor
             * 
             * @throw runtime_error, if the encryptor is not set
             */
            RSAEncryptor * getEncryptor()
            {
                if(encryptor == nullptr)
                    throw std::runtime_error("RSACryptoDataContainer::getDecryptor() > The encryptor is not set.");
                else
                    return encryptor;
            }
            
        private:
            RSADecryptor * decryptor;
            RSAEncryptor * encryptor;
    };
    
    /**
     * Container class for elliptic curve crypto data.
     */
    class ECCryptoDataContainer
    {
        public:
            /**
             * Creates a new container with the supplied data.\n
             * 
             * Note: Once the encryptor & decryptor are assigned to a container,
             * it becomes responsible for managing their life-cycle and they will
             * be deleted when the container is destroyed.
             * 
             * @param decryptor EC decryptor
             * @param encryptor EC encryptor
             */
            ECCryptoDataContainer(ECDecryptor * decryptor, ECEncryptor * encryptor)
            : decryptor(decryptor), encryptor(encryptor)
            {}
            
            /**
             * Deletes the encryptor and the decryptor.
             */
            ~ECCryptoDataContainer()
            {
                delete decryptor;
                delete encryptor;
            }
            
            /**
             * Retrieves a pointer to the stored decryptor.
             * 
             * @return the decryptor
             * 
             * @throw runtime_error, if the decryptor is not set
             */
            ECDecryptor * getDecryptor()
            {
                if(decryptor == nullptr)
                    throw std::runtime_error("ECCryptoDataContainer::getDecryptor() > The decryptor is not set.");
                else
                    return decryptor;
            }
            
            /**
             * Retrieves a pointer to the stored encryptor.
             * 
             * @return the encryptor
             * 
             * @throw runtime_error, if the encryptor is not set
             */
            ECEncryptor * getEncryptor()
            {
                if(encryptor == nullptr)
                    throw std::runtime_error("ECCryptoDataContainer::getDecryptor() > The encryptor is not set.");
                else
                    return encryptor;
            }
            
        private:
            ECDecryptor * decryptor;
            ECEncryptor * encryptor;
    };
    
    /** Boost unique_ptr deleter for asymmetric crypto data containers. */
    struct AsymmetricCryptoDataContainerDeleter
    {
        void operator()(RSACryptoDataContainer * container) { delete container; }
        void operator()(ECCryptoDataContainer * container) { delete container; }
    };
    
    typedef boost::interprocess::unique_ptr<RSACryptoDataContainer, AsymmetricCryptoDataContainerDeleter> RSACryptoDataContainerPtr;
    typedef boost::interprocess::unique_ptr<ECCryptoDataContainer, AsymmetricCryptoDataContainerDeleter> ECCryptoDataContainerPtr;
}

#endif	/* CRYPTO_CONTAINERS_H */

