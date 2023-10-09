#include "Simulation.h"
#include "ThreadData.h"
#include "IO.h"

#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <random>
#include <thread>
#include <vector>
#include <deque>
#include <future>



    //**DONE
    // handle rolls, basic case 1 roll for 1 item no restrictions
    // ##BasicUniques, BasicRarityN/D, iterations, count
    // case multiple rolls for all items   (clue casket)
    // ##numRollsPerAttempt

    //**INPROGRESS
    // case 1+ basic rolls + tertiary roll (zulrah)
    // ##tertiaryDrops teriarityRate -- multiple rates, one for each drop
    // case multiple rolls with exclusions (barrows)
    // #specific case for barrows


bool Simulation::checkForZero(const std::vector<int>& vec) {
    return *(find(vec.cbegin(), vec.cend(), 0)) == 0;
}

std::deque<std::deque<SimResult>> Simulation::runSims(std::vector<ThreadData> threadArguments,SimArgs args){
    std::deque<std::deque<SimResult>> results;
    std::deque<std::future<std::deque<SimResult>>> threadFutures;
    for (int i = 0; i < args.threads; i++) {
        if(args.endCondition == EndCondition::Uniques)
            if(args.weightings.size()){
                threadFutures.push_back(std::async(&Simulation::runVanillaWeight, this, threadArguments[i]));
            } else{
                threadFutures.push_back(std::async(&Simulation::runVanillaNoWeight, this, threadArguments[i]));
            }
        if(args.endCondition == EndCondition::Weight1to1)
            threadFutures.push_back(std::async(&Simulation::run1to1Weight, this, threadArguments[i]));
        if(args.endCondition == EndCondition::Attempts)
            threadFutures.push_back(std::async(&Simulation::runAttempts, this, threadArguments[i]));
    }
    for (int i = 0; i < args.threads; i++) {
        results.push_back(threadFutures[i].get());
    }
    return results;
}

//TODO modernize comment
std::deque<SimResult> Simulation::runVanillaNoWeight(const ThreadData& args) {
    //setup inital item state
    std::vector<int> givenItems = args.items;
    SimResult output;

    int item = 0;
    int roll = 0;
    int itemArraySize = args.uniques + args.tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;
    if (args.useTargetItems)
        targetItemsArray = args.targetItems;
    int count = args.count;

    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args.iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    std::random_device rd;
    thread_local std::mt19937 generator(rd());
    generator.seed(rd());
    std::uniform_int_distribution<int> chanceDistrib(1, args.rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args.uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::deque<SimResult> simResults;
    for (std::pair<int, int> roll : args.tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }
    //TODO

    // just uniques, no weight file
    for (unsigned long long iteration = 0; iteration < args.iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            args.globalProgressCounter->fetch_add(iteration - iterationsReported); //it's atomic right? shouldn't need a mutex
            iterationsReported = iteration;
        }
        //initalize or reinitalize variables for iteration start
        if (givenItems.size() > 0) {
            itemsArray = givenItems;
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        int uniquesGained = 0;
        bool endConditionMet = false;
        int endUniques = 0;
        if(count)
            endUniques = count;
        else 
            endUniques = args.uniques;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int rarity = args.rarityN;

        //run iteration
        while (!endConditionMet) {
            attempts++;
            //regular loot table rolls
            for (int i = 0; i < args.numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= rarity) {
                    if (itemsArray[roll-1] == 0)
                        uniquesGained++;
                    itemsArray[roll-1]++;
                    endConditionMet = uniquesGained >= endUniques;
                }
            }
            // tertiary rolls
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args.uniques] == 0)
                        uniquesGained++;
                    itemsArray[i + args.uniques]++;
                    endConditionMet = uniquesGained >= endUniques;
                }
            }
        }
        output.attempts = attempts;
        for (int i : itemsArray) {
            output.totalUniques += i;
        }
        int oneToOneWeightTotalSum = 0;
        int totalWeightTotalSum = 0;
        output.oneToOneWieght = uniquesGained;
        output.totalWeight = output.totalUniques;
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults.push_back(output);
    }
    // report last section Data
    args.globalProgressCounter->fetch_add(args.iterations - iterationsReported);
    return simResults;
}



//TODO modernize, comment

