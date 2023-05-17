#ifndef SIM_RESULT_H
#define SIM_RESULT_H

class SimResult {
    public:
    SimResult();
    unsigned int attempts;
    unsigned int totalUniques;
    unsigned int oneToOneWieght;
    unsigned int totalWeight;
    long double timeTaken;
};
#endif