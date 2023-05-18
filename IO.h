#ifndef IO_H
#define IO_H
#include <string>
#include "SimArgs.h"

class IOUtils{
    public:
        static void printHelpMsg(char *exeName);
        static void printHelpMsg(char *exeName, const std::string& extraMsg);
        static bool parseArgs(const int& argc, char *argv[], SimArgs& argsStruct);
        template <class T>
        static auto FmtCmma(T value);
        static void printStartParameters(const SimArgs& args);
    protected:

};

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; }    // groups of 3 digit
};
#endif