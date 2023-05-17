#include <vector>
#include <string>

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

ReporterThreadData::ReporterThreadData(pthread_mutex_t *nprogressMutex, unsigned long long *nglobalProgress, unsigned long long niterations, std::chrono::high_resolution_clock::time_point *nstartTimePoint) {
    progressMutex = nprogressMutex;
    globalProgress = nglobalProgress;
    iterations = niterations;
    startTimePoint = nstartTimePoint;
}


struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; }    // groups of 3 digit
};