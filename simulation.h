#include <fstream>
#include <vector>
#include <mutex>
#include <pthread.h>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <locale>
#include <string.h>

static bool checkForZero(std::vector<int> vec);
void* runIteration(void* args);
template<class T>
std::string FmtCmma(T value);
void printHelpMsg();
bool parseArgs(int argc, char* argv[], int &uniques, int& rarityN, int&rarityD, int&threads, int& sims, int& itemCount, std::string& fileName, std::string& uniqueWeighting, std::string& obtainedItems, bool& verboseLogging);