#ifndef SIMULATION_H
#define SIMULATION_H
#include "SimResult.h"
#include "ReportThreadData.h"
#include "ThreadData.h"
#include <deque>



class Simulation {
    public:
        static bool checkForZero(const std::vector<int>& vec);
        std::deque<SimResult> runVanillaNoWeight(const ThreadData& args);
        std::deque<SimResult> runVanillaWeight(const ThreadData& args);
        std::deque<SimResult> run1to1Weight(const ThreadData& args);
        std::deque<SimResult> runAttempts(const ThreadData& args);
        std::deque<std::deque<SimResult>> runSims(std::vector<ThreadData> threadArguments, SimArgs args);
        void runIteration(void *data);
        static void trackProgress(const ReporterThreadData&);
    protected:

};

#endif