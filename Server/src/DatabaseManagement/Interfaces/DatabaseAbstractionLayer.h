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

#ifndef DATABASEABSTRACTIONLAYER_H
#define	DATABASEABSTRACTIONLAYER_H

#include <string>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>
#include "../Types/Types.h"
#include "../Containers/DataContainer.h"
#include "DatabaseSettingsContainer.h"
#include "DatabaseInformationContainer.h"

using DatabaseManagement_Types::DatabaseObjectType;
using DatabaseManagement_Types::DBObjectID;
using DatabaseManagement_Types::DatabaseRequestID;
using DatabaseManagement_Types::DatabaseAbstractionLayerID;

using DatabaseManagement_Containers::DataContainerPtr;

namespace DatabaseManagement_Interfaces
{
    /**
     * Interface for defining a Database Abstraction Layer (DAL), to be used
     * in DB management.
     */
    class DatabaseAbstractionLayer
    {
        public:
            virtual ~DatabaseAbstractionLayer()
            {
                onFailure.disconnect_all_slots();
                onSuccess.disconnect_all_slots();
            }
            
            /**
             * Requests an object retrieval.
             * 
             * Note: The result will be supplied via an onSuccess/onFailure event, with the given ID.
             * 
             * @param requestID the ID of the request
             * @param constraintType the type of the constraint (selected from the enum subclasses of DatabaseManagement_Types::DatabaseSelectConstraints)
             * @param constraintValue associated constraint value (if any)
             * @return true, if the request was successful
             */
            virtual bool getObject(DatabaseRequestID requestID, boost::any constraintType, boost::any constraintValue) = 0;
            
            /**
             * Requests an object insertion.
             * 
             * Note: The result will be supplied via an onSuccess/onFailure event, with the given ID.
             * 
             * @param requestID the ID of the request
             * @param inputData the object to be inserted
             * @return true, if the request was successful
             */
            virtual bool putObject(DatabaseRequestID requestID, const DataContainerPtr inputData) = 0;
            
            /**
             * Requests an object update.
             * 
             * Note: The result will be supplied via an onSuccess/onFailure event, with the given ID.
             * 
             * @param requestID the ID of the request
             * @param inputData the object to updated
             * @return true, if the request was successful
             */
            virtual bool updateObject(DatabaseRequestID requestID, const DataContainerPtr inputData) = 0;
            
            /**
             * Requests an object removal.
             * 
             * Note: The result will be supplied via an onSuccess/onFailure event, with the given ID.
             * 
             * @param requestID the ID of the request
             * @param id the ID of the object to be removed
             * @return true, if the request was successful
             */
            virtual bool removeObject(DatabaseRequestID requestID, DBObjectID id) = 0;
            
            /**
             * Updates the DAL's database settings, if applicable.
             * 
             * @param settings the new settings
             * @return true, if the update was successful
             */
            virtual bool changeDatabaseSettings(const DatabaseSettingsContainer settings) = 0;
            
            /**
             * Builds the database structure, if applicable.
             * 
             * @return true, if the build was successful
             */
            virtual bool buildDatabase() = 0;
            
            /**
             * Rebuilds the database structure, if applicable.
             * 
             * @return true, if the rebuild was successful
             */
            virtual bool rebuildDatabase() = 0;
            
            /**
             * Clears the data from the database, if applicable.
             * 
             * Note: The database structure is maintained.
             * 
             * @return true, if the clearing was successful
             */
            virtual bool clearDatabase() = 0;
            
            /**
             * Initialises the connection to the database.
             * 
             * @return true, if the connection was successfully established
             */
            virtual bool connect() = 0;
            
            /**
             * Terminates the connection to the database.
             * 
             * @return true, if the connection was successfully terminated
             */
            virtual bool disconnect() = 0;
            
            /**
             * Retrieves the information associated with the database, if applicable.
             * 
             * @return the requested database information
             */
            virtual const DatabaseInformationContainer* getDatabaseInfo() const = 0;
            
            /**
             * Retrieves the type of the DAL.
             * 
             * @return the DAL type
             */
            virtual DatabaseObjectType getType() const = 0;
            
            /**
             * Sets the DAL ID.
             * 
             * Note: This ID is set by the system.
             * 
             * @param id the DAL ID
             */
            virtual void setID(DatabaseAbstractionLayerID id)= 0;
            
            /**
             * Retrieves the ID associated with the DAL.
             * 
             * Note: This ID is set by the system.
             * 
             * @return the DAL id
             */
            virtual DatabaseAbstractionLayerID getID() const = 0;
            
            /**
             * Attaches the specified event handler to the "onFailure" event of the DAL.
             * 
             * @param function the handler to be attached
             * @return the associated connection object
             */
            boost::signals2::connection onFailureEventAttach(std::function<void(DatabaseAbstractionLayerID, DatabaseRequestID, DBObjectID)> function) { return onFailure.connect(function); }
            
            /**
             * Attaches the specified event handler to the "onSuccess" event of the DAL.
             * 
             * @param function the handler to be attached
             * @return the associated connection object
             */
            boost::signals2::connection onSuccessEventAttach(std::function<void(DatabaseAbstractionLayerID, DatabaseRequestID, DataContainerPtr)> function) { return onSuccess.connect(function); }
            
        protected:
            boost::signals2::signal<void (DatabaseAbstractionLayerID, DatabaseRequestID, DBObjectID)> onFailure;
            boost::signals2::signal<void (DatabaseAbstractionLayerID, DatabaseRequestID, DataContainerPtr)> onSuccess;
    };
    
    //TODO - move to unique_ptr
    typedef boost::shared_ptr<DatabaseManagement_Interfaces::DatabaseAbstractionLayer> DALPtr;
}
#endif	/* DATABASEABSTRACTIONLAYER_H */

