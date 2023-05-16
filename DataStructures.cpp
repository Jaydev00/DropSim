#include <vector>
#include <string>

enum class EndCondition : int {
    Uniques,
    Weight1to1,
    WeightTotal,
    Attempts
};

struct SimArgs {
    EndCondition endCondition = EndCondition::Uniques;
    unsigned long long iterations = 0;
    int threads = 0;
    int rarityN = 0;
    int rarityD = 0;
    int uniques = 0;
    int count = 0;
    int numRollsPerAttempt = 1;
    int weightFactor = 1;
    bool verboseLogging = false;
    bool useTargetItems = false;
    std::string resultsFileName = "";
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> obtainedItems;
    std::vector<int> targetItems;
    std::vector<std::pair<int, int>> tertiaryRolls;
};

ThreadData::ThreadData(SimArgs args, const std::shared_ptr<std::atomic_ullong>& nglobalprogressCounter, const std::shared_ptr<std::mutex>& nprogressMutexPtr) {
        endCondition = args.endCondition;
        rarityN = args.rarityN;
        rarityD = args.rarityD;
        uniques = args.uniques;
        iterations = args.iterations / args.threads;
        count = args.count;
        numRollsPerAttempt = args.numRollsPerAttempt;
        globalProgressCounter = nglobalprogressCounter;
        progressMutexPtr = nprogressMutexPtr;
        for (int i = 0; i < args.weightings.size(); i++)
            weightings.push_back(args.weightings[i]);
        for (int i = 0; i < args.tertiaryRolls.size(); i++)
            tertiaryRolls.push_back(args.tertiaryRolls[i]);
        for (int i : args.obtainedItems)
            items.push_back(i);
        for (int i : args.targetItems)
            targetItems.push_back(i);
        useTargetItems = args.useTargetItems;
        weightFactor = args.weightFactor;
    }

struct ReporterThreadData {
   public:
    ReporterThreadData(pthread_mutex_t *nprogressMutex, unsigned long long *nglobalProgress, unsigned long long niterations, std::chrono::high_resolution_clock::time_point *nstartTimePoint) {
        progressMutex = nprogressMutex;
        globalProgress = nglobalProgress;
        iterations = niterations;
        startTimePoint = nstartTimePoint;
    }
    pthread_mutex_t *progressMutex;
    unsigned long long *globalProgress;
    unsigned long long iterations;
    std::chrono::high_resolution_clock::time_point *startTimePoint;
};

struct SimResult {
    unsigned int attempts = 0;
    unsigned int totalUniques = 0;
    unsigned int oneToOneWieght = 0;
    unsigned int totalWeight = 0;
    long double timeTaken = 0.0;
};

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; }    // groups of 3 digit
};