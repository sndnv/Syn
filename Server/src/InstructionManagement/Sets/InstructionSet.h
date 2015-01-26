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

#ifndef BASICINSTRUCTIONSET_H
#define	BASICINSTRUCTIONSET_H

#include <atomic>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/future.hpp>
#include "../Types/Types.h"
#include "../../Common/Types.h"
#include "../../SecurityManagement/Types/SecurityTokens.h"

namespace InstructionManagement_Sets
{
    template <typename TInstructionTypeEnum>
    class Instruction;
    
    template <typename TInstructionTypeEnum>
    class InstructionSet;
    
    template <typename TInstructionTypeEnum>
    class InstructionResult;
    
    template <typename TInstructionTypeEnum>
    using InstructionPtr = boost::shared_ptr<Instruction<TInstructionTypeEnum>>;
    
    template <typename TInstructionTypeEnum>
    using InstructionSetPtr = boost::shared_ptr<InstructionSet<TInstructionTypeEnum>>;
    
    template <typename TInstructionTypeEnum>
    using InstructionResultPtr = boost::shared_ptr<InstructionResult<TInstructionTypeEnum>>;
    
    template <typename TInstructionTypeEnum>
    using InstructionResultPromise = boost::promise<InstructionResultPtr<TInstructionTypeEnum>>;
    
    template <typename TInstructionTypeEnum>
    using InstructionResultFuture = boost::unique_future<InstructionResultPtr<TInstructionTypeEnum>> ;
    
    /**
     * Exception class signifying that the <code>InstructionSet</code> is closed and
     * can no longer be used.
     */
    class SetClosedException : public std::exception
    {
        public:
            SetClosedException() {}
            ~SetClosedException() noexcept {}
            const char* what() const noexcept { return "SetClosedException"; }
    };
    
    /**
     * Exception class signifying that the requested <code>Instruction</code>
     * does not have a defined handler.
     */
    class InstructionNotSetException : public std::exception
    {
        public:
            InstructionNotSetException() {}
            ~InstructionNotSetException() noexcept {}
            const char* what() const noexcept { return "InstructionNotSetException"; }
    };
    
    /**
     * Exception class signifying that the requested <code>Instruction</code>
     * is not recognised/has not been defined.
     */
    class InstructionNotRecognisedException : public std::exception
    {
        public:
            InstructionNotRecognisedException() {}
            ~InstructionNotRecognisedException() noexcept {}
            const char* what() const noexcept { return "InstructionNotRecognisedException"; }
    };
    
    /**
     * Base class template for instruction results.
     * 
     * Note: Provides no functionality.
     * 
     * @param (template) TInstructionTypeEnum template parameter denoting the type of the instructions
     * that will be processed
     */
    template <typename TInstructionTypeEnum>
    struct InstructionResult
    {
        //restricts the template parameter
        static_assert(std::is_enum<TInstructionTypeEnum>::value, "InstructionResult > Supplied instruction type for <TInstructionTypeEnum> is not an enum class.");
        virtual ~InstructionResult() {}
    };
    
    /**
     * Base class for all instructions.
     */
    class InstructionBase
    {
        public:
            virtual ~InstructionBase() {}
            
            /** Retrieves the type of the parent instruction set.\n\n@return the instruction set type */
            InstructionManagement_Types::InstructionSetType getParentSet() const { return parentSet; }
            /** Validates the instruction.\n\n@return <code>true</code>, if the instruction and its arguments (if any) are valid */
            virtual bool isValid() = 0;
            
        protected:
            InstructionBase(InstructionManagement_Types::InstructionSetType setType) : parentSet(setType) {}
            InstructionManagement_Types::InstructionSetType parentSet;
    };
    typedef boost::shared_ptr<InstructionBase> InstructionBasePtr;
    
    /**
     * Base class template for instructions.
     * 
     * @param (template) TInstructionTypeEnum template parameter denoting the type of instructions
     * that will be processed
     */
    template <typename TInstructionTypeEnum>
    class Instruction : public InstructionBase
    {
        //restricts the template parameter
        static_assert(std::is_enum<TInstructionTypeEnum>::value, "Instruction > Supplied instruction type for <TInstructionTypeEnum> is not an enum class.");
        