std::deque<SimResult> Simulation::runVanillaWeight(const ThreadData& args) {
    //setup inital item state
    std::vector<int> givenItems = args.items;
    SimResult output;

    int item = 0;
    int roll = 0;
    int itemArraySize = args.uniques + args.tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;
    if (args.useTargetItems)
        targetItemsArray = args.targetItems;
    int count = args.count;

    std::vector<int> weightLookupTable;
    for(int i = 0; i < args.weightings.size(); i++){
        if(i == 0)
            for(int j = 0; j < args.weightings[i].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
        else
            for(int j = 0; j < args.weightings[i].first - args.weightings[i-1].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
    }

    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args.iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    std::random_device rd;
    thread_local std::mt19937 generator(rd());
    generator.seed(rd());
    std::uniform_int_distribution<int> chanceDistrib(1, args.rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args.uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::deque<SimResult> simResults;
    for (std::pair<int, int> roll : args.tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }
    // uniques with weight file
    for (unsigned long long iteration = 0; iteration < args.iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            args.globalProgressCounter->fetch_add(iteration - iterationsReported); //it's atomic right? shouldn't need a mutex
            iterationsReported = iteration;
        }
        //initalize or reinitalize variables for iteration start
        if (givenItems.size() > 0) {
            itemsArray = givenItems; //investigate this, might not work as expected
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        int uniquesGained = 0;
        bool endConditionMet = false;
        int endUniques = 0;
        if(count)
            endUniques = count;
        else 
            endUniques = args.uniques;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int rarity = args.rarityN;

        //run iteration
        while (!endConditionMet) {
            attempts++;
            //regular loot table rolls
            for (int i = 0; i < args.numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= rarity) {
                    if (itemsArray[weightLookupTable[roll-1]] == 0)
                        uniquesGained++;
                    itemsArray[weightLookupTable[roll-1]]++;
                    endConditionMet = uniquesGained >= endUniques;
                }
            }
            // tertiary rolls
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args.uniques] == 0)
                        uniquesGained++;
                    itemsArray[i + args.uniques]++;
                    endConditionMet = uniquesGained >= endUniques;
                }
            }
        }
        output.attempts = attempts;
        auto totalUniques = 0;
        for(auto item : itemsArray)
            totalUniques += item;
        output.totalUniques = totalUniques;
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults.push_back(output);
    }
    // report last section Data
    args.globalProgressCounter->fetch_add(args.iterations - iterationsReported);
    return simResults;
}


std::deque<SimResult> Simulation::run1to1Weight(const ThreadData& args) {
    //setup inital item state
    std::vector<int> givenItems = args.items;
    SimResult output;

    int item = 0;
    int roll = 0;
    int itemArraySize = args.uniques + args.tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;
    if (args.useTargetItems)
        targetItemsArray = args.targetItems;
    int count = args.count;

    std::vector<int> weightLookupTable;
    std::vector<int> progressLookupTable;
    for(int i = 0; i < args.weightings.size(); i++){
        if(i == 0){
            progressLookupTable.push_back(args.rarityD/args.weightings[i].first);
            for(int j = 0; j < args.weightings[i].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
        } else {
            progressLookupTable.push_back(args.rarityD/(args.weightings[i].first - args.weightings[i-1].first));
            for(int j = 0; j < args.weightings[i].first - args.weightings[i-1].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
        }
    }
    int totalWeight = 0;
    for(int weight : progressLookupTable){
        totalWeight += weight;
    }
    int startingProgress = 0;
    if(givenItems.size() > 0)
        for(int i = 0; i < givenItems.size(); i++)
            if(givenItems[i] > 0)
                startingProgress += progressLookupTable[i];
    //TODO make this actually work
    for(auto roll : args.tertiaryRolls){
        progressLookupTable.push_back(roll.second);
        totalWeight += roll.second;
    }

    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args.iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    std::random_device rd;
    thread_local std::mt19937 generator(rd());
    generator.seed(rd());
    std::uniform_int_distribution<int> chanceDistrib(1, args.rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args.uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::deque<SimResult> simResults;
    for (std::pair<int, int> roll : args.tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }
    // uniques with weight file
    for (unsigned long long iteration = 0; iteration < args.iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            args.globalProgressCounter->fetch_add(iteration - iterationsReported); //it's atomic right? shouldn't need a mutex
            iterationsReported = iteration;
        }
        //initalize or reinitalize variables for iteration start
        if (givenItems.size() > 0) {
            itemsArray = givenItems; //investigate this, might not work as expected
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        int currentProgress = startingProgress;
        bool endConditionMet = false;
        int endWeight = 0;
        if(count)
            totalWeight = count;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int rarity = args.rarityN;

        //run iteration
        while (!endConditionMet) {
            attempts++;
            //regular loot table rolls
            for (int i = 0; i < args.numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= rarity) {
                    if (itemsArray[weightLookupTable[roll-1]] == 0)
                        currentProgress += progressLookupTable[weightLookupTable[roll-1]];
                    itemsArray[weightLookupTable[roll-1]]++;
                    endConditionMet = currentProgress >= totalWeight;
                }
            }
            // tertiary rolls TODO test
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args.uniques] == 0)
                        currentProgress+= args.tertiaryRolls[i].second;
                    itemsArray[i + args.uniques]++;
                    endConditionMet = currentProgress >= totalWeight;
                }
            }
        }
        output.attempts = attempts;
        output.oneToOneWieght = currentProgress;
        auto totalUniques = 0;
        for(auto item : itemsArray)
            totalUniques += item;
        output.totalUniques = totalUniques;
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults.push_back(output);
    }
    // report last section Data
    args.globalProgressCounter->fetch_add(args.iterations - iterationsReported);
    return simResults;

}

