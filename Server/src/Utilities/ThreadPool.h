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

#ifndef THREADPOOL_H
#define	THREADPOOL_H

#include <atomic>
#include <string>
#include "FileLogger.h"
#include <boost/thread.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace Utilities
{
    /**
     * Class for executing generic tasks via a set of threads.
     */
    class ThreadPool
    {
        public:
            /** Exception used only for notifying threads in a Utilities::ThreadPool that they need to terminate. */
            class StopThreadException;
            
            /**
             * Constructs a new thread pool with the specified number of threads.
             * 
             * Note: The logger is for debugging purposes only and need not be
             * specified under normal circumstances.
             * 
             * @param poolSize the initial number of threads to be created in the pool
             * @param parentLogger pointer to an initialised FileLogger (optional)
             */
            ThreadPool(unsigned long poolSize, Utilities::FileLogger * parentLogger = nullptr);
            
            /**
             * Destroys the pool and all associated threads.
             * 
             * Note: All tasks already submitted will be completed; no new tasks will be accepted.
             */
            ~ThreadPool();
            
            ThreadPool() = delete;                                  //No default constructor
            ThreadPool(const ThreadPool& orig) = delete;            //Copying not allowed (pass/access only by reference/pointer)
            ThreadPool& operator=(const ThreadPool& orig) = delete; //Copying not allowed (pass/access only by reference/pointer)

            /**
             * Submits a new task for the thread pool to process.
             * 
             * @param task the task to be processed by the pool
             */
            void assignTask(std::function<void(void)> task);
            
            /**
             * Creates the requested number of new threads and adds them to the pool.
             * 
             * @param number the amount of new threads to create
             */
            void addThreads(unsigned long number);
            
            /**
             * Removes the requested number of threads from the pool.
             * 
             * Note: Cannot remove more threads than are available (getPoolSize() >= number)
             * 
             * @param number
             */
            void removeThreads(unsigned long number);
            
            /**
             * Stops and destroys all threads in the pool.
             * 
             * Note: stopAllThreads() is identical to removeThreads(getPoolSize())
             */
            void stopAllThreads();
            
            /**
             * Retrieves the number of threads in the pool.
             * 
             * @return the thread pool size
             */
            unsigned long getPoolSize() const { return threadGroup.size(); }
            
        private:
            //Pool Configuration
            Utilities::FileLogger * logger;
            boost::uuids::uuid poolID;
            
            //Task Management
            std::atomic<bool> stopPool{false};
            boost::asio::io_service threadService;
            boost::shared_ptr<boost::asio::io_service::work> poolWork;
            boost::mutex threadDataMutex;
            boost::thread_group threadGroup;
            boost::unordered_map<boost::thread::id, boost::thread *> threads;
            
            /**
             * Main method for handling each thread's life cycle.
             * 
             * Note: Calls boost::asio::io_service::run() to work on tasks.
             */
            void threadHandler();
            
            /**
             * Logs the specified message, if a logger is assigned to the pool.
             * 
             * Note: Always logs debugging messages.
             * 
             * @param message the message to be logged
             */
            void logMessage(std::string message)
            {
                if(logger != nullptr)
                    logger->logMessage(Utilities::FileLogSeverity::Debug, "ThreadPool <" + boost::lexical_cast<std::string>(poolID) + "> > " + message);
            }
    };
    
    class ThreadPool::StopThreadException : public std::exception
    {
        public:
            StopThreadException() {}
            ~StopThreadException() noexcept {}
            const char * what() const noexcept { return "StopThreadException"; }
    };
}
#endif	/* THREADPOOL_H */

