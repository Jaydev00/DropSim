#include <thread>
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
#include <cstring>
#include <unistd.h>

enum Endcondition {
    Uniques,
    Weight1to1,
    WeightTotal,
    Attempts 
};

struct SimArgs{
    Endcondition endCondition = Uniques;
    unsigned long long iterations = 0;
    int threads = 0;
    int rarityN = 0;
    int rarityD = 0;
    int uniques = 0;
    int count = 0;
    int numRollsPerAttempt = 1;
    bool verboseLogging = false;
    std::string resultsFileName = "";
    std::vector<std::pair<int,int>> weightings;
    std::vector<int> obtainedItems;
    std::vector<std::pair<int,int>> tertiaryRolls;
};

struct ThreadData
{
    //**DONE
    //handle rolls, basic case 1 roll for 1 item no restrictions 
    //##BasicUniques, BasicRarityN/D, iterations, count
    //case multiple rolls for all items   (clue casket)
    //##numRollsPerAttempt

    //**INPROGRESS
    //case 1+ basic rolls + tertiary roll (zulrah)
    //##tertiaryDrops teriarityRate -- multiple rates, one for each drop
    //case multiple rolls with exclusions (barrows)
    //#specific case for barrows
    public:
        ThreadData(SimArgs args, pthread_mutex_t* nprogressMutex, unsigned long long* nglobalProgress){
            endCondition = args.endCondition;
            rarityN = args.rarityN;
            rarityD = args.rarityD;
            uniques = args.uniques;
            iterations = args.iterations/args.threads;
            count = args.count;
            numRollsPerAttempt = args.numRollsPerAttempt;
            progressMutex = nprogressMutex;
            globalProgress = nglobalProgress;
            for(int i = 0; i < args.weightings.size(); i++)
                weightings.push_back(args.weightings[i]);
            for(int i = 0; i < args.tertiaryRolls.size(); i++)
                tertiaryRolls.push_back(args.tertiaryRolls[i]);
            for(int i : args.obtainedItems)
                items.push_back(i);
        }
        pthread_mutex_t* progressMutex;
        std::vector<std::pair<int,int>> weightings;
        std::vector<int> items;
        std::vector<std::pair<int,int>> tertiaryRolls;
        Endcondition endCondition;
        int rarityN;
        int rarityD;
        int uniques;
        int count;
        int numRollsPerAttempt;
        unsigned long long iterations;
        unsigned long long* globalProgress;
};

struct ReporterThreadData {
    public:
        ReporterThreadData(pthread_mutex_t* nprogressMutex, unsigned long long* nglobalProgress, unsigned long long niterations, std::chrono::high_resolution_clock::time_point* nstartTimePoint){
            progressMutex = nprogressMutex;
            globalProgress = nglobalProgress;
            iterations = niterations;
            startTimePoint = nstartTimePoint;
        }
    pthread_mutex_t* progressMutex;
    unsigned long long* globalProgress;
    unsigned long long iterations;
    std::chrono::high_resolution_clock::time_point* startTimePoint;

};

void printHelpMsg(char * exeName){
    std::cout << "Usage " << exeName << std::endl;
    std::cout << "-h \t\t\t\t: show help" << std::endl;
    std::cout << "-s <simulations>\t\t: (Required) number of simulations to run." << std::endl;
    std::cout << "-t <threads>\t\t\t: (Required) number of threads to run the simulation on." << std::endl;
    std::cout << "-u <uniques>\t\t\t: (Required) number of unique items to collect." << std::endl;
    std::cout << "-r <rarity N>/<rarity D>\t: (Required) Rarity of items n/d" << std::endl;
    std::cout << "-f <fileName>\t\t\t: name of file to output simulation results." << std::endl;
    std::cout << "-w <fileName>\t\t\t: name of the file with unique weighting for uniques with different rates" << std::endl;
    std::cout << "-c <num items>\t\t\t: only simulate until a given number of items are obtained instead of all." << std::endl;
    std::cout << "-g <fileName>\t\t\t: File with already obtained items include all items | in the same order as weighting if applicable" << std::endl;
    std::cout << "-l <[1-3]>\t\t\t: set the end condition of the iteration to (1) 1 to 1 weight (2) total weight (3) given attempts. " << std::endl;
    std::cout << "-v \t\t\t\t: Verbose output" << std::endl;

}

