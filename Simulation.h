#ifndef SIMULATION_H
#define SIMULATION_H
#include "SimResult.h"
#include "ReportThreadData.h"
#include "ThreadData.h"
#include <deque>

enum class EndCondition : int {
    Uniques,
    Weight1to1,
    WeightTotal,
    Attempts
};

class Simulation {
    public:
        static bool checkForZero(const std::vector<int>& vec);
        std::deque<SimResult> runVanillaNoWeight(const ThreadData& args);
        std::deque<SimResult> runVanillaWeight(const ThreadData& args);
        void runIteration(void *data);
        void trackProgress(const ReporterThreadData&);
    protected:

};

#endif