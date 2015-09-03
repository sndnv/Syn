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

#include "CompressionHandler.h"

void Utilities::Compression::CompressionHandler::compressData(const ByteData & inputData, const int acceleration, ByteData & compressedData)
{
    int maxOutputSize = LZ4_compressBound(inputData.size());

    if(maxOutputSize <= 0 || inputData.size() >= maxUncompressedDataSize)
    {
        throw std::invalid_argument("CompressionHandler::compressData() >"
                " Cannot process input data with size [" + Utilities::Strings::toString(inputData.size()) + "].");
    }

    //compresses the supplied data
    char * outputData = new char[maxOutputSize];
    int outputDataSize = LZ4_compress_fast(
            inputData.c_str(),
            outputData,
            inputData.size(),
            maxOutputSize,
            acceleration);

    if(outputDataSize <= 0)
    {
        delete[] outputData;
        throw std::runtime_error("CompressionHandler::compressData() >"
                " Failed to compress data;"
                " compression function returned unexpected result: ["
                + Utilities::Strings::toString(outputDataSize) + "].");
    }

    //stores the compressed data
    compressedData.assign(outputData, outputDataSize);
    delete[] outputData;

    //converts the uncompressed data size to bytes (in network byte order)
    auto nOriginalDataSize = htonl(inputData.size());
    if(sizeof nOriginalDataSize != CompressionHandler::UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH)
    {
        throw std::logic_error("CompressionHandler::compressData() >"
                " The converted original data size does not have the expected byte length.");
    }

    ByteData convertedOriginalDataSize(CompressionHandler::UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH, '\0');
    Byte* rawConvertedOriginalDataSize = static_cast<Byte*>(static_cast<void*>(&nOriginalDataSize));

    for(std::size_t i = 0; i < (sizeof nOriginalDataSize); i++)
        convertedOriginalDataSize[i] = rawConvertedOriginalDataSize[i];

    assert(convertedOriginalDataSize.size() == CompressionHandler::UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH);

    //stores the converted uncompressed data size at the end of the compressed data object
    compressedData.append(convertedOriginalDataSize);
}

void Utilities::Compression::CompressionHandler::decompressData(const ByteData & compressedData, ByteData & decompressedData)
{
    auto compressedDataSize = compressedData.size() - CompressionHandler::UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH;

    ByteData convertedOriginalDataSize = compressedData.substr(compressedDataSize);
    if(CompressionHandler::UNCOMPRESSED_DATA_SIZE_BYTE_LENGTH != convertedOriginalDataSize.size())
    {
        throw std::invalid_argument("CompressionHandler::decompressData() >"
                " Failed to retrieve original data size.");
    }

    //retrieves the original uncompressed data size
    Byte nOriginalDataSize[convertedOriginalDataSize.size()];
    for(std::size_t i = 0; i < convertedOriginalDataSize.size(); i++)
        nOriginalDataSize[i] = convertedOriginalDataSize[i];

    auto originalDataSize = ntohl(*static_cast<std::size_t*>(static_cast<void*>(nOriginalDataSize)));

    if(LZ4_compressBound(originalDataSize) <= 0 || originalDataSize >= maxUncompressedDataSize)
    {
        throw std::invalid_argument("CompressionHandler::decompressData() >"
                " Invalid original data size encountered: ["
                + Utilities::Strings::toString(originalDataSize) + "].");
    }

    //decompresses the supplied data
    char * outputData = new char[originalDataSize];
    int outputDataSize = LZ4_decompress_fast(
            compressedData.substr(0, compressedDataSize).c_str(),
            outputData,
            originalDataSize);

    if(outputDataSize <= 0 || ((unsigned int) outputDataSize) != compressedDataSize)
    {
        delete[] outputData;
        throw std::runtime_error("CompressionHandler::decompressData() >"
                " Failed to decompress data;"
                " decompression function returned unexpected result: ["
                + Utilities::Strings::toString(outputDataSize) + "].");
    }

    //stores the decompressed data
    decompressedData.assign(outputData, outputDataSize);
    delete[] outputData;
}
