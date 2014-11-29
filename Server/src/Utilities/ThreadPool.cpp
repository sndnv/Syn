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

#include "ThreadPool.h"

Utilities::ThreadPool::ThreadPool(unsigned long poolSize, Utilities::FileLogger * parentLogger)
            : logger(parentLogger), poolID(boost::uuids::random_generator()()), poolWork(new boost::asio::io_service::work(threadService))
{
    boost::lock_guard<boost::mutex> threadsLock(threadDataMutex);
    for(unsigned long i = 0; i < poolSize; i++)
    {
        boost::thread * newThread = threadGroup.create_thread(boost::bind(&Utilities::ThreadPool::threadHandler, this));
        threads.insert(std::pair<boost::thread::id, boost::thread *>(newThread->get_id(), newThread));
        logMessage("Thread <" + boost::lexical_cast<std::string>(newThread->get_id()) + "> added to pool.");
    }
}

Utilities::ThreadPool::~ThreadPool()
{
    logMessage("Destruction initiated.");
    boost::lock_guard<boost::mutex> threadsLock(threadDataMutex);
    stopPool = true;
    poolWork.reset();
    logMessage("Waiting for all threads to terminate.");
    threadGroup.join_all();
    logMessage("All threads terminated.");
    logger = nullptr;
}

void Utilities::ThreadPool::assignTask(std::function<void(void)> task)
{
    if(stopPool)
        return;
    
    threadService.post(task);
    logMessage("New task added to pool.");
}

void Utilities::ThreadPool::addThreads(unsigned long number)
{
    if(stopPool)
        return;
    
    boost::lock_guard<boost::mutex> threadsLock(threadDataMutex);
    for(unsigned long i = 0; i < number; i++)
    {
        boost::thread * newThread = threadGroup.create_thread(boost::bind(&Utilities::ThreadPool::threadHandler, this));
        threads.insert(std::pair<boost::thread::id, boost::thread *>(newThread->get_id(), newThread));
        logMessage("Thread <" + boost::lexical_cast<std::string>(newThread->get_id()) + "> added to pool.");
    }
}

void Utilities::ThreadPool::removeThreads(unsigned long number)
{
    if(stopPool)
        return;
    
    if(number > threadGroup.size())
    {
        logMessage("Failed to remove [" + boost::lexical_cast<std::string>(number) + "] threads; the pool has only [" + boost::lexical_cast<std::string>(threadGroup.size()) + "].");
        return;
    }
    
    for(unsigned long i = 0; i < number; i++)
        threadService.post([](){throw StopThreadException();});
}

void Utilities::ThreadPool::stopAllThreads()
{
    if(stopPool)
        return;
    
    removeThreads(threadGroup.size());
}

void Utilities::ThreadPool::threadHandler()
{
    logMessage("Thread <" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + "> started.");
    
    while(!stopPool)
    {
        try
        {
            threadService.run(); //the thread becomes available for working on tasks
        }
        catch(ThreadPool::StopThreadException)
        {
            logMessage("Thread stop requested for <" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + ">");
            boost::lock_guard<boost::mutex> groupLock(threadDataMutex);
            boost::thread * currentThread = threads[boost::this_thread::get_id()];
            threads.erase(boost::this_thread::get_id());
            threadGroup.remove_thread(currentThread);
            currentThread->detach();
            break;
        }
        catch(std::exception & ex)
        {
            logMessage("Exception encountered in thread <" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + ">: [" + ex.what() + "]");
        }
    }
    
    logMessage("Thread <" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + "> stopped.");
}