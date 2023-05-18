#ifndef IO_H
#define IO_H
#include <string>
#include <iostream>
#include <iomanip>
#include "SimArgs.h"
#include "enum.h"

class IOUtils{
    public:
        static void printHelpMsg(char *exeName);
        static void printHelpMsg(char *exeName, const std::string& extraMsg);
        static bool parseArgs(const int& argc, char *argv[], SimArgs& argsStruct);
        template <class T>
        static std::string FmtCmma(T value);
        static void printStartParameters(const SimArgs& args);
    protected:

};

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; }    // groups of 3 digit
};

template <class T>
std::string IOUtils::FmtCmma(T value) {
    auto thousands = std::make_unique<separate_thousands>();
    std::stringstream ss;
    ss.imbue(std::locale(std::cout.getloc(), thousands.release()));
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
};

#endif