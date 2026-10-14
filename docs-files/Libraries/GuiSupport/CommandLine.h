/*
    CommandLine.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <list>
#include <map>

namespace GuiSupport{
    namespace CliSupport{
        struct OptNode
        {
            char optName;
            std::string longOptName;
            std::string description;
            int argNum;
        };
        using OptNodeT = struct OptNode;
        using Pair = std::pair<char,std::list<std::string>>;

        class CommandLine
        {
        private:
            const char _delimiter = '-';
            const int OPT_LIST_MAX = 20;
            std::string _appName;
            std::string _version;
            std::map<char, OptNodeT> _optNodeMap;
            bool findShortOpt(int &curArgc, const int argc, const char **argv, char &key);
            bool findLongOpt(int &curArgc, const int argc, const char **argv, char &key);
            std::list<std::string> getArgs(int &curArgc, const int argc, const char **argv);

        public:
            CommandLine(const std::string appName, const std::string version);
            virtual ~CommandLine();
            CommandLine& operator=(const CommandLine &t) {_appName = t._appName; _version = t._version; _optNodeMap = t._optNodeMap; return *this;}
            
            /**
                Add option into the opt table

                @param optName the short option name in single char.
                @param longOptName the long option name.
                @param description the description of this option.
                @param argNum the amount of arguments.
                @return Success or Fail to add option
            */
            bool addOption(const char optName, const std::string longOptName, const std::string description, const int argNum);

            /**
                Find option one by one in the opt table

                @param argc the number of arguments
                @param argv the vector of arguments
                @param outP OUPUT: the pair of short opt name and arg list
                @return Success or Fail to find option
            */
            bool findOption(const int argc,const char **argv, Pair &outP);
            
            /**
                Print out the usage of opt table.
                @return void
            */
            void usage();
            /**
                Keep the loop until the CLI process finshed.

                @param msec Milliseconds to sleep on this thread.
                @return void
            */
            virtual void runUntilProcessEnd(int msec);
        };
    }
}
