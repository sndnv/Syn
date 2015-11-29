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

#ifndef SCHEDULEDATACONTAINER_H
#define	SCHEDULEDATACONTAINER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "DataContainer.h"
#include "../Types/Types.h"

using DatabaseManagement_Types::ScheduleIntervalType;
using Common_Types::ScheduleID;

namespace DatabaseManagement_Containers
{
    class ScheduleDataContainer : public DatabaseManagement_Containers::DataContainer
    {
        public:
            ScheduleDataContainer(bool active, boost::posix_time::ptime nextScheduleRun, int numberOfRepetitions, ScheduleIntervalType type, 
                                  unsigned long length, bool runMissed, bool deleteAfterCompletion, ScheduleID id = boost::uuids::random_generator()())
                    : DataContainer(id, DatabaseObjectType::SCHEDULE), isActive(active), nextRun(nextScheduleRun), repetitions(numberOfRepetitions), 
                      intervalType(type), intervalLength(length), runIfMissed(runMissed), deleteWhenDone(deleteAfterCompletion)
            {}
            
            ScheduleDataContainer() = delete;                                           //No default constructor
            ScheduleDataContainer(const ScheduleDataContainer&) = default;              //Default copy constructor
            ~ScheduleDataContainer() = default;                                         //Default destructor
            ScheduleDataContainer& operator=(const ScheduleDataContainer&) = default;   //Default assignment operator
                    
            ScheduleID getScheduleID()              const { return containerID; }
            bool isScheduleActive()                 const { return isActive; }
            boost::posix_time::ptime getNextRun()   const { return nextRun; }
            int getNumberOfRepetitions()            const { return repetitions; }
            ScheduleIntervalType getIntervalType()  const { return intervalType; }
            unsigned long getIntervalLength()       const { return intervalLength; }
            bool runScheduleIfMissed()              const { return runIfMissed; }
            bool deleteScheduleAfterCompletion()    const { return deleteWhenDone; }
                    
            void activateSchedule()
            {
                isActive = true;
                modified = true;
            }
            
            void deactivateSchedule()
            {
                isActive = false;
                modified = true;
            }
            
            /**
             * Redefines the schedule's timing.
             * 
             * @param nextScheduleRun the next schedule window
             * @param numberOfRepetitions number of schedule repetitions
             * @param type schedule interval type
             * @param length schedule interval length
             */
            void updateScheduleTiming(boost::posix_time::ptime nextScheduleRun, int numberOfRepetitions, ScheduleIntervalType type, unsigned long length)
            {
                nextRun = nextScheduleRun;
                repetitions = numberOfRepetitions;
                intervalType = type;
                intervalLength = length;
                modified = true;
            }
            
            /**
             * Redefines the schedule behaviour.
             * 
             * @param runMissed true, if the schedule is to be started, even if its specified window was missed
             * @param deleteAfterCompletion true, if the schedule is to be removed after completion
             */
            void updateScheduleBehaviour(bool runMissed, bool deleteAfterCompletion)
            {
                runIfMissed = runMissed;
                deleteWhenDone = deleteAfterCompletion;
                modified = true;
            }
            
        private:
            bool isActive;
            boost::posix_time::ptime nextRun;
            int repetitions;
            ScheduleIntervalType intervalType;
            unsigned long intervalLength;
            bool runIfMissed;
            bool deleteWhenDone;
    };
    
    typedef boost::shared_ptr<DatabaseManagement_Containers::ScheduleDataContainer> ScheduleDataContainerPtr;
}

#endif	/* SCHEDULEDATACONTAINER_H */

