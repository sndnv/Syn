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

#ifndef COMMON_TYPES_H
#define	COMMON_TYPES_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace Common_Types
{
    typedef unsigned char Byte;
    typedef std::vector<Byte> ByteVector;
    typedef boost::shared_ptr<ByteVector> ByteVectorPtr;
    
    //struct ByteVectorDeleter { void operator()(ByteVector * vector) {  delete vector; } };
    //typedef boost::interprocess::unique_ptr<ByteVector, ByteVectorDeleter> ByteVectorUniquePtr;
    
    typedef unsigned long TransferredDataAmount;
    const TransferredDataAmount INVALID_TRANSFERRED_DATA_AMOUNT = 0; //TODO - value?
    
    typedef unsigned long TransferredFilesAmount;
    const TransferredFilesAmount INVALID_TRANSFERRED_FILES_AMOUNT = 0; //TODO - value?
    
    typedef std::string IPAddress;
    const IPAddress INVALID_IP_ADDRESS = "0/0"; //for IPv4 and IPv6
    
    typedef unsigned int IPPort;
    const IPPort INVALID_IP_PORT = 0; //TODO - value?
    
    typedef boost::posix_time::ptime Timestamp;
    const boost::posix_time::ptime INVALID_DATE_TIME = boost::posix_time::ptime(boost::posix_time::min_date_time);
    
    typedef unsigned long DataPoolSize;
    const DataPoolSize INVALID_DATA_POOL_SIZE = 0; //TODO - value?
    
    typedef std::string DataPoolPath;
    const DataPoolPath INVALID_DATA_POOL_PATH = ""; //TODO - value?
    
    typedef unsigned long DataPoolRetention;
    const DataPoolRetention INVALID_DATA_POOL_RETENTION = 0; //TODO - value?
}

#endif	/* COMMON_TYPES_H */

