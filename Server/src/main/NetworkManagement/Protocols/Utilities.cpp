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

#include "Utilities.h"

void NetworkManagement_Protocols::Utilities::verifyRequestSignature
(const PlaintextData & decrypted)
{
    ConnectionSetupRequestSignature signature;
    signature.ParseFromString(decrypted);

    //validates the signature
    if(!signature.IsInitialized()
       || signature.signature_size() != signature.signature_data().size())
    {
        throw std::runtime_error("Utilities::verifyRequestSignature(PlaintextData) >"
                " Failed to verify request signature (invalid signature).");
    }
}

void NetworkManagement_Protocols::Utilities::verifyRequestSignature
(const PlaintextData & decrypted, const PlaintextData & stored)
{
    if(stored.compare(decrypted) != 0)
    {
        throw std::runtime_error("Utilities::verifyRequestSignature(PlaintextData, PlaintextData) >"
                " Failed to verify request signature (data mismatch).");
    }

    verifyRequestSignature(decrypted);
}