#ifndef SIMULATION_H
#define SIMULATION_H
class Simulation {
    public:
        static bool checkForZero(const std::vector<int>& vec);
        void *runVanillaNoWeight(void *data);
        void *runVanillaWeight(void *data);
        void *runIteration(void *data);
        void *trackProgress(void *data);
    protected:

};

#endif