        friend class InstructionSet<TInstructionTypeEnum>;
        
        public:
            virtual ~Instruction() {}
            /** Retrieves a reference to the promise object used by the instruction.\n\n@return the promise object reference */
            InstructionResultPromise<TInstructionTypeEnum> & getPromise() { return promise; }
            /** Retrieves the future associated with the instruction's promise.\n\n@return the future object */
            InstructionResultFuture<TInstructionTypeEnum> getFuture() { return promise.get_future(); }
            /** Retrieves the type of type of the instruction.\n\n@return the instruction type */
            TInstructionTypeEnum getType() const { return type; }
            /** Retrieves the associated authorization token.\n\n@return the requested token (if it has been set) or an empty shared_ptr */
            SecurityManagement_Types::AuthorizationTokenPtr getToken() const { return authorizationToken; }
            
        protected:
            Instruction(InstructionManagement_Types::InstructionSetType setType, TInstructionTypeEnum instructionType) : InstructionBase(setType), type(instructionType) {}
            InstructionResultPromise<TInstructionTypeEnum> promise;
            TInstructionTypeEnum type;
            
        private:
            SecurityManagement_Types::AuthorizationTokenPtr authorizationToken;
    };
    
    /**
     * Base class for all instruction sets.
     * 
     * Note: Provides no functionality.
     */
    class InstructionSetBase
    {
        public:
            virtual ~InstructionSetBase() {}
            virtual void processInstruction(InstructionBasePtr instruction, SecurityManagement_Types::AuthorizationTokenPtr token) = 0;
            virtual Common_Types::UserAccessLevel getMinimumAccessLevel() const = 0;
    };
    typedef boost::shared_ptr<InstructionSetBase> InstructionSetBasePtr;
    
    /**
     * Class template for instruction sets.
     * 
     * @param (template) TInstructionTypeEnum template parameter denoting the type of instructions
     * that will be processed
     */
    template <typename TInstructionTypeEnum>
    class InstructionSet : public InstructionSetBase
    {
        //restricts the template parameter
        static_assert(std::is_enum<TInstructionTypeEnum>::value, "InstructionSet > Supplied instruction type for <TInstructionTypeEnum> is not an enum class.");
        
        public:
            /** Creates a new instruction set and initialises its instructions table. */
            InstructionSet()
            : minAccessLevel(Common_Types::UserAccessLevel::INVALID)
            { buildTable(); }
            
            /** Disables the instruction set and clears the instructions table. */
            virtual ~InstructionSet() { closed = true; instructionHandlers.clear(); }
            
            /**
             * Binds the specified handler to the specified instruction identifier/type.
             * 
             * Note: The handler signature must be: <code>void(InstructionPtr<TInstructionTypeEnum>)</code>
             * 
             * @param type the type/identifier of the instruction
             * @param handler the handler that will perform the work required by the instruction
             * @throw std::invalid_argument if the specified type was not found in the handlers table
             * 
             * Example:
             * <code>\n
             * ...\n
             * //some setup\n
             * namespace InstructionResults = InstructionManagement_Sets::DatabaseManagerInstructions::Results;\n
             * InstructionSetPtr<DatabaseManagerInstructionType> set;\n
             * ...\n
             * auto getDefaultCacheParametersHandler = [&](InstructionPtr<DatabaseManagerInstructionType> instruction)\n
             * {\n
             *     auto result = boost::shared_ptr<InstructionResults::GetDefaultDALCacheParametersResult>
             *                   (new InstructionResults::GetDefaultDALCacheParametersResult{getDefaultCacheParameters()});\n
             *     instruction->getPromise().set_value(result);\n
             * };\n
             * set->bindInstructionHandler(DatabaseManagerInstructionType::GET_DEFAULT_CACHE_PARAMS, getDefaultCacheParametersHandler);\n
             * ...\n
             * </code>
             */
            void bindInstructionHandler(TInstructionTypeEnum type, std::function<void(InstructionPtr<TInstructionTypeEnum>)> handler)
            {
                if(instructionHandlers.find(type) != instructionHandlers.end())
                    instructionHandlers[type] = handler;
                else
                    throw std::invalid_argument("InstructionSet::bindInstructionHandler() > Supplied instruction type not found in table.");
            }
            
