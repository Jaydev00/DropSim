#ifndef SIMULATION_H
#define SIMULATION_H
class Simulation {
    public:
        static bool checkForZero(const std::vector<int>& vec);
        std::deque<SimResult> runVanillaNoWeight(const ThreadData& args);
        std::deque<SimResult> runVanillaWeight(void *data);
        void runIteration(void *data);
        void trackProgress(void *data);
    protected:

};

#endif