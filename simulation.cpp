#include "simulation.h"

struct ThreadData
{
    public:
        ThreadData(int nrarityN, int nrarityD, int nuniques, int niterations){
            rarityN = nrarityN;
            rarityD = nrarityD;
            uniques = nuniques;
            iterations = niterations;
        }
        int rarityN;
        int rarityD;
        int uniques;
        int iterations;
};

void printHelpMsg(){
    std::cout << "Flags " << std::endl;
    std::cout << "-h \t\t\t\t: show help" << std::endl;
    std::cout << "-s <simulations>\t\t: (Required) number of simulations to run." << std::endl;
    std::cout << "-t <threads>\t\t\t: (Required) number of threads to run the simulation on." << std::endl;
    std::cout << "-u <uniques>\t\t\t: (Required) number of unique items to collect." << std::endl;
    std::cout << "-r <rarity N> <rarity D>\t: (Required) Rarity of items n/d" << std::endl;
    std::cout << "-f <fileName>\t\t\t: name of file to output simulation results." << std::endl;
}

bool parseArgs(int argc, char* argv[], int &uniques, int& rarityN, int&rarityD, int&threads, int& sims, std::string& fileName){
    bool result = true;
    for(int i = 1; i < argc; i++){
        if(!strcmp(argv[i], "-h")){
            printHelpMsg();
            return 0;
        } else if(!strcmp(argv[i], "-s")){
            if(argc >= i+1){
                sims = atoi(argv[i+1]);
            } else {
                std::cout << "-s specified but no argument given.";
                result = false;
            }
        } else if(!strcmp(argv[i], "-t")){
            if(argc >= i+1){
                threads = atoi(argv[i+1]);
            } else {
                std::cout << "-t specified but no argument given.";
                result = false;
            }
        } else if(!strcmp(argv[i], "-f")){
            if(argc >= i+1){
                fileName = argv[i+1];
            } else {
                std::cout << "-f specified but no argument given.";
                result = false;
            }
        } else if(!strcmp(argv[i], "-u")){
            if(argc >= i+1){
                uniques = atoi(argv[i+1]);
            } else {
                std::cout << "-u specified but no argument given.";
                result = false;
            }
        } else if(!strcmp(argv[i], "-r")){
            if(argc >= i+2){
                rarityN = atoi(argv[i+1]);
                rarityD = atoi(argv[i+2]);
            } else {
                std::cout << "-r specified but no argument given.";
                result = false;
            }
        }
    }
    if (sims == 0 ||
        threads == 0 ||
        uniques == 0 ||
        rarityN == 0 ||
        rarityD == 0) {
            std::cout << "Missing required input" << std::endl;
            std::cout << "have " 
            << "sims " << sims 
            << " threads " << threads
            << " uniques " << uniques
            << " rairtyN " << rarityN
            << " rarityD " << rarityD 
            << std::endl;
            result = false;
        }
    return result;
}

int main(int argc, char* argv[]){
    int uniques = 0;
    int rarityN = 0;
    int rarityD = 0;
    int threads = 0;
    int sims = 0;
    std::string fileName = "";
    if(!parseArgs(argc, argv, uniques, rarityN, rarityD, threads, sims, fileName)){
        printHelpMsg();
        return -1;
    }
    //argument parsing
    if(argc <= 1) {
        printHelpMsg();
        return 0;
    }

    std::printf("Running %d Simulations of %d slots with %d/%d rarity on %d threads.\n", sims, uniques, rarityN, rarityD, threads);
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    std::vector<std::pair<unsigned long long, unsigned long long>> results;
    std::vector<ThreadData*> threadArguments;
    for(int i = 0; i < threads; i++)
        threadArguments.push_back(new ThreadData(rarityN, rarityD, uniques, (sims/threads)+1));
    pthread_t threadIds[threads];
    std::vector<std::pair<unsigned long long, unsigned long long>>* threadResults;
    for(int i = 0; i < threads; i++){
        pthread_create(&threadIds[i], NULL, &runIteration, (void*)threadArguments[i]);
        std::cout << "Creating thread " << i << " " << threadIds[i] << std::endl;
    }

    for(int i = 0; i < threads; i++){
        std::cout << "waiting for thread " << i << " " << threadIds[i] << std::endl;
        pthread_join(threadIds[i],(void**) &threadResults);
        std::vector<std::pair<unsigned long long, unsigned long long>> localVector = *threadResults;
        std::cout << "pulling " << FmtCmma(localVector.size()) << " sims" << std::endl;
        for(std::pair<unsigned long long, unsigned long long> singleIteration : localVector){
            results.push_back(singleIteration);
        }
        free(threadResults);
    }
    for(ThreadData* ptr : threadArguments){
        delete ptr;
    }
    //runIteration((void*) data);
    unsigned long long sumAttempts = 0;
    unsigned long long sumItems = 0;
    unsigned long long highest_attempt = 0;
    unsigned long long highestAttemptItems = 0;
    unsigned long long lowest_attempt = 0;
    unsigned long long lowestAttemptItems = 0;
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    std::printf("\n\n\n============================================================================================\n");
    printf("Simulation took %f Seconds.\n", time_span.count());

    if(fileName != ""){
        std::ofstream myfile;
        myfile.open(fileName);
        for(std::pair<unsigned long long, unsigned long long> iteration : results){
            myfile << iteration.first << "," << iteration.second << std::endl;
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
        myfile.close();
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
    if(fileName != ""){
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
        printf("finished Writing to file in %f Seconds.\n", time_span2.count());
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



template<class T>
std::string FmtCmma(T value)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
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
    //data init
    ThreadData* args = ((ThreadData*) data);
    int numUniques = args->uniques;
    int rarityN = args->rarityN;
    int rarityD = args->rarityD;
    int iterations = args->iterations;
    int lastCount = 0;
    std::pair<unsigned long long, unsigned long long> output;
    int item;
    int roll = 0;
    int itemsArray[numUniques+1];
    static thread_local std::mt19937 generator;
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> chanceDistrib(1,rarityD);
    std::uniform_int_distribution<int> uniqueDistrib(0,numUniques-1);
    std::vector<std::pair<unsigned long long, unsigned long long>>* simResults = new std::vector<std::pair<unsigned long long, unsigned long long>>();
    simResults->reserve(iterations);
    //loop start
    for(int iteration = 0; iteration < iterations; iteration++){
        //setup sim
        for(int i = 0; i < numUniques+1; i++){
            itemsArray[i] = 0;
        }
        output.first = 0;
        output.second = 0;
        bool containsZero = true;    
        while (containsZero){
            roll = chanceDistrib(generator);
            if (roll <= rarityN){
                item = uniqueDistrib(generator);
                itemsArray[item]++;
                containsZero = false;
                for(int i : itemsArray){
                    if(i == 0)
                        containsZero = true;
                }
            }
            itemsArray[numUniques]++;
        }
        output.first = itemsArray[numUniques];
        for(int i : itemsArray){
            output.second = output.second + i;
        }
        output.second = output.second - itemsArray[numUniques];
        simResults->push_back(output);
    }
    pthread_exit((void*) simResults);
}