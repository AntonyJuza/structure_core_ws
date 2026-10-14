/*
    CommandLine.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/
#include "CommandLine.h"
#include "Utility.h"
#include <chrono>
#include <thread>
#include <ctype.h>
#include <cstring>

using CommandLine = GuiSupport::CliSupport::CommandLine;

CommandLine::CommandLine(const std::string appName, const std::string version) : _appName(appName), _version(version) {}

CommandLine::~CommandLine()
{
}

bool CommandLine::findShortOpt(int &curArgc, const int argc, const char **argv, char &key)
{
    bool ret = false;
    if (curArgc <= argc && argv[curArgc])
    {
        /*For now, only take one opt per time*/
        if (argv[curArgc][0] && argv[curArgc][0] == _delimiter)
        {
            if (argv[curArgc][1] && isalpha(argv[curArgc][1]))
            {
                auto it = _optNodeMap.find(argv[curArgc][1]);
                if (_optNodeMap.end() != it)
                {
                    key = argv[curArgc][1];
                    curArgc++;
                    ret = true;
                }
            }
        }
    }
    return ret;
}

bool CommandLine::findLongOpt(int &curArgc, const int argc, const char **argv, char &key)
{
    bool ret = false;
    if (curArgc <= argc && argv[curArgc])
    {
        /*For now, only take one opt per time*/
        if (argv[curArgc][0] && argv[curArgc][0] == _delimiter && argv[curArgc][1] && argv[curArgc][1] == _delimiter)
        {
            std::string s;
            for (int i = 2; i < (int)std::strlen(argv[curArgc]); i++)
            {
                s = s + argv[curArgc][i];
            }
            if(s.size() > 0){
                /*Need to search the whole map. Takes time*/
                for(auto it : _optNodeMap)
                {
                    if(!s.compare(it.second.longOptName))
                    {
                        key = it.first;
                        curArgc++;
                        ret = true;
                    }
                }
            }
        }
    }
    return ret;
}
std::list<std::string> CommandLine::getArgs(int &curArgc, const int argc, const char **argv)
{
    std::list<std::string> retList;
    while (curArgc <= argc && argv[curArgc] && argv[curArgc][0] != _delimiter)
    {
        retList.push_back(argv[curArgc++]);
    }
    return retList;
}

bool CommandLine::findOption(const int argc, const char **argv, GuiSupport::CliSupport::Pair &outP)
{
    bool ret = false;
    int curArgc = 1;
    char key = ' ';
    std::list<std::string> argList;
    if (findShortOpt(curArgc, argc, argv, key) || findLongOpt(curArgc, argc, argv, key))
    {
        argList = getArgs(curArgc, argc, argv);
        outP = Pair(key, argList);
        ret = true;
    }
    return ret;
}

bool CommandLine::addOption(const char optName, const std::string longOptName, const std::string description, const int argNum)
{
    OptNodeT optNode;
    if ((int)_optNodeMap.size() >= OPT_LIST_MAX)
    {
        GuiSupport::log("Failed to add more option! The maximum amount (%d) of OPT table has reached", OPT_LIST_MAX);
        return false;
    }
    optNode.optName = optName;
    optNode.longOptName = longOptName;
    optNode.description = description;
    optNode.argNum = argNum;

    _optNodeMap[optNode.optName] = optNode;
    return true;
}

void CommandLine::usage(void)
{
    GuiSupport::log("%s %s", _appName.c_str(), _version.c_str());
    GuiSupport::log("Usage:\n");
    for (auto it : _optNodeMap)
    {
        char strBuf[128];
        int size = std::snprintf(strBuf, 128, "-%c, --%s \n  %s", it.second.optName,
                                 it.second.longOptName.c_str(), it.second.description.c_str());
        if (size <= 0)
        {
            throw std::runtime_error("Error during string formatting.");
        }
        GuiSupport::log("%s", strBuf);
    }
}

void CommandLine::runUntilProcessEnd(int msec)
{
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    } while (1);
}