void printHelpMsg(char * exeName, std::string extraMsg){
    std::cout << extraMsg << std::endl;
    printHelpMsg(exeName);
}

//get opts
bool parseArgs(int argc, char* argv[], SimArgs &argsStruct){
    int opt;
    std::string temp;
    std::ifstream infile;
    while((opt = getopt(argc, argv, "hvs:t:u:r:f:w:c:g:3:p:")) != -1){
        switch(opt) {
            case 'h':
                printHelpMsg(argv[0]);
                return false;
            case 's':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Simulations input");
                    return false;
                }
                argsStruct.iterations = std::stoi(optarg);
                break;

            case 't':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Thread input");
                    return false;
                }
                argsStruct.threads = std::stoi(optarg);
                break;

            case 'u':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Uniques input");
                    return false;
                }
                argsStruct.uniques = std::stoi(optarg);
                break;

            case 'r':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing basic rarity input");
                    return false;
                }
                temp = optarg;
                argsStruct.rarityN = std::stoi(temp.substr(0,temp.find("/")));
                argsStruct.rarityD = std::stoi(temp.substr(temp.find("/") + 1, temp.length()));
                break;
            case 'f':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing file output input");
                    return false;
                }
                argsStruct.resultsFileName = optarg;
                break;
            case 'w':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Weight File input");
                    return false;
                }
                infile.open(optarg);
                    for(std::string temp; std::getline(infile, temp);){
    
                        argsStruct.weightings.push_back(std::pair<int,int>(
                            stoi(temp.substr(0,temp.find(","))),
                            stoi(temp.substr(temp.find(",") + 1, temp.length()))
                            ));
                    }
                infile.close();
                break;
            case 'c':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                argsStruct.count = std::stoi(optarg);
                break;   
            case 'g':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Simulations input");
                    return false;
                }
                infile.open(optarg);
                       for(std::string temp; std::getline(infile, temp);){
                            argsStruct.obtainedItems.push_back(stoi(temp));
                        }
                    
                infile.close();
                break; 
            case '3':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing tertiary input");
                    return false;
                }
                temp = optarg;
                argsStruct.tertiaryRolls.push_back(std::pair<int,int>(
                    stoi(temp.substr(0,temp.find("/"))), 
                    stoi(temp.substr(temp.find("/") + 1, temp.length()))));
                break;
            case 'p':
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing Rolls per attempt input");
                    return false;
                }
                argsStruct.numRollsPerAttempt = std::stoi(optarg);
                break;     
            case 'v':
                argsStruct.verboseLogging = true;
                break;
            case 'l': //use end condition 1 to 1 weighting
                if(optarg == NULL || strlen(optarg) <= 0) {
                    printHelpMsg(argv[0], "Missing end condition input");
                    return false;
                }
                if(argsStruct.endCondition != Uniques)
                    printHelpMsg(argv[0], "Multiple conflicting end condtitions given");
                    return false;
                switch(atoi(optarg)){
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

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; } // groups of 3 digit
};


template<class T>
std::string FmtCmma(T value)
{
    auto thousands = std::make_unique<separate_thousands>();
    std::stringstream ss;
    ss.imbue(std::locale(std::cout.getloc(), thousands.release()));
    ss << value;
    return ss.str();
}


static bool checkForZero(std::vector<int>* vec){
    for(int i : *vec){
        if(i == 0)
            return true;
    }
    return false;
}

