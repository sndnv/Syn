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

#ifndef FILELOGGER_H
#define	FILELOGGER_H

#include <string>
#include <queue>
#include <atomic>
#include <fstream>
#include <cstdio>
#include <boost/unordered_map.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

namespace Utilities
{
    /** FileLogger event severity type */
    enum class FileLogSeverity { Debug, Info, Warning, Error };
    
    /** Parameter structure for FileLogger initialisation */
    struct FileLoggerParameters
    {
        std::string logFilePath;
        unsigned long maximumFileSize;
        FileLogSeverity minimumSeverity;
    };
    
    /**
     * Class representing a multi-threaded file logging facility.
     */
    class FileLogger
    {
        public:
            /**
             * Builds the FileLogger object.
             * 
             * Note: The constructor initializes the logger's thread.
             * 
             * @param fullFilePath full path to the log file
             * @param maximumFileSize maximum log file size (for file rotation)
             * @param minimumSeverity the lowes severity level that will be recorded (DEBUG < INFO < WARNING < ERROR)
             * @return the new logger object
             */
            FileLogger(std::string fullFilePath, unsigned long maximumFileSize, FileLogSeverity minimumSeverity = FileLogSeverity::Debug);
            
            /**
             * Builds the FileLogger object.
             * 
             * Note: The constructor initializes the logger's thread.
             * 
             * @param parameters the parameters for the logger
             * @return the new logger object
             */
            FileLogger(FileLoggerParameters parameters);
            
            /**
             * Destroys the FileLogger object.
             * 
             * Note: The destructor stops the logger's thread.
             */
            ~FileLogger();

            FileLogger() = delete;                                  //No default constructor
            FileLogger(const FileLogger& orig) = delete;            //Copy not allowed (pass/access only by reference/pointer)
            FileLogger& operator=(const FileLogger& orig) = delete; //Copy not allowed (pass/access only by reference/pointer)
            
            /**
             * Logs the supplied message to file.
             * 
             * Note: The current time/date is used for the timestamp.
             * 
             * @param severity event severity
             * @param message event message
             */
            void logMessage(FileLogSeverity severity, std::string message);
            
            /**
             * Logs the supplied message to file.
             * 
             * @param timestamp event time/date
             * @param severity event severity
             * @param message event message
             */
            void logMessage(boost::posix_time::ptime timestamp, FileLogSeverity severity, std::string message);
            
            /** Retrieves the number of processed messages.\n\n@return the number of processed messages  */
            unsigned long getNumberOfProcessedMessages()    const { return processedLogs; }
            /** Retrieves the number of currently pending messages, waiting for processing.\n\n@return the number of pending messages */
            unsigned int getNumberOfPendingMessages()       const { return messages.size(); }
            /** Retrieves the full path to the log file.\n\n@return the log file path */
            std::string getFilePath()                       const { return filePath; }
            /** Retrieves the maximum file size, in bytes. \n\n@return the maximum file size (bytes) */
            unsigned long getMaxFileSize()                  const { return maxFileSize; }
            /** Retrieves the number of times the log file has been rotated (renamed and recreated).\n\n@return the number of log rotations */
            unsigned int getNumberOfLogRotationsDone()      const { return logRotations; }
            /** Retrieves the state of the logger.\n\n@return 'true', if the logger is not stopped and the thread is running */
            bool isLoggerActive()                           const { return (!stopLogger && threadRunning); }

        private:
            //Logger stats
            unsigned long processedLogs = 0;                //# of processed logs
            unsigned int logRotations = 0;                  //# of log file rotations
            
            //Logger management
            //severity type -> state (true -> if messages with that severity are to be logger)
            boost::unordered_map<FileLogSeverity, bool> minSeverityMap; 
            unsigned long maxFileSize;                      //max file size limit (bytes), before a log rotation is triggered
            std::string filePath;                           //full log file path
            std::ofstream fileStream;                       //output stream for the log file
            
            //Thread management
            boost::thread * mainThread;                     //main thread object
            boost::mutex threadMutex;                       //main thread mutex
            boost::condition_variable threadLockCondition;  //thread condition variable
            std::atomic<bool> stopLogger{false};            //atomic variable for denoting the state of the logger
            std::atomic<bool> threadRunning;                //atomic variable for denoting the state of the main thread
            
            //Requests management
            std::queue<std::string> messages;               //list of pending messages
            
            /**
             * Converts the supplied severity to std::string.
             * 
             * @param severity the severity to be converted
             * @return the string representation of the severity
             */
            std::string severityToString(FileLogSeverity severity);
            
            /**
             * Main logger thread.
             * 
             * Note: The log file is managed by the thread (incl. opening, closing and rotating).
             */
            void mainLoggerThread();
    };
}
#endif	/* FILELOGGER_H */

