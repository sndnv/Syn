/**
 * Copyright (C) 2016 https://github.com/sndnv
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

#ifndef TESTLOGGINGSOURCE_H
#define TESTLOGGINGSOURCE_H

namespace Testing
{
    class TestLoggingSource : public EntityManagement_Interfaces::DatabaseLoggingSource
    {
        public:
            void logTestMessage(LogSeverity severity, const std::string & message)
            {
                testHandler(severity, message);
            }

            std::string getSourceName() const override
            {
                return "TEST_LOGGING_SOURCE";
            }

            bool registerLoggingHandler(const std::function<void(Common_Types::LogSeverity, const std::string &)> handler) override
            {
                testHandler = handler;
                return true;
            }

        private:
            std::function<void(Common_Types::LogSeverity, const std::string &)> testHandler;
    };
}

#endif /* TESTLOGGINGSOURCE_H */
