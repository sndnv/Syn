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

#ifndef DISKPOOLSTREAMS_H
#define	DISKPOOLSTREAMS_H

#include <boost/filesystem/fstream.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include "PoolStreams.h"

using Common_Types::Byte;
using Common_Types::ByteVector;
using Common_Types::ByteVectorPtr;
using StorageManagement_Types::StoredDataID;
using StorageManagement_Types::DataSize;
using StorageManagement_Types::DiskDataAddress;
using StorageManagement_Pools::PoolInputStream;
using StorageManagement_Pools::PoolOutputStream;

namespace StorageManagement_Pools
{
    class DiskPoolOutputStream;
    
    /**
     * Stream class for reading from a disk data pool.
     * 
     * Note: Streaming between internal streams is supported directly by the class
     * itself. Interoperability with C++ streams can be achieved with <code>boost::iostreams</code>.
     */
    class DiskPoolInputStream : public PoolInputStream
    {
        friend class DiskPoolOutputStream;
        
        public:
            /**
             * Constructs a new input stream object.
             * 
             * Note: The data associated with the stream can be read only once.
             * 
             * @param dataID the ID associated with the data to be read
             * @param maxData the maximum amount of data allowed to be read by this stream
             * @param startOffset the address from which to start reading
             * @param parentPoolReadMutex parent mutex for locking the file stream
             * @param inputFileStream file stream from which to read the data
             * @param readLocksCounter a reference to the read lock counter of the associated data
             */
            DiskPoolInputStream(StoredDataID dataID, DataSize maxData, DiskDataAddress startOffset,
                                boost::mutex & parentPoolReadMutex, boost::filesystem::fstream & inputFileStream,
                                unsigned int & readLocksCounter)
            : id(dataID), remainingData(maxData), start(startOffset), poolReadMutex(parentPoolReadMutex), 
              fileStream(inputFileStream), readLocksCounter(readLocksCounter)
            {}
            
            /**
             * Reads the specified amount of data to the specified buffer.
             * 
             * Note: The data associated with the stream can be fully read only once.
             * Multiple calls to <code>read</code> can be made, as needed, but total
             * amount of data read can never go above the amount set with <code>maxData</code>.
             * 
             * @param s the buffer in which to put the requested data; the size of <code>s</code>
             * must be at least <code>n</code>
             * @param n the amount of data to write in <code>s</code> (in bytes)
             * @return the amount of data read into <code>s</code> or -1, if the underlying
             * read operation fails
             * 
             * @throw invalid_argument if <code>n</code> is not a valid value or if 
             * more data is requested to be read than is available
             */
            std::streamsize read(char_type * s, std::streamsize n)
            {
                if(n <= 0)
                    throw std::invalid_argument("DiskPoolInputStream::read() > The number of bytes to read must be larger than 0.");
                
                boost::lock_guard<boost::mutex> readLock(poolReadMutex);
                if((DataSize)n > remainingData)
                    throw std::invalid_argument("DiskPoolInputStream::read() > Attempted to read more data than is allowed.");
                
                fileStream.seekg(start);
                fileStream.read((char*)s, n);
                
                if(fileStream.fail())
                    return -1;
                
                start += n;
                remainingData -= n;
                
                if(remainingData == 0)
                    --readLocksCounter;
                
                return n;
            }
            
            StoredDataID getDataID() const { return id; }
            void resetDataID(StoredDataID newID) { id = newID; }
            DataSize getMaxReadableBytes() const { return remainingData; }
            
        private:
            StoredDataID id;
            DataSize remainingData;
            DiskDataAddress start;
            boost::mutex & poolReadMutex;
            boost::filesystem::fstream & fileStream;
            unsigned int & readLocksCounter;
    };
    
    /**
     * Stream class for writing to a disk data pool.
     * 
     * Note: Streaming between internal streams is supported directly by the class
     * itself. Interoperability with C++ streams can be achieved with <code>boost::iostreams</code>.
     */
    class DiskPoolOutputStream : public PoolOutputStream
    {
        public:
            /**
             * Constructs a new output stream object.
             * 
             * Note: The data associated with the stream can be written only once.
             * 
             * @param dataID the ID associated with the data to be written
             * @param maxData the maximum amount of data allowed to be written by this stream
             * @param startOffset the address from which to start writing
             * @param parentPoolWriteMutex parent mutex for locking the file stream
             * @param outputFileStream file stream to which to write the data
             * @param writeLocked a reference to the write lock associated with the data
             */
            DiskPoolOutputStream(StoredDataID dataID, DataSize maxData, DiskDataAddress startOffset,
                                 boost::mutex & parentPoolWriteMutex, boost::filesystem::fstream & outputFileStream,
                                 bool & writeLocked)
            : id(dataID), remainingData(maxData), start(startOffset), poolWriteMutex(parentPoolWriteMutex), 
              fileStream(outputFileStream), writeLocked(writeLocked)
            {}
            
