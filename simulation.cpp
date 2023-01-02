#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
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

enum Endcondition {
    Uniques,
    Weight1to1,
    WeightTotal,
    Attempts
};

struct SimArgs {
    Endcondition endCondition = Uniques;
    unsigned long long iterations = 0;
    int threads = 0;
    int rarityN = 0;
    int rarityD = 0;
    int uniques = 0;
    int count = 0;
    int numRollsPerAttempt = 1;
    int weightFactor = 1;
    bool verboseLogging = false;
    bool useTargetItems = false;
    std::string resultsFileName = "";
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> obtainedItems;
    std::vector<int> targetItems;
    std::vector<std::pair<int, int>> tertiaryRolls;
};

struct SimResult {
    unsigned int attempts = 0;
    unsigned int totalUniques = 0;
    unsigned int oneToOneWieght = 0;
    unsigned int totalWeight = 0;
    long double timeTaken = 0.0;
};

struct ThreadData {
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
   public:
    ThreadData(SimArgs args, pthread_mutex_t *nprogressMutex, unsigned long long *nglobalProgress) {
        endCondition = args.endCondition;
        rarityN = args.rarityN;
        rarityD = args.rarityD;
        uniques = args.uniques;
        iterations = args.iterations / args.threads;
        count = args.count;
        numRollsPerAttempt = args.numRollsPerAttempt;
        progressMutex = nprogressMutex;
        globalProgress = nglobalProgress;
        for (int i = 0; i < args.weightings.size(); i++)
            weightings.push_back(args.weightings[i]);
        for (int i = 0; i < args.tertiaryRolls.size(); i++)
            tertiaryRolls.push_back(args.tertiaryRolls[i]);
        for (int i : args.obtainedItems)
            items.push_back(i);
        for (int i : args.targetItems)
            targetItems.push_back(i);
        useTargetItems = args.useTargetItems;
        weightFactor = args.weightFactor;
    }
    pthread_mutex_t *progressMutex;
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> items;
    std::vector<std::pair<int, int>> tertiaryRolls;
    std::vector<int> targetItems;
    bool useTargetItems;
    Endcondition endCondition;
    int rarityN;
    int rarityD;
    int uniques;
    int count;
    int numRollsPerAttempt;
    int weightFactor;
    unsigned long long iterations;
    unsigned long long *globalProgress;
};

struct ReporterThreadData {
   public:
    ReporterThreadData(pthread_mutex_t *nprogressMutex, unsigned long long *nglobalProgress, unsigned long long niterations, std::chrono::high_resolution_clock::time_point *nstartTimePoint) {
        progressMutex = nprogressMutex;
        globalProgress = nglobalProgress;
        iterations = niterations;
        startTimePoint = nstartTimePoint;
    }
    pthread_mutex_t *progressMutex;
    unsigned long long *globalProgress;
    unsigned long long iterations;
    std::chrono::high_resolution_clock::time_point *startTimePoint;
};

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; }    // groups of 3 digit
};

void printHelpMsg(char *exeName) {
    std::cout << "Usage " << exeName << std::endl;
    std::cout << "-h \t\t\t\t: show help" << std::endl;
    std::cout << "-a <filename> \t\t\t: csv of target items gained, should be in same order as weight and include any tertiary drops. " << std::endl;
    std::cout << "-b <factor> \t\t\t: factor by which the weighting of items is multiplied in the weights csv" << std::endl;
    std::cout << "-c <num items>\t\t\t: only simulate until a given number of items are obtained instead of all." << std::endl;
    std::cout << "-f <fileName>\t\t\t: name of file to output simulation results." << std::endl;
    std::cout << "-g <fileName>\t\t\t: File with already obtained items include all items | in the same order as weighting if applicable" << std::endl;
    std::cout << "-l <[1-3]>\t\t\t: set the end condition of the iteration to (1) 1 to 1 weight (2) total weight (3) given attempts. " << std::endl;
    std::cout << "-p <number of rolls>\t\t: number of rolls per attempt" << std::endl;
    std::cout << "-r <rarity N>/<rarity D>\t: (Required) Rarity of items n/d" << std::endl;
    std::cout << "-s <simulations>\t\t: (Required) number of simulations to run." << std::endl;
    std::cout << "-t <threads>\t\t\t: (Required) number of threads to run the simulation on." << std::endl;
    std::cout << "-u <uniques>\t\t\t: (Required) number of unique items to collect." << std::endl;
    std::cout << "-v \t\t\t\t: Verbose output" << std::endl;
    std::cout << "-w <fileName>\t\t\t: name of the file with unique weighting for uniques with different rates" << std::endl;
    std::cout << "-3 <rarity N>/<rarity D>\t: add teritary roll. eg -3 1/100" << std::endl;
}

