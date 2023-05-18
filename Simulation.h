#ifndef SIMULATION_H
#define SIMULATION_H
#include "SimResult.h"
#include "ReportThreadData.h"
#include "ThreadData.h"
#include <deque>



class Simulation {
    public:
        static bool checkForZero(const std::vector<int>& vec);
        static std::deque<SimResult> runVanillaNoWeight(const ThreadData& args);
        static std::deque<SimResult> runVanillaWeight(const ThreadData& args);
        void runIteration(void *data);
        static void trackProgress(const ReporterThreadData&);
    protected:

};

#endif