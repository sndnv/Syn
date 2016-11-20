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

#ifndef BASICSPEC_H
#define	BASICSPEC_H

#include <algorithm>
#include <catch/catch.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include "../main/Common/Types.h"

/**
 * Puts the current thread to sleep for the specified number of seconds.
 * 
 * @param seconds the number of seconds to sleep
 */
inline void waitFor(double seconds)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(seconds*1000));
}

/**
 * Converts the supplied string to a ByteVector shared pointer.
 * 
 * @param input the input data
 * @return the converted data
 */
inline Common_Types::ByteVectorPtr getByteVectorPtrFromString(std::string input)
{
    return boost::shared_ptr<Common_Types::ByteVector>(
            new Common_Types::ByteVector(input.begin(), input.end()));
}

/**
 * Checks if the supplied ByteVectors contain the same data.
 * 
 * @param a ByteVector A
 * @param b ByteVector B
 * @return <code>true</code>, if the two vectors contain the same data
 */
inline bool equal(Common_Types::ByteVectorPtr a, Common_Types::ByteVectorPtr b)
{
    if(a->size() != b->size())
        return false;
    
    std::sort(a->begin(), a->end());
    std::sort(b->begin(), b->end());
    return (*a == *b);
}

#endif	/* BASICSPEC_H */