void printHelpMsg(char *exeName, std::string extraMsg) {
    std::cout << extraMsg << std::endl;
    printHelpMsg(exeName);
}

// get opts
bool parseArgs(int argc, char *argv[], SimArgs &argsStruct) {
    int opt;
    std::string temp;
    std::ifstream infile;
    while ((opt = getopt(argc, argv, "hvs:t:u:r:f:w:c:g:3:p:a:b:l:")) != -1) {
        switch (opt) {
            case 'h':
                printHelpMsg(argv[0]);
                return false;

            case 'a':  // target Items
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing target items end point input");
                    return false;
                }
                infile.open(optarg);
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.targetItems.push_back(stoi(temp));
                }
                argsStruct.useTargetItems = true;
                infile.close();
                break;

            case 'b':  // weight factor
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Weight Factor input");
                    return false;
                }
                argsStruct.weightFactor = std::stoi(optarg);
                break;

            case 'c':  // conditional end point
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                if (optarg)
                    argsStruct.count = std::stoi(optarg);
                break;

            case 'f':  // output file
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing file output input");
                    return false;
                }
                argsStruct.resultsFileName = optarg;
                break;

            case 'g':  // items already gained/starting point csv
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing gained items starting point input");
                    return false;
                }
                infile.open(optarg);
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.obtainedItems.push_back(stoi(temp));
                }
                infile.close();
                break;
            case 'l':  // use end condition 1 to 1 weighting
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                if (argsStruct.endCondition != Uniques) {
                    printHelpMsg(argv[0], "Multiple conflicting end condtitions given");
                    return false;
                } else {
                    switch (atoi(optarg)) {
                        case Weight1to1:
                            argsStruct.endCondition = Weight1to1;
                            break;
                        case WeightTotal:
                            argsStruct.endCondition = WeightTotal;
                            break;
                        case Attempts:
                            argsStruct.endCondition = Attempts;
                            break;
                        default:
                            printHelpMsg(argv[0], "Unkown end condtitions given");
                            return false;
                    }
                }
                break;
            case 'p':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Rolls per attempt input");
                    return false;
                }
                argsStruct.numRollsPerAttempt = std::stoi(optarg);
                break;

            case 'r':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing basic rarity input");
                    return false;
                }
                temp = optarg;
                argsStruct.rarityN = std::stoi(temp.substr(0, temp.find("/")));
                argsStruct.rarityD = std::stoi(temp.substr(temp.find("/") + 1, temp.length()));
                break;
            case 's':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Simulations input");
                    return false;
                }
                argsStruct.iterations = std::stoi(optarg);
                break;
            case 't':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Thread input");
                    return false;
                }
                argsStruct.threads = std::stoi(optarg);
                break;

            case 'u':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Uniques input");
                    return false;
                }
                argsStruct.uniques = std::stoi(optarg);
                break;
            case 'v':
                argsStruct.verboseLogging = true;
                break;

            case 'w':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Weight File input");
                    return false;
                }
                infile.open(optarg);
                for (std::string temp; std::getline(infile, temp);) {
                    argsStruct.weightings.push_back(std::pair<int, int>(
                        stoi(temp.substr(0, temp.find(","))),
                        stoi(temp.substr(temp.find(",") + 1, temp.length()))));
                }
                infile.close();
                break;

            case '3':
                if (optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing tertiary input");
                    return false;
                }
                temp = optarg;
                argsStruct.tertiaryRolls.push_back(std::pair<int, int>(
                    stoi(temp.substr(0, temp.find("/"))),
                    stoi(temp.substr(temp.find("/") + 1, temp.length()))));
                break;
        }
    }
    if (argsStruct.iterations == 0 ||
        argsStruct.threads == 0 ||
        argsStruct.uniques == 0 ||
        argsStruct.rarityN == 0 ||
        argsStruct.rarityD == 0) {
        std::cout << "Missing required input" << std::endl;
        std::cout << "have"
                  << " sims " << argsStruct.iterations
                  << " threads " << argsStruct.threads
                  << " uniques " << argsStruct.uniques
                  << " rairtyN " << argsStruct.rarityN
                  << " rarityD " << argsStruct.rarityD
                  << std::endl;
        printHelpMsg(argv[0]);
        return false;
    }
    return true;
}

