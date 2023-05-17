#include "Simulation.h"
#include "DataStructures.h"
#include "IO.h"

#include <thread>
#include <fstream>

int main(int argc, char *argv[]) {
    //New
    
    //End New
    SimArgs args;
    int progressStep = 10;
    bool verboseLogging = false;
    if (!IOUtils::parseArgs(argc, argv, args)) {
        return -1;
    }
    // argument parsing
    if (argc <= 1) {
        return 0;
    }
    pthread_mutex_t progressMutex = PTHREAD_MUTEX_INITIALIZER;
    unsigned long long iterationProgress = 0;

    IOUtils::printStartParameters(args);

    ThreadData data;

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
        if(args.endCondition == EndCondition::Uniques)
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
