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

#ifndef SESSIONMANAGERINSTRUCTIONSET_H
#define	SESSIONMANAGERINSTRUCTIONSET_H

#include <vector>
#include <string>
#include "InstructionSet.h"
#include "../Types/Types.h"
#include "../../Common/Types.h"
#include "../../SessionManagement/Types/Types.h"
#include "../../DatabaseManagement/Containers/SessionDataContainer.h"

using InstructionManagement_Sets::Instruction;
using InstructionManagement_Sets::InstructionResult;
using InstructionManagement_Types::InstructionSetType;
using InstructionManagement_Types::SessionManagerInstructionType;

using Common_Types::DBObjectID;
using Common_Types::Seconds;
using Common_Types::INVALID_OBJECT_ID;
using SessionManagement_Types::InternalSessionID;
using SessionManagement_Types::INVALID_INTERNAL_SESSION_ID;
using SessionManagement_Types::GetSessionsConstraintType;
using DatabaseManagement_Containers::SessionDataContainerPtr;

namespace InstructionManagement_Sets
{
    namespace SessionManagerInstructions
    {
        struct GetSession : public Instruction<SessionManagerInstructionType>
        {
            GetSession(InternalSessionID id)
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::GET_SESSION),
              sessionID(id)
            {}
            
            bool isValid() { return (sessionID > INVALID_INTERNAL_SESSION_ID); }
            InternalSessionID sessionID;
        };
        
        struct GetSessionsByConstraint : public Instruction<SessionManagerInstructionType>
        {
            GetSessionsByConstraint(GetSessionsConstraintType type, DBObjectID id = INVALID_OBJECT_ID)
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::GET_SESSIONS_BY_CONSTRAINT),
              constraintType(type), constraintValue(id)
            {}
            
            bool isValid()
            {
                if(constraintType == GetSessionsConstraintType::INVALID)
                    return false;
                
                if((constraintType == GetSessionsConstraintType::ALL_FOR_DEVICE
                        || constraintType == GetSessionsConstraintType::ALL_FOR_USER)
                   && constraintValue == INVALID_OBJECT_ID)
                    return false;
                
                return true;
            }
            GetSessionsConstraintType constraintType;
            DBObjectID constraintValue;
        };
        
        struct ForceSessionExpiration : public Instruction<SessionManagerInstructionType>
        {
            ForceSessionExpiration(InternalSessionID id)
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::FORCE_SESSION_EXPIRATION),
              sessionID(id)
            {}
            
            bool isValid() { return (sessionID > INVALID_INTERNAL_SESSION_ID); }
            InternalSessionID sessionID;
        };
        
        struct ForceSessionReauthentication : public Instruction<SessionManagerInstructionType>
        {
            ForceSessionReauthentication(InternalSessionID id)
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::FORCE_SESSION_REAUTHENTICATION),
              sessionID(id)
            {}
            
            bool isValid() { return (sessionID > INVALID_INTERNAL_SESSION_ID); }
            InternalSessionID sessionID;
        };
        
        struct ForceExpirationProcess : public Instruction<SessionManagerInstructionType>
        {
            ForceExpirationProcess(Seconds delay)
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::FORCE_EXPIRATION_PROCESS),
              delayTime(delay)
            {}
            
            bool isValid() { return true; }
            Seconds delayTime;
        };
        
        struct DebugGetState : public Instruction<SessionManagerInstructionType>
        {
            DebugGetState()
            : Instruction(InstructionSetType::SESSION_MANAGER, SessionManagerInstructionType::DEBUG_GET_STATE)
            {}
            
            bool isValid() { return true; }
        };
        
        namespace Results
        {
            struct GetSession : public InstructionResult<SessionManagerInstructionType>
            {
                GetSession(SessionDataContainerPtr input) : result(input) {}
                SessionDataContainerPtr result;
            };
            
            struct GetSessionsByConstraint : public InstructionResult<SessionManagerInstructionType>
            {
                GetSessionsByConstraint(std::vector<SessionDataContainerPtr> input) : result(input) {}
                std::vector<SessionDataContainerPtr> result;
            };
            
            struct ForceSessionExpiration : public InstructionResult<SessionManagerInstructionType>
            {
                ForceSessionExpiration(bool input) : result(input) {}
                bool result;
            };
            
            struct ForceSessionReauthentication : public InstructionResult<SessionManagerInstructionType>
            {
                ForceSessionReauthentication(bool input) : result(input) {}
                bool result;
            };
            
            struct ForceExpirationProcess : public InstructionResult<SessionManagerInstructionType>
            {
                ForceExpirationProcess(bool input) : result(input) {}
                bool result;
            };
            
            struct DebugGetState : public InstructionResult<SessionManagerInstructionType>
            {
                DebugGetState(std::string input) : result(input) {}
                std::string result;
            };
        }
    }
}

#endif	/* SESSIONMANAGERINSTRUCTIONSET_H */