            /**
             * Writes the specified amount of data from the specified buffer.
             * 
             * Note: The data associated with the stream can be fully written only once.
             * Multiple calls to <code>write</code> can be made, as needed, but total
             * amount of data written can never go above the amount set with <code>maxData</code>.
             * 
             * @param s the buffer from which to read the data; the size of <code>s</code>
             * must be at least <code>n</code>
             * @param n the amount of data to read from <code>s</code> (in bytes)
             * @return the amount of data written
             * 
             * @throw invalid_argument if <code>n</code> is not a valid value or if 
             * more data is requested to be written than is allowed
             */
            std::streamsize write(const char_type * s, std::streamsize n)
            {
                if(n <= 0)
                    throw std::invalid_argument("DiskPoolOutputStream::write() > The number of bytes to write must be larger than 0.");
                
                boost::lock_guard<boost::mutex> writeLock(poolWriteMutex);
                
                if((DataSize)n > remainingData)
                    throw std::invalid_argument("DiskPoolOutputStream::write() > Attempted to write more data than is allowed.");
                
                fileStream.seekp(start);
                fileStream.write((const char*)s, n);
                
                if(fileStream.fail())
                    return -1;
                
                start += n;
                remainingData -= n;
                
                if(remainingData == 0)
                    writeLocked = false;
                
                return n;
            }
            
            /**
             * Attempts to copy all available data from the input to the output stream.
             * 
             * @param input the stream to be read from
             * 
             * @throw logic_error if an unexpected input stream is encountered
             */
            void streamData(PoolInputStream & input)
            {
                try { streamDataFromDisk(dynamic_cast<DiskPoolInputStream &>(input)); return; } catch(std::bad_cast & ex) {}
                //support for more streaming source is added here
                //try { streamDataFromX(dynamic_cast<source class ref>(input)); return; } catc(const std::bac_cast &) {}
                
                throw std::logic_error("DiskPoolOutputStream::streamData() > Unexpected input stream type encountered.");
            }
            
            /**
             * Attempts to copy all available data from the disk input stream to the output stream.
             * 
             * @param input the stream to be read from
             * 
             * @throw invalid_argument if there is no data to be read from the input stream, or if the input
             * stream has too much data for the output stream, or if both stream use the same underlying buffer
             */
            void streamDataFromDisk(DiskPoolInputStream & input)
            {
                boost::lock(poolWriteMutex, input.poolReadMutex);
                boost::lock_guard<boost::mutex> writeLock(poolWriteMutex, boost::adopt_lock);
                boost::lock_guard<boost::mutex> readLock(input.poolReadMutex, boost::adopt_lock);

                if(input.remainingData == 0)
                    throw std::invalid_argument("::operator<<(DiskPoolOutputStream, DiskPoolInputStream) > The number of bytes to write must be larger than 0.");

                if(remainingData < input.remainingData)
                    throw std::invalid_argument("::operator<<(DiskPoolOutputStream, DiskPoolInputStream) > The output stream is unable to store all the data available in the input stream.");

                if(fileStream.rdbuf() == input.fileStream.rdbuf())
                    throw std::invalid_argument("::operator<<(DiskPoolOutputStream, DiskPoolInputStream) > Both streams use the same buffer.");

                //disables white-space skipping
                input.fileStream >> std::noskipws;
                fileStream >> std::noskipws;

                input.fileStream.seekg(input.start);
                fileStream.seekp(start);

                DataSize bytesToRead = input.remainingData;
                input.start += bytesToRead;
                input.remainingData = 0;
                start += bytesToRead;
                remainingData -= bytesToRead;

                std::copy_n(std::istream_iterator<DiskPoolInputStream::char_type>(input.fileStream), bytesToRead, std::ostream_iterator<DiskPoolOutputStream::char_type>(fileStream));

                fileStream.flush();

                //releases the locks
                --input.readLocksCounter;
                if(remainingData == 0)
                    writeLocked = false;
            }
            
            /**
             * Flushes the underlying stream.
             * 
             * @return reference to this stream
             */
            PoolOutputStream & flush()
            {
                boost::lock_guard<boost::mutex> writeLock(poolWriteMutex);
                fileStream.flush();
                return *this;
            }
            
            StoredDataID getDataID() const { return id; }
            void resetDataID(StoredDataID newID) { id = newID; }
            DataSize getMaxWritableBytes() const { return remainingData; }
            
        private:
            StoredDataID id;
            DataSize remainingData;
            DiskDataAddress start;
            boost::mutex & poolWriteMutex;
            boost::filesystem::fstream & fileStream;
            bool & writeLocked;
    };
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>DiskPoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param output the stream to be written to
     * @param input the stream to be read from
     * @return a reference to the output stream
     * 
     * @throw invalid_argument if there is no data to be read from the input stream, or if the input
     * stream has too much data for the output stream, or if both stream use the same underlying buffer
     */
    inline DiskPoolOutputStream & operator<< (DiskPoolOutputStream & output, DiskPoolInputStream & input)
    {
        output.streamDataFromDisk(input);
        return output;
    }
    
    /**
     * Attempts to copy all available data from the input to the output stream.
     * 
     * Note: Uses <code>DiskPoolOutputStream::streamData()</code> to perform the operation.
     * 
     * @param input the stream to be read from
     * @param output the stream to be written to
     * @return a reference to the input stream
     * 
     * @throws see <code>StorageManagement_Pools::operator\<\<(DiskPoolOutputStream, DiskPoolInputStream)</code>
     */
    inline DiskPoolInputStream & operator>> (DiskPoolInputStream & input, DiskPoolOutputStream & output)
    {
        output.streamDataFromDisk(input);
        return input;
    }
}

#endif	/* DISKPOOLSTREAMS_H */

