#include <fstream>
#include <vector>
#include <mutex>
#include <pthread.h>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <locale>
#include <string>

static bool checkForZero(std::vector<int> vec);
void* runIteration(void* args);
template<class T>
std::string FmtCmma(T value);