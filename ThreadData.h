#ifndef THREAD_DATA_H
#define THREAD_DATA_H
#include "SimArgs.h"
#include <vector>
#include <memory>
#include <string>
#include <atomic>

class ThreadData {

    public:
    ThreadData();
    ThreadData(const SimArgs&, const std::shared_ptr<std::atomic_ullong>&); 
    std::shared_ptr<std::atomic_ullong> globalProgressCounter;
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> items;
    std::vector<std::pair<int, int>> tertiaryRolls;
    std::vector<int> targetItems;
    bool useTargetItems;
    EndCondition endCondition;
    int rarityN;
    int rarityD;
    int uniques;
    int count;
    int numRollsPerAttempt;
    int weightFactor;
    unsigned long long iterations;
};

#endif