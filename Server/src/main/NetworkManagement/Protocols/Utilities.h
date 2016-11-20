/* 
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

#ifndef PROTOCOL_UTILITIES_H
#define	PROTOCOL_UTILITIES_H

#include "../../SecurityManagement/Types/Types.h"
#include "../../../compiled/protobuf/BaseComm.pb.h"

using NetworkManagement_Protocols::ConnectionSetupRequestSignature;
using SecurityManagement_Types::PlaintextData;

namespace NetworkManagement_Protocols
{
    namespace Utilities
    {
        /**
         * Verifies the supplied request signature.
         * 
         * @param decrypted the raw decrypted signature data to be verified
         * @throw runtime_error if the signature could not be verified
         */
        void verifyRequestSignature(const PlaintextData & decrypted);

        /**
         * Verifies the supplied request signature and compares it to an already stored reference signature.
         * 
         * @param decrypted the raw decrypted signature data to be verified
         * @param stored stored signature data to be used for comparison
         * @throw runtime_error if the signature could not be verified or if the two supplied signatures do not match
         */
        void verifyRequestSignature(const PlaintextData & decrypted, const PlaintextData & stored);
    }
}

#endif	/* PROTOCOL_UTILITIES_H */