void* runIteration(void* data){
    
    //parse arg data
    ThreadData* args = ((ThreadData*) data);
    int count = 0;
    count = (args->count == 0 ? args->uniques : args->count);
    count += args->tertiaryRolls.size();
    std::vector<int> givenItems = args->items;
    std::vector<std::pair<int,int>> weightings = args->weightings;
    Endcondition localEndCondition = args->endCondition;

    //init local variables
    std::pair<unsigned long long, unsigned long long> output;
    int missingUniques = 0;
    int item = 0;
    int roll = 0;
    int itemsArray[args->uniques + args->tertiaryRolls.size()];
    unsigned long long attempts = 0;
    unsigned long long progress = 0;
    unsigned long long reportIncrement = (args->iterations/100);
    if(!reportIncrement > 0)
        reportIncrement = 1;
    unsigned long iterationsReported = 0;
    static thread_local std::mt19937 generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> chanceDistrib(1,args->rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0,args->uniques-1);
    std::vector<std::uniform_int_distribution<int>> tertiaryDistrubtions;
    std::vector<std::pair<unsigned long long, unsigned long long>>* simResults = new std::vector<std::pair<unsigned long long, unsigned long long>>();
    simResults->reserve(args->iterations);
    
    for(std::pair<int,int> roll : args->tertiaryRolls){
        tertiaryDistrubtions.push_back(std::uniform_int_distribution<int>(roll.first,roll.second));
    }

    //loop start

    //handle rolls, basic case 1 roll 1 item no restrictions
    //case multiple rolls for all items   (clue casket)
    //case 1+ basic rolls + tertiary roll (zulrah)
    //case multiple rolls with exclusions (barrows)
    for(unsigned long long iteration = 0; iteration < args->iterations; iteration++){
        if(iteration > 0 && iteration % reportIncrement == 0){ // add progress
            pthread_mutex_lock(args->progressMutex);
            *(args->globalProgress) += (iteration - iterationsReported);
            pthread_mutex_unlock(args->progressMutex);
            iterationsReported = iteration;
        }
        //setup sim
        if(givenItems.size() > 0){
            for(int i = 0; i < args->uniques+args->tertiaryRolls.size() && i < givenItems.size(); i++){
                itemsArray[i] = givenItems[i];
            }
        }
        else
            for(int i = 0; i < args->uniques + args->tertiaryRolls.size(); i++){
                itemsArray[i] = 0;
        }
        output.first = 0;
        output.second = 0;
        attempts = 0;
        bool endConditionNotMet = true;    
        while (endConditionNotMet){
            for(int i = 0; i < args->numRollsPerAttempt; i++){
                item = 0;
                roll = chanceDistrib(generator);
                if (roll <= args->rarityN){
                    if(weightings.size() > 0){
                        //translate roll into item based on weighting
                        for(int i = 0; i < weightings.size(); i++){
                            if(roll > weightings[i].first)
                                continue;    
                            item = weightings[i].second;
                            break;
                        }
                    } else
                        item = uniqueDistrib(generator);
                    itemsArray[item]++;
                    //End condition with no teriaries
                    switch(localEndCondition){
                    case Uniques:
                        missingUniques = 0;
                        if(tertiaryDistrubtions.size() <= 0){
                            for(int i = 0; i < args->uniques; i++){
                                if(itemsArray[i] == 0)
                                    missingUniques++;
                            }
                            if(args->uniques - missingUniques >= count)
                                endConditionNotMet = false;
                        }
                        break;
                    case Weight1to1:
                        endConditionNotMet = false;
                        break;
                    case WeightTotal:
                        endConditionNotMet = false;
                        break;
                    case Attempts:
                        endConditionNotMet = false;
                        break;
                    }
                }
            }
            //tertiary rolls
            for(int i = 0; i < tertiaryDistrubtions.size(); i++){
                int temp = tertiaryDistrubtions[i](generator);
                if(temp == 1)
                    itemsArray[i + args->uniques]++;
            }

            //end condition with tertiary rolls
            if(tertiaryDistrubtions.size() > 0){
                missingUniques = 0;
                for(int i = 0; i < args->uniques + args->tertiaryRolls.size(); i++){
                    if(itemsArray[i] == 0)
                        missingUniques++;
                }
                if(args->uniques + args->tertiaryRolls.size() - missingUniques >= count)
                    endConditionNotMet = false;
            }
            attempts++;
        }

        output.first = attempts;
        for(int i : itemsArray){
            output.second = output.second + i;
        }
        simResults->push_back(output);
    }
    //report last section Data
    pthread_mutex_lock(args->progressMutex);
    *(args->globalProgress) += args->iterations - iterationsReported;
    pthread_mutex_unlock(args->progressMutex);
    pthread_exit((void*) simResults);
}

