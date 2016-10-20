/* 
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

#include "Handlers.h"
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include "KeyGenerator.h"

void SecurityManagement_Crypto::SymmetricCryptoHandler::encryptData(const PlaintextData & plaintext, CiphertextData & ciphertext)
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

void SecurityManagement_Crypto::SymmetricCryptoHandler::decryptData(const CiphertextData & ciphertext, PlaintextData & plaintext)
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

void SecurityManagement_Crypto::AsymmetricCryptoHandler::EncryptDataVisitor::operator()
(const PlaintextData & plaintext, CiphertextData & ciphertext, RSACryptoDataContainerPtr rsaCryptoData) const
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

void SecurityManagement_Crypto::AsymmetricCryptoHandler::DecryptDataVisitor::operator()
(const CiphertextData & ciphertext, PlaintextData & plaintext, RSACryptoDataContainerPtr rsaCryptoData) const
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

void SecurityManagement_Crypto::AsymmetricCryptoHandler::SignDataVisitor::operator()
(const PlaintextData & plaintext, SignedData & signature, RSACryptoDataContainerPtr rsaCryptoData) const
{
    RSASigner signer(rsaCryptoData->getPrivateKey());
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::StringSource source(plaintext, true,
            new CryptoPP::SignerFilter(rng, signer,
                new CryptoPP::StringSink(signature),
                true));
}

void SecurityManagement_Crypto::AsymmetricCryptoHandler::VerifyAndRecoverDataVisitor::operator()
(const SignedData & signature, PlaintextData & plaintext, RSACryptoDataContainerPtr rsaCryptoData) const
{
    RSAVerifier verifier(rsaCryptoData->getPublicKey());
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::StringSource source(signature, true,
            new CryptoPP::SignatureVerificationFilter(verifier,
                new CryptoPP::StringSink(plaintext),
                CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
                | CryptoPP::SignatureVerificationFilter::PUT_MESSAGE));
}
