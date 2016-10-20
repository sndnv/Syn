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

#ifndef COMPRESSIONHANDLER_H
#define	COMPRESSIONHANDLER_H

#include "../../Common/Types.h"

using Common_Types::ByteData;

namespace Utilities
{
    namespace Compression
    {
        /**
         * Class for managing data compressions and decompression.
         */
        class CompressionHandler
        {
            public:
                /** Length of uncompressed data size, when converted to bytes. */
                static const std::size_t UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH = sizeof(std::size_t);
                
                /**
                 * Creates a new compression handler with the specified settings.
                 * 
                 * @param defaultAcceleration the default compression acceleration
                 * @param maxDataSize the maximum allowed data size (per compression operation)
                 */
                CompressionHandler(int acceleration, std::size_t maxDataSize)
                : defaultAccelerationLevel(acceleration), maxUncompressedDataSize(maxDataSize)
                {}
                
                ~CompressionHandler() {}
                
                CompressionHandler() = delete;
                CompressionHandler(const CompressionHandler& orig) = delete;
                CompressionHandler& operator=(const CompressionHandler& orig) = delete;
                
                /**
                 * Compresses the supplied input data and stores the result in
                 * the specified variable.
                 * 
                 * Note: Uses the default acceleration level set in the object.
                 * 
                 * @param inputData reference to the data to be compressed
                 * @param compressedData reference to the variable in which to store compressed data
                 */
                void compressData(const ByteData & inputData, ByteData & compressedData)
                {
                    compressData(inputData, defaultAccelerationLevel, compressedData);
                }
                
                /**
                 * Compresses the supplied input data and stores the result in
                 * the specified variable, with the specified acceleration level.
                 * 
                 * Note: The default acceleration level for the LZ4 library is 1;
                 * any higher value will increase the speed of the compression
                 * but it will decrease the compression ratio.
                 * 
                 * @param inputData reference to the data to be compressed
                 * @param acceleration acceleration level for the compression
                 * @param compressedData reference to the variable in which to store compressed data
                 * @throw invalid_argument if too much data is supplied
                 * @throw runtime_error if compression fails
                 */
                void compressData(const ByteData & inputData, const int acceleration, ByteData & compressedData);
                
                /**
                 * Decompresses the supplied data and stores the result in
                 * the specified variable.
                 * 
                 * @param compressedData reference to the data to be decompressed
                 * @param decompressedData reference to the variable in which to store uncompressed data
                 * @throw invalid_argument if the uncompressed data size could not be retrieved
                 * @throw runtime_error if the decompression fails
                 */
                void decompressData(const ByteData & compressedData, ByteData & decompressedData);
                
            private:
                int defaultAccelerationLevel;        //default compression acceleration level
                std::size_t maxUncompressedDataSize; //maximum processable uncompressed data size
        };
    }
}

#endif	/* COMPRESSIONHANDLER_H */

