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
#include "../../../main/SecurityManagement/Crypto/Containers.h"
#include "../../../main/SecurityManagement/Crypto/KeyGenerator.h"

SCENARIO("Containers can be created and their data managed/retrieved",
         "[Containers][Crypto][SecurityManagement][Utilities]")
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
    
    GIVEN("A new SymmetricCryptoDataContainer")
    {
        auto testContainer = testGenerator.getSymmetricCryptoDataFromPassphrase("test");
        
        CHECK(testContainer->getKey().size() == testGenerator.getDerivedKeySize());
        
        THEN("its IV can be updated/retrieved successfully")
        {
            auto newIV = testGenerator.getIV(0);
            testContainer->updateIV(newIV);
            CHECK(testContainer->getIV().operator ==(newIV));
        }
        
        THEN("its data can be retrieved successfully")
        {
            CHECK_NOTHROW(testContainer->getEncryptor());
            CHECK_NOTHROW(testContainer->getDecryptor());
        }
    }
    
    GIVEN("A new RSACryptoDataContainer")
    {
        auto testContainer = testGenerator.getRSACryptoData(0);
        
        THEN("its data can be retrieved successfully")
        {
            CHECK(testContainer->isPrivateKeySet());
            CHECK(testContainer->isPublicKeySet());
            CHECK_NOTHROW(testContainer->getPrivateKey());
            CHECK_NOTHROW(testContainer->getPublicKey());
        }
        
        AND_WHEN("new containers are built with the original's data")
        {
            std::string publicKey, privateKey;
            testContainer->getPublicKeyForStorage(publicKey);
            testContainer->getPrivateKeyForStorage(privateKey);
            auto testContainerFromPublic = SecurityManagement_Crypto::RSACryptoDataContainer::getContainerPtrFromPublicKey(publicKey, 3);
            auto testContainerFromPrivate = SecurityManagement_Crypto::RSACryptoDataContainer::getContainerPtrFromPrivateKey(privateKey, 3);
            
            THEN("their data can be retrieved successfully")
            {
                CHECK_FALSE(testContainerFromPublic->isPrivateKeySet());
                CHECK(testContainerFromPublic->isPublicKeySet());
                CHECK_THROWS_AS(testContainerFromPublic->getPrivateKey(), std::runtime_error);
                CHECK_NOTHROW(testContainerFromPublic->getPublicKey());
                
                CHECK(testContainerFromPrivate->isPrivateKeySet());
                CHECK(testContainerFromPrivate->isPublicKeySet());
                CHECK_NOTHROW(testContainerFromPrivate->getPrivateKey());
                CHECK_NOTHROW(testContainerFromPrivate->getPublicKey());
            }
        }
    }
    
    GIVEN("A new ECDHCryptoDataContainer")
    {
        auto testContainer = testGenerator.getECDHCryptoData();
        
        THEN("its data can be retrieved successfully")
        {
            CHECK(testContainer->isPrivateKeySet());
            CHECK(testContainer->isPublicKeySet());
            CHECK_NOTHROW(testContainer->getPrivateKey());
            CHECK_NOTHROW(testContainer->getPublicKey());
        }
        
        AND_WHEN("new containers are built with the original's data")
        {
            std::string publicKey, privateKey;
            testContainer->getPublicKeyForStorage(publicKey);
            testContainer->getPrivateKeyForStorage(privateKey);
            auto testContainerFromPublic = SecurityManagement_Crypto::ECDHCryptoDataContainer::getContainerPtrFromPublicKey(publicKey);
            auto testContainerFromPair = SecurityManagement_Crypto::ECDHCryptoDataContainer::getContainerPtrFromKeyPair(privateKey, publicKey);
            
            THEN("their data can be retrieved successfully")
            {
                CHECK_FALSE(testContainerFromPublic->isPrivateKeySet());
                CHECK(testContainerFromPublic->isPublicKeySet());
                CHECK_THROWS_AS(testContainerFromPublic->getPrivateKey(), std::runtime_error);
                CHECK_NOTHROW(testContainerFromPublic->getPublicKey());
                
                CHECK(testContainerFromPair->isPrivateKeySet());
                CHECK(testContainerFromPair->isPublicKeySet());
                CHECK_NOTHROW(testContainerFromPair->getPrivateKey());
                CHECK_NOTHROW(testContainerFromPair->getPublicKey());
            }
        }
    }
}
