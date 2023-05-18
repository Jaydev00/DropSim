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
#include <mutex>
#include <random>
#include <thread>
#include <vector>
#include <deque>

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

//TODO modernize comment
std::deque<SimResult> Simulation::runVanillaNoWeight(const ThreadData& args) {
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
    static thread_local std::mt19937 generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> chanceDistrib(1, args.rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args.uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::deque<SimResult> simResults;
    for (std::pair<int, int> roll : args.tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }

    // just uniques, no weight file
    for (unsigned long long iteration = 0; iteration < args.iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            args.globalProgressCounter->store(args.globalProgressCounter->load() + (iteration - iterationsReported)); //it's atomic right? shouldn't need a mutex
            iterationsReported = iteration;
        }
        // setup sim
        // set up starting point
        if (givenItems.size() > 0) {
            itemsArray = givenItems;
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        // set up endpoint

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        int uniquesGained = 0;
        bool endConditionMet = false;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        while (!endConditionMet) {
            attempts++;
            // regular table rolls
            for (int i = 0; i < args.numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= args.rarityN) {
                    item = uniqueDistrib(generator);
                    if (itemsArray[item] == 0)
                        uniquesGained++;
                    itemsArray[item]++;
                    if (count)
                        endConditionMet = uniquesGained >= count;
                    else
                        endConditionMet = uniquesGained >= args.uniques;
                }
            }
            // tertiary rolls
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args.uniques] == 0)
                        uniquesGained++;
                    itemsArray[i + args.uniques]++;
                    if (count) {
                        endConditionMet = uniquesGained >= count;
                    } else {
                        endConditionMet = uniquesGained >= args.uniques;
                    }
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
    args.globalProgressCounter->store(args.globalProgressCounter->load() + (args.iterations - iterationsReported));
    return simResults;
}

//TODO modernize, comment

std::deque<SimResult> Simulation::runVanillaWeight(const ThreadData& args) {
        std::deque<SimResult> result;
    return result;
    //TODO
    /*
    // parse arg data
    ThreadData *args = ((ThreadData *)data);
    std::vector<int> givenItems = args->items;
    std::vector<std::pair<int, int>> weightings = args->weightings;
    std::vector<int> staticWeights;
    for(int i = 0; i < weightings.size(); i++){
        if(i == 0)
            for(int j = 0; j < weightings[i].first; j++)
                staticWeights.push_back(weightings[i].second);
        else
            for(int j = 0; j < weightings[i].first - weightings[i-1].first; j++)
                staticWeights.push_back(weightings[i].second);
    }

    // init local variables
    // output
    SimResult output;
    // interal sim
    int missingUniques = 0;
    int item = 0;
    int roll = 0;
    int tertiaryTotalWeight = 0;
    int itemArraySize = args->uniques + args->tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;

    // initalize ending variables. by default the run will go until all uniques are collected
    if (args->useTargetItems)
        targetItemsArray = args->targetItems;
    int count = args->count;

    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args->iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    static thread_local std::mt19937 generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> chanceDistrib(0, args->rarityD - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::vector<SimResult> *simResults = new std::vector<SimResult>();
    simResults->reserve(args->iterations);

    for (std::pair<int, int> roll : args->tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }

    // just uniques, no weight file
    for (unsigned long long iteration = 0; iteration < args->iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            pthread_mutex_lock(args->progressMutex);
            *(args->globalProgress) += (iteration - iterationsReported);
            pthread_mutex_unlock(args->progressMutex);
            iterationsReported = iteration;
        }
        // setup sim
        // set up starting point
        if (givenItems.size() > 0) {
            itemsArray = givenItems;
        } else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);

        // set up endpoint

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        int uniquesGained = 0;
        bool endConditionMet = false;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        while (!endConditionMet) {
            attempts++;
            // regular table rolls
            for (int i = 0; i < args->numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll < args->rarityN) {
                    // translate roll into item based on weighting
                    item = staticWeights[roll];
                    if (itemsArray[item] == 0)
                        uniquesGained++;
                    itemsArray[item]++;
                    if (count)
                        endConditionMet = uniquesGained >= count;
                    else
                        endConditionMet = uniquesGained >= args->uniques;
                }
            }
            // tertiary rolls
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1) {
                    if (itemsArray[i + args->uniques] == 0)
                        uniquesGained++;
                    itemsArray[i + args->uniques]++;
                    if (count) {
                        endConditionMet = uniquesGained >= count;
                    } else {
                        endConditionMet = uniquesGained >= args->uniques;
                    }
                }
            }
        }
        output.attempts = attempts;
        for (int i : itemsArray) {
            output.totalUniques += i;
        }
        int oneToOneWeightTotalSum = 0;
        int totalWeightTotalSum = 0;
        for (int i = 0; i < args->uniques - 1; i++)
            if (itemsArray[i] > 0) {
                if (i == 0) {
                    oneToOneWeightTotalSum += weightings[i].first;
                    totalWeightTotalSum += weightings[i].first * itemsArray[i];
                } else {
                    oneToOneWeightTotalSum += (weightings[i].first - weightings[i - 1].first);
                    totalWeightTotalSum += (weightings[i].first - weightings[i - 1].first) * itemsArray[i];
                }
            }
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults->push_back(output);
    }
    // report last section Data
    pthread_mutex_lock(args->progressMutex);
    *(args->globalProgress) += args->iterations - iterationsReported;
    pthread_mutex_unlock(args->progressMutex);
    pthread_exit((void *)simResults);
    */
}