std::deque<SimResult> Simulation::runAttempts(const ThreadData& args) {
     //setup inital item state
    std::vector<int> givenItems = args.items;
    SimResult output;

    int item = 0;
    int roll = 0;
    int itemArraySize = args.uniques + args.tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;
    std::vector<int> weightLookupTable;
    std::vector<int> progressLookupTable;
    if (args.useTargetItems)
        targetItemsArray = args.targetItems;
    int count = args.count;

    //populate lookup tables
    for(int i = 0; i < args.weightings.size(); i++){
        if(i == 0){
            progressLookupTable.push_back(args.rarityD/args.weightings[i].first);
            for(int j = 0; j < args.weightings[i].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
        } else {
            progressLookupTable.push_back(args.rarityD/(args.weightings[i].first - args.weightings[i-1].first));
            for(int j = 0; j < args.weightings[i].first - args.weightings[i-1].first; j++)
                weightLookupTable.push_back(args.weightings[i].second);
        }
    }

    //initalize progress variables
    int totalWeight = 0;
    for(int weight : progressLookupTable){
        totalWeight += weight;
    }
    int startingProgress = 0;
    if(givenItems.size() > 0)
        for(int i = 0; i < givenItems.size(); i++)
            if(givenItems[i] > 0)
                startingProgress += progressLookupTable[i];
    //TODO make this actually work
    for(auto roll : args.tertiaryRolls){
        progressLookupTable.push_back(roll.second);
        totalWeight += roll.second;
    }


    //declare sim variables
    unsigned long long attempts = 0;
    unsigned long long totalattempts =0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args.iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;

    //initalize rng
    std::random_device rd;
    thread_local std::mt19937 generator(rd());
    generator.seed(rd());
    std::uniform_int_distribution<int> chanceDistrib(1, args.rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args.uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::deque<SimResult> simResults;
    for (std::pair<int, int> roll : args.tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }
    // uniques with weight file
    for (unsigned long long iteration = 0; iteration < args.iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            args.globalProgressCounter->fetch_add(iteration - iterationsReported); //it's atomic right? shouldn't need a mutex
            iterationsReported = iteration;
        }
        //initalize or reinitalize variables for iteration start
        if (givenItems.size() > 0) {
            itemsArray = givenItems; //investigate this, might not work as expected
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        output.oneToOneUniques = 0;
        attempts = 0;
        int currentProgress = startingProgress;
        bool endConditionMet = false;
        int endWeight = 0;
        if(count)
            totalattempts = args.count;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int rarity = args.rarityN;

        //run iteration
        for(int i = 0; i < totalattempts; i++) {
            attempts++;
            //regular loot table rolls
            for (int i = 0; i < args.numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= rarity) {
                    if (itemsArray[weightLookupTable[roll-1]] == 0)
                        currentProgress += progressLookupTable[weightLookupTable[roll-1]];
                    itemsArray[weightLookupTable[roll-1]]++;
                }
            }
            // tertiary rolls TODO test
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args.uniques] == 0)
                        currentProgress+= args.tertiaryRolls[i].second;
                    itemsArray[i + args.uniques]++;
                }
            }
        }
        output.attempts = attempts;
        output.oneToOneWieght = currentProgress;
        for(auto item : itemsArray){
            output.totalUniques += item;
            if(!item == 0)
                output.oneToOneUniques++;
        }
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults.push_back(output);
    }
    // report last section Data
    args.globalProgressCounter->fetch_add(args.iterations - iterationsReported);
    return simResults;
}
/*
void *runBarrows(void *data) {
    ThreadData *args = ((ThreadData *)data);
}
*/


//TODO figure out how to IO manage this
void Simulation::trackProgress(const ReporterThreadData& args) { 
    unsigned long long lastReported = 0;
    unsigned long long reportInterval = args.iterations / 1000;
    unsigned long long currentProgress = 0;
    std::string test = "";
    std::cout << "\033[?25l";
    std::cout << std::endl;
    while (lastReported < args.iterations && lastReported + reportInterval < args.iterations) {
        std::this_thread::sleep_for(std::chrono::microseconds(250000));
        std::chrono::high_resolution_clock::time_point tx = std::chrono::high_resolution_clock::now();
        currentProgress = args.globalprogressCounter->load();
        if (currentProgress >= lastReported + reportInterval) {
            long double timestamp = std::chrono::duration_cast<std::chrono::duration<double>>(tx - args.startTimePoint).count();
            std::cout << "\33[2K\33[A\33[2K\r";  // clear lines
            std::cout << std::fixed << std::setprecision(2) << "Time Elapsed " << std::setw(7) << timestamp << "s. Current progress:" << std::setw(3) << (currentProgress * 100) / args.iterations << "%" << std::setw(0) << " (" << IOUtils::FmtCmma(currentProgress) << "/" << IOUtils::FmtCmma(args.iterations) << ")" << std::endl
                      << std::flush;
            for (int i = 1; i <= 80; i++) {
                if (i > ((currentProgress * 80) / args.iterations))
                    std::cout << "." << std::flush;
                else
                    std::cout << "\u2588" << std::flush;
            }
            lastReported = currentProgress;
        }
    }
    std::cout << "\033[?25h";
    std::cout << std::endl;
}

