#include "ThreadData.h"
#include "SimArgs.h"

ThreadData::ThreadData(SimArgs args, const std::shared_ptr<std::atomic_ullong>& nglobalprogressCounter) {
        endCondition = args.endCondition;
        rarityN = args.rarityN;
        rarityD = args.rarityD;
        uniques = args.uniques;
        iterations = args.iterations / args.threads;
        count = args.count;
        numRollsPerAttempt = args.numRollsPerAttempt;
        globalProgressCounter = nglobalprogressCounter;
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
