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

#ifndef TESTSECURABLE_H
#define	TESTSECURABLE_H

#include <vector>
#include "../../main/Utilities/Strings/Common.h"
#include "../../main/SecurityManagement/Types/Types.h"
#include "../../main/SecurityManagement/Types/SecurityRequests.h"
#include "../../main/SecurityManagement/Types/SecurityTokens.h"
#include "../../main/SecurityManagement/Interfaces/Securable.h"

namespace Testing
{
    class TestSecurable : public SecurityManagement_Interfaces::Securable
    {
        public:
            void postAuthorizationToken(const SecurityManagement_Types::AuthorizationTokenPtr token) override
            {
                if(std::find(tokens.begin(), tokens.end(), token) == tokens.end())
                    tokens.push_back(token);
                else
                    throw std::logic_error("Token with ID [" + Utilities::Strings::toString(token->getID()) + "] found in table");
            }

            SecurityManagement_Types::SecurableComponentType getComponentType() const override
            {
                return SecurityManagement_Types::SecurableComponentType::SESSION_MANAGER;
            }

        private:
            std::vector<SecurityManagement_Types::AuthorizationTokenPtr> tokens;
    };
}

#endif	/* TESTSECURABLE_H */

