/*
Drop Simulator
Version 1.0
Last Modified 5/29/2023
*/

#include "Simulation.h"
#include "IO.h"
#include "ReportThreadData.h"
#include "enum.h"
#include <thread>
#include <fstream>
#include <future>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <unistd.h>

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
    unsigned long long iterationProgress = 0;

    IOUtils::printStartParameters(args);

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    std::deque<std::deque<SimResult>> results;
    std::shared_ptr<std::atomic_ullong> progress(new std::atomic_ullong);
    progress->store(0);
    std::vector<ThreadData> threadArguments;
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> givenItems;
    std::ifstream inputFile;

    for (int i = 0; i < args.threads; i++)
        threadArguments.push_back(ThreadData(args, progress));
    for (int i = 0; i < args.iterations % args.threads; i++)
        threadArguments[i].iterations++;

    // create reporter thread
    ReporterThreadData reporterThreadData(progress, args.iterations, t1);
    Simulation simulation;
    auto reporterThreadFuture = std::async(std::launch::async, Simulation::trackProgress, reporterThreadData);
    // create worker threads, need to make them async
    results = simulation.runSims(threadArguments, args);

    // end threaded work
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    reporterThreadFuture.wait();

    unsigned long long sumAttempts = 0;
    unsigned long long sumItems = 0;
    unsigned long long sum1to1Weight = 0;
    unsigned long long sumTotalWeight = 0;
    unsigned long long sum1To1Uniques = 0;
    unsigned long long highest_attempt = 0;
    unsigned long long highestAttemptItems = 0;
    unsigned long long lowest_attempt = 0;
    unsigned long long lowestAttemptItems = 0;
    long double sumtime = 0.0;
    double averageTime = 0.0;
    std::cout << std::endl
              << std::endl
              << std::endl
              << "================================================================================" << std::endl;
    std::cout << "Simulation took " << time_span.count() << " Seconds." << std::endl;

    if (args.resultsFileName != "") {
        std::ofstream outFile;
        outFile.open(args.resultsFileName);
        for (std::deque<SimResult> singleThreadResult : results) {
            for (SimResult iteration : singleThreadResult) {
                outFile << iteration.attempts << "," << iteration.totalUniques << "," << iteration.oneToOneWieght << "," << iteration.totalWeight << std::endl;
            }
        }
        outFile.close();
    } else {
        for (std::deque<SimResult> singleThreadResult : results) {
            for (SimResult iteration : singleThreadResult) {
                sumAttempts += iteration.attempts;
                sumItems += iteration.totalUniques;
                sum1to1Weight += iteration.oneToOneWieght;
                sumTotalWeight += iteration.totalWeight;
                sum1To1Uniques += iteration.oneToOneUniques;

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
    if (args.resultsFileName != "") {
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
        std::cout << "Finished Writing to file in " << time_span2.count() << " Seconds." << std::endl;
    }

    // parse Results
    long double totalIterationsd = 0.0;
    unsigned long long totalIterationsi = 0;
    for (std::deque<SimResult> singleThreadResult : results){
        totalIterationsi += singleThreadResult.size();
        totalIterationsd += singleThreadResult.size();
    }
    long double averageAttempts = ceil(((sumAttempts * 1.0) / (totalIterationsd)) * 100.0) / 100.0;
    double averageItems = ceil((sumItems * 1.0) / (totalIterationsd)*100.0) / 100.0;
    double average1to1Weight = ceil((sum1to1Weight * 1.0) / (totalIterationsd)*100.0) / 100.0;
    double average1to1Uniques = ceil((sum1To1Uniques * 1.0) / (totalIterationsd)*100.0) / 100.0;
    std::string formattedAverageAttempts = IOUtils::FmtCmma(averageAttempts);
    std::string formattedAverageItems = IOUtils::FmtCmma(averageItems);

    for (std::deque<SimResult> thread : results) {
        for (SimResult iteration : thread)
            sumtime += iteration.timeTaken;
    }
    averageTime = sumtime / totalIterationsd;

    for (std::deque<SimResult> thread : results) {
        for (SimResult iteration : thread)
            stdSum += pow(iteration.attempts - averageAttempts, 2);
    }
    standardDev = sqrt(stdSum / args.iterations);

    // output results
    std::cout << std::setprecision(6);
    std::cout << "Iterations:\t\t\t" << IOUtils::FmtCmma(totalIterationsi) << std::endl;
    std::cout << "Sum Attempts:\t\t\t" << IOUtils::FmtCmma(sumAttempts) << std::endl;
    std::cout << "Average attempts:\t\t" << formattedAverageAttempts << ", Average items: " << formattedAverageItems << "." << std::endl;
    std::cout << "Average 1 to 1 uniques:\t\t" << IOUtils::FmtCmma(average1to1Uniques) << " slots." << std::endl;
    std::cout << "Average 1 to 1 Weight:\t\t" << IOUtils::FmtCmma(average1to1Weight) << " progress." << std::endl;
    std::cout << "Total Simulation CPU Time:\t" << sumtime << " seconds." << std::endl;
    std::cout << "Average Time Taken per Sim:\t" << averageTime << std::endl;
    std::cout << "Attempts Standard Deviation:\t" << IOUtils::FmtCmma(standardDev) << std::endl;
    std::cout << "highest attempts:" << IOUtils::FmtCmma(highest_attempt) << ", with " << IOUtils::FmtCmma(highestAttemptItems)
              << " items, Lowest attempts " << IOUtils::FmtCmma(lowest_attempt) << ", with " << IOUtils::FmtCmma(lowestAttemptItems) << " items" << std::endl;
}
