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

#include "FileLogger.h"
#include <cstdio>

Utilities::FileLogger::FileLogger(std::string fullFilePath, unsigned long maximumFileSize, FileLogSeverity minimumSeverity)
: minSeverity(minimumSeverity), maxFileSize(maximumFileSize), filePath(fullFilePath),
  mainThread(new boost::thread(&Utilities::FileLogger::mainLoggerThread, this))
{}

Utilities::FileLogger::FileLogger(FileLoggerParameters parameters)
: FileLogger(parameters.logFilePath, parameters.maximumFileSize, parameters.minimumSeverity)
{}

Utilities::FileLogger::~FileLogger()
{
    {
        boost::unique_lock<boost::mutex> dataLock(threadMutex);
        stopLogger = true;
        threadLockCondition.notify_all();
    }
    
    mainThread->join();
}

void Utilities::FileLogger::logMessage(FileLogSeverity severity, std::string message)
{
    logMessage(boost::posix_time::second_clock::local_time(), severity,  message);
}

void Utilities::FileLogger::logMessage(boost::posix_time::ptime timestamp, Utilities::FileLogSeverity severity, std::string message)
{
    if(stopLogger || !threadRunning)
        return;
    
    if(fileLogSeverityToInt(severity) < fileLogSeverityToInt(minSeverity))
        return;
        
    std::string severityStr = severityToString(severity);
    
    //YYYY-MM-DD HH:MM:SS,<severity>,<message>
    //21 chars static size + size of severity + size of message
    unsigned int bufferSize = severityStr.size() + message.size() + 22;
    char messageBuffer[bufferSize];
    snprintf(messageBuffer, bufferSize, "%04u-%02u-%02u %02d:%02d:%02d,%s,%s", 
            (unsigned short)timestamp.date().year(), (unsigned short)timestamp.date().month(), (unsigned short)timestamp.date().day(), 
            timestamp.time_of_day().hours(), timestamp.time_of_day().minutes(), timestamp.time_of_day().seconds(),
            severityStr.c_str(), message.c_str());
    
    boost::lock_guard<boost::mutex> dataLock(threadMutex);
    messages.push(std::string(messageBuffer));
    threadLockCondition.notify_all();
}

std::string Utilities::FileLogger::severityToString(FileLogSeverity severity)
{
    switch(severity)
    {
        case FileLogSeverity::Debug: return "D";
        case FileLogSeverity::Info: return "I";
        case FileLogSeverity::Warning: return "W";
        case FileLogSeverity::Error: return "E";
        default: return "U";
    }
}

void Utilities::FileLogger::mainLoggerThread()
{
    threadRunning = true;
    
    fileStream.open(filePath, std::ios::app);
    
    //the main thread continues to run until it is requested to stop or the file stream fails
    while(!stopLogger && fileStream.is_open())
    {
        boost::unique_lock<boost::mutex> dataLock(threadMutex);
        
        if(messages.size() == 0 && !stopLogger)
            threadLockCondition.wait(dataLock);
        
        for(unsigned int i = 0; i < messages.size(); i++)
        {
            fileStream << messages.front() << std::endl;
            messages.pop();
            processedLogs++;
        }
        
        //checks if the max file size has been reached
        if(fileStream.tellp() >= maxFileSize)
        {
            fileStream.close();
            
            //builds the new name of the current log file
            boost::posix_time::ptime timestamp = boost::posix_time::second_clock::local_time();
            //_YYYYMMDD_HHMMSS
            //size of file name + 16 chars static size + 1 char for \0
            unsigned int bufferSize = filePath.size() + 17;
            char newFilePathBuffer[bufferSize];
            snprintf(newFilePathBuffer, bufferSize, "%s_%04u%02u%02u_%02d%02d%02d",
                    filePath.c_str(),
                    (unsigned short)timestamp.date().year(), (unsigned short)timestamp.date().month(), (unsigned short)timestamp.date().day(), 
                    timestamp.time_of_day().hours(), timestamp.time_of_day().minutes(), timestamp.time_of_day().seconds());
            std::rename(filePath.c_str(), newFilePathBuffer);
            
            //opens/creates a new file with the old name
            fileStream.open(filePath, std::ios::app);
        }
    } //thread execution ends
    
    fileStream.close();
    
    threadRunning = false;
    return;
}

