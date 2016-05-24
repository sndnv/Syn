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

#ifndef CRYPTO_HANDLERS_H
#define	CRYPTO_HANDLERS_H

#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include "Containers.h"
#include "../Types/Types.h"
#include "KeyGenerator.h"

using SecurityManagement_Types::RSADecryptor;
using SecurityManagement_Types::RSAEncryptor;
using SecurityManagement_Types::RSASigner;
using SecurityManagement_Types::RSAVerifier;
using SecurityManagement_Types::CiphertextData;
using SecurityManagement_Types::PlaintextData;
using SecurityManagement_Types::SignedData;

namespace SecurityManagement_Crypto
{
    /**
     * Class for handling encryption and decryption with symmetric crypto data.
     * 
     * Note: Supports authenticated modes only.
     */
    class SymmetricCryptoHandler
    {
        public:
            /**
             * Builds a new handler using the supplied symmetric crypto data.
             * 
             * @param symCryptoData the data to be used for encryption/decryption
             */
            explicit SymmetricCryptoHandler(SymmetricCryptoDataContainerPtr symCryptoData)
            : cryptoData(symCryptoData)
            {}
            
            /**
             * Encrypts the supplied plaintext data.
             * 
             * @param plaintext the input plaintext data
             * @param ciphertext the output ciphertext data
             */
            void encryptData(const PlaintextData & plaintext, CiphertextData & ciphertext)
            {
                if(cryptoData->getEncryptor().NeedsPrespecifiedDataLengths())
                    cryptoData->getEncryptor().SpecifyDataLengths(cryptoData->getIVSize(), plaintext.size(), 0);
                
                CryptoPP::AutoSeededRandomPool rng;
                IVData nextIV = KeyGenerator::getIV(cryptoData->getIVSize(), rng);
                
                CryptoPP::AuthenticatedEncryptionFilter filter(
                        cryptoData->getEncryptor(),
                        nullptr);
                
                //does authentication on the new IV
                filter.ChannelPut(CryptoPP::AAD_CHANNEL, nextIV.data(), nextIV.size());
                filter.ChannelMessageEnd(CryptoPP::AAD_CHANNEL);
                //does encryption + authentication on the plaintext
                filter.ChannelPut(CryptoPP::DEFAULT_CHANNEL, reinterpret_cast<const byte *>(plaintext.data()), plaintext.size());
                filter.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);
                
                CryptoPP::SecByteBlock rawCiphertext(filter.MaxRetrievable());
                filter.Get(rawCiphertext, filter.MaxRetrievable());
                ciphertext.resize(filter.MaxRetrievable());
                ciphertext.assign(reinterpret_cast<const char *>(rawCiphertext.data()), rawCiphertext.size());
                cryptoData->updateIV(nextIV);
                appendIVData(ciphertext, nextIV);
            }
            
            /**
             * Decrypts the supplied ciphertext data.
             * 
             * @param ciphertext the input ciphertext data
             * @param plaintext the output plaintext data
             */
            void decryptData(const CiphertextData & ciphertext, PlaintextData & plaintext)
            {
                if(cryptoData->getDecryptor().NeedsPrespecifiedDataLengths())
                {
                    cryptoData->getDecryptor().SpecifyDataLengths(cryptoData->getIVSize(),
                            (ciphertext.size() - cryptoData->getDecryptor().TagSize() - cryptoData->getIVSize()), 0);
                }
                
                IVData nextIV = extractIVData(ciphertext);
                
                unsigned int dataSize = ciphertext.size() - cryptoData->getDecryptor().TagSize() - cryptoData->getIVSize();
                std::string data = ciphertext.substr(0, dataSize);
                std::string tag = ciphertext.substr(dataSize, cryptoData->getDecryptor().TagSize());
                
                CryptoPP::AuthenticatedDecryptionFilter filter(
                        cryptoData->getDecryptor(),
                        nullptr,
                        CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_END
                        | CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION,
                        cryptoData->getDecryptor().TagSize());
                
                filter.ChannelPut(CryptoPP::AAD_CHANNEL, nextIV.data(), nextIV.size());
                filter.ChannelPut(CryptoPP::DEFAULT_CHANNEL, reinterpret_cast<const byte *>(data.data()), data.size());
                filter.ChannelPut(CryptoPP::DEFAULT_CHANNEL, reinterpret_cast<const byte *>(tag.data()), tag.size());
                filter.MessageEnd();
                
                if(filter.GetLastResult() && filter.MaxRetrievable() > 0)
                {
                    CryptoPP::SecByteBlock rawPlaintext(filter.MaxRetrievable());
                    filter.Get(rawPlaintext.data(), filter.MaxRetrievable());
                    plaintext.assign(reinterpret_cast<const char *>(rawPlaintext.data()), rawPlaintext.size());
                }
                else
                {
                    throw std::runtime_error("SymmetricCryptoHandler::decryptData() > Failed to decrypt/authenticate data.");
                }
                
                cryptoData->updateIV(nextIV);
            }
            
            /**
             * Retrieves the crypto data used by the handler.
             * 
             * @return the requested crypto data
             */
            SymmetricCryptoDataContainerPtr getCryptoData() const
            {
                return cryptoData;
            }
            
        private:
            SymmetricCryptoDataContainerPtr cryptoData;
            CryptoPP::AutoSeededRandomPool ivRng;
            
            void appendIVData(CiphertextData & ciphertext, const IVData & iv) const
            {
                ciphertext.append(reinterpret_cast<const char *>(iv.data()), iv.size());
            }
            
            const IVData extractIVData(const CiphertextData & ciphertext) const
            {
                std::string iv = ciphertext.substr(ciphertext.size() - cryptoData->getIVSize());
                return IVData(reinterpret_cast<const byte *>(iv.data()), iv.size());
            }
    };
    
    typedef boost::shared_ptr<SymmetricCryptoHandler> SymmetricCryptoHandlerPtr;
    
    /**
     * Class for handling encryption, decryption, signing and verification 
     * with asymmetric crypto data.
     */
    class AsymmetricCryptoHandler
    {
        public:
            /**
             * Builds a new handler using RSA crypto data.
             * 
             * @param rsaCryptoData the data to be used for encryption/decryption
             * 
             * @throw logic_error if no keys were supplied
             */
            explicit AsymmetricCryptoHandler(RSACryptoDataContainerPtr rsaCryptoData)
            : cryptoData(rsaCryptoData)
            {
                privateKeyAvailable = rsaCryptoData->isPrivateKeySet();
                publicKeyAvailable = rsaCryptoData->isPublicKeySet();
                
                if(!privateKeyAvailable && !publicKeyAvailable)
                {
                    throw std::logic_error("AsymmetricCryptoHandler::(RSA) > No keys were supplied.");
                }
            }
            
            /**
             * Signs the supplied plaintext data with the stored PRIVATE key.
             * 
             * @param plaintext the input plaintext data
             * @param signature the output signature data
             * 
             * @throw logic_error if a private key is not available
             */
            void signDataWithPrivateKey(const PlaintextData & plaintext, SignedData & signature)
            {
                if(privateKeyAvailable)
                {
                    boost::apply_visitor(boost::bind(SignDataVisitor(), boost::cref(plaintext), boost::ref(signature), _1), cryptoData);
                }
                else
                {
                    throw std::logic_error("AsymmetricCryptoHandler::signDataWithPrivateKey() > The private key is not available.");
                }
            }
            
            /**
             * Encrypts the supplied plaintext data with the stored PUBLIC key.
             * 
             * @param plaintext the input plaintext data
             * @param ciphertext the output ciphertext data
             * 
             * @throw logic_error if a public key is not available
             */
            void encryptDataWithPublicKey(const PlaintextData & plaintext, CiphertextData & ciphertext)
            {
                if(publicKeyAvailable)
                {
                    boost::apply_visitor(boost::bind(EncryptDataVisitor(), boost::cref(plaintext), boost::ref(ciphertext), _1), cryptoData);
                }
                else
                {
                    throw std::logic_error("AsymmetricCryptoHandler::encryptDataWithPublicKey() > The public key is not available.");
                }
            }
            
            /**
             * Decrypts the supplied ciphertext data with the stored PRIVATE key.
             * 
             * @param ciphertext the input ciphertext data
             * @param plaintext the output plaintext data
             * 
             * @throw logic_error if a private key is not available
             */
            void decryptDataWithPrivateKey(const CiphertextData & ciphertext, PlaintextData & plaintext)
            {
                if(privateKeyAvailable)
                {
                    boost::apply_visitor(boost::bind(DecryptDataVisitor(), boost::cref(ciphertext), boost::ref(plaintext), _1), cryptoData);
                }
                else
                {
                    throw std::logic_error("AsymmetricCryptoHandler::decryptDataWithPrivateKey() > The private key is not available.");
                }
            }
            
            /**
             * Verifies the signature data and recovers the plaintext data with the stored PUBLIC key.
             * 
             * @param signature the input signature data
             * @param plaintext the output plaintext data
             * 
             * @throw logic_error if a public key is not available
             */
            void verifyAndRecoverDataWithPublicKey(const SignedData & signature, PlaintextData & plaintext)
            {
                if(publicKeyAvailable)
                {
                    boost::apply_visitor(boost::bind(VerifyAndRecoverDataVisitor(), boost::cref(signature), boost::ref(plaintext), _1), cryptoData);
                }
                else
                {
                    throw std::logic_error("AsymmetricCryptoHandler::verifyAndRecoverDataWithPublicKey() > The public key is not available.");
                }
            }
            
        private:
            bool privateKeyAvailable;
            bool publicKeyAvailable;
            
            boost::variant<RSACryptoDataContainerPtr> cryptoData;
            
            class EncryptDataVisitor : public boost::static_visitor<>
            {
                public:
                    void operator()(const PlaintextData & plaintext, CiphertextData & ciphertext,
                                    RSACryptoDataContainerPtr rsaCryptoData) const
                    {
                        RSAEncryptor encryptor(rsaCryptoData->getPublicKey());
                        
                        //TODO - set maximum number of allowed chunks (i.e. max encrypt-able data size)
                        CryptoPP::AutoSeededRandomPool rng;
                        unsigned int maxPlaintextSize = encryptor.FixedMaxPlaintextLength();
                        if(maxPlaintextSize < plaintext.size())
                        {//splits the plaintext into chunks and encrypts each separately 
                            CiphertextData currentCiphertextChunk;
                            for(unsigned int i = 0; i <= (plaintext.size() / maxPlaintextSize); i++)
                            {
                                currentCiphertextChunk.clear();
                                
                                CryptoPP::StringSource source(plaintext.substr(i*maxPlaintextSize, maxPlaintextSize), true,
                                    new CryptoPP::PK_EncryptorFilter(rng, encryptor,
                                        new CryptoPP::StringSink(currentCiphertextChunk)));
                                
                                ciphertext.append(currentCiphertextChunk);
                            }
                        }
                        else
                        {
                            CryptoPP::StringSource source(plaintext, true,
                                    new CryptoPP::PK_EncryptorFilter(rng, encryptor,
                                        new CryptoPP::StringSink(ciphertext)));
                        }
                    }
            };
            
            class DecryptDataVisitor : public boost::static_visitor<>
            {
                public:
                    void operator()(const CiphertextData & ciphertext, PlaintextData & plaintext,
                                    RSACryptoDataContainerPtr rsaCryptoData) const
                    {
                        RSADecryptor decryptor(rsaCryptoData->getPrivateKey());
                        
                        CryptoPP::AutoSeededRandomPool rng;
                        unsigned int ciphertextChunkSize = decryptor.FixedCiphertextLength();
                        if(ciphertextChunkSize < ciphertext.size())
                        {//splits the ciphertext into chunks and decrypts each separately
                            PlaintextData currentPlaintextChunk;
                            for(unsigned int i = 0; i < (ciphertext.size() / ciphertextChunkSize); i++)
                            {
                                currentPlaintextChunk.clear();
                                CryptoPP::StringSource source(ciphertext.substr(i*ciphertextChunkSize, ciphertextChunkSize), true,
                                    new CryptoPP::PK_DecryptorFilter(rng, decryptor,
                                        new CryptoPP::StringSink(currentPlaintextChunk)));
                                
                                plaintext.append(currentPlaintextChunk);
                            }
                        }
                        else
                        {
                            CryptoPP::StringSource source(ciphertext, true,
                                    new CryptoPP::PK_DecryptorFilter(rng, decryptor,
                                        new CryptoPP::StringSink(plaintext)));
                        }
                    }
            };
            
            class SignDataVisitor : public boost::static_visitor<>
            {
                public:
                    void operator() (const PlaintextData & plaintext, SignedData & signature,
                            RSACryptoDataContainerPtr rsaCryptoData) const
                    {
                        RSASigner signer(rsaCryptoData->getPrivateKey());
                        CryptoPP::AutoSeededRandomPool rng;
                        
                        CryptoPP::StringSource source(plaintext, true,
                                new CryptoPP::SignerFilter(rng, signer,
                                    new CryptoPP::StringSink(signature),
                                    true));
                    }
            };
            
            class VerifyAndRecoverDataVisitor : public boost::static_visitor<>
            {
                public:
                    void operator() (const SignedData & signature, PlaintextData & plaintext,
                            RSACryptoDataContainerPtr rsaCryptoData) const
                    {
                        RSAVerifier verifier(rsaCryptoData->getPublicKey());
                        CryptoPP::AutoSeededRandomPool rng;
                        
                        CryptoPP::StringSource source(signature, true,
                                new CryptoPP::SignatureVerificationFilter(verifier,
                                    new CryptoPP::StringSink(plaintext),
                                    CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
                                    | CryptoPP::SignatureVerificationFilter::PUT_MESSAGE));
                    }
            };
    };
    
    typedef boost::shared_ptr<AsymmetricCryptoHandler> AsymmetricCryptoHandlerPtr;
}

#endif	/* CRYPTO_HANDLERS_H */

