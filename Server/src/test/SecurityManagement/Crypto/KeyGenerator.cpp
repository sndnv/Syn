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

#include <vector>
#include "../../BasicSpec.h"
#include "../../../main/Utilities/Strings/Common.h"
#include "../../../main/Utilities/Strings/Security.h"
#include "../../../main/SecurityManagement/Crypto/KeyGenerator.h"

using SecurityManagement_Crypto::KeyGenerator;
using SecurityManagement_Types::AuthenticatedSymmetricCipherModeType;
using SecurityManagement_Types::SymmetricCipherType;
using SecurityManagement_Types::PasswordDerivationFunction;
using SecurityManagement_Types::EllipticCurveType;
using SecurityManagement_Types::AsymmetricKeyValidationLevel;

SCENARIO("A key generator is created and uses valid expected/supplied parameters or fails to initialise with invalid parameters",
         "[KeyGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("valid KeyGenerator parameters")
    {
        KeyGenerator::DerivedKeysParameters derivedKeyParams
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        WHEN("a KeyGenerator is created")
        {
            KeyGenerator testGenerator(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams);
            
            THEN("the requested parameters are used by the generator")
            {
                CHECK(testGenerator.getDerivedKeyDefaultIterationsCount() == derivedKeyParams.derivedKeyIterations);
                CHECK(testGenerator.getDerivedKeySize() == derivedKeyParams.derivedKeySize);
                CHECK(testGenerator.getDerivedKeyMinSaltSize() == derivedKeyParams.derivedKeyMinSaltSize);
                CHECK(testGenerator.getDerivedKeyDefaultSaltSize() == derivedKeyParams.derivedKeyDefaultSaltSize);
                
                CHECK(testGenerator.getDefaultSymmetricCipher() == symmetricKeyParams.defaultSymmetricCipher);
                CHECK(testGenerator.getDefaultSymmetricCipherMode() == symmetricKeyParams.defaultSymmetricCipherMode);
                CHECK(testGenerator.getDefaultIVSize() == symmetricKeyParams.defaultIVSize);
                CHECK(testGenerator.getMinSymmetricKeySize() == symmetricKeyParams.minSymmetricKeySize);
                CHECK(testGenerator.getDefaultSymmetricKeySize() == symmetricKeyParams.defaultSymmetricKeySize);
                
                CHECK(testGenerator.getMinRSAKeySize() == asymmetricKeyParams.minRSAKeySize);
                CHECK(testGenerator.getDefaultRSAKeySize() == asymmetricKeyParams.defaultRSAKeySize);
                CHECK(testGenerator.getDefaultEllipticCurve() == asymmetricKeyParams.defaultEllipticCurve);
                CHECK(testGenerator.getDefaultKeyValidationLevel() == 3);
            }
        }
    }
    
    GIVEN("a set of invalid KeyGenerator parameters")
    {
        KeyGenerator::DerivedKeysParameters derivedKeyParams_Valid
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_Invalid_1
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            32,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_Invalid_2
        {
            PasswordDerivationFunction::PBKDF2_SHA3_256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_Invalid_3
        {
            PasswordDerivationFunction::PBKDF2_SHA3_512,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_Invalid_4
        {
            PasswordDerivationFunction::INVALID,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize
                    
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_Valid
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_Invalid_1
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            16  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_Invalid_2
        {
            SymmetricCipherType::INVALID,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,      //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_Invalid_3
        {
            SymmetricCipherType::AES,                       //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::INVALID,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_Valid
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_Invalid_1
        {
            2048,                                   //minRSAKeySize
            1024,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_Invalid_2
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::INVALID,             //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_Invalid_3
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::INVALID   //keyValidationLevel
        };
        
        WHEN("'KeyGenerator's are attempted to be created")
        {
            THEN("they throw exceptions")
            {
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Invalid_1, symmetricKeyParams_Valid, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Invalid_2, symmetricKeyParams_Valid, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Invalid_3, symmetricKeyParams_Valid, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Invalid_4, symmetricKeyParams_Valid, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Invalid_1, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Invalid_2, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Invalid_3, asymmetricKeyParams_Valid),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Valid, asymmetricKeyParams_Invalid_1),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Valid, asymmetricKeyParams_Invalid_2),
                        std::invalid_argument);
                
                CHECK_THROWS_AS(
                        KeyGenerator testGenerator(derivedKeyParams_Valid, symmetricKeyParams_Valid, asymmetricKeyParams_Invalid_3),
                        std::invalid_argument);
            }
        }
    }
}

SCENARIO("A key generator can generate derived key data for all available functions",
         "[KeyGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("a set of valid KeyGenerator parameters")
    {
        KeyGenerator::DerivedKeysParameters derivedKeyParams_1
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize     
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_2
        {
            PasswordDerivationFunction::PBKDF2_SHA512,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize     
        };
        
        KeyGenerator::DerivedKeysParameters derivedKeyParams_3
        {
            PasswordDerivationFunction::PBKDF2_SHA512,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            32,     //derivedKeyDefaultSaltSize     
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        WHEN("'KeyGenerator's are created with them")
        {
            KeyGenerator testGenerator_1(derivedKeyParams_1, symmetricKeyParams, asymmetricKeyParams);
            KeyGenerator testGenerator_2(derivedKeyParams_2, symmetricKeyParams, asymmetricKeyParams);
            KeyGenerator testGenerator_3(derivedKeyParams_3, symmetricKeyParams, asymmetricKeyParams);
            
            THEN("they generate keys successfully")
            {
                //crypto data for comparisons
                auto originalData_1 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_1");
                auto originalData_2 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_2");
                auto originalData_3 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_3");
                auto originalData_4 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_4");
                auto originalData_5 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_5");
                auto originalData_6 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_6");
                auto originalData_7 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_7");
                auto originalData_8 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_8");
                auto originalData_9 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_9");
                
                //crypto data built based on the original data
                auto rebuiltData_1 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_1", originalData_1->getSalt(), originalData_1->getIV());
                auto rebuiltData_2 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_2", originalData_2->getSalt(), originalData_2->getIV());
                auto rebuiltData_3 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_3", originalData_3->getSalt(), originalData_3->getIV());
                auto rebuiltData_4 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_4", originalData_4->getSalt(), originalData_4->getIV());
                auto rebuiltData_5 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_5", originalData_5->getSalt(), originalData_5->getIV());
                auto rebuiltData_6 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_6", originalData_6->getSalt(), originalData_6->getIV());
                auto rebuiltData_7 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_7", originalData_7->getSalt(), originalData_7->getIV());
                auto rebuiltData_8 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_8", originalData_8->getSalt(), originalData_8->getIV());
                auto rebuiltData_9 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_9", originalData_9->getSalt(), originalData_9->getIV());
                
                //crypto data with same passwords but not salt/iv
                auto otherData_1 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_1");
                auto otherData_2 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_2");
                auto otherData_3 = testGenerator_1.getSymmetricCryptoDataFromPassphrase("test_password_3");
                auto otherData_4 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_4");
                auto otherData_5 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_5");
                auto otherData_6 = testGenerator_2.getSymmetricCryptoDataFromPassphrase("test_password_6");
                auto otherData_7 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_7");
                auto otherData_8 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_8");
                auto otherData_9 = testGenerator_3.getSymmetricCryptoDataFromPassphrase("test_password_9");
                
                //checks that the same password & salt/iv generated the same crypto data
                CHECK(originalData_1->getKey().operator ==(rebuiltData_1->getKey()));
                CHECK(originalData_2->getKey().operator ==(rebuiltData_2->getKey()));
                CHECK(originalData_3->getKey().operator ==(rebuiltData_3->getKey()));
                CHECK(originalData_4->getKey().operator ==(rebuiltData_4->getKey()));
                CHECK(originalData_5->getKey().operator ==(rebuiltData_5->getKey()));
                CHECK(originalData_6->getKey().operator ==(rebuiltData_6->getKey()));
                CHECK(originalData_7->getKey().operator ==(rebuiltData_7->getKey()));
                CHECK(originalData_8->getKey().operator ==(rebuiltData_8->getKey()));
                CHECK(originalData_9->getKey().operator ==(rebuiltData_9->getKey()));
                
                CHECK(originalData_1->getIV().operator ==(rebuiltData_1->getIV()));
                CHECK(originalData_2->getIV().operator ==(rebuiltData_2->getIV()));
                CHECK(originalData_3->getIV().operator ==(rebuiltData_3->getIV()));
                CHECK(originalData_4->getIV().operator ==(rebuiltData_4->getIV()));
                CHECK(originalData_5->getIV().operator ==(rebuiltData_5->getIV()));
                CHECK(originalData_6->getIV().operator ==(rebuiltData_6->getIV()));
                CHECK(originalData_7->getIV().operator ==(rebuiltData_7->getIV()));
                CHECK(originalData_8->getIV().operator ==(rebuiltData_8->getIV()));
                CHECK(originalData_9->getIV().operator ==(rebuiltData_9->getIV()));
                
                CHECK(originalData_1->getSalt().operator ==(rebuiltData_1->getSalt()));
                CHECK(originalData_2->getSalt().operator ==(rebuiltData_2->getSalt()));
                CHECK(originalData_3->getSalt().operator ==(rebuiltData_3->getSalt()));
                CHECK(originalData_4->getSalt().operator ==(rebuiltData_4->getSalt()));
                CHECK(originalData_5->getSalt().operator ==(rebuiltData_5->getSalt()));
                CHECK(originalData_6->getSalt().operator ==(rebuiltData_6->getSalt()));
                CHECK(originalData_7->getSalt().operator ==(rebuiltData_7->getSalt()));
                CHECK(originalData_8->getSalt().operator ==(rebuiltData_8->getSalt()));
                CHECK(originalData_9->getSalt().operator ==(rebuiltData_9->getSalt()));
                
                //checks that the same password w/ different salt/iv generates different crypto data
                CHECK_FALSE(originalData_1->getKey().operator ==(otherData_1->getKey()));
                CHECK_FALSE(originalData_2->getKey().operator ==(otherData_2->getKey()));
                CHECK_FALSE(originalData_3->getKey().operator ==(otherData_3->getKey()));
                CHECK_FALSE(originalData_4->getKey().operator ==(otherData_4->getKey()));
                CHECK_FALSE(originalData_5->getKey().operator ==(otherData_5->getKey()));
                CHECK_FALSE(originalData_6->getKey().operator ==(otherData_6->getKey()));
                CHECK_FALSE(originalData_7->getKey().operator ==(otherData_7->getKey()));
                CHECK_FALSE(originalData_8->getKey().operator ==(otherData_8->getKey()));
                CHECK_FALSE(originalData_9->getKey().operator ==(otherData_9->getKey()));
                
                CHECK_FALSE(originalData_1->getIV().operator ==(otherData_1->getIV()));
                CHECK_FALSE(originalData_2->getIV().operator ==(otherData_2->getIV()));
                CHECK_FALSE(originalData_3->getIV().operator ==(otherData_3->getIV()));
                CHECK_FALSE(originalData_4->getIV().operator ==(otherData_4->getIV()));
                CHECK_FALSE(originalData_5->getIV().operator ==(otherData_5->getIV()));
                CHECK_FALSE(originalData_6->getIV().operator ==(otherData_6->getIV()));
                CHECK_FALSE(originalData_7->getIV().operator ==(otherData_7->getIV()));
                CHECK_FALSE(originalData_8->getIV().operator ==(otherData_8->getIV()));
                CHECK_FALSE(originalData_9->getIV().operator ==(otherData_9->getIV()));
                
                CHECK_FALSE(originalData_1->getSalt().operator ==(otherData_1->getSalt()));
                CHECK_FALSE(originalData_2->getSalt().operator ==(otherData_2->getSalt()));
                CHECK_FALSE(originalData_3->getSalt().operator ==(otherData_3->getSalt()));
                CHECK_FALSE(originalData_4->getSalt().operator ==(otherData_4->getSalt()));
                CHECK_FALSE(originalData_5->getSalt().operator ==(otherData_5->getSalt()));
                CHECK_FALSE(originalData_6->getSalt().operator ==(otherData_6->getSalt()));
                CHECK_FALSE(originalData_7->getSalt().operator ==(otherData_7->getSalt()));
                CHECK_FALSE(originalData_8->getSalt().operator ==(otherData_8->getSalt()));
                CHECK_FALSE(originalData_9->getSalt().operator ==(otherData_9->getSalt()));
            }
        }
    }
}

SCENARIO("A key generator can generate symmetric key data for all available ciphers & modes",
         "[KeyGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("a set of valid KeyGenerator parameters")
    {
        KeyGenerator::DerivedKeysParameters derivedKeyParams
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize     
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_1
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_2
        {
            SymmetricCipherType::SERPENT,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_3
        {
            SymmetricCipherType::TWOFISH,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_4
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::CCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_5
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::GCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_6
        {
            SymmetricCipherType::SERPENT,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::CCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_7
        {
            SymmetricCipherType::SERPENT,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::GCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_8
        {
            SymmetricCipherType::TWOFISH,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::CCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams_9
        {
            SymmetricCipherType::TWOFISH,               //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::GCM,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            32, //minSymmetricKeySize
            32  //defaultSymmetricKeySize
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        WHEN("'KeyGenerator's are created with them")
        {
            KeyGenerator testGenerator_1(derivedKeyParams, symmetricKeyParams_1, asymmetricKeyParams);
            KeyGenerator testGenerator_2(derivedKeyParams, symmetricKeyParams_2, asymmetricKeyParams);
            KeyGenerator testGenerator_3(derivedKeyParams, symmetricKeyParams_3, asymmetricKeyParams);
            KeyGenerator testGenerator_4(derivedKeyParams, symmetricKeyParams_4, asymmetricKeyParams);
            KeyGenerator testGenerator_5(derivedKeyParams, symmetricKeyParams_5, asymmetricKeyParams);
            KeyGenerator testGenerator_6(derivedKeyParams, symmetricKeyParams_6, asymmetricKeyParams);
            KeyGenerator testGenerator_7(derivedKeyParams, symmetricKeyParams_7, asymmetricKeyParams);
            KeyGenerator testGenerator_8(derivedKeyParams, symmetricKeyParams_8, asymmetricKeyParams);
            KeyGenerator testGenerator_9(derivedKeyParams, symmetricKeyParams_9, asymmetricKeyParams);
            
            std::vector<KeyGenerator *> testGenerators{
                    &testGenerator_1, &testGenerator_2, &testGenerator_3,
                    &testGenerator_4, &testGenerator_5, &testGenerator_6,
                    &testGenerator_7, &testGenerator_8, &testGenerator_9};
            
            CHECK(testGenerators.size() == 9);
            
            THEN("they generate keys successfully")
            {
                for(KeyGenerator * currentGenerator : testGenerators)
                {
                    INFO("Checking key generator with cipher ["
                            + Utilities::Strings::toString(currentGenerator->getDefaultSymmetricCipher())
                            + "], mode ["
                            + Utilities::Strings::toString(currentGenerator->getDefaultSymmetricCipherMode())
                            + "] and default key size [" + Utilities::Strings::toString(currentGenerator->getDefaultSymmetricKeySize()) + "] ...");
                    
                    auto originalData = currentGenerator->getSymmetricCryptoData();
                    auto rebuiltData = currentGenerator->getSymmetricCryptoData(originalData->getKey(), originalData->getIV());
                    
                    CHECK(originalData->getKey().operator ==(rebuiltData->getKey()));
                    CHECK(originalData->getSalt().operator ==(rebuiltData->getSalt()));
                    CHECK(originalData->getSalt().operator ==(SaltData())); //no salt data should be generated
                    CHECK(originalData->getIV().operator ==(rebuiltData->getIV()));
                }
            }
        }
    }
}

SCENARIO("A key generator can generate asymmetric key data for all available elliptic curves and various RSA key sizes",
         "[KeyGenerator][Crypto][SecurityManagement][Utilities]")
{
    GIVEN("a set of valid KeyGenerator parameters")
    {
        KeyGenerator::DerivedKeysParameters derivedKeyParams
        {
            PasswordDerivationFunction::PBKDF2_SHA256,  //derivedKeyFunction
            10000,  //derivedKeyIterations
            32,     //derivedKeySize
            16,     //derivedKeyMinSaltSize
            16,     //derivedKeyDefaultSaltSize     
        };
        
        KeyGenerator::SymmetricKeysParameters symmetricKeyParams
        {
            SymmetricCipherType::AES,                   //defaultSymmetricCipher
            AuthenticatedSymmetricCipherModeType::EAX,  //defaultSymmetricCipherMode
            12, //defaultIVSize
            20, //minSymmetricKeySize
            20  //defaultSymmetricKeySize
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_1
        {
            512,                                    //minRSAKeySize
            1024,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_2
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::HIGH_2    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_3
        {
            2048,                                   //minRSAKeySize
            4096,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::LOW_1     //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_4
        {
            4096,                                   //minRSAKeySize
            4096,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P384R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::BASIC_0   //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_5
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P160R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_6
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P192R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_7
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P224R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_8
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P256R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_9
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P320R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_10
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::BP_P512R1,           //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_11
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::P192R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_12
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::P224R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_13
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::P256R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_14
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::P384R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_15
        {
            1024,                                   //minRSAKeySize
            2048,                                   //defaultRSAKeySize
            EllipticCurveType::P521R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        KeyGenerator::AsymmetricKeysParameters asymmetricKeyParams_16
        {
            4096,                                   //minRSAKeySize
            8192,                                   //defaultRSAKeySize
            EllipticCurveType::P521R1,              //defaultEllipticCurve
            AsymmetricKeyValidationLevel::FULL_3    //keyValidationLevel
        };
        
        WHEN("'KeyGenerator's are created with them")
        {
            KeyGenerator testGenerator_1(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_1);
            KeyGenerator testGenerator_2(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_2);
            KeyGenerator testGenerator_3(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_3);
            KeyGenerator testGenerator_4(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_4);
            KeyGenerator testGenerator_5(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_5);
            KeyGenerator testGenerator_6(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_6);
            KeyGenerator testGenerator_7(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_7);
            KeyGenerator testGenerator_8(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_8);
            KeyGenerator testGenerator_9(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_9);
            KeyGenerator testGenerator_10(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_10);
            KeyGenerator testGenerator_11(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_11);
            KeyGenerator testGenerator_12(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_12);
            KeyGenerator testGenerator_13(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_13);
            KeyGenerator testGenerator_14(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_14);
            KeyGenerator testGenerator_15(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_15);
            KeyGenerator testGenerator_16(derivedKeyParams, symmetricKeyParams, asymmetricKeyParams_16);
            
            std::vector<KeyGenerator *> testGenerators{
                    &testGenerator_1, &testGenerator_2, &testGenerator_3,
                    &testGenerator_4, &testGenerator_5, &testGenerator_6,
                    &testGenerator_7, &testGenerator_8, &testGenerator_9,
                    &testGenerator_10, &testGenerator_11, &testGenerator_12,
                    &testGenerator_13, &testGenerator_14, &testGenerator_15,
                    &testGenerator_16};
            
            CHECK(testGenerators.size() == 16);
            
            THEN("they generate keys successfully")
            {
                for(KeyGenerator * currentGenerator : testGenerators)
                {
                    INFO("Checking key generator with elliptic curve ["
                            + Utilities::Strings::toString(currentGenerator->getDefaultEllipticCurve())
                            + "] and RSA key size [" + Utilities::Strings::toString(currentGenerator->getDefaultRSAKeySize()) + "] ...");
                    
                    auto ecData_1 = currentGenerator->getECDHCryptoData();
                    auto ecData_2 = currentGenerator->getECDHCryptoData();
                    auto rsaData = currentGenerator->getRSACryptoData(0);
                    
                    auto dhKey_1 = currentGenerator->getDiffieHellmanKeyEncryptionKey(ecData_1->getPrivateKey(), ecData_2->getPublicKey());
                    auto dhKey_2 = currentGenerator->getDiffieHellmanKeyEncryptionKey(ecData_2->getPrivateKey(), ecData_1->getPublicKey());
                    
                    CHECK(ecData_1->isPublicKeySet());
                    CHECK(ecData_1->isPrivateKeySet());
                    CHECK(ecData_2->isPublicKeySet());
                    CHECK(ecData_2->isPrivateKeySet());
                    CHECK(rsaData->isPublicKeySet());
                    CHECK(rsaData->isPrivateKeySet());
                    CHECK(dhKey_1.operator ==(dhKey_2));
                }
            }
        }
    }
}