/*
void *runWeight(void *data) {
    ThreadData *args = ((ThreadData *)data);
}
void *runAttempts(void *data) {
    ThreadData *args = ((ThreadData *)data);
}
void *runBarrows(void *data) {
    ThreadData *args = ((ThreadData *)data);
}
*/

//todo delete after implementing the rest of the cases
/*
void* Simulation::runIteration(void *data) {
    // parse arg data
    ThreadData *args = ((ThreadData *)data);

    EndCondition localEndCondition = args->endCondition;
    std::vector<int> givenItems = args->items;
    std::vector<std::pair<int, int>> weightings = args->weightings;

    // init local variables
    // output
    SimResult output;
    // interal sim
    int missingUniques = 0;
    int item = 0;
    int roll = 0;
    int tertiaryTotalWeight = 0;
    int itemArraySize = args->uniques + args->tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;

    // initalize ending variables. by default the run will go until all uniques are collected
    if (args->useTargetItems)
        targetItemsArray = args->targetItems;
    int count = args->count;
    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args->iterations / 100);
    if (!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    static thread_local std::mt19937 generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> chanceDistrib(1, args->rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0, args->uniques - 1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::vector<SimResult> *simResults = new std::vector<SimResult>();

    for (std::pair<int, int> roll : args->tertiaryRolls) {
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first, roll.second));
    }

    // loop start

    // handle rolls, basic case 1 roll 1 item no restrictions
    // case multiple rolls for all items   (clue casket)
    // case 1+ basic rolls + tertiary roll (zulrah)
    // case multiple rolls with exclusions (barrows)
    for (unsigned long long iteration = 0; iteration < args->iterations; iteration++) {
        if (iteration > 0 && iteration % reportIncrement == 0) {  // add progress
            pthread_mutex_lock(args->progressMutex);
            *(args->globalProgress) += (iteration - iterationsReported);
            pthread_mutex_unlock(args->progressMutex);
            iterationsReported = iteration;
        }
        // setup sim
        // set up starting point
        if (givenItems.size() > 0)
                itemsArray = givenItems;
        else
            std::fill(itemsArray.begin(), itemsArray.end(), 0);
        // set up endpoint

        output.attempts = 0;
        output.totalUniques = 0;
        output.oneToOneWieght = 0;
        output.totalWeight = 0;
        attempts = 0;
        bool endConditionMet = false;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        while (!endConditionMet) {
            attempts++;
            // regular table rolls
            for (int i = 0; i < args->numRollsPerAttempt; i++) {
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= args->rarityN) {
                    if (weightings.size() > 0) {
                        // translate roll into item based on weighting
                        for (int i = 0; i < weightings.size(); i++) {
                            if (roll > weightings[i].first)
                                continue;
                            item = weightings[i].second;
                            break;
                        }
                    } else
                        item = uniqueDistrib(generator);
                    itemsArray[item]++;

                }
            }
            // tertiary rolls
            for (int i = 0; i < tertiaryDistrubtions.size(); i++) {
                int temp = tertiaryDistrubtions[i](generator);
                if (temp == 1)
                    itemsArray[i + args->uniques]++;
            }
            // end condition checking
            int oneToOneWeightTotal = 0;
            int totalWeightTotal = 0;
            if (count) {
                switch (localEndCondition) {
                    case EndCondition::Uniques:
                        if (args->useTargetItems) {
                            bool done = true;
                            for (int i = 0; i < itemsArray.size(); i++)
                                if (itemsArray[i] < targetItemsArray[i]) {
                                    done = false;
                                }
                            endConditionMet = done;
                        } else if (itemsArray.size() - std::count(itemsArray.begin(), itemsArray.end(), 0) > count)
                            endConditionMet = true;
                        break;

                    case EndCondition::Weight1to1:
                        oneToOneWeightTotal = 0;
                        for (int i = 0; i < args->uniques - 1; i++)
                            if (itemsArray[i] > 0) {
                                if (i == 0)
                                    oneToOneWeightTotal += weightings[i].first;
                                else
                                    oneToOneWeightTotal += (weightings[i].first - weightings[i - 1].first);
                            }
                        // tertiary weight
                        for (int i = args->uniques; i < itemsArray.size(); i++)
                            if (itemsArray[i] > 0)
                                oneToOneWeightTotal += (args->tertiaryRolls[i - args->uniques].second) * args->weightFactor * args->numRollsPerAttempt;
                        if (oneToOneWeightTotal >= count)
                            endConditionMet = true;
                        break;

                    case EndCondition::WeightTotal:
                        totalWeightTotal = 0;
                        for (int i = 0; i < args->uniques - 1; i++)
                            if (itemsArray[i] > 0) {
                                if (i == 0)
                                    totalWeightTotal += weightings[i].first * itemsArray[i];
                                else
                                    totalWeightTotal += (weightings[i].first - weightings[i - 1].first) * itemsArray[i];
                            }
                        // tertiary weight
                        for (int i = args->uniques; i < itemsArray.size(); i++)
                            if (itemsArray[i] > 0)
                                totalWeightTotal += (args->tertiaryRolls[i - args->uniques].second) * args->weightFactor * args->numRollsPerAttempt * itemsArray[i];
                        if (totalWeightTotal >= count)
                            endConditionMet = true;
                        break;

                    case EndCondition::Attempts:
                        if (attempts >= count)
                            endConditionMet = true;
                        break;
                    default:
                        break;
                }
            } else {  // if (std::count(itemsArray.begin(), itemsArray.end(), 0) == 0)
                endConditionMet = true;
                for (auto &val : itemsArray)
                    if (val == 0)
                        endConditionMet = false;
            }
        }

        output.attempts = attempts;
        for (int i : itemsArray) {
            output.totalUniques += i;
        }
        int oneToOneWeightTotalSum = 0;
        int totalWeightTotalSum = 0;
        for (int i = 0; i < args->uniques - 1; i++)
            if (itemsArray[i] > 0) {
                if (i == 0) {
                    if (weightings.size() > 0) {
                        oneToOneWeightTotalSum += weightings[i].first;
                        totalWeightTotalSum += weightings[i].first * itemsArray[i];
                    } else {
                        oneToOneWeightTotalSum++;
                        totalWeightTotalSum++;
                    }
                } else {
                    if (weightings.size() > 0) {
                        oneToOneWeightTotalSum += (weightings[i].first - weightings[i - 1].first);
                        totalWeightTotalSum += (weightings[i].first - weightings[i - 1].first) * itemsArray[i];
                    } else {
                        oneToOneWeightTotalSum++;
                        totalWeightTotalSum += itemsArray[i];
                    }
                }
            }
        // tertiary weight
        for (int i = args->uniques; i < itemsArray.size(); i++)
            if (itemsArray[i] > 0) {
                oneToOneWeightTotalSum += (args->tertiaryRolls[i - args->uniques].second) * args->weightFactor * args->numRollsPerAttempt;
                totalWeightTotalSum += (args->tertiaryRolls[i - args->uniques].second) * args->weightFactor * args->numRollsPerAttempt * itemsArray[i];
            }
        output.oneToOneWieght = oneToOneWeightTotalSum;
        output.totalWeight = totalWeightTotalSum;
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        output.timeTaken = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        simResults->push_back(output);
    }
    // report last section Data
    pthread_mutex_lock(args->progressMutex);
    *(args->globalProgress) += args->iterations - iterationsReported;
    pthread_mutex_unlock(args->progressMutex);
    pthread_exit((void *)simResults);
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
            std::cout << "[" << std::flush;
            for (int i = 1; i <= 80; i++) {
                if (i > ((currentProgress * 80) / args.iterations))
                    std::cout << " " << std::flush;
                else
                    std::cout << "\u2588" << std::flush;
            }
            std::cout << "]" << std::flush;
            lastReported = currentProgress;
        }
    }
    std::cout << "\033[?25h";
    std::cout << std::endl;
}

