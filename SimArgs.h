#ifndef SIM_ARGS_H
#define SIM_ARGS_H
#include "enum.h"
#include <vector>
#include <string>

class SimArgs {
    public:
        SimArgs();
        EndCondition endCondition;
        unsigned long long iterations;
        int threads;
        int rarityN;
        int rarityD;
        int uniques;
        int count;
        int numRollsPerAttempt;
        int weightFactor;
        bool verboseLogging;
        bool useTargetItems;
        std::string resultsFileName;
        std::vector<std::pair<int, int>> weightings;
        std::vector<int> obtainedItems;
        std::vector<int> targetItems;
        std::vector<std::pair<int, int>> tertiaryRolls;
};

#endif