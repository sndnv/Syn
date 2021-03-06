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

#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>
#include <string>
#include <vector>
#include <cstdio>
#include <iosfwd>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include "../main/Utilities/Strings/Common.h"

const std::string getNewDirectoryName()
{
    boost::posix_time::ptime timestamp = boost::posix_time::second_clock::local_time();
    std::string id = Utilities::Strings::toString(boost::uuids::random_generator()());
    
    unsigned int bufferSize = id.size() + 17;
    char stringBuffer[bufferSize];
    snprintf(stringBuffer, bufferSize, "%04u%02u%02u_%02d%02d%02d_%s",
            (unsigned short)timestamp.date().year(), (unsigned short)timestamp.date().month(), (unsigned short)timestamp.date().day(), 
            timestamp.time_of_day().hours(), timestamp.time_of_day().minutes(), timestamp.time_of_day().seconds(), id.c_str());
    
    return std::string(stringBuffer);
}

int main(int argc, char* argv[])
{
    int reps;
    std::string testDirectoryPath;
    std::vector<int> results;
    
    boost::program_options::options_description optDescription("Allowed options");
    optDescription.add_options()
            ("help", "Show help message")
            (
                "test-reps",
                boost::program_options::value<int>(&reps)->default_value(1),
                "Number of times to repeat the tests"
            )
            (
                "output-path",
                boost::program_options::value<std::string>(&testDirectoryPath)->default_value("test_data"),
                "The directory to be used for storing test output"
            );
    
    boost::program_options::variables_map params;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, optDescription), params);
    boost::program_options::notify(params);    
    
    if(params.count("help"))
    {
        std::cout << optDescription << std::endl;
        return 0;
    }
    
    boost::filesystem::path testDirectory(testDirectoryPath);
    if(!boost::filesystem::exists(testDirectory) || !boost::filesystem::is_directory(testDirectory))
    {
        throw std::invalid_argument("Invalid test directory specified.");
    }
    boost::filesystem::current_path(testDirectory);
    
    Catch::Session session;
    
    std::cout << "Starting with [" << reps << "] test repetition(s) in directory [" << testDirectoryPath << "]." << std::endl;
    for(int i = 0; i < reps; i++)
    {
        if(!boost::filesystem::is_empty("./"))
        {
            boost::filesystem::path newDirectory("./" + getNewDirectoryName());
            boost::filesystem::create_directory(newDirectory);

            boost::filesystem::directory_iterator end;
            for (boost::filesystem::directory_iterator itr("./"); itr != end; ++itr)
            {
                boost::filesystem::path current(itr->path());
                if(boost::filesystem::is_regular_file(current))
                {
                    try
                    {
                        boost::filesystem::rename(current, newDirectory / current.filename());
                    }
                    catch(std::exception & e)
                    {
                        std::cout << "Exception encountered while moving test files: [" << e.what() << "]." << std::endl;
                    }
                }
            }
        }

        std::cout << "Running test #" << (i+1) << std::endl;
        int result = session.run();
        std::cout << "Finished test #" << (i+1) << " with return code [" << result << "]" << std::endl;
        results.push_back(result);
    }
    
    for(unsigned int i = 0; i < results.size(); i++)
    {
        std::cout << "Test #" << (i+1) << " result: [" << results[i] << "]" << std::endl;
    }
    
    return 0;
}