template <class T>
std::string FmtCmma(T value) {
    auto thousands = std::make_unique<separate_thousands>();
    std::stringstream ss;
    ss.imbue(std::locale(std::cout.getloc(), thousands.release()));
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

static bool checkForZero(std::vector<int> *vec) {
    for (int i : *vec) {
        if (i == 0)
            return true;
    }
    return false;
}

void *runVanillaNoWeight(void *data) {
    ThreadData *args = ((ThreadData *)data);
    std::vector<int> givenItems = args->items;
    SimResult output;

    int item = 0;
    int roll = 0;
    int itemArraySize = args->uniques + args->tertiaryRolls.size();
    std::vector<int> itemsArray(itemArraySize);
    std::vector<int> targetItemsArray;
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
                if (roll <= args->rarityN) {
                    item = uniqueDistrib(generator);
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
        output.oneToOneWieght = uniquesGained;
        output.totalWeight = output.totalUniques;
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

void *runVanillaWeight(void *data) {
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
void *runIteration(void *data) {
    // parse arg data
    ThreadData *args = ((ThreadData *)data);

    Endcondition localEndCondition = args->endCondition;
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
                    case Endcondition::Uniques:
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

                    case Endcondition::Weight1to1:
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

                    case Endcondition::WeightTotal:
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

                    case Endcondition::Attempts:
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

void *trackProgress(void *data) {
    ReporterThreadData *args = (ReporterThreadData *)data;
    unsigned long long lastReported = 0;
    unsigned long long reportInterval = args->iterations / 1000;
    unsigned long long currentProgress = 0;
    std::string test = "";
    std::cout << "\033[?25l";
    std::cout << std::endl;
    while (lastReported < args->iterations && lastReported + reportInterval < args->iterations) {
        std::this_thread::sleep_for(std::chrono::microseconds(250000));
        std::chrono::high_resolution_clock::time_point tx = std::chrono::high_resolution_clock::now();
        pthread_mutex_lock(args->progressMutex);
        currentProgress = *args->globalProgress;
        pthread_mutex_unlock(args->progressMutex);
        if (currentProgress >= lastReported + reportInterval) {
            long double timestamp = std::chrono::duration_cast<std::chrono::duration<double>>(tx - *args->startTimePoint).count();
            std::cout << "\33[2K\33[A\33[2K\r";  // clear lines
            std::cout << std::fixed << std::setprecision(2) << "Time Elapsed " << std::setw(7) << timestamp << "s. Current progress:" << std::setw(3) << (currentProgress * 100) / args->iterations << "%" << std::setw(0) << " (" << FmtCmma(currentProgress) << "/" << FmtCmma(args->iterations) << ")" << std::endl
                      << std::flush;
            std::cout << "[" << std::flush;
            for (int i = 1; i <= 80; i++) {
                if (i > ((currentProgress * 80) / args->iterations))
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
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]) {
    SimArgs args;
    int progressStep = 10;
    bool verboseLogging = false;
    std::string outputFileName = "";
    std::string uniquesFileName = "";
    std::string obtainedFileName = "";
    if (!parseArgs(argc, argv, args)) {
        return -1;
    }
    // argument parsing
    if (argc <= 1) {
        return 0;
    }
    pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
    unsigned long long iterationProgress = 0;

    std::cout << "Running " << FmtCmma(args.iterations) << " Simulations with " << args.uniques << " slots with " << FmtCmma(args.rarityN) << "/" << FmtCmma(args.rarityD) << " rarity on " << args.threads << " threads." << std::endl;
    if (args.count > 0)
        std::cout << "Stopping after " << args.count << " Items Obtained." << std::endl;
    if (args.numRollsPerAttempt > 1)
        std::cout << "Rolling for Loot " << args.numRollsPerAttempt << " Times per Attempt." << std::endl;
    if (args.weightings.size() > 0)
        std::cout << "Using Weight File" << std::endl;
    if (args.obtainedItems.size() > 0)
        std::cout << "Using Obtained Items File" << std::endl;
    if (args.tertiaryRolls.size() > 0)
        std::cout << "Rolling " << args.tertiaryRolls.size() << " Tertiary Roll(s) per attempt" << std::endl;
    std::cout << std::endl;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<SimResult>> results;
    std::vector<ThreadData *> threadArguments;
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> givenItems;
    std::ifstream inputFile;

    for (int i = 0; i < args.threads; i++)
        threadArguments.push_back(new ThreadData(args, &progressMutex, &iterationProgress));
    for (int i = 0; i < args.iterations % args.threads; i++)
        threadArguments[i]->iterations++;

    pthread_t threadIds[args.threads];
    pthread_t reportThread;
    std::vector<SimResult> *threadResults;
    // create reporter thread
    ReporterThreadData *reporterThreadData = new ReporterThreadData(&progressMutex, &iterationProgress, args.iterations, &t1);
    pthread_create(&reportThread, NULL, &trackProgress, (void *)reporterThreadData);
    // create worker threads
    for (int i = 0; i < args.threads; i++) {
        if(args.endCondition == Uniques)
            if(args.weightings.size())
                pthread_create(&threadIds[i], NULL, &runVanillaWeight, (void *)threadArguments[i]);
            else
                pthread_create(&threadIds[i], NULL, &runVanillaNoWeight, (void *)threadArguments[i]);
        if (verboseLogging)
            std::cout << "Creating thread " << i << " " << threadIds[i] << std::endl;
    }
    for (int i = 0; i < args.threads; i++) {
        if (verboseLogging)
            std::cout << "waiting for thread " << i << " " << threadIds[i] << std::endl;
        pthread_join(threadIds[i], (void **)&threadResults);
        results.push_back(*threadResults);
    }
    free(threadResults);
    for (ThreadData *ptr : threadArguments) {
        delete ptr;
    }
    // end threaded work
    pthread_join(reportThread, NULL);
    delete (reporterThreadData);

    unsigned long long sumAttempts = 0;
    unsigned long long sumItems = 0;
    unsigned long long sum1to1Weight = 0;
    unsigned long long sumTotalWeight = 0;
    unsigned long long highest_attempt = 0;
    unsigned long long highestAttemptItems = 0;
    unsigned long long lowest_attempt = 0;
    unsigned long long lowestAttemptItems = 0;
    long double sumtime = 0.0;
    double averageTime = 0.0;
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    std::cout << std::endl
              << std::endl
              << std::endl
              << "============================================================================================" << std::endl;
    std::cout << "Simulation took " << time_span.count() << " Seconds." << std::endl;

    if (outputFileName != "") {
        std::ofstream outFile;
        outFile.open(outputFileName);
        for (std::vector<SimResult> thread : results) {
            for (SimResult iteration : thread) {
                outFile << iteration.attempts << "," << iteration.totalUniques << "," << iteration.oneToOneWieght << "," << iteration.totalWeight << std::endl;
            }
        }
        outFile.close();
    } else {
        for (std::vector<SimResult> thread : results) {
            for (SimResult iteration : thread) {
                sumAttempts += iteration.attempts;
                sumItems += iteration.totalUniques;
                sum1to1Weight += iteration.oneToOneWieght;
                sumTotalWeight += iteration.totalWeight;

                if (iteration.attempts > highest_attempt) {
                    highest_attempt = iteration.attempts;
                    highestAttemptItems = iteration.totalUniques;
                }
                if (iteration.attempts < lowest_attempt || lowest_attempt == 0) {
                    lowest_attempt = iteration.attempts;
                    lowestAttemptItems = iteration.totalUniques;
                }
            }
        }
    }
    unsigned long long stdSum = 0;
    double standardDev = 0.0;
    if (outputFileName != "") {
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
        std::cout << "Finished Writing to file in " << time_span2.count() << " Seconds." << std::endl;
    }

    // parse Results
    long double totalIterations = 0.0;
    for (std::vector<SimResult> threadResult : results)
        totalIterations += threadResult.size();
    long double averageAttempts = ceil(((sumAttempts * 1.0) / (totalIterations)) * 100.0) / 100.0;
    double averageItems = ceil((sumItems * 1.0) / (totalIterations)*100.0) / 100.0;
    std::string formattedAverageAttempts = FmtCmma(averageAttempts);
    std::string formattedAverageItems = FmtCmma(averageItems);

    for (std::vector<SimResult> thread : results) {
        for (SimResult iteration : thread)
            sumtime += iteration.timeTaken;
    }
    averageTime = sumtime / totalIterations;

    for (std::vector<SimResult> thread : results) {
        for (SimResult iteration : thread)
            stdSum += pow(iteration.attempts - averageAttempts, 2);
    }
    standardDev = sqrt(stdSum / args.iterations);

    // output results
    std::cout << std::setprecision(6) << std::endl;
    std::cout << "Iterations: " << FmtCmma(totalIterations) << std::endl;
    std::cout << "Sum Attempts " << FmtCmma(sumAttempts) << std::endl;
    std::cout << "Average attempts: " << formattedAverageAttempts << ", Average items: " << formattedAverageItems << "." << std::endl;
    std::cout << "Average Time Taken per Sim: " << averageTime << std::endl;
    std::cout << "Attempts Standard Deviation: " << FmtCmma(standardDev) << std::endl;
    std::cout << "highest attempts: " << FmtCmma(highest_attempt) << ", with " << FmtCmma(highestAttemptItems)
              << " items, Lowest attempts " << FmtCmma(lowest_attempt) << ", with " << FmtCmma(lowestAttemptItems) << " items" << std::endl;
}
