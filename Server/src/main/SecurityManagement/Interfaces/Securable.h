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

#ifndef SECURABLE_H
#define	SECURABLE_H

#include "../Types/Types.h"
#include "../Types/SecurityTokens.h"

namespace SecurityManagement_Interfaces
{
    /**
     * Interface for enabling token-based instruction authorization security.
     * 
     * It allows posting of one-time tokens to <code>Securable</code> components,
     * which are to be used for authorizing actions/instructions. A <code>Securable</code>
     * component must always discard each token after the instruction for which it
     * was generated has been executed.
     */
    class Securable
    {
        public:
            /**
             * Gives the specified authorization token to the securable component.
             * 
             * @param token the token to be posted
             */
            virtual void postAuthorizationToken(const SecurityManagement_Types::AuthorizationTokenPtr token) = 0;
            
            /**
             * Retrieves the type of the securable component;
             * 
             * @return the component type
             */
            virtual SecurityManagement_Types::SecurableComponentType getComponentType() const = 0;
    };
}

#endif	/* SECURABLE_H */