void* trackProgress(void* data){
    ReporterThreadData* args = (ReporterThreadData*) data;
    unsigned long long lastReported = 0;
    unsigned long long tenPercent = args->iterations/100;
    while(lastReported < args->iterations && lastReported + tenPercent < args->iterations){
        std::this_thread::sleep_for(std::chrono::microseconds(50000));
        std::chrono::high_resolution_clock::time_point tx = std::chrono::high_resolution_clock::now(); 
        pthread_mutex_lock(args->progressMutex);
        if(*(args->globalProgress) >= lastReported + tenPercent){
            std::cout << std::fixed << std::setprecision(2) << 
            "Time Elapsed " << std::chrono::duration_cast<std::chrono::duration<double>>(tx - *args->startTimePoint).count() <<
            ": Current Progress " << (*args->globalProgress * 100) / args->iterations << "% (" << FmtCmma(*args->globalProgress) << 
            "/" <<  FmtCmma(args->iterations) << ")"<< std::endl;
            lastReported = *args->globalProgress;
        }
        pthread_mutex_unlock(args->progressMutex);
    }
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char* argv[]){
    SimArgs args;
    int progressStep = 10;
    bool verboseLogging = false;
    std::string outputFileName = "";
    std::string uniquesFileName = "";
    std::string obtainedFileName = "";
    if(!parseArgs(argc, argv, args)){
        return -1;
    }
    //argument parsing
    if(argc <= 1) {
        return 0;
    }
    pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
    unsigned long long iterationProgress = 0;

    std::cout << "Running " << FmtCmma(args.iterations) << " Simulations with " << args.uniques << " slots with " << FmtCmma(args.rarityN) << "/" <<FmtCmma(args.rarityD)  << " rarity on " << args.threads << " threads." << std::endl;
    if(args.count > 0)
        std::cout << "Stopping after " << args.count << " Items Obtained." << std::endl;
    if(args.numRollsPerAttempt > 1)
        std::cout << "Rolling for Loot " << args.numRollsPerAttempt << " Times per Attempt." << std::endl;
    if(args.weightings.size()> 0)
        std::cout << "Using Weight File" << std::endl;
    if(args.obtainedItems.size() > 0)
        std::cout << "Using Obtained Items File" << std::endl;
    if(args.tertiaryRolls.size() > 0)
        std::cout << "Rolling " << args.tertiaryRolls.size() << " Tertiary Roll(s) per attempt" << std::endl;
    std::cout << std::endl;
    
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<unsigned long long, unsigned long long>> results;
    std::vector<ThreadData*> threadArguments;
    std::vector<std::pair<int,int>> weightings;
    std::vector<int> givenItems;
    std::ifstream inputFile;

    for(int i = 0; i < args.threads; i++)
        threadArguments.push_back(new ThreadData(args, &progressMutex, &iterationProgress));
    for(int i = 0; i < args.iterations % args.threads; i++)
        threadArguments[i]->iterations++;

    pthread_t threadIds[args.threads];
    pthread_t reportThread;
    std::vector<std::pair<unsigned long long, unsigned long long>>* threadResults;
    //create reporter thread
    ReporterThreadData* reporterThreadData =  new ReporterThreadData(&progressMutex, &iterationProgress, args.iterations, &t1);
    pthread_create(&reportThread, NULL, &trackProgress, (void*)reporterThreadData);
    //create worker threads
    for(int i = 0; i < args.threads; i++){
        pthread_create(&threadIds[i], NULL, &runIteration, (void*)threadArguments[i]);
        if(verboseLogging)
            std::cout << "Creating thread " << i << " " << threadIds[i] << std::endl;
    }
    for(int i = 0; i < args.threads; i++){
        if(verboseLogging)
            std::cout << "waiting for thread " << i << " " << threadIds[i] << std::endl;
        pthread_join(threadIds[i],(void**) &threadResults);
        std::vector<std::pair<unsigned long long, unsigned long long>> localVector = *threadResults;
        if(verboseLogging)
            std::cout << "pulling " << FmtCmma(localVector.size()) << " sims" << std::endl;
        for(std::pair<unsigned long long, unsigned long long> singleIteration : localVector){
            results.push_back(singleIteration);
        }
        free(threadResults);
    }
    for(ThreadData* ptr : threadArguments){
        delete ptr;
    }
    //end threaded work
    pthread_join(reportThread, NULL);
    delete(reporterThreadData);

    unsigned long long sumAttempts = 0;
    unsigned long long sumItems = 0;
    unsigned long long highest_attempt = 0;
    unsigned long long highestAttemptItems = 0;
    unsigned long long lowest_attempt = 0;
    unsigned long long lowestAttemptItems = 0;
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    std::cout << std::endl << std::endl << std::endl << "============================================================================================" << std::endl;
    std::cout << "Simulation took " << time_span.count() << " Seconds." << std::endl;
    

    if(outputFileName != ""){
        std::ofstream outFile;
        outFile.open(outputFileName);
        for(std::pair<unsigned long long, unsigned long long> iteration : results){
            outFile << iteration.first << "," << iteration.second << std::endl;
            sumAttempts += iteration.first;
            sumItems += iteration.second;
            if(iteration.first > highest_attempt){
                highest_attempt = iteration.first;
                highestAttemptItems = iteration.second;
            }
            if(iteration.first < lowest_attempt || lowest_attempt == 0){
                lowest_attempt = iteration.first;
                lowestAttemptItems = iteration.second;
            }
        }
        outFile.close();
    } else {
        for(std::pair<unsigned long long, unsigned long long> iteration : results){
            sumAttempts += iteration.first;
            sumItems += iteration.second;
            if(iteration.first > highest_attempt){
                highest_attempt = iteration.first;
                highestAttemptItems = iteration.second;
            }
            if(iteration.first < lowest_attempt || lowest_attempt == 0){
                lowest_attempt = iteration.first;
                lowestAttemptItems = iteration.second;
            }
        }
    }
    unsigned long long stdSum = 0;
    double standardDev = 0.0;
    if(outputFileName != ""){
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
        std::cout << "Finished Writing to file in " << time_span2.count() << " Seconds." << std::endl;
    }

    //parse Results
    long double averageAttempts = ceil(((sumAttempts * 1.0) / (results.size()*1.0))*100.0)/100.0;
    double averageItems = ceil((sumItems * 1.0)/(results.size()*1.0)*100.0)/100.0;
    std::string formattedAverageAttempts = FmtCmma(averageAttempts);
    std::string formattedAverageItems = FmtCmma(averageItems);

    for(std::pair<unsigned long long, unsigned long long> iteration : results){
        stdSum += pow(iteration.first - averageAttempts, 2);
    }
    standardDev = sqrt(stdSum/results.size());


    //output results
    std::cout << "Iterations: " << FmtCmma(results.size()) << std::endl;
    std::cout << "Sum Attempts " << FmtCmma(sumAttempts) << std::endl;
    std::cout << "Average attempts: " << formattedAverageAttempts << ", Average items: " << formattedAverageItems << "." << std::endl;
    std::cout << "Standard Deviation: " << FmtCmma(standardDev) << std::endl;
    std::cout << "highest attempts: " << FmtCmma(highest_attempt) << ", with " << FmtCmma(highestAttemptItems)
     << " items, Lowest attempts " << FmtCmma(lowest_attempt) << ", with " << FmtCmma(lowestAttemptItems) << " items" << std::endl; 
}

