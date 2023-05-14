#ifndef IO_H
#define IO_H
#include <string>
#include "DataStructures.h"

class IOManager{
    public:
        static void printHelpMsg(char *exeName);
        static void printHelpMsg(char *exeName, std::string extraMsg);
        static bool parseArgs(int argc, char *argv[], SimArgs &argsStruct);
        template <class T>
        auto FmtCmma(T value);
    protected:

};

#endif