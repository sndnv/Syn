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

#include "../../BasicSpec.h"
#include "../../../main/Utilities/Strings/Common.h"
#include "../../../main/SecurityManagement/Crypto/Handlers.h"
#include "../../../main/SecurityManagement/Crypto/KeyGenerator.h"
#include "../../../main/SecurityManagement/Crypto/PasswordGenerator.h"

SCENARIO("Crypto handlers can encrypt and decrypt data",
         "[Handlers][Crypto][SecurityManagement][Utilities]")
{
    SecurityManagement_Crypto::KeyGenerator::DerivedKeysParameters derivedKeyParams
    {
        PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
        10000,  //derivedKeyIterations
        32,     //derivedKeySize
        16,     //derivedKeyMinSaltSize
        16,     //derivedKeyDefaultSaltSize

    };

    SecurityManagement_Crypto::KeyGenerator::SymmetricKeysParameters symmetricKeyParams
    {
        SymmetricCipherType::AES,                   //defaultSymmetricCipher
        AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
        12, //defaultIVSize
        32, //minSymmetricKeySize
        32  //defaultSymmetricKeySize
    };

    SecurityManagement_Crypto::KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams
    {
        1024,                                   //minRSAKeySize
        2048,                                   //defaultRSAKeySize
        EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
        AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
    };
    
    SecurityManagement_Crypto::KeyGenerator testGenerator(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams);
    
    GIVEN("A SymmetricCryptoHandler and crypto data generated for it")
    {
        auto cryptoData_1 = testGenerator.getSymmetricCryptoData();
        auto cryptoData_2 = testGenerator.getSymmetricCryptoData(cryptoData_1->getKey(), cryptoData_1->getIV());
        SecurityManagement_Crypto::SymmetricCryptoHandler testEncryptor(cryptoData_1);
        SecurityManagement_Crypto::SymmetricCryptoHandler testDecryptor(cryptoData_2);
        
        THEN("it can encrypt and decrypt messages")
        {
            unsigned int testMessagesCount = 1000;
            
            for(unsigned int i = 1; i < testMessagesCount; i++)
            {
                INFO("Iteration #[" + Utilities::Strings::toString(i) + "].");
                
                std::string encryptedData, decryptedData;
                std::string randomData = SecurityManagement_Crypto::PasswordGenerator::getRandomASCIIPassword(i);
                CHECK(randomData.size() == i);
                
                testEncryptor.encryptData(randomData, encryptedData);
                CHECK_FALSE(encryptedData.empty());
                
                testDecryptor.decryptData(encryptedData, decryptedData);
                CHECK_FALSE(decryptedData.empty());
                CHECK(decryptedData == randomData);
            }
        }
    }
    
    GIVEN("An AsymmetricCryptoHandler and crypto data generated for it")
    {
        auto cryptoData_1 = testGenerator.getRSACryptoData(0);
        std::string publicKey;
        cryptoData_1->getPublicKeyForStorage(publicKey);
        auto cryptoData_2 = SecurityManagement_Crypto::RSACryptoDataContainerPtr(SecurityManagement_Crypto::RSACryptoDataContainer::getContainerPtrFromPublicKey(publicKey, 3));
        
        SecurityManagement_Crypto::AsymmetricCryptoHandler testHandler_1(cryptoData_1);
        SecurityManagement_Crypto::AsymmetricCryptoHandler testHandler_2(cryptoData_2);
        
        THEN("it can encrypt/decrypt and sign/verify messages")
        {
            unsigned int testMessagesCount = 1000;
            
            for(unsigned int i = 1; i < testMessagesCount; i++)
            {
                INFO("Iteration #[" + Utilities::Strings::toString(i) + "].");
                
                std::string encryptedData, decryptedData, signedData, verifiedData;
                std::string randomData = SecurityManagement_Crypto::PasswordGenerator::getRandomASCIIPassword(i);
                CHECK(randomData.size() == i);
                
                testHandler_1.signDataWithPrivateKey(randomData, signedData);
                CHECK_FALSE(signedData.empty());
                
                testHandler_2.verifyAndRecoverDataWithPublicKey(signedData, verifiedData);
                CHECK_FALSE(verifiedData.empty());
                CHECK(verifiedData == randomData);
                
                testHandler_2.encryptDataWithPublicKey(randomData, encryptedData);
                CHECK_FALSE(encryptedData.empty());
                
                testHandler_1.decryptDataWithPrivateKey(encryptedData, decryptedData);
                CHECK_FALSE(decryptedData.empty());
                CHECK(decryptedData == randomData);
            }
        }
    }
}