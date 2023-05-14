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

struct ThreadData {
    //**DONE
    // handle rolls, basic case 1 roll for 1 item no restrictions
    // ##BasicUniques, BasicRarityN/D, iterations, count
    // case multiple rolls for all items   (clue casket)
    // ##numRollsPerAttempt

    //**INPROGRESS
    // case 1+ basic rolls + tertiary roll (zulrah)
    // ##tertiaryDrops teriarityRate -- multiple rates, one for each drop
    // case multiple rolls with exclusions (barrows)
    // #specific case for barrows
   public:
    ThreadData(SimArgs args, pthread_mutex_t *nprogressMutex, unsigned long long *nglobalProgress) {
        endCondition = args.endCondition;
        rarityN = args.rarityN;
        rarityD = args.rarityD;
        uniques = args.uniques;
        iterations = args.iterations / args.threads;
        count = args.count;
        numRollsPerAttempt = args.numRollsPerAttempt;
        progressMutex = nprogressMutex;
        globalProgress = nglobalProgress;
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
    pthread_mutex_t *progressMutex;
    std::vector<std::pair<int, int>> weightings;
    std::vector<int> items;
    std::vector<std::pair<int, int>> tertiaryRolls;
    std::vector<int> targetItems;
    bool useTargetItems;
    EndCondition endCondition;
    int rarityN;
    int rarityD;
    int uniques;
    int count;
    int numRollsPerAttempt;
    int weightFactor;
    unsigned long long iterations;
    unsigned long long *globalProgress;
};

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