/* 
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

#ifndef POOLSTREAMS_H
#define	POOLSTREAMS_H

#include <boost/iostreams/concepts.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

#include "../../../Common/Types.h"
#include "../../Types/Types.h"

using Common_Types::Byte;
using Common_Types::ByteVector;
using Common_Types::ByteVectorPtr;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::DataSize;

namespace StorageManagement_Pools
{
    /**
     * Base input stream class for data pools.
     */
    class PoolInputStream
    {
        public:
            typedef Byte char_type;
            typedef boost::iostreams::source_tag category;
            
            virtual ~PoolInputStream() {}
            
            /**
             * Reads the specified amount of data to the specified buffer.
             * 
             * @param s the buffer in which to put the requested data; the size of <code>s</code>
             * must be at least <code>n</code>
             * @param n the amount of data to write in <code>s</code> (in bytes)
             * @return the amount of data read into <code>s</code> or -1, if the underlying
             * read operation fails
             * 
             * @throw implementation-specific exceptions
             */
            virtual std::streamsize read(char_type * s, std::streamsize n) = 0;
            
            /**
             * Attempts to read one byte from the stream and to write it to the supplied buffer.
             * 
             * @param output the buffer to write to
             * @return a reference to the current stream
             */
            PoolInputStream & operator>> (Byte & output)
            {
                read(&output, 1);
                return *this;
            }
            
            /**
             * Attempts to read as many bytes from the stream as can be written to the supplied buffer.
             * 
             * @param output the buffer to write to
             * @return a reference to the current stream
             * 
             * @throw invalid_argument if the size of the output buffer is 0
             */
            PoolInputStream & operator>> (ByteVector & output)
            {
                if(output.size() == 0)
                    throw std::invalid_argument("PoolInputStream::operator>>() > The size of the supplied buffer must be larger than 0.");
                
                read(&output[0], output.size());
                return *this;
            }
            
            /**
             * Attempts to read as many bytes from the stream as can be written to the supplied buffer.
             * 
             * @param output the buffer to write to
             * @return a reference to the current stream
             * 
             * @throw invalid_argument if the size of the output buffer is 0
             */
            PoolInputStream & operator>> (ByteVectorPtr output)
            {
                if(output->size() == 0)
                    throw std::invalid_argument("PoolInputStream::operator>>() > The size of the supplied buffer must be larger than 0.");
                
                read(&(*output)[0], output->size());
                return *this;
            }
            
            /**
             * Retrieves the data ID associated with the stream.
             * 
             * @return the data ID
             */
            virtual StoredDataID getDataID() const = 0;
            
            /**
             * Resets the data ID associated with the stream.
             * 
             * Note: It is up to the implementing stream to decide whether
             * to allow ID reset or how a failure is to be transmitted 
             * (if at all) to the caller.
             * 
             * @param newID the new data ID
             */
            virtual void resetDataID(StoredDataID newID) = 0;
            
            /**
             * Retrieves the maximum number of bytes that can be read from the stream.
             * 
             * @return the number of bytes that can be read
             */
            virtual DataSize getMaxReadableBytes() const = 0;
    };
    
    /**
     * Base output stream class for data pools.
     */
    class PoolOutputStream
    {
        public:
            typedef Byte char_type;
            typedef boost::iostreams::sink_tag category;
            
            virtual ~PoolOutputStream() {}
            
            /**
             * Writes the specified amount of data from the specified buffer.
             * 
             * @param s the buffer from which to read the data; the size of <code>s</code>
             * must be at least <code>n</code>
             * @param n the amount of data to read from <code>s</code> (in bytes)
             * @return the amount of data written
             * 
             * @throw implementation-specific exceptions
             */
            virtual std::streamsize write(const char_type * s, std::streamsize n) = 0;
            
            /**
             * Attempts to copy all available data from the input to the output stream.
             * 
             * @param input the stream to be read from
             * 
             * @throw implementation-specific exceptions
             */
            virtual void streamData(PoolInputStream & input) = 0;
            
            /**
             * Attempts to read one byte from the input and to write it to the stream.
             * 
             * @param input the buffer to read from
             * @return a reference to the current stream
             */
            PoolOutputStream & operator<< (const Byte & input)
            {
                write(&input, 1);
                return *this;
            }
            
            /**
             * Attempts to write as many bytes to the stream as can be read from the supplied buffer.
             * 
             * @param input the buffer to read from
             * @return a reference to the current stream
             * 
             * @throw invalid_argument if the size of the input buffer is 0
             */
            PoolOutputStream & operator<< (const ByteVector & input)
            {
                if(input.size() == 0)
                    throw std::invalid_argument("PoolOutputStream::operator<<() > The number of bytes to write must be larger than 0.");
                
                write(&input[0], input.size());
                return *this;
            }
            
            /**
             * Attempts to write as many bytes to the stream as can be read from the supplied buffer.
             * 
             * @param input the buffer to read from
             * @return a reference to the current stream
             * 
             * @throw invalid_argument if the size of the input buffer is 0
             */
            PoolOutputStream & operator<< (const ByteVectorPtr input)
            {
                if(input->size() == 0)
                    throw std::invalid_argument("PoolOutputStream::operator<<() > The number of bytes to write must be larger than 0.");
                
                write(&(*input)[0], input->size());
                return *this;
            }
            
            /**
             * Flushes the underlying stream.
             * 
             * @return reference to this stream
             */
            virtual PoolOutputStream & flush() = 0;
            
            /**
             * Retrieves the data ID associated with the stream.
             * 
             * @return the data ID
             */
            virtual StoredDataID getDataID() const = 0;
            
            /**
             * Resets the data ID associated with the stream.
             * 
             * Note: It is up to the implementing stream to decide whether
             * to allow ID reset or how a failure is to be transmitted 
             * (if at all) to the caller.
             * 
             * @param newID the new data ID
             */
            virtual void resetDataID(StoredDataID newID) = 0;
            
            /**
             * Retrieves the maximum number of bytes that can be written to the stream.
             * 
             * @return the number of bytes that can be written
             */
            virtual DataSize getMaxWritableBytes() const = 0;
    };
    
    struct PoolStreamDeleter
    {
        void operator()(PoolInputStream * container) { delete container; }
        void operator()(PoolOutputStream * container) { delete container; }
    };
    
    typedef boost::interprocess::unique_ptr<PoolInputStream, PoolStreamDeleter> PoolInputStreamPtr;
    typedef boost::interprocess::unique_ptr<PoolOutputStream, PoolStreamDeleter> PoolOutputStreamPtr;
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>PoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param output the stream to be written to
     * @param input the stream to be read from
     */
    inline PoolOutputStream & operator<< (PoolOutputStream & output, PoolInputStream & input)
    {
        output.streamData(input);
        return output;
    }
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>PoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param input the stream to be read from
     * @param output the stream to be written to
     */
    inline PoolInputStream & operator>> (PoolInputStream & input, PoolOutputStream & output)
    {
        output.streamData(input);
        return input;
    }
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>PoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param output the stream to be written to
     * @param input the stream to be read from
     */
    inline PoolOutputStreamPtr & operator<< (PoolOutputStreamPtr & output, PoolInputStreamPtr & input)
    {
        output->streamData(*input);
        return output;
    }
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>PoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param input the stream to be read from
     * @param output the stream to be written to
     */
    inline PoolInputStreamPtr & operator>> (PoolInputStreamPtr & input, PoolOutputStreamPtr & output)
    {
        output->streamData(*input);
        return input;
    }
}

#endif	/* POOLSTREAMS_H */