            /**
             * Begins processing the specified instruction.
             * 
             * @param instruction the instruction to be processed
             * @param token the authorization token associated with the instruction
             */
            void processInstruction(InstructionPtr<TInstructionTypeEnum> instruction, SecurityManagement_Types::AuthorizationTokenPtr token)
            {
                if(closed)
                {
                    try {  boost::throw_exception(SetClosedException()); }
                    catch(const SetClosedException &) { instruction->getPromise().set_exception(boost::current_exception()); }
                    return;
                }
                
                if(instructionHandlers.find(instruction->getType()) != instructionHandlers.end())
                {
                    instruction->authorizationToken = token;
                    instructionHandlers[instruction->getType()](instruction);
                }
                else
                {
                    try { boost::throw_exception(InstructionNotRecognisedException()); }
                    catch(const InstructionNotRecognisedException &) { instruction->getPromise().set_exception(boost::current_exception()); }
                }
            }
            
            /**
             * Attempts to process the specified instruction.
             * 
             * Note: The instruction is dynamically cast to the type expected by
             * the instruction set. An exception is thrown if the cast fails.
             * 
             * @param instruction the instruction to be processed
             * @param token the authorization token associated with the instruction
             * @throw std::invalid_argument if the specified instruction is not of the expected type
             */
            void processInstruction(InstructionBasePtr instruction, SecurityManagement_Types::AuthorizationTokenPtr token)
            {
                InstructionPtr<TInstructionTypeEnum> actualInstruction = boost::dynamic_pointer_cast<Instruction<TInstructionTypeEnum>>(instruction);
                if(actualInstruction)
                    processInstruction(actualInstruction, token);
                else
                    throw std::invalid_argument("InstructionSet::processInstruction(InstructionBasePtr) > Supplied instruction is not of the expected type.");
            }
            
            /**
             * Sets the minimum access level for the instruction set.
             * 
             * Note: The access level should be set by the target component at registration
             * time and it can be set only once.
             * 
             * @param minimumAccessLevel the minimum access level required for using 
             * any instructions in the set
             * 
             * @throw logic_error if the access level has already been set
             */
            void setMinimumAccessLevel(Common_Types::UserAccessLevel minimumAccessLevel)
            {
                if(minAccessLevel == Common_Types::UserAccessLevel::INVALID)
                    minAccessLevel = minimumAccessLevel;
                else
                    throw std::logic_error("InstructionSet::setMinimumAccessLevel() > The minimum access level has already been set.");
            }
            
            /**
             * Retrieves the minimum access level required for using any instructions in the set.
             * 
             * @return the minimum access level
             */
            Common_Types::UserAccessLevel getMinimumAccessLevel() const
            {
                return minAccessLevel;
            }
            
        protected:
            std::atomic<bool> closed{false}; //denotes whether the set can be used
            Common_Types::UserAccessLevel minAccessLevel; //the minimum user access level required for working with the set
            boost::unordered_map<TInstructionTypeEnum, std::function<void(InstructionPtr<TInstructionTypeEnum>)>> instructionHandlers;
            
            /**
             * Instruction handler placeholder which signifies that a proper
             * handler was not specified during the initial instruction registration.
             * 
             * An exception is set as the promise result.
             * 
             * @param instruction the instruction object that was received
             */
            static void instructionNotSet(InstructionPtr<TInstructionTypeEnum> instruction)
            {
                try {  boost::throw_exception(InstructionNotSetException()); }
                catch(const InstructionNotSetException &) { instruction->getPromise().set_exception(boost::current_exception()); }
            }
            
            /**
             * Builds the default instructions table with all handlers set to
             * <code>instructionNotSet</code>.
             */
            virtual void buildTable();
    };
}

#endif	/* BASICINSTRUCTIONSET_H */

