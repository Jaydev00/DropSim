#ifndef THREAD_DATA_H
#define THREAD_DATA_H

class ThreadData {

    public:
    ThreadData();
    ThreadData(SimArgs args, const std::shared_ptr<std::atomic_ullong>& nglobalprogressCounter); 